
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
#ifndef PAPERSCOPE_LAUNCHER_SRC_DEMO_ICON_H_
#define PAPERSCOPE_LAUNCHER_SRC_DEMO_ICON_H_

#include <math.h>

#include <cmath>
#include <string>

#include "text_renderer.h"
#include "util.h"

namespace cardboard_launcher {

struct DemoIconData {
  std::string name;
  std::string display_name;
};

class DemoIcon {
 public:
  /**
   * Constructor for a demo icon.
   *
   * @param demo_name The name of the demo.
   * @param pos The position of the demo.
   */
  DemoIcon(const cardboard_launcher::DemoIconData& demo_icon_data,
           const float pos, cardboard_demo::TextRenderer* const text_renderer)
      : demo_icon_data_{demo_icon_data}, text_renderer_{text_renderer} {
    // Rotate the demo icon and place it in front of the user.
    const float yaw{180.0f * M_PI / 180.0f};
    model_demo_ = cardboard_demo::GetTranslationMatrix({-pos, 2.2f, -3.0f})
        * cardboard_demo::GetYawRotationMatrix(yaw);
  }

  /**
   * Returns the name of the demo.
   */
  std::string GetDemoName() const { return demo_icon_data_.name; }

  /**
   * Returns the model of the demo.
   */
  cardboard_demo::Matrix4x4 GetModel() const { return model_demo_; }

  /**
   * Returns the model view projection of the demo.
   */
  cardboard_demo::Matrix4x4 GetModelViewProjection() const {
    return modelview_projection_demo_;
  }

  /**
   * Sets the model of the demo.
   *
   * @param model The model of the demo.
   */
  void SetModel(const cardboard_demo::Matrix4x4& model) { model_demo_ = model; }

  /**
   * Sets the model view projection of the demo.
   *
   * @param modelview_projection The model view projection of the demo.
   */
  void SetModelViewProjection(
      const cardboard_demo::Matrix4x4& modelview_projection) {
    modelview_projection_demo_ = modelview_projection;
  }

  /**
   * Sets whether the user is pointing at the demo.
   *
   * @param is_pointing_at_demo Whether the user is pointing at the demo.
   */
  void SetPointingAtDemo(bool is_pointing_at_demo) {
    is_pointing_at_demo_ = is_pointing_at_demo;
  }

  /**
   * Loads the demo mesh.
   */
  void LoadDemo(GLuint obj_position_param_, GLuint obj_uv_param_,
                AAssetManager* asset_mgr_, JNIEnv* env,
                jobject java_asset_mgr_);

  /**
   * Draws the demo mesh.
   */
  void Draw() const;

  void DrawDemoName() const;

 private:
  cardboard_launcher::DemoIconData demo_icon_data_;
  cardboard_demo::Matrix4x4 model_demo_;
  cardboard_demo::Matrix4x4 modelview_projection_demo_;

  cardboard_demo::TexturedMesh demo_mesh_;
  cardboard_demo::Texture demo_not_selected_texture_;
  cardboard_demo::Texture demo_selected_texture_;

  cardboard_demo::TextRenderer* const text_renderer_;

  bool is_pointing_at_demo_;
};

}  // namespace cardboard_launcher

#endif  // PAPERSCOPE_LAUNCHER_SRC_DEMO_ICON_H_
