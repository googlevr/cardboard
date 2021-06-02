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
#ifndef CARDBOARD_SDK_QRCODE_CARDBOARD_V1_CARDBOARD_V1_H_
#define CARDBOARD_SDK_QRCODE_CARDBOARD_V1_CARDBOARD_V1_H_

#include <stdint.h>

#include <vector>

namespace cardboard::qrcode {

/// Device params for Cardboard V1 released at Google I/O 2014.
/// {@
constexpr float kCardboardV1InterLensDistance = 0.06f;
constexpr float kCardboardV1TrayToLensDistance = 0.035f;
constexpr float kCardboardV1ScreenToLensDistance = 0.042f;
constexpr float kCardboardV1FovHalfDegrees[] = {40.0f, 40.0f, 40.0f, 40.0f};
constexpr float kCardboardV1DistortionCoeffs[] = {0.441f, 0.156f};
constexpr int kCardboardV1DistortionCoeffsSize = 2;
constexpr int kCardboardV1VerticalAlignmentType = 0;
constexpr char kCardboardV1Vendor[] = "Google, Inc.";
constexpr char kCardboardV1Model[] = "Cardboard v1";
/// @}

std::vector<uint8_t> getCardboardV1DeviceParams();
}  // namespace cardboard::qrcode

#endif  // CARDBOARD_SDK_QRCODE_CARDBOARD_V1_CARDBOARD_V1_H_
