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
#include "qrcode/cardboard_v1/cardboard_v1.h"

#include <string>

#include "cardboard_device.pb.h"

namespace cardboard {
namespace qrcode {

// Device params for Cardboard V1 released at Google I/O 2014.
static constexpr float kCardboardV1InterLensDistance = 0.06f;
static constexpr float kCardboardV1TrayToLensDistance = 0.035f;
static constexpr float kCardboardV1VirtualEyeToScreenDistance = 0.042f;
static constexpr float kCardboardV1FovHalfDegrees[] = {40.0f, 40.0f, 40.0f,
                                                       40.0f};
static constexpr float kCardboardV1DistortionCoeffs[] = {0.441f, 0.156f};
static constexpr char kCardboardV1Vendor[] = "Google, Inc.";
static constexpr char kCardboardV1Model[] = "Cardboard v1";

std::vector<uint8_t> getCardboardV1DeviceParams() {
  DeviceParams cardboard_v1_params;
  cardboard_v1_params.set_vendor(kCardboardV1Vendor);
  cardboard_v1_params.set_model(kCardboardV1Model);
  cardboard_v1_params.set_screen_to_lens_distance(
      kCardboardV1VirtualEyeToScreenDistance);
  cardboard_v1_params.set_inter_lens_distance(kCardboardV1InterLensDistance);
  for (float degree : kCardboardV1FovHalfDegrees) {
    cardboard_v1_params.add_left_eye_field_of_view_angles(degree);
  }

  cardboard_v1_params.set_primary_button(DeviceParams_ButtonType_MAGNET);
  cardboard_v1_params.set_vertical_alignment(
      DeviceParams_VerticalAlignmentType_BOTTOM);
  cardboard_v1_params.set_tray_to_lens_distance(kCardboardV1TrayToLensDistance);
  for (float coefficient : kCardboardV1DistortionCoeffs) {
    cardboard_v1_params.add_distortion_coefficients(coefficient);
  }

  std::string serialized_params;
  cardboard_v1_params.SerializeToString(&serialized_params);
  return std::vector<uint8_t>(serialized_params.begin(),
                              serialized_params.end());
}

}  // namespace qrcode
}  // namespace cardboard
