/*
 * Copyright 2020 Google LLC. All Rights Reserved.
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
#ifndef THIRD_PARTY_CARDBOARD_OSS_UNITY_PLUGIN_SOURCE_LOAD_H_
#define THIRD_PARTY_CARDBOARD_OSS_UNITY_PLUGIN_SOURCE_LOAD_H_

#include "IUnityInterface.h"
#include "UnitySubsystemTypes.h"

/// @brief Loads the Unity XR Display subsystem.
/// @param[in] xr_interfaces Unity XR interface provider to create the display
///            subsystem.
/// @return kUnitySubsystemErrorCodeSuccess When it succeeds to initialize the
///         display subsystem. Otherwise a valid Unity XR Subsystem error code
///         indicating the status of the failure.
UnitySubsystemErrorCode LoadDisplay(IUnityInterfaces* xr_interfaces);

/// @brief Loads the Unity XR Input subsystem.
/// @param[in] xr_interfaces Unity XR interface provider to create the input
///            subsystem.
/// @return kUnitySubsystemErrorCodeSuccess When it succeeds to initialize the
///         input subsystem. Otherwise a valid Unity XR Subsystem error code
///         indicating the status of the failure.
UnitySubsystemErrorCode LoadInput(IUnityInterfaces* xr_interfaces);

#endif  // THIRD_PARTY_CARDBOARD_OSS_UNITY_PLUGIN_SOURCE_LOAD_H_
