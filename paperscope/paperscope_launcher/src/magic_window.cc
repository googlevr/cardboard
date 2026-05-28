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
#include "magic_window.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>

#include <array>
#include <cmath>
#include <cstdint>

#include "cardboard.h"
#include "util/matrix_4x4.h"
#include "util.h"

namespace magic_window {

namespace {

constexpr float kDefaultFloorHeight = -1.7f;

// 6 Hz cutoff frequency for the velocity filter of the head tracker.
constexpr int kVelocityFilterCutoffFrequency = 6;

constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

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
}  // anonymous namespace

MagicWindow::MagicWindow(JavaVM* vm, jobject obj, jobject asset_mgr_obj)
    : head_tracker_(nullptr),
      screen_width_(0),
      screen_height_(0),
      obj_program_(0),
      obj_model_view_projection_param_(0) {
  JNIEnv* env;
  vm->GetEnv((void**)&env, JNI_VERSION_1_6);
  java_asset_mgr_ = env->NewGlobalRef(asset_mgr_obj);
  asset_mgr_ = AAssetManager_fromJava(env, asset_mgr_obj);

  Cardboard_initializeAndroid(vm, obj);
  head_tracker_ = CardboardHeadTracker_create();
  CardboardHeadTracker_setLowPassFilter(head_tracker_,
                                        kVelocityFilterCutoffFrequency);
}

MagicWindow::~MagicWindow() { CardboardHeadTracker_destroy(head_tracker_); }

void MagicWindow::OnSurfaceCreated(JNIEnv* env) {
  const int obj_vertex_shader =
      cardboard_demo::LoadGLShader(GL_VERTEX_SHADER, kObjVertexShader);
  const int obj_fragment_shader =
      cardboard_demo::LoadGLShader(GL_FRAGMENT_SHADER, kObjFragmentShader);

  obj_program_ = glCreateProgram();
  glAttachShader(obj_program_, obj_vertex_shader);
  glAttachShader(obj_program_, obj_fragment_shader);
  glLinkProgram(obj_program_);
  glUseProgram(obj_program_);

  GLuint obj_position_param = glGetAttribLocation(obj_program_, "a_Position");
  GLuint obj_uv_param = glGetAttribLocation(obj_program_, "a_UV");
  obj_model_view_projection_param_ =
      glGetUniformLocation(obj_program_, "u_MVP");

  CARDBOARDDEMO_CHECK(background_.Initialize(obj_position_param, obj_uv_param,
                                             "World.obj", asset_mgr_));
  CARDBOARDDEMO_CHECK(
      background_tex_.Initialize(env, java_asset_mgr_, "home_mono.jpg"));
}

void MagicWindow::SetScreenParams(int width, int height) {
  screen_width_ = width;
  screen_height_ = height;
  SetFov();
  glViewport(0, 0, screen_width_, screen_height_);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MagicWindow::OnDrawFrame() {
  // Update Head Pose.
  cardboard_demo::Matrix4x4 head_view = GetPose();

  // Incorporate the height from the floor into the head_view.
  head_view = head_view * cardboard_demo::GetTranslationMatrix(
                              {0.0f, kDefaultFloorHeight, 0.0f});

  const cardboard_demo::Matrix4x4 projection_matrix =
      cardboard_demo::GetMatrixFromGlArray(magic_window_projection_matrix_);

  model_view_projection_background_ = projection_matrix * head_view;

  // Draw background.
  DrawBackground();
}

void MagicWindow::OnPause() { CardboardHeadTracker_pause(head_tracker_); }

void MagicWindow::ScanViewer() {
  CardboardQrCode_scanQrCodeAndSaveDeviceParams();
}

bool MagicWindow::HasSavedDeviceParams() {
  // Get saved device parameters.
  uint8_t* buffer;
  int size;
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);

  return size > 0;
}

void MagicWindow::OnResume() { CardboardHeadTracker_resume(head_tracker_); }

void MagicWindow::SetFov() {
  float ar = (float)screen_width_ / screen_height_;
  float fov_h = 2 * atan(tan(kFov_v / 2) * ar);
  std::array<float, 4> magic_window_fov = {fov_h, fov_h, kFov_v, kFov_v};
  cardboard::Matrix4x4::Perspective(magic_window_fov, kZNear, kZFar)
      .ToArray(magic_window_projection_matrix_);
}

cardboard_demo::Matrix4x4 MagicWindow::GetPose() {
  std::array<float, 4> out_orientation;
  std::array<float, 3> out_position;
  CardboardHeadTracker_getPose(
      head_tracker_,
      cardboard_demo::GetBootTimeNano() + kPredictionTimeWithoutVsyncNanos,
      kPortrait, &out_position[0], &out_orientation[0]);
  return cardboard_demo::GetTranslationMatrix(out_position) *
         cardboard_demo::Quatf::FromXYZW(&out_orientation[0]).ToMatrix();
}

void MagicWindow::DrawBackground() {
  glClearColor(.0f, .0f, .0f, 1.0f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  std::array<float, 16> background_array =
      model_view_projection_background_.ToGlArray();
  glUniformMatrix4fv(obj_model_view_projection_param_, 1, GL_FALSE,
                     background_array.data());

  background_tex_.Bind();
  background_.Draw();
}

}  // namespace magic_window
