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
#include "tutorial_animator.h"

#include <android/log.h>

#include <array>
#include <chrono>  // NOLINT(build/c++11)

#include "cardboard_strings.h"
#include "util.h"

namespace cardboard_launcher {

namespace {
constexpr const char* kVertexShader = R"glsl(
  uniform mat4 u_MVP;
  attribute vec4 a_Position;
  void main () {
    gl_Position = u_MVP * a_Position;
  }
)glsl";

constexpr const char* kFragmentShader = R"glsl(
  precision mediump float;
  void main () {
    gl_FragColor = vec4(1.0, 0.431, 0.251, 1.0); // Orange button
  }
)glsl";

constexpr std::array<float, 3> kTextTutorialScale{-0.0013f, 0.0013f, 1.0f};
constexpr std::array<float, 3> kTextTutorialModelMatrix{0.0f, -0.25f, 0.0f};

constexpr std::array<float, 3> kTextInstructionsModelMatrix{0.0f, -0.25f,
                                                            0.0f};

constexpr std::array<float, 3> kNextButtonModelMatrix{0.0f, -0.5f, 0.0f};
constexpr float kNextButtonTextXYScale{0.0012f};
constexpr std::array<float, 3> kNextButtonTextScale{-kNextButtonTextXYScale,
                                                      kNextButtonTextXYScale,
                                                      1.0f};
}  // namespace

TutorialAnimator::TutorialAnimator(
    const std::vector<std::string>& button_animation_files,
    const std::vector<std::string>& tilt_animation_files,
    cardboard_demo::TextRenderer* const text_renderer)
    : button_animation_(button_animation_files),
      tilt_animation_(tilt_animation_files),
      text_renderer_(text_renderer) {
  model_next_button_ =
      cardboard_demo::GetTranslationMatrix(kNextButtonModelMatrix);
}

void TutorialAnimator::LoadAnimations(GLuint obj_position_param,
                                      GLuint obj_uv_param,
                                      AAssetManager* asset_mgr, JNIEnv* env,
                                      jobject java_asset_mgr) {
  button_animation_.LoadAnimations(obj_position_param, obj_uv_param, asset_mgr,
                                   env, java_asset_mgr);
  tilt_animation_.LoadAnimations(obj_position_param, obj_uv_param, asset_mgr,
                                 env, java_asset_mgr);
  LoadNextButton();
}

void TutorialAnimator::SetScreenPosition(
    const cardboard_demo::Matrix4x4& eye_view,
    const cardboard_demo::Matrix4x4& projection_matrix) {
  tilt_animation_.SetModelViewProjection(eye_view, projection_matrix);
  button_animation_.SetModelViewProjection(eye_view, projection_matrix);

  // Set the model view projection of the next button.
  model_view_projection_next_button_ = projection_matrix * eye_view *
                                       button_animation_.GetModelAnimation() *
                                       model_next_button_ * bg_scale_matrix_;

  // Set the model view projection of the next button text.
  model_view_projection_next_button_text_ =
      projection_matrix * eye_view * button_animation_.GetModelAnimation() *
      button_text_model_matrix_ * button_text_scale_matrix_;

  // Set the model view projection of the text
  const cardboard_demo::Matrix4x4 text_scale_matrix =
      cardboard_demo::GetScaleMatrix(kTextTutorialScale);
  const cardboard_demo::Matrix4x4 text_model_matrix =
      cardboard_demo::GetTranslationMatrix(kTextTutorialModelMatrix);
  model_view_projection_text_model_matrix_ =
      projection_matrix * eye_view * button_animation_.GetModelAnimation() *
      text_model_matrix * text_scale_matrix;

  // Set the model view projection of the tilt instructions text.
  const cardboard_demo::Matrix4x4 instructions_text_model_matrix =
      cardboard_demo::GetTranslationMatrix(kTextInstructionsModelMatrix);
  model_view_projection_instructions_text_matrix_ =
      projection_matrix * eye_view * tilt_animation_.GetModelAnimation() *
      instructions_text_model_matrix * text_scale_matrix;
}

void TutorialAnimator::DisplayTutorialText() const {
  std::string press_button_string =
      cardboard::GetLocalizedString(cardboard::kStrPressTheButtonOnYourViewer,
                                    env_);
  // Break the text into 2 lines.
  size_t line_index {0};
  for (size_t i = press_button_string.length() / 2;
      i < press_button_string.length();
      i++) {
    if (press_button_string[i] == ' ') {
      line_index = i;
      press_button_string[i] = '\n';
      break;
    }
  }

  const float x_offset {
      text_renderer_->GetTextWidth(press_button_string.substr(0, line_index)) *
      0.5f};

  text_renderer_->RenderText(press_button_string,
                             model_view_projection_text_model_matrix_,
                             1.0f,  // spacing
                             -x_offset);
}

void TutorialAnimator::DisplayTiltInstructionsText() const {
  std::string rotate_viewer_string =
      cardboard::GetLocalizedString(cardboard::kStrRotateYourViewer,
                                    env_);
  // Break the text into 2 lines.
  size_t line_index {0};
  for (size_t i = rotate_viewer_string.length() / 2;
      i < rotate_viewer_string.length();
      i++) {
    if (rotate_viewer_string[i] == ' ') {
      line_index = i;
      rotate_viewer_string[i] = '\n';
      break;
    }
  }

  float x_offset {
      text_renderer_->GetTextWidth(rotate_viewer_string.substr(0, line_index)) *
      0.5f};

  text_renderer_->RenderText(
      rotate_viewer_string,
      model_view_projection_instructions_text_matrix_, 1.0f, -x_offset);

  const std::string try_it_out_now_string {
      cardboard::GetLocalizedString(cardboard::kStrTryItOutNow, env_)};
  x_offset = text_renderer_->GetTextWidth(
    try_it_out_now_string) * 0.5f;

  text_renderer_->RenderText(
    try_it_out_now_string,
      model_view_projection_instructions_text_matrix_,
      1.0f,  // spacing
      -x_offset,  // x_offset
      -3.0f);  // y_offset
}

void TutorialAnimator::DisplayButtonAnimation(GLuint program,
                                              GLuint obj_mvp_param) {
  glUseProgram(program);
  const std::array<float, 16> target_array =
      button_animation_.GetModelViewProjectionAnimation().ToGlArray();
  glUniformMatrix4fv(obj_mvp_param, 1, GL_FALSE, target_array.data());
  button_animation_.DisplayFrame(std::chrono::steady_clock::now());
  DisplayTutorialText();
  DrawNextButton();
  DrawNextButtonText();
}

void TutorialAnimator::DisplayTiltAnimation(GLuint program,
                                            GLuint obj_mvp_param) {
  glUseProgram(program);
  const std::array<float, 16> target_array =
      tilt_animation_.GetModelViewProjectionAnimation().ToGlArray();
  glUniformMatrix4fv(obj_mvp_param, 1, GL_FALSE, target_array.data());

  tilt_animation_.DisplayFrame(std::chrono::steady_clock::now());
  DisplayTiltInstructionsText();
}

void TutorialAnimator::LoadNextButton() {
  LoadNextButtonGLObjects();
  CalculateNextButtonMatrices();
}

void TutorialAnimator::LoadNextButtonGLObjects() {
  const float vertices[]{
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

  glGenBuffers(1, &next_button_background_vbo_);
  glGenBuffers(1, &next_button_background_ebo_);

  glBindBuffer(GL_ARRAY_BUFFER, next_button_background_vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  cardboard_demo::CHECKGLERROR("LoadNextButton bind next button VBO");

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, next_button_background_ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  const GLuint vertex_shader{
      cardboard_demo::LoadGLShader(GL_VERTEX_SHADER, kVertexShader)};
  const GLuint fragment_shader{
      cardboard_demo::LoadGLShader(GL_FRAGMENT_SHADER, kFragmentShader)};

  next_button_background_program_id_ = glCreateProgram();
  glAttachShader(next_button_background_program_id_, vertex_shader);
  glAttachShader(next_button_background_program_id_, fragment_shader);
  cardboard_demo::CHECKGLERROR("LoadBlackBackground attach shaders");

  glLinkProgram(next_button_background_program_id_);
  cardboard_demo::CHECKGLERROR("LoadBlackBackground link program");

  next_button_background_mvp_param_ =
      glGetUniformLocation(next_button_background_program_id_, "u_MVP");
}

void TutorialAnimator::CalculateNextButtonMatrices() {
  next_button_text_width_ =
      text_renderer_->GetTextWidth(
          cardboard::GetLocalizedString(cardboard::kStrNext, env_), 0.9f) *
      kNextButtonTextXYScale;
  next_button_text_x_offset_ = next_button_text_width_ / 2.0f;

  const float bg_width{1.0f * next_button_text_width_ * 1.3f};
  bg_scale_matrix_ = cardboard_demo::GetScaleMatrix({bg_width, 0.10f, 1.0f});

  button_text_scale_matrix_ =
      cardboard_demo::GetScaleMatrix(kNextButtonTextScale);
  button_text_model_matrix_ =
      cardboard_demo::GetTranslationMatrix({next_button_text_x_offset_,
                                            -0.52f,  // y_offset
                                            0.0f});
}

void TutorialAnimator::DrawNextButton() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glUseProgram(next_button_background_program_id_);

  const std::array<float, 16> target_array{
      model_view_projection_next_button_.ToGlArray()};
  glUniformMatrix4fv(next_button_background_mvp_param_, 1, GL_FALSE,
                     target_array.data());

  glBindBuffer(GL_ARRAY_BUFFER, next_button_background_vbo_);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, next_button_background_ebo_);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  cardboard_demo::CHECKGLERROR("DrawNextButton draw elements");

  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void TutorialAnimator::DrawNextButtonText() const {
  text_renderer_->RenderText(
      cardboard::GetLocalizedString(cardboard::kStrNext, env_),
                             model_view_projection_next_button_text_,
                             0.9f);
}

bool TutorialAnimator::IsNextClicked(
    const cardboard_demo::Matrix4x4& device_pose, float angle_limit) const {
  // Compute vectors pointing towards the reticle and towards the next button
  // in head space.
  cardboard_demo::Matrix4x4 head_from_demo =
      device_pose * button_animation_.GetModelAnimation() * model_next_button_;
  const std::array<float, 4> unit_quaternion {0.f, 0.f, 0.f, 1.f};
  const std::array<float, 4> point_vector {0.f, 0.f, -1.f, 0.f};
  const std::array<float, 4> target_vector = head_from_demo * unit_quaternion;

  const float angle{
      cardboard_demo::AngleBetweenVectors(point_vector, target_vector)};
  return angle < angle_limit;
}
}  // namespace cardboard_launcher
