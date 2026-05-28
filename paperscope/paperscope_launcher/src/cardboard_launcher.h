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
#ifndef PAPERSCOPE_LAUNCHER_SRC_CARDBOARD_LAUNCHER_H_
#define PAPERSCOPE_LAUNCHER_SRC_CARDBOARD_LAUNCHER_H_

#include <android/asset_manager.h>
#include <jni.h>

#include <vector>

#include <GLES2/gl2.h>
#include "cardboard.h"
#include "demo_icon.h"
#include "text_renderer.h"
#include "tutorial_animator.h"
#include "util.h"

namespace cardboard_launcher {

/**
 * This is a sample app for the Cardboard SDK. It loads a simple environment and
 * objects that you can click on.
 */
class CardboardLauncher {
 public:
  /**
   * Creates a DemoCardboardDemo.
   *
   * @param vm JavaVM pointer.
   * @param obj Android activity object.
   * @param asset_mgr_obj The asset manager object.
   */
  CardboardLauncher(JavaVM* vm, jobject obj, jobject asset_mgr_obj);

  ~CardboardLauncher();

  /**
   * Initializes any GL-related objects. This should be called on the rendering
   * thread with a valid GL context.
   *
   * @param env The JNI environment.
   */
  void OnSurfaceCreated(JNIEnv* env);

  /**
   * Sets screen parameters.
   *
   * @param width Screen width
   * @param height Screen height
   */
  void SetScreenParams(int width, int height);

  /**
   * Draws the scene. This should be called on the rendering thread.
   */
  void OnDrawFrame();

  /**
   * Hides the target object if it's being targeted.
   */
  void OnTriggerEvent();

  /**
   * Pauses head tracking.
   */
  void OnPause();

  /**
   * Resumes head tracking.
   */
  void OnResume();

  /**
   * Allows user to switch viewer.
   */
  void SwitchViewer();

  /**
   *  Checks if the demo should be launched.
   */
  bool ShouldLaunchDemo();

  /**
   *  Forwards the demo name to launch.
   */
  std::string DemoToLaunch();

  /**
   *  Resets the should_launch_demo flag after the demo is launched.
   */
  void DemoLaunched();

 private:
  /**
   * Current state of the launcher.
   */
  enum class State { SHOW_DEMOS, BUTTON_TUTORIAL, TILT_TUTORIAL };

  /**
   * State events of the launcher.
   */
  enum class StateEvent {
    TUTORIAL_SELECTED,
    TUTORIAL_NEXT_SELECTED,
    DEVICE_IN_PORTRAIT,
    NONE
  };

  /**
   * Default near clip plane z-axis coordinate.
   */
  static constexpr float kZNear = 0.1f;

  /**
   * Default far clip plane z-axis coordinate.
   */
  static constexpr float kZFar = 100.f;

  /**
   * Updates device parameters, if necessary.
   *
   * @return true if device parameters were successfully updated.
   */
  bool UpdateDeviceParams();

  /**
   * Initializes GL environment.
   */
  void GlSetup();

  /**
   * Deletes GL environment.
   */
  void GlTeardown();

  /**
   * Gets head's pose as a 4x4 matrix.
   *
   * @return matrix containing head's pose.
   */
  cardboard_demo::Matrix4x4 GetPose() const;

  /**
   * Draws all world-space objects for the given eye.
   */
  void DrawWorld();

  /**
   * Draws the background.
   */
  void DrawBackground();

  /**
   * Draws once demo.
   */
  void DrawDemos();

  /**
   * Checks if user is pointing or looking at the demo mesh by calculating
   * whether the angle between the user's gaze and the vector pointing towards
   * the mesh is lower than some threshold.
   *
   * @return true if the user is pointing at the demo mesh.
   */
  bool IsPointingAtDemo(const cardboard_demo::Matrix4x4& model_demo);

  /**
   * Checks if user is pointing or looking at the next button by calculating
   * whether the angle between the user's gaze and the vector pointing towards
   * the button is lower than some threshold.
   *
   * @return true if the user is pointing at the next button.
   */
  bool IsPointingAtNextButton();

  /**
   * Loads the demo meshes.
   *
   * @param env The JNI environment.
   */
  void LoadDemos(JNIEnv* env);

  /**
   * Loads the tutorial animations.
   *
   * @param env The JNI environment.
   */
  void LoadTutorial(JNIEnv* env);

  /**
   * Shows the button tutorial.
   */
  void ShowButtonTutorial();

  /**
   * Updates the state of the launcher state machine.
   */
  void UpdateState();

  /**
   * Loads the black background and its parameters.
   */
  void LoadBlackBackground();

  /**
   * Draws the black background with the given scale.
   *
   * @param scale The scale of the black background.
   */
  void DrawBlackBackgroundGL(const std::array<float, 3>& scale);

  jobject java_asset_mgr_;
  AAssetManager* asset_mgr_;

  CardboardHeadTracker* head_tracker_;
  CardboardLensDistortion* lens_distortion_;
  CardboardDistortionRenderer* distortion_renderer_;

  CardboardEyeTextureDescription left_eye_texture_description_;
  CardboardEyeTextureDescription right_eye_texture_description_;

  bool screen_params_changed_;
  bool device_params_changed_;
  int screen_width_;
  int screen_height_;

  float projection_matrices_[2][16];
  float eye_matrices_[2][16];

  GLuint depthRenderBuffer_;  // depth buffer
  GLuint framebuffer_;        // framebuffer object
  GLuint texture_;            // distortion texture

  GLuint obj_program_;
  GLuint obj_position_param_;
  GLuint obj_uv_param_;
  GLuint obj_model_view_projection_param_;

  cardboard_demo::Matrix4x4 head_view_;

  std::vector<DemoIcon> demos_;

  cardboard_demo::Matrix4x4 model_view_projection_background_;

  cardboard_demo::TexturedMesh background_;
  cardboard_demo::Texture background_tex_;

  // Indicates whether to launch the demo.
  bool should_launch_demo_;

  // Indicates the demo to launch.
  std::string demo_to_launch_;

  // Indicates the current state of the launcher.
  State state_;
  // Indicates the last state event of the launcher.
  StateEvent last_state_event_;

  // Tutorial animations.
  TutorialAnimator tutorial_animator_;

  GLuint black_background_vbo_;
  GLuint black_background_ebo_;
  GLuint black_background_program_id_;
  GLuint black_background_mvp_param_;
  GLuint black_background_scale_param_;

  cardboard_demo::Matrix4x4 model_view_projection_black_square_;
  cardboard_demo::TextRenderer text_renderer_;
};

}  // namespace cardboard_launcher

#endif  // PAPERSCOPE_LAUNCHER_SRC_CARDBOARD_LAUNCHER_H_
