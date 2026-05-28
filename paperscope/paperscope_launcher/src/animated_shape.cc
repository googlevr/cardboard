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
#include "animated_shape.h"

#include <android/log.h>

#include <chrono>  // NOLINT(build/c++11)
#include "util.h"

namespace cardboard_launcher {

// Duration of each frame in the animation.
const std::chrono::milliseconds kAnimationFrameDuration(67);

void AnimatedShape::SetModelViewProjection(
    const cardboard_demo::Matrix4x4& eye_view,
    const cardboard_demo::Matrix4x4& projection_matrix) {
  cardboard_demo::Matrix4x4 model_view_tutorial = eye_view * model_animation_;
  model_view_projection_animation_ = projection_matrix * model_view_tutorial;
}

void AnimatedShape::LoadAnimations(GLuint obj_position_param_,
                                   GLuint obj_uv_param_,
                                   AAssetManager* asset_mgr_, JNIEnv* env,
                                   jobject java_asset_mgr) {
  env_ = env;
  java_asset_mgr_ = java_asset_mgr;
  for (int i = 0; i < static_cast<int>(animation_frames_.size()); ++i) {
    CARDBOARDDEMO_CHECK(animation_meshes_[i].Initialize(
        obj_position_param_, obj_uv_param_, "IconMesh.obj", asset_mgr_));
    CARDBOARDDEMO_CHECK(animation_textures_[i].Initialize(
        env, java_asset_mgr_, animation_frames_[i]));
  }

  // Z- points forward, set the animation to be in front of the user,
  // facing the user.
  float yaw = 180.0f * M_PI / 180.0f;  // rotate 180 degrees.
  model_animation_ = cardboard_demo::GetTranslationMatrix({0, 2.2f, -1.5}) *
      cardboard_demo::GetYawRotationMatrix(yaw);
  cardboard_demo::CHECKGLERROR("LoadAnimations");
}

void AnimatedShape::Init() {
  for (auto& texture : animation_textures_) {
    texture.set_texture_id(0);
  }
}

void AnimatedShape::DisplayFrame(
    const std::chrono::steady_clock::time_point timestamp) {
  if (!animation_textures_[animation_index_].get_texture_id() ||
      !glIsTexture(animation_textures_[animation_index_].get_texture_id())) {
    CARDBOARDDEMO_CHECK(animation_textures_[animation_index_].Initialize(
        env_, java_asset_mgr_, animation_frames_[animation_index_]));
  }
  animation_textures_[animation_index_].Bind();
  animation_meshes_[animation_index_].Draw();

  if (!animation_frames_.empty()) {
    if (timestamp - last_update_time_ >= kAnimationFrameDuration) {
      animation_index_ = (animation_index_ + 1) % animation_frames_.size();
      last_update_time_ = std::chrono::steady_clock::now();
    }
  }
  cardboard_demo::CHECKGLERROR("DisplayFrame");
}

}  // namespace cardboard_launcher
