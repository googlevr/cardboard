/*
 * Copyright 2020 Google LLC
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

#ifndef CARDBOARD_SDK_UNITY_XR_PROVIDER_MATH_TOOLS_H_
#define CARDBOARD_SDK_UNITY_XR_PROVIDER_MATH_TOOLS_H_

#include <array>
#include <cmath>

#include "UnityXRTypes.h"

namespace cardboard::unity {

/// @brief Creates a UnityXRPose from a Cardboard rotation.
/// @param rotation A Cardboard rotation quaternion expressed as [x, y, z, w].
/// @returns A UnityXRPose from Cardboard @p rotation.
UnityXRPose CardboardRotationToUnityPose(const std::array<float, 4>& rotation);

/// @brief Creates a UnityXRPose from a Cardboard transformation matrix.
/// @param transform A 4x4 float transformation matrix.
/// @returns A UnityXRPose from Cardboard @p transform.
UnityXRPose CardboardTransformToUnityPose(
    const std::array<float, 16>& transform);

}  // namespace cardboard::unity

#endif  // CARDBOARD_SDK_UNITY_XR_PROVIDER_MATH_TOOLS_H_
