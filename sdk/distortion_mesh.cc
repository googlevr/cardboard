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
#include "distortion_mesh.h"

#include <vector>

#include "include/cardboard.h"

namespace cardboard {

DistortionMesh::DistortionMesh(
    const PolynomialRadialDistortion& distortion,
    // Units of the following parameters are tan-angle units.
    float screen_width, float screen_height, float x_eye_offset_screen,
    float y_eye_offset_screen, float texture_width, float texture_height,
    float x_eye_offset_texture, float y_eye_offset_texture) {
  vertex_data_.resize(kResolution * kResolution *
                      2);                           // 2 components per vertex
  uvs_data_.resize(kResolution * kResolution * 2);  // 2 components per uv
  float u_screen, v_screen, u_texture, v_texture;
  std::array<float, 2> p_texture;
  std::array<float, 2> p_screen;
  for (int row = 0; row < kResolution; row++) {
    for (int col = 0; col < kResolution; col++) {
      // Note that we warp the mesh vertices using the inverse of
      // the distortion function instead of warping the texture
      // coordinates by the distortion function so that the mesh
      // exactly covers the screen area that gets rendered to.
      // Helps avoid visible aliasing in the vignette.
      u_texture = (static_cast<float>(col) / (kResolution - 1));
      v_texture = (static_cast<float>(row) / (kResolution - 1));

      // texture position & radius relative to eye center in meters - I believe
      // this is tanangle
      p_texture[0] = u_texture * texture_width - x_eye_offset_texture;
      p_texture[1] = v_texture * texture_height - y_eye_offset_texture;

      p_screen = distortion.DistortInverse(p_texture);

      u_screen = (p_screen[0] + x_eye_offset_screen) / screen_width;
      v_screen = (p_screen[1] + y_eye_offset_screen) / screen_height;

      const int index = (row * kResolution + col) * 2;

      vertex_data_[index + 0] = 2 * u_screen - 1;
      vertex_data_[index + 1] = 2 * v_screen - 1;
      uvs_data_[index + 0] = u_texture;
      uvs_data_[index + 1] = v_texture;
    }
  }

  // Strip method described at:
  // http://dan.lecocq.us/wordpress/2009/12/25/triangle-strip-for-grids-a-construction/
  //
  // For a grid with 4 rows and 4 columns of vertices, the strip would
  // look like:
  //
  //     0  -  1  -  2  -  3
  //     ↓  ↗  ↓  ↗  ↓  ↗  ↓
  //     4  -  5  -  6  -  7 ↺
  //     ↓  ↖  ↓  ↖  ↓  ↖  ↓
  //   ↻ 8  -  9  - 10  - 11
  //     ↓  ↗  ↓  ↗  ↓  ↗  ↓
  //    12  - 13  - 14  - 15
  //
  // Note the little circular arrows next to 7 and 8 that indicate
  // repeating that vertex once so as to produce degenerate triangles.

  // Number of indices:
  //   1 vertex per triangle
  //   2 triangles per quad
  //   (rows - 1) * (cols - 1) quads
  //   2 vertices at the start of each row for the first triangle
  //   1 extra vertex per row (except first and last) for a
  //     degenerate triangle
  const int n_indices = 2 * (kResolution - 1) * kResolution + (kResolution - 2);
  index_data_.resize(n_indices);
  int index_offset = 0;
  int vertex_offset = 0;
  for (int row = 0; row < kResolution - 1; row++) {
    if (row > 0) {
      index_data_[index_offset] = index_data_[index_offset - 1];
      index_offset++;
    }
    for (int col = 0; col < kResolution; col++) {
      if (col > 0) {
        if (row % 2 == 0) {
          // Move right on even rows.
          vertex_offset++;
        } else {
          // Move left on odd rows.
          vertex_offset--;
        }
      }
      index_data_[index_offset++] = vertex_offset;
      index_data_[index_offset++] = vertex_offset + kResolution;
    }
    vertex_offset = vertex_offset + kResolution;
  }
}

CardboardMesh DistortionMesh::GetMesh() const {
  CardboardMesh mesh;
  mesh.indices = const_cast<int*>(index_data_.data());
  mesh.vertices = const_cast<float*>(vertex_data_.data());
  mesh.uvs = const_cast<float*>(uvs_data_.data());
  mesh.n_indices = static_cast<int>(index_data_.size());
  mesh.n_vertices = static_cast<int>(vertex_data_.size() / 2);
  return mesh;
}

}  // namespace cardboard
