/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cardboard_launcher.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <string>

#include "cardboard.h"
#include "cardboard_strings.h"
#include "demo_icon.h"
#include "util.h"

namespace cardboard_launcher {

namespace {

constexpr float kDefaultFloorHeight{-1.7f};

// 6 Hz cutoff frequency for the velocity filter of the head tracker.
constexpr int32_t kVelocityFilterCutoffFrequency{6};

constexpr uint64_t kPredictionTimeWithoutVsyncNanos{50000000};

// Angle threshold for determining whether the controller is pointing at the
// demo icon.
constexpr float kAngleLimit{0.1f};

// Distance between the demos.
constexpr float kDistanceBetweenDemos{0.7f};

// Vertical offset for the black square.
constexpr float kBlackSquareYOffset{2.2f};

// Distance from the user for the black square.
constexpr float kBlackSquareZOffset{-3.0f};

// Scale for the background when showing demos.
constexpr std::array<float, 3> kDemosBackgroundScale{3.1f, 1.1f, 1.0f};

// Scale for the background during the tutorial.
constexpr std::array<float, 3> kTutorialBackgroundScale{2.0f, 2.2f, 1.0f};

// Simple shaders to render .obj files without any lighting.
constexpr const char* kObjVertexShader =
    R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    attribute vec2 a_UV;
    varying vec2 v_UV;

    void main() {
      v_UV = a_UV;
      gl_Position = u_MVP * a_Position;
    })glsl";

constexpr const char* kObjFragmentShader =
    R"glsl(
    precision mediump float;

    uniform sampler2D u_Texture;
    varying vec2 v_UV;

    void main() {
      // The y coordinate of this sample's textures is reversed compared to
      // what OpenGL expects, so we invert the y coordinate.
      gl_FragColor = texture2D(u_Texture, vec2(v_UV.x, 1.0 - v_UV.y));
    })glsl";

constexpr const char* kVertexShader = R"glsl(
  uniform mat4 u_MVP;
  uniform mat4 u_Scale;
  attribute vec4 a_Position;
  void main () {
    gl_Position = u_MVP * u_Scale * a_Position;
  }
)glsl";

constexpr const char* kFragmentShader = R"glsl(
  precision mediump float;
  void main () {
    gl_FragColor = vec4(0, 0, 0, 0.4);
  }
)glsl";

// Button animation.
const std::vector<std::string> kTutorialButtonAssets{
    "button_animation_0001.png", "button_animation_0002.png",
    "button_animation_0003.png", "button_animation_0004.png",
    "button_animation_0005.png", "button_animation_0006.png",
    "button_animation_0007.png", "button_animation_0008.png",
    "button_animation_0009.png", "button_animation_0010.png",
    "button_animation_0011.png", "button_animation_0012.png",
    "button_animation_0013.png", "button_animation_0014.png",
    "button_animation_0015.png"};

// Tilt animation.
const std::vector<std::string> kTutorialTiltAssets{
    "tilt_animation_0000.png", "tilt_animation_0001.png",
    "tilt_animation_0002.png", "tilt_animation_0003.png",
    "tilt_animation_0004.png", "tilt_animation_0005.png",
    "tilt_animation_0006.png", "tilt_animation_0007.png",
    "tilt_animation_0008.png", "tilt_animation_0009.png",
    "tilt_animation_0010.png", "tilt_animation_0011.png",
    "tilt_animation_0012.png", "tilt_animation_0013.png",
    "tilt_animation_0014.png"};

}  // anonymous namespace

CardboardLauncher::CardboardLauncher(JavaVM* vm, jobject obj,
                                     jobject asset_mgr_obj)
    : head_tracker_{nullptr},
      lens_distortion_{nullptr},
      distortion_renderer_{nullptr},
      screen_params_changed_{false},
      device_params_changed_{false},
      screen_width_{0},
      screen_height_{0},
      depthRenderBuffer_{0},
      framebuffer_{0},
      texture_{0},
      obj_program_{0},
      obj_position_param_{0},
      obj_uv_param_{0},
      obj_model_view_projection_param_{0},
      should_launch_demo_{false},
      state_{State::SHOW_DEMOS},
      last_state_event_{StateEvent::NONE},
      tutorial_animator_{kTutorialButtonAssets, kTutorialTiltAssets,
                         &text_renderer_},
      black_background_mvp_param_{0},
      black_background_scale_param_{0} {
  JNIEnv* env;
  vm->GetEnv((void**)&env, JNI_VERSION_1_6);
  java_asset_mgr_ = env->NewGlobalRef(asset_mgr_obj);
  asset_mgr_ = AAssetManager_fromJava(env, asset_mgr_obj);

  Cardboard_initializeAndroid(vm, obj);
  head_tracker_ = CardboardHeadTracker_create();
  CardboardHeadTracker_setLowPassFilter(head_tracker_,
                                        kVelocityFilterCutoffFrequency);
}

CardboardLauncher::~CardboardLauncher() {
  CardboardHeadTracker_destroy(head_tracker_);
  CardboardLensDistortion_destroy(lens_distortion_);
  CardboardDistortionRenderer_destroy(distortion_renderer_);
}

void CardboardLauncher::OnSurfaceCreated(JNIEnv* env) {
  const int obj_vertex_shader =
      cardboard_demo::LoadGLShader(GL_VERTEX_SHADER, kObjVertexShader);
  const int obj_fragment_shader =
      cardboard_demo::LoadGLShader(GL_FRAGMENT_SHADER, kObjFragmentShader);

  obj_program_ = glCreateProgram();
  glAttachShader(obj_program_, obj_vertex_shader);
  glAttachShader(obj_program_, obj_fragment_shader);
  glLinkProgram(obj_program_);
  glUseProgram(obj_program_);

  cardboard_demo::CHECKGLERROR("Obj program");

  obj_position_param_ = glGetAttribLocation(obj_program_, "a_Position");
  obj_uv_param_ = glGetAttribLocation(obj_program_, "a_UV");
  obj_model_view_projection_param_ =
      glGetUniformLocation(obj_program_, "u_MVP");

  cardboard_demo::CHECKGLERROR("Obj program params");

  CARDBOARDDEMO_CHECK(background_.Initialize(obj_position_param_, obj_uv_param_,
                                             "World.obj", asset_mgr_));
  CARDBOARDDEMO_CHECK(
      background_tex_.Initialize(env, java_asset_mgr_, "home_mono.jpg"));

  text_renderer_.Init();
  text_renderer_.LoadFont(asset_mgr_, "Roboto-Regular.ttf");

  const std::array demo_icon_data{
      DemoIconData{"tutorial", cardboard::GetLocalizedString(
                                   cardboard::kStrTutorialDemoTitle, env)},
      // BEGIN-INTERNAL-EXHIBIT
      // Hide the exhibit demo from being open sourced. Do not remove the
      // guards.
      DemoIconData{"exhibit", cardboard::GetLocalizedString(
                                  cardboard::kStrExhibitDemoTitle, env)},
      // END-INTERNAL-EXHIBIT
      DemoIconData{"mymovies",
                   cardboard::GetLocalizedString(
                       cardboard::kStrMyVideosAndroidDemoTitle, env)},
      DemoIconData{"photospheres",
                   cardboard::GetLocalizedString(
                       cardboard::kStrPhotoSphereAndroidDemoTitle, env)}};

  demos_.clear();
  demos_.reserve(demo_icon_data.size());

  // Create demos.
  for (size_t i = 0; i < demo_icon_data.size(); ++i) {
    // Demo meshes appear directly in front of user.
    const float pos{((demo_icon_data.size() - 1) / 2.0f - i) *
                    kDistanceBetweenDemos};
    demos_.emplace_back(demo_icon_data.at(i), pos, &text_renderer_);
  }

  tutorial_animator_.SetJNIEnv(env);

  LoadBlackBackground();
  LoadDemos(env);
  LoadTutorial(env);

  cardboard_demo::CHECKGLERROR("OnSurfaceCreated");
}

void CardboardLauncher::LoadBlackBackground() {
  const float vertices[] {
      0.5f,  0.5f,  0.0f,  // top right
      0.5f,  -0.5f, 0.0f,  // bottom right
      -0.5f, -0.5f, 0.0f,  // bottom left
      -0.5f, 0.5f,  0.0f   // top left
  };

  const uint32_t indices[]{
      // note that we start from 0!
      0, 1, 3,  // first triangle
      1, 2, 3   // second triangle
  };

  glGenBuffers(1, &black_background_vbo_);
  glGenBuffers(1, &black_background_ebo_);

  glBindBuffer(GL_ARRAY_BUFFER, black_background_vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, black_background_ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  const int32_t vertex_shader =
      cardboard_demo::LoadGLShader(GL_VERTEX_SHADER, kVertexShader);
  const int32_t fragment_shader =
      cardboard_demo::LoadGLShader(GL_FRAGMENT_SHADER, kFragmentShader);

  black_background_program_id_ = glCreateProgram();
  glAttachShader(black_background_program_id_, vertex_shader);
  glAttachShader(black_background_program_id_, fragment_shader);
  cardboard_demo::CHECKGLERROR("LoadBlackBackground attach shaders");

  glLinkProgram(black_background_program_id_);
  cardboard_demo::CHECKGLERROR("LoadBlackBackground link program");

  black_background_mvp_param_ =
      glGetUniformLocation(black_background_program_id_, "u_MVP");
  black_background_scale_param_ =
      glGetUniformLocation(black_background_program_id_, "u_Scale");
}

void CardboardLauncher::LoadDemos(JNIEnv* env) {
  for (DemoIcon& demo : demos_) {
    demo.LoadDemo(obj_position_param_, obj_uv_param_, asset_mgr_, env,
                  java_asset_mgr_);
  }
}

void CardboardLauncher::LoadTutorial(JNIEnv* env) {
  tutorial_animator_.LoadAnimations(obj_position_param_, obj_uv_param_,
                                    asset_mgr_, env, java_asset_mgr_);
}

void CardboardLauncher::SetScreenParams(int width, int height) {
  screen_width_ = width;
  screen_height_ = height;
  screen_params_changed_ = true;
}

void CardboardLauncher::OnDrawFrame() {
  if (!UpdateDeviceParams()) {
    return;
  }

  // Update Head Pose.
  head_view_ = GetPose();

  // Incorporate the height from the floor into the head_view.
  head_view_ = head_view_ * cardboard_demo::GetTranslationMatrix(
                                {0.0f, kDefaultFloorHeight, 0.0f});

  // Bind buffer.
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Draw eyes views.
  for (int eye = 0; eye < 2; ++eye) {
    glViewport(eye == kLeft ? 0 : screen_width_ / 2, 0, screen_width_ / 2,
               screen_height_);

    const cardboard_demo::Matrix4x4 eye_matrix =
        cardboard_demo::GetMatrixFromGlArray(eye_matrices_[eye]);
    const cardboard_demo::Matrix4x4 eye_view = eye_matrix * head_view_;

    const cardboard_demo::Matrix4x4 projection_matrix =
        cardboard_demo::GetMatrixFromGlArray(projection_matrices_[eye]);

    for (DemoIcon& demo : demos_) {
      const cardboard_demo::Matrix4x4 model_view_demo =
          eye_view * demo.GetModel();
      demo.SetModelViewProjection(projection_matrix * model_view_demo);
    }

    if (state_ == State::TILT_TUTORIAL || state_ == State::BUTTON_TUTORIAL) {
      tutorial_animator_.SetScreenPosition(eye_view, projection_matrix);
    }

    model_view_projection_background_ = projection_matrix * eye_view;
    const cardboard_demo::Matrix4x4 model_black_square =
        cardboard_demo::GetTranslationMatrix({0.0f, kBlackSquareYOffset,
                                              kBlackSquareZOffset});
    model_view_projection_black_square_ =
        projection_matrix * eye_view * model_black_square;

    // Draw background and demos.
    DrawWorld();

    // Update State Machine.
    UpdateState();
  }

  // Render.
  CardboardDistortionRenderer_renderEyeToDisplay(
      distortion_renderer_, /* target_display = */ 0, /* x = */ 0, /* y = */ 0,
      screen_width_, screen_height_, &left_eye_texture_description_,
      &right_eye_texture_description_);

  cardboard_demo::CHECKGLERROR("onDrawFrame");
}

void CardboardLauncher::OnTriggerEvent() {
  // Check if the pointed demo represents the built-in tutorial.
  if (IsPointingAtDemo(demos_.at(0).GetModel())) {
    LOGD("Launch Tutorial");
    last_state_event_ = StateEvent::TUTORIAL_SELECTED;
  } else {
    for (size_t i = 1; i < demos_.size(); ++i) {
      if (IsPointingAtDemo(demos_.at(i).GetModel()) &&
          state_ == State::SHOW_DEMOS) {
        LOGD("Launching Demo %s", demos_.at(i).GetDemoName().c_str());
        should_launch_demo_ = true;
        demo_to_launch_ = demos_.at(i).GetDemoName();
      }
    }
  }

  if (tutorial_animator_.IsNextClicked(head_view_, kAngleLimit)) {
    last_state_event_ = StateEvent::TUTORIAL_NEXT_SELECTED;
  }
}

void CardboardLauncher::OnPause() { CardboardHeadTracker_pause(head_tracker_); }

void CardboardLauncher::OnResume() {
  CardboardHeadTracker_resume(head_tracker_);

  // Parameters may have changed.
  device_params_changed_ = true;

  // Check for device parameters existence in external storage. If they're
  // missing, we must scan a Cardboard QR code and save the obtained parameters.
  uint8_t* buffer;
  int size;
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);
  if (size == 0) {
    SwitchViewer();
  }
  CardboardQrCode_destroy(buffer);
}

void CardboardLauncher::SwitchViewer() {
  CardboardQrCode_scanQrCodeAndSaveDeviceParams();
}

bool CardboardLauncher::UpdateDeviceParams() {
  // Checks if screen or device parameters changed.
  if (!screen_params_changed_ && !device_params_changed_) {
    return true;
  }

  // Get saved device parameters.
  uint8_t* buffer;
  int size;
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);

  // If there are no parameters saved yet, returns false.
  if (size == 0) {
    return false;
  }

  CardboardLensDistortion_destroy(lens_distortion_);
  lens_distortion_ = CardboardLensDistortion_create(buffer, size, screen_width_,
                                                    screen_height_);

  CardboardQrCode_destroy(buffer);

  GlSetup();

  CardboardDistortionRenderer_destroy(distortion_renderer_);
  const CardboardOpenGlEsDistortionRendererConfig config{kGlTexture2D};
  distortion_renderer_ = CardboardOpenGlEs2DistortionRenderer_create(&config);

  CardboardMesh left_mesh;
  CardboardMesh right_mesh;
  CardboardLensDistortion_getDistortionMesh(lens_distortion_, kLeft,
                                            &left_mesh);
  CardboardLensDistortion_getDistortionMesh(lens_distortion_, kRight,
                                            &right_mesh);

  CardboardDistortionRenderer_setMesh(distortion_renderer_, &left_mesh, kLeft);
  CardboardDistortionRenderer_setMesh(distortion_renderer_, &right_mesh,
                                      kRight);

  // Get eye matrices.
  CardboardLensDistortion_getEyeFromHeadMatrix(lens_distortion_, kLeft,
                                               eye_matrices_[0]);
  CardboardLensDistortion_getEyeFromHeadMatrix(lens_distortion_, kRight,
                                               eye_matrices_[1]);
  CardboardLensDistortion_getProjectionMatrix(lens_distortion_, kLeft, kZNear,
                                              kZFar, projection_matrices_[0]);
  CardboardLensDistortion_getProjectionMatrix(lens_distortion_, kRight, kZNear,
                                              kZFar, projection_matrices_[1]);

  screen_params_changed_ = false;
  device_params_changed_ = false;

  cardboard_demo::CHECKGLERROR("UpdateDeviceParams");

  return true;
}

void CardboardLauncher::GlSetup() {
  LOGD("GL SETUP");

  if (framebuffer_ != 0) {
    GlTeardown();
  }

  // Create render texture.
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screen_width_, screen_height_, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);

  left_eye_texture_description_.texture = texture_;
  left_eye_texture_description_.left_u = 0;
  left_eye_texture_description_.right_u = 0.5;
  left_eye_texture_description_.top_v = 1;
  left_eye_texture_description_.bottom_v = 0;

  right_eye_texture_description_.texture = texture_;
  right_eye_texture_description_.left_u = 0.5;
  right_eye_texture_description_.right_u = 1;
  right_eye_texture_description_.top_v = 1;
  right_eye_texture_description_.bottom_v = 0;

  // Generate depth buffer to perform depth test.
  glGenRenderbuffers(1, &depthRenderBuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width_,
                        screen_height_);
  cardboard_demo::CHECKGLERROR("Create Render buffer");

  // Create render demo icon.
  glGenFramebuffers(1, &framebuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthRenderBuffer_);

  cardboard_demo::CHECKGLERROR("GlSetup");
}

void CardboardLauncher::GlTeardown() {
  if (framebuffer_ == 0) {
    return;
  }
  glDeleteRenderbuffers(1, &depthRenderBuffer_);
  depthRenderBuffer_ = 0;
  glDeleteFramebuffers(1, &framebuffer_);
  framebuffer_ = 0;
  glDeleteTextures(1, &texture_);
  texture_ = 0;

  cardboard_demo::CHECKGLERROR("GlTeardown");
}

cardboard_demo::Matrix4x4 CardboardLauncher::GetPose() const {
  std::array<float, 4> out_orientation;
  std::array<float, 3> out_position;
  CardboardHeadTracker_getPose(
      head_tracker_,
      cardboard_demo::GetBootTimeNano() + kPredictionTimeWithoutVsyncNanos,
      kLandscapeLeft, &out_position.at(0), &out_orientation.at(0));
  return cardboard_demo::GetTranslationMatrix(out_position) *
         cardboard_demo::Quatf::FromXYZW(&out_orientation.at(0)).ToMatrix();
}

void CardboardLauncher::DrawWorld() { DrawBackground(); }

void CardboardLauncher::UpdateState() {
  switch (state_) {
    case State::SHOW_DEMOS:
      DrawBlackBackgroundGL(kDemosBackgroundScale);
      DrawDemos();
      // Check transition to button tutorial state.
      if (last_state_event_ == StateEvent::TUTORIAL_SELECTED) {
        state_ = State::BUTTON_TUTORIAL;
      }
      break;
    case State::BUTTON_TUTORIAL:
      DrawBlackBackgroundGL(kTutorialBackgroundScale);
      tutorial_animator_.DisplayButtonAnimation(
          obj_program_, obj_model_view_projection_param_);
      // Check transition to tilt tutorial state.
      if (last_state_event_ == StateEvent::TUTORIAL_NEXT_SELECTED) {
        state_ = State::TILT_TUTORIAL;
      }
      break;
    case State::TILT_TUTORIAL:
      DrawBlackBackgroundGL(kTutorialBackgroundScale);
      const float roll_angle_ =
          atan2(head_view_.m[1][0], head_view_.m[0][0]) * 180.0 / M_PI;
      if (roll_angle_ < -80.0f && roll_angle_ > -100.0f) {
        last_state_event_ = StateEvent::DEVICE_IN_PORTRAIT;
      }
      tutorial_animator_.DisplayTiltAnimation(obj_program_,
                                              obj_model_view_projection_param_);
      // Check transition to show demos state.
      if (last_state_event_ == StateEvent::DEVICE_IN_PORTRAIT) {
        state_ = State::SHOW_DEMOS;
      }
      break;
  }
}

void CardboardLauncher::DrawDemos() {
  glUseProgram(obj_program_);

  for (DemoIcon& demo : demos_) {
    const std::array<float, 16> target_array {
        demo.GetModelViewProjection().ToGlArray()};
    glUniformMatrix4fv(obj_model_view_projection_param_, 1, GL_FALSE,
                       target_array.data());

    demo.Draw();
    demo.SetPointingAtDemo(IsPointingAtDemo(demo.GetModel()));
  }
  cardboard_demo::CHECKGLERROR("DrawDemos");
}

void CardboardLauncher::DrawBackground() {
  glUseProgram(obj_program_);

  std::array<float, 16> background_array =
      model_view_projection_background_.ToGlArray();
  glUniformMatrix4fv(obj_model_view_projection_param_, 1, GL_FALSE,
                     background_array.data());

  background_tex_.Bind();
  background_.Draw();

  cardboard_demo::CHECKGLERROR("DrawBackground");
}

void CardboardLauncher::DrawBlackBackgroundGL(
    const std::array<float, 3>& scale) {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glUseProgram(black_background_program_id_);

  const std::array<float, 16> target_array{
      model_view_projection_black_square_.ToGlArray()};
  glUniformMatrix4fv(black_background_mvp_param_, 1, GL_FALSE,
                     target_array.data());

  cardboard_demo::Matrix4x4 scale_matrix =
      cardboard_demo::GetScaleMatrix(scale);
  const std::array<float, 16> scale_array{scale_matrix.ToGlArray()};
  glUniformMatrix4fv(black_background_scale_param_, 1, GL_FALSE,
                     scale_array.data());

  glBindBuffer(GL_ARRAY_BUFFER, black_background_vbo_);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, black_background_ebo_);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

bool CardboardLauncher::IsPointingAtDemo(
    const cardboard_demo::Matrix4x4& model_demo) {
  // Compute vectors pointing towards the reticle and towards the demo icon in
  // head space.
  cardboard_demo::Matrix4x4 head_from_demo = head_view_ * model_demo;
  const std::array<float, 4> unit_quaternion {0.f, 0.f, 0.f, 1.f};
  const std::array<float, 4> point_vector {0.f, 0.f, -1.f, 0.f};
  const std::array<float, 4> demo_vector { head_from_demo * unit_quaternion };

  const float angle {
    cardboard_demo::AngleBetweenVectors(point_vector, demo_vector)};
  return angle < kAngleLimit;
}

bool CardboardLauncher::ShouldLaunchDemo() { return should_launch_demo_; }

std::string CardboardLauncher::DemoToLaunch() { return demo_to_launch_; }

void CardboardLauncher::DemoLaunched() { should_launch_demo_ = false; }

}  // namespace cardboard_launcher
