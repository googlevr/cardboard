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
#ifndef CARDBOARD_SDK_UTIL_MATRIX_4X4_H_
#define CARDBOARD_SDK_UTIL_MATRIX_4X4_H_

#include <array>

namespace cardboard {

// The Matrix4x4 class defines a square 4-dimensional matrix. Elements are
// stored in row-major order.
class Matrix4x4 {
 public:
  // @brief Constructs an identity matrix.
  // @returns An identity matrix.
  static Matrix4x4 Identity();

  // @brief Constructs an all zeros matrix.
  // @returns A zero matrix.
  static Matrix4x4 Zeros();

  // @brief Constructs a translation matrix from [@p x, @p y, @p z] position.
  // @param x The x position coordinate.
  // @param y The y position coordinate.
  // @param z The z position coordinate.
  // @returns A translation matrix.
  static Matrix4x4 Translation(float x, float y, float z);

  // @brief Constructs a projection matrix from the field of view half angles
  //        and the z-coordinate of the near and far clipping planes.
  // @param fov An array with the half angles of the field of view.
  // @param zNear The z coordinate of the near clipping plane.
  // @param zFar The z coordinate of the far clipping plane.
  // @return A projection matrix.
  static Matrix4x4 Perspective(const std::array<float, 4>& fov, float zNear,
                               float zFar);

  // @brief Copies into @p array the contents of `this` matrix.
  // @param[out] array A pointer to a float array of size 16.
  void ToArray(float* array) const;

 private:
  std::array<std::array<float, 4>, 4> m;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_UTIL_MATRIX4X4_H_
