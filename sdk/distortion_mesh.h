/*
 * Copyright 2019 Google LLC
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
#ifndef CARDBOARD_SDK_DISTORTION_MESH_H_
#define CARDBOARD_SDK_DISTORTION_MESH_H_

#include <vector>

#include "include/cardboard.h"
#include "polynomial_radial_distortion.h"

namespace cardboard {

class DistortionMesh {
 public:
  DistortionMesh(const PolynomialRadialDistortion& distortion,
                 // Units of the following parameters are tan-angle units.
                 float screen_width, float screen_height,
                 float x_eye_offset_screen, float y_eye_offset_screen,
                 float texture_width, float texture_height,
                 float x_eye_offset_texture, float y_eye_offset_texture);
  virtual ~DistortionMesh() = default;
  CardboardMesh GetMesh() const;

 private:
  static constexpr int kResolution = 40;
  std::vector<int> index_data_;
  std::vector<float> vertex_data_;
  std::vector<float> uvs_data_;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_DISTORTION_MESH_H_
