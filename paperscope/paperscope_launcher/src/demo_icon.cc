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
#include "demo_icon.h"

#include <android/log.h>

#include "util.h"

namespace cardboard_launcher {

namespace {
constexpr float kTextScale{0.002f};
constexpr std::array<float, 3> kTextTutorialScale{-kTextScale,
                                                  kTextScale,
                                                  1.0f};
const cardboard_demo::Matrix4x4 kTextScaleMatrix{
      cardboard_demo::GetScaleMatrix(kTextTutorialScale)};
constexpr float kTextYOffset{-0.3f};
}  // namespace

void DemoIcon::LoadDemo(GLuint obj_position_param, GLuint obj_uv_param,
                        AAssetManager* asset_mgr, JNIEnv* env,
                        jobject java_asset_mgr) {
  CARDBOARDDEMO_CHECK(demo_mesh_.Initialize(obj_position_param, obj_uv_param,
                                            "IconMesh.obj", asset_mgr));
  CARDBOARDDEMO_CHECK(demo_not_selected_texture_.Initialize(
      env, java_asset_mgr, demo_icon_data_.name + "-bw.png"));
  CARDBOARDDEMO_CHECK(demo_selected_texture_.Initialize(
      env, java_asset_mgr, demo_icon_data_.name + "-color.png"));
}

void DemoIcon::Draw() const {
  if (is_pointing_at_demo_) {
    demo_selected_texture_.Bind();
  } else {
    demo_not_selected_texture_.Bind();
  }
  demo_mesh_.Draw();

  if (is_pointing_at_demo_) {
    DrawDemoName();
  }

  cardboard_demo::CHECKGLERROR("DrawDemoExhibit");
}

void DemoIcon::DrawDemoName() const {
  const float x_offset{
      text_renderer_->GetTextWidth(demo_icon_data_.display_name) * 0.5f};

  cardboard_demo::Matrix4x4 translation_matrix{
      cardboard_demo::GetTranslationMatrix({x_offset * kTextScale,
                                              kTextYOffset,
                                              0.0f})};

  const cardboard_demo::Matrix4x4 model_view_projection =
      GetModelViewProjection() * translation_matrix * kTextScaleMatrix;
  text_renderer_->RenderText(demo_icon_data_.display_name,
                             model_view_projection);
}
}  // namespace cardboard_launcher
