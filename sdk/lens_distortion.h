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
#ifndef CARDBOARD_SDK_LENSDISTORTION_H_
#define CARDBOARD_SDK_LENSDISTORTION_H_

#include <array>
#include <memory>

#ifdef __ANDROID__
#include "device_params/android/device_params.h"
#else
#include "cardboard_device.pb.h"
#endif

#include "distortion_mesh.h"
#include "include/cardboard.h"
#include "polynomial_radial_distortion.h"
#include "util/matrix_4x4.h"

namespace cardboard {

class LensDistortion {
 public:
  LensDistortion(const uint8_t* encoded_device_params, int size,
                 int display_width, int display_height);
  virtual ~LensDistortion();
  // Tan angle units. "DistortedUvForUndistoredUv" goes through the forward
  // distort function. I.e. the lens. UndistortedUvForDistortedUv uses the
  // inverse distort function.
  std::array<float, 2> DistortedUvForUndistortedUv(
      const std::array<float, 2>& in, CardboardEye eye) const;
  std::array<float, 2> UndistortedUvForDistortedUv(
      const std::array<float, 2>& in, CardboardEye eye) const;
  void GetEyeFromHeadMatrix(CardboardEye eye,
                            float* eye_from_head_matrix) const;
  void GetEyeProjectionMatrix(CardboardEye eye, float z_near, float z_far,
                              float* projection_matrix) const;
  void GetEyeFieldOfView(CardboardEye eye, float* field_of_view) const;
  CardboardMesh GetDistortionMesh(CardboardEye eye) const;
 private:
  struct ViewportParams;

  void UpdateParams();
  static float GetYEyeOffsetMeters(const DeviceParams& device_params,
                                   float screen_height_meters);
  static DistortionMesh* CreateDistortionMesh(
      CardboardEye eye, const cardboard::DeviceParams& device_params,
      const cardboard::PolynomialRadialDistortion& distortion,
      const std::array<float, 4>& fov, float screen_width_meters,
      float screen_height_meters);
  static std::array<float, 4> CalculateFov(
      const cardboard::DeviceParams& device_params,
      const cardboard::PolynomialRadialDistortion& distortion,
      float screen_width_meters, float screen_height_meters);
  static void CalculateViewportParameters(CardboardEye eye,
                                          const DeviceParams& device_params,
                                          const std::array<float, 4>& fov,
                                          float screen_width_meters,
                                          float screen_height_meters,
                                          ViewportParams* screen_params,
                                          ViewportParams* texture_params);
  static constexpr float DegreesToRadians(float angle);

  DeviceParams device_params_;

  float screen_width_meters_;
  float screen_height_meters_;
  std::array<std::array<float, 4>, 2> fov_;  // L, R, B, T
  std::array<Matrix4x4, 2> eye_from_head_matrix_;
  std::unique_ptr<DistortionMesh> left_mesh_;
  std::unique_ptr<DistortionMesh> right_mesh_;
  std::unique_ptr<PolynomialRadialDistortion> distortion_;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_LENSDISTORTION_H_
