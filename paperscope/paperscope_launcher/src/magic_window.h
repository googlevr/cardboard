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
#ifndef PAPERSCOPE_LAUNCHER_SRC_MAGIC_WINDOW_H_
#define PAPERSCOPE_LAUNCHER_SRC_MAGIC_WINDOW_H_

#include <android/asset_manager.h>
#include <jni.h>

#include <GLES2/gl2.h>
#include "cardboard.h"
#include "util.h"

namespace magic_window {

/**
 * This is a magic window of the sample app for the Cardboard SDK. It shows a
 * full-screen monoscopic view of a sample VR world.
 */
class MagicWindow {
 public:
  /**
   * Creates the Magic Window of the sample app.
   *
   * @param vm JavaVM pointer.
   * @param obj Android activity object.
   * @param asset_mgr_obj The asset manager object.
   */
  MagicWindow(JavaVM* vm, jobject obj, jobject asset_mgr_obj);

  ~MagicWindow();

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
   * Resumes head tracking.
   */
  void OnResume();

  /**
   * Pauses head tracking.
   */
  void OnPause();

  /**
  * Allows user to scan a Cardboard viewer.
  */
  void ScanViewer();

  /**
  * Checks if a Cardboard viewer has previously been scanned and its parameters
  * saved.
  */
  bool HasSavedDeviceParams();

 private:
  /**
   * Default near clip plane z-axis coordinate.
   */
  static constexpr float kZNear = 0.1f;

  /**
   * Default far clip plane z-axis coordinate.
   */
  static constexpr float kZFar = 100.f;

  /**
   * Default vertical FOV set to 45 degrees.
   */
  static constexpr float kFov_v = 0.785398f;

  /**
   * Gets head's pose as a 4x4 matrix.
   *
   * @return matrix containing head's pose.
   */
  cardboard_demo::Matrix4x4 GetPose();

  /**
   * Initializes the FOV settings for the Magic Window.
   */
  void SetFov();

  /**
   * Draws the background.
   */
  void DrawBackground();

  jobject java_asset_mgr_;
  AAssetManager* asset_mgr_;

  CardboardHeadTracker* head_tracker_;

  int screen_width_;
  int screen_height_;

  float magic_window_projection_matrix_[16];

  GLuint obj_program_;
  GLuint obj_model_view_projection_param_;

  cardboard_demo::Matrix4x4 model_view_projection_background_;
  cardboard_demo::TexturedMesh background_;
  cardboard_demo::Texture background_tex_;
};

}  // namespace magic_window

#endif  // PAPERSCOPE_LAUNCHER_SRC_MAGIC_WINDOW_H_
