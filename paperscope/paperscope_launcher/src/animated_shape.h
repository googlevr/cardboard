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
#ifndef PAPERSCOPE_LAUNCHER_SRC_ANIMATED_SHAPE_H_
#define PAPERSCOPE_LAUNCHER_SRC_ANIMATED_SHAPE_H_

#include <android/asset_manager.h>
#include <jni.h>

#include <chrono>  // NOLINT(build/c++11)
#include <vector>

#include <GLES2/gl2.h>
#include "util.h"

namespace cardboard_launcher {

// An animated shape is a renderable that renders a series of images in order
// to create frame-by-frame animations.
class AnimatedShape {
 public:
  AnimatedShape() = default;

  /**
   * Creates an AnimatedShape from a vector of animation frames.
   *
   * @param animation_frames A vector of animation frames.
   */
  AnimatedShape(const std::vector<std::string>& animation_frames)
      : animation_frames_(animation_frames),
        last_update_time_(std::chrono::steady_clock::now()),
        animation_index_(0),
        animation_meshes_(animation_frames.size()),
        animation_textures_(animation_frames.size()) {}

  /**
   * Gets the model matrix for the animation.
   *
   * @return The model matrix for the animation.
   */
  cardboard_demo::Matrix4x4 GetModelAnimation() const {
    return model_animation_;
  }

  /**
   * Gets the modelview projection matrix for the animation.
   *
   * @return The modelview projection matrix for the animation.
   */
  cardboard_demo::Matrix4x4 GetModelViewProjectionAnimation() const {
    return model_view_projection_animation_;
  }
  /**
   * Sets the modelview projection matrix for the animation.
   *
   * @param eye_view The eye view matrix.
   * @param projection_matrix The projection matrix.
   */
  void SetModelViewProjection(
      const cardboard_demo::Matrix4x4& eye_view,
      const cardboard_demo::Matrix4x4& projection_matrix);

  /**
   * Loads the animations from the given asset manager.
   *
   * @param obj_position_param The object position parameter.
   * @param obj_uv_param The object uv parameter.
   * @param asset_mgr The asset manager.
   * @param env The JNI environment.
   * @param java_asset_mgr The Java asset manager.
   */
  void LoadAnimations(GLuint obj_position_param_, GLuint obj_uv_param_,
                      AAssetManager* asset_mgr_, JNIEnv* env,
                      jobject java_asset_mgr_);

  /**
   * Prepares the animation frame for the current timestamp and updates the
   * animation index if necessary.
   *
   * @param timestamp The current timestamp.
   */
  void DisplayFrame(std::chrono::steady_clock::time_point timestamp);

  // Initializes the animated shape.
  void Init();

 private:
  // List of animation frames.
  std::vector<std::string> animation_frames_;
  // Last time the animation was updated.
  std::chrono::steady_clock::time_point last_update_time_;
  // Index of the current animation.
  int animation_index_;

  // Model and modelview projection matrices for the animation.
  cardboard_demo::Matrix4x4 model_animation_;
  cardboard_demo::Matrix4x4 model_view_projection_animation_;
  std::vector<cardboard_demo::TexturedMesh> animation_meshes_;
  std::vector<cardboard_demo::Texture> animation_textures_;

  // JNI environment and Java asset manager for reloading textures.
  JNIEnv* env_ = nullptr;
  jobject java_asset_mgr_ = nullptr;
};

}  // namespace cardboard_launcher

#endif  // PAPERSCOPE_LAUNCHER_SRC_ANIMATED_SHAPE_H_
