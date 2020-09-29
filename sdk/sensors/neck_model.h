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
#ifndef CARDBOARD_SDK_SENSORS_NECK_MODEL_H_
#define CARDBOARD_SDK_SENSORS_NECK_MODEL_H_

#include <array>

namespace cardboard {

// The neck model parameters may be exposed as a per-user preference in the
// future, but that's only a marginal improvement, since getting accurate eye
// offsets would require full positional tracking. For now, use hardcoded
// defaults. The values only have an effect when the neck model is enabled.

// Position of the point between the eyes, relative to the neck pivot:
constexpr float kDefaultNeckHorizontalOffset = -0.080f;  // meters in Z
constexpr float kDefaultNeckVerticalOffset = 0.075f;     // meters in Y

// ApplyNeckModel applies a neck model offset based on the rotation of
// |orientation|.
// The value of |factor| is clamped from zero to one.
std::array<float, 3> ApplyNeckModel(const std::array<float, 4>& orientation,
                                    double factor);

}  // namespace cardboard

#endif  // CARDBOARD_SDK_SENSORS_NECK_MODEL_H_
