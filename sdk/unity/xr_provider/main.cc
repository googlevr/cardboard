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
#include "unity/xr_provider/load.h"
#include "IUnityGraphics.h"
#include "IUnityXRTrace.h"

// @def Logs to @p trace the @p message.
#define CARDBOARD_MAIN_XR_TRACE_LOG(trace, message) \
  XR_TRACE_LOG(trace, "[CardboardXrMain]: " message "\n")

#ifdef __ANDROID__
static IUnityInterfaces *global_unity_interfaces = nullptr;
#endif

// @brief Loads Unity XR Display and Input subsystems.
// @details It tries to load the display subsystem first, if it fails it
//          returns. Then, it continues with the input subsystem.
// @param unity_interfaces Unity Interfaces pointer.
static void LoadXrSubsystems(IUnityInterfaces *unity_interfaces) {
  auto *xr_trace = unity_interfaces->Get<IUnityXRTrace>();

  if (LoadDisplay(unity_interfaces) != kUnitySubsystemErrorCodeSuccess) {
    CARDBOARD_MAIN_XR_TRACE_LOG(xr_trace, "Error loading display subsystem.");
    return;
  }
  CARDBOARD_MAIN_XR_TRACE_LOG(xr_trace,
                              "Display subsystem successfully loaded.");

  if (LoadInput(unity_interfaces) != kUnitySubsystemErrorCodeSuccess) {
    CARDBOARD_MAIN_XR_TRACE_LOG(xr_trace, "Error loading input subsystem.");
    return;
  }
  CARDBOARD_MAIN_XR_TRACE_LOG(xr_trace, "Input subsystem successfully loaded.");
}

#ifdef __ANDROID__
// @brief Callback function for graphic device events.
// @param event_type Graphic device event.
static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType event_type) {
  // Currently, we don't need to process any event other than initialization.
  if (event_type != kUnityGfxDeviceEventInitialize) {
    return;
  }

  // The returned renderer will be null until Unity initialization has finished.
  if (global_unity_interfaces->Get<IUnityGraphics>()->GetRenderer() ==
      kUnityGfxRendererNull) {
    return;
  }

  LoadXrSubsystems(global_unity_interfaces);
}
#endif

extern "C" {

// @note See https://docs.unity3d.com/Manual/NativePluginInterface.html for
// further information about UnityPluginLoad() and UnityPluginUnload().
// UnityPluginLoad() will be called just once when the first native call in C#
// is executed. Before the library is torn down,
// UnityPluginUnload() will be called to destruct and release all taken
// resources.

// @brief Loads Unity XR Plugin.
// @details On Android platforms, it registers the `OnGraphicsDeviceEvent`
// callback to perform the subsystems initialization.
// @param unity_interfaces Unity Interface pointer.
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces *unity_interfaces) {
#ifdef __ANDROID__
  // Cache the Unity interfaces since it will be used by the callback function.
  global_unity_interfaces = unity_interfaces;
  global_unity_interfaces->Get<IUnityGraphics>()->RegisterDeviceEventCallback(
      OnGraphicsDeviceEvent);

  extern void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces *);
  RenderAPI_Vulkan_OnPluginLoad(unity_interfaces);
#else
  LoadXrSubsystems(unity_interfaces);
#endif
}

// @brief Unloads Unity XR Display and Input subsystems.
// @details On Android platforms, it also unregisters the
// `OnGraphicsDeviceEvent` callback.
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  UnloadDisplay();
  UnloadInput();
#ifdef __ANDROID__
  global_unity_interfaces->Get<IUnityGraphics>()->UnregisterDeviceEventCallback(
      OnGraphicsDeviceEvent);
  global_unity_interfaces = nullptr;
#endif
}

}  // extern "C"
