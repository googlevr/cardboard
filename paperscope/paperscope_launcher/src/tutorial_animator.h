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
#ifndef PAPERSCOPE_LAUNCHER_SRC_TUTORIAL_ANIMATOR_H_
#define PAPERSCOPE_LAUNCHER_SRC_TUTORIAL_ANIMATOR_H_

#include <android/asset_manager.h>
#include <jni.h>

#include <string>
#include <vector>

#include <GLES2/gl2.h>
#include "animated_shape.h"
#include "text_renderer.h"
#include "util.h"

namespace cardboard_launcher {

// Handles all of the logic related to the Cardboard Demo App Tutorial.
class TutorialAnimator {
 public:
  /**
   * Creates a TutorialAnimator from vectors of button animation frames and
   * tilt animation frames.
   *
   * @param button_animation_files A vector of button animation frames.
   * @param tilt_animation_files A vector of tilt animation frames.
   * @param device_pose pointer to the device pose. Used to determine when the
   * device is in Portrait orientation.
   */
  TutorialAnimator(const std::vector<std::string>& button_animation_files,
                   const std::vector<std::string>& tilt_animation_files,
                   cardboard_demo::TextRenderer* const text_renderer);

  /**
   * Loads the animation files for both the button and tilt animations.
   *
   * @param obj_position_param The object position parameter.
   * @param obj_uv_param The object uv parameter.
   * @param asset_mgr The asset manager.
   * @param env The JNI environment.
   * @param java_asset_mgr The Java asset manager.
   */
  void LoadAnimations(GLuint obj_position_param, GLuint obj_uv_param,
                      AAssetManager* asset_mgr, JNIEnv* env,
                      jobject java_asset_mgr);

  /**
   * Sets the position of the animation on screen.
   *
   * @param eye_view The eye view matrix.
   * @param projection_matrix The projection matrix.
   */
  void SetScreenPosition(const cardboard_demo::Matrix4x4& eye_view,
                         const cardboard_demo::Matrix4x4& projection_matrix);

  /**
   * Draw the current frame of the animation to be drawn on screen.
   *
   * @param program The OpenGL program.
   * @param obj_mvp_param The OpenGL MVP matrix.
   */
  void DisplayButtonAnimation(GLuint program, GLuint obj_mvp_param);

  /**
   * Draw the current frame of the animation to be drawn on screen.
   *
   * @param program The OpenGL program.
   * @param obj_mvp_param The OpenGL MVP matrix.
   */
  void DisplayTiltAnimation(GLuint program, GLuint obj_mvp_param);

  /**
   * Displays the tutorial text on screen.
   */
  void DisplayTutorialText() const;

  /**
   * Displays the tilt instructions on screen.
   */
  void DisplayTiltInstructionsText() const;

  /**
   * Loads the assets for the "Next" button.
   */
  void LoadNextButton();

  /**
   * Loads the OpenGL objects for the "Next" button background.
   */
  void LoadNextButtonGLObjects();

  /**
   * Calculates the matrices needed for the "Next" button background
   * and text to be drawn on screen.
   */
  void CalculateNextButtonMatrices();

  /**
   * Draws the "Next" button background.
   */
  void DrawNextButton();

  /**
   * Draws the text for the "Next" button.
   */
  void DrawNextButtonText() const;

  /**
   * Indicates whether the Next button has been clicked.
   * @param device_pose The device pose.
   * @param angle_limit Angle threshold for determining whether the controller
   * is pointing at the next button.
   */
  bool IsNextClicked(const cardboard_demo::Matrix4x4& device_pose,
                     float angle_limit) const;

  /**
   * Sets the JNI environment.
   * @param env The JNI environment.
   */
  void SetJNIEnv(JNIEnv* env) { env_ = env; }

 private:
  AnimatedShape button_animation_;
  AnimatedShape tilt_animation_;
  cardboard_demo::TextRenderer* const text_renderer_;
  JNIEnv* env_;

  cardboard_demo::Matrix4x4 model_next_button_;
  cardboard_demo::Matrix4x4 model_view_projection_next_button_;
  cardboard_demo::Matrix4x4 model_view_projection_next_button_text_;
  cardboard_demo::Matrix4x4 model_view_projection_text_model_matrix_;
  cardboard_demo::Matrix4x4 model_view_projection_instructions_text_matrix_;

  cardboard_demo::Matrix4x4 bg_scale_matrix_;

  float next_button_text_width_{};
  float next_button_text_x_offset_{};
  cardboard_demo::Matrix4x4 button_text_scale_matrix_;
  cardboard_demo::Matrix4x4 button_text_model_matrix_;

  GLuint next_button_background_vbo_{};
  GLuint next_button_background_ebo_{};
  GLuint next_button_background_program_id_{};
  GLuint next_button_background_mvp_param_{};
};

}  // namespace cardboard_launcher
#endif  // PAPERSCOPE_LAUNCHER_SRC_TUTORIAL_ANIMATOR_H_
