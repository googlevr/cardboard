/*
 * Copyright 2019 Google Inc. All Rights Reserved.
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
#include "lens_distortion.h"

#include "include/cardboard.h"
#include "screen_params.h"
#include "util/logging.h"

namespace cardboard {

constexpr float kDefaultBorderSizeMeters = 0.003f;

// All values in tanangle units.
struct LensDistortion::ViewportParams {
  float width;
  float height;
  float x_eye_offset;
  float y_eye_offset;
};

LensDistortion::LensDistortion(const uint8_t* encoded_device_params, int size,
                               int display_width, int display_height) {
  device_params_.ParseFromArray(encoded_device_params, size);

  eye_from_head_matrix_[kLeft] = cardboard::Matrix4x4::Translation(
      device_params_.inter_lens_distance() * 0.5f, 0.f, 0.f);
  eye_from_head_matrix_[kRight] = cardboard::Matrix4x4::Translation(
      -device_params_.inter_lens_distance() * 0.5f, 0.f, 0.f);

  std::vector<float> distortion_coefficients(
      device_params_.distortion_coefficients().data(),
      device_params_.distortion_coefficients().data() +
          device_params_.distortion_coefficients_size());
  distortion_ = std::unique_ptr<PolynomialRadialDistortion>(
      new PolynomialRadialDistortion(distortion_coefficients));

  screen_params::getScreenSizeInMeters(display_width, display_height,
                                       &screen_width_meters_,
                                       &screen_height_meters_);
  UpdateParams();
}

LensDistortion::~LensDistortion() {}

void LensDistortion::GetEyeMatrices(float* projection_matrix,
                                    float* eye_from_head_matrix,
                                    CardboardEye eye) const {
  this->eye_from_head_matrix_[eye].ToArray(eye_from_head_matrix);
  this->projection_matrix_[eye].ToArray(projection_matrix);
}

CardboardMesh LensDistortion::GetDistortionMesh(CardboardEye eye) const {
  return eye == kLeft ? left_mesh_->GetMesh() : right_mesh_->GetMesh();
}

void LensDistortion::UpdateParams() {
  fov_[kLeft] = CalculateFov(device_params_, *distortion_, screen_width_meters_,
                             screen_height_meters_);
  // Mirror fov for right eye.
  fov_[kRight] = fov_[kLeft];
  fov_[kRight][0] = fov_[kLeft][1];
  fov_[kRight][1] = fov_[kLeft][0];

  projection_matrix_[kLeft] = Matrix4x4::Perspective(fov_[kLeft], 0.1f, 100.f);
  projection_matrix_[kRight] =
      Matrix4x4::Perspective(fov_[kRight], 0.1f, 100.f);

  left_mesh_ = std::unique_ptr<DistortionMesh>(
      CreateDistortionMesh(kLeft, device_params_, *distortion_, fov_[kLeft],
                           screen_width_meters_, screen_height_meters_));
  right_mesh_ = std::unique_ptr<DistortionMesh>(
      CreateDistortionMesh(kRight, device_params_, *distortion_, fov_[kRight],
                           screen_width_meters_, screen_height_meters_));
}

std::array<float, 2> LensDistortion::DistortedUvForUndistortedUv(
    const std::array<float, 2>& in, CardboardEye eye) const {
  if (screen_width_meters_ == 0 || screen_height_meters_ == 0) {
    return {0, 0};
  }

  ViewportParams screen_params, texture_params;

  CalculateViewportParameters(eye, device_params_, fov_[eye],
                              screen_width_meters_, screen_height_meters_,
                              &screen_params, &texture_params);

  // Convert input from normalized [0, 1] screen coordinates to eye-centered
  // tanangle units.
  std::array<float, 2> undistorted_uv_tanangle = {
      in[0] * screen_params.width - screen_params.x_eye_offset,
      in[1] * screen_params.height - screen_params.y_eye_offset};

  std::array<float, 2> distorted_uv_tanangle =
      distortion_->Distort(undistorted_uv_tanangle);

  // Convert output from tanangle units to normalized [0, 1] pre distort texture
  // space.
  return {(distorted_uv_tanangle[0] + texture_params.x_eye_offset) /
              texture_params.width,
          (distorted_uv_tanangle[1] + texture_params.y_eye_offset) /
              texture_params.height};
}

std::array<float, 2> LensDistortion::UndistortedUvForDistortedUv(
    const std::array<float, 2>& in, CardboardEye eye) const {
  if (screen_width_meters_ == 0 || screen_height_meters_ == 0) {
    return {0, 0};
  }

  ViewportParams screen_params, texture_params;

  CalculateViewportParameters(eye, device_params_, fov_[eye],
                              screen_width_meters_, screen_height_meters_,
                              &screen_params, &texture_params);

  // Convert input from normalized [0, 1] pre distort texture space to
  // eye-centered tanangle units.
  std::array<float, 2> distorted_uv_tanangle = {
      in[0] * texture_params.width - texture_params.x_eye_offset,
      in[1] * texture_params.height - texture_params.y_eye_offset};

  std::array<float, 2> undistorted_uv_tanangle =
      distortion_->DistortInverse(distorted_uv_tanangle);

  // Convert output from tanangle units to normalized [0, 1] screen coordinates.
  return {(undistorted_uv_tanangle[0] + screen_params.x_eye_offset) /
              screen_params.width,
          (undistorted_uv_tanangle[1] + screen_params.y_eye_offset) /
              screen_params.height};
}

std::array<float, 4> LensDistortion::CalculateFov(
    const DeviceParams& device_params,
    const PolynomialRadialDistortion& distortion, float screen_width_meters,
    float screen_height_meters) {
  // This in in degrees.
  std::array<float, 4> device_fov = {
      device_params.left_eye_field_of_view_angles(0),
      device_params.left_eye_field_of_view_angles(1),
      device_params.left_eye_field_of_view_angles(2),
      device_params.left_eye_field_of_view_angles(3),
  };

  const float eye_to_screen_distance = device_params.screen_to_lens_distance();
  const float outer_distance =
      (screen_width_meters - device_params.inter_lens_distance()) / 2.0f;
  const float inner_distance = device_params.inter_lens_distance() / 2.0f;
  const float bottom_distance =
      GetYEyeOffsetMeters(device_params, screen_height_meters);
  const float top_distance = screen_height_meters - bottom_distance;

  const float outer_angle =
      atan(
          distortion.Distort({outer_distance / eye_to_screen_distance, 0})[0]) *
      180.0f / M_PI;
  const float inner_angle =
      atan(
          distortion.Distort({inner_distance / eye_to_screen_distance, 0})[0]) *
      180.0f / M_PI;
  const float bottom_angle =
      atan(distortion.Distort(
          {0, bottom_distance / eye_to_screen_distance})[1]) *
      180.0f / M_PI;
  const float top_angle =
      atan(distortion.Distort({0, top_distance / eye_to_screen_distance})[1]) *
      180.0f / M_PI;

  return {
      std::min(outer_angle, device_fov[0]),
      std::min(inner_angle, device_fov[1]),
      std::min(bottom_angle, device_fov[2]),
      std::min(top_angle, device_fov[3]),
  };
}

float LensDistortion::GetYEyeOffsetMeters(const DeviceParams& device_params,
                                          float screen_height_meters) {
  switch (device_params.vertical_alignment()) {
    case DeviceParams::CENTER:
    default:
      return screen_height_meters / 2.0f;
    case DeviceParams::BOTTOM:
      return device_params.tray_to_lens_distance() - kDefaultBorderSizeMeters;
    case DeviceParams::TOP:
      return screen_height_meters - device_params.tray_to_lens_distance() -
             kDefaultBorderSizeMeters;
  }
}

DistortionMesh* LensDistortion::CreateDistortionMesh(
    CardboardEye eye, const DeviceParams& device_params,
    const PolynomialRadialDistortion& distortion,
    const std::array<float, 4>& fov, float screen_width_meters,
    float screen_height_meters) {
  ViewportParams screen_params, texture_params;

  CalculateViewportParameters(eye, device_params, fov, screen_width_meters,
                              screen_height_meters, &screen_params,
                              &texture_params);

  return new DistortionMesh(distortion, screen_params.width,
                            screen_params.height, screen_params.x_eye_offset,
                            screen_params.y_eye_offset, texture_params.width,
                            texture_params.height, texture_params.x_eye_offset,
                            texture_params.y_eye_offset);
}

void LensDistortion::CalculateViewportParameters(
    CardboardEye eye, const DeviceParams& device_params,
    const std::array<float, 4>& fov, float screen_width_meters,
    float screen_height_meters, ViewportParams* screen_params,
    ViewportParams* texture_params) {
  screen_params->width =
      screen_width_meters / device_params.screen_to_lens_distance();
  screen_params->height =
      screen_height_meters / device_params.screen_to_lens_distance();

  screen_params->x_eye_offset =
      eye == kLeft
          ? ((screen_width_meters - device_params.inter_lens_distance()) / 2) /
                device_params.screen_to_lens_distance()
          : ((screen_width_meters + device_params.inter_lens_distance()) / 2) /
                device_params.screen_to_lens_distance();
  screen_params->y_eye_offset =
      GetYEyeOffsetMeters(device_params, screen_height_meters) /
      device_params.screen_to_lens_distance();

  texture_params->width = tan(fov[0] * M_PI / 180) + tan(fov[1] * M_PI / 180);
  texture_params->height = tan(fov[2] * M_PI / 180) + tan(fov[3] * M_PI / 180);

  texture_params->x_eye_offset = tan(fov[0] * M_PI / 180);
  texture_params->y_eye_offset = tan(fov[2] * M_PI / 180);
}

}  // namespace cardboard
