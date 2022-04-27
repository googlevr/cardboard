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
#include <time.h>

#include <array>
#include <cmath>

#include "unity/xr_provider/load.h"
#include "unity/xr_provider/math_tools.h"
#include "unity/xr_unity_plugin/cardboard_input_api.h"
#include "IUnityInterface.h"
#include "IUnityXRInput.h"
#include "IUnityXRTrace.h"
#include "UnitySubsystemTypes.h"

#define CARDBOARD_INPUT_XR_TRACE_LOG(trace, message, ...)          \
  XR_TRACE_LOG(trace, "[CardboardXrInputProvider]: " message "\n", \
               ##__VA_ARGS__)
namespace {

class CardboardInputProvider {
 public:
  CardboardInputProvider(IUnityXRTrace* trace, IUnityXRInputInterface* input)
      : trace_(trace), input_(input) {
    cardboard_input_api_.reset(new cardboard::unity::CardboardInputApi());
  }

  IUnityXRInputInterface* GetInput() { return input_; }

  IUnityXRTrace* GetTrace() { return trace_; }

  /// @return A reference to the static instance of this class, which is thought
  ///         to be the only one.
  static std::unique_ptr<CardboardInputProvider>& GetInstance();

  /// Callback executed when a subsystem should initialize in preparation for
  /// becoming active.
  UnitySubsystemErrorCode Initialize(UnitySubsystemHandle handle,
                                     void* /*data*/) {
    CARDBOARD_INPUT_XR_TRACE_LOG(cardboard_input_provider_->GetTrace(),
                                 "Lifecycle initialized");

    // Initializes XR Input Provider
    UnityXRInputProvider input_provider;
    input_provider.userData = nullptr;
    input_provider.Tick = [](UnitySubsystemHandle, void*,
                             UnityXRInputUpdateType) {
      return GetInstance()->Tick();
    };
    input_provider.FillDeviceDefinition =
        [](UnitySubsystemHandle, void*, UnityXRInternalInputDeviceId device_id,
           UnityXRInputDeviceDefinition* definition) {
          return GetInstance()->FillDeviceDefinition(device_id, definition);
        };
    input_provider.UpdateDeviceState =
        [](UnitySubsystemHandle, void*, UnityXRInternalInputDeviceId device_id,
           UnityXRInputUpdateType, UnityXRInputDeviceState* state) {
          return GetInstance()->UpdateDeviceState(device_id, state);
        };
    input_provider.HandleEvent = [](UnitySubsystemHandle, void*, unsigned int,
                                    UnityXRInternalInputDeviceId, void*,
                                    unsigned int) {
      CARDBOARD_INPUT_XR_TRACE_LOG(cardboard_input_provider_->GetTrace(),
                                   "No events to handle");
      // HandEvent is omitted, a failure must be returned as explained in
      // https://docs.unity3d.com/2021.3/Documentation/Manual/xrsdk-input.html
      // under "UnityXRInputProvider.HandleEvent" paragraph.
      return kUnitySubsystemErrorCodeFailure;
    };
    input_provider.QueryTrackingOriginMode =
        [](UnitySubsystemHandle, void*,
           UnityXRInputTrackingOriginModeFlags* tracking_origin_mode) {
          return GetInstance()->QueryTrackingOriginMode(tracking_origin_mode);
        };
    input_provider.QuerySupportedTrackingOriginModes =
        [](UnitySubsystemHandle, void*,
           UnityXRInputTrackingOriginModeFlags*
               supported_tracking_origin_modes) {
          return GetInstance()->QuerySupportedTrackingOriginModes(
              supported_tracking_origin_modes);
        };
    input_provider.HandleSetTrackingOriginMode =
        [](UnitySubsystemHandle, void*,
           UnityXRInputTrackingOriginModeFlags tracking_origin_mode) {
          return GetInstance()->HandleSetTrackingOriginMode(
              tracking_origin_mode);
        };
    input_provider.HandleRecenter = nullptr;
    input_provider.HandleHapticImpulse = nullptr;
    input_provider.HandleHapticBuffer = nullptr;
    input_provider.QueryHapticCapabilities = nullptr;
    input_provider.HandleHapticStop = nullptr;
    GetInstance()->GetInput()->RegisterInputProvider(handle, &input_provider);

    // Initializes Cardboard's Head Tracker module.
    GetInstance()->InitHeadTracker();

    return kUnitySubsystemErrorCodeSuccess;
  }

  /// Initializes the head tracker module.
  void InitHeadTracker() { cardboard_input_api_->InitHeadTracker(); }

  /// Callback executed when a subsystem should become active.
  UnitySubsystemErrorCode Start(UnitySubsystemHandle handle) {
    CARDBOARD_INPUT_XR_TRACE_LOG(trace_, "Lifecycle started");
    input_->InputSubsystem_DeviceConnected(handle, kDeviceIdCardboardHmd);
    cardboard_input_api_->ResumeHeadTracker();
    return kUnitySubsystemErrorCodeSuccess;
  }

  /// Callback executed when a subsystem should become inactive.
  void Stop(UnitySubsystemHandle handle) {
    CARDBOARD_INPUT_XR_TRACE_LOG(trace_, "Lifecycle stopped");
    input_->InputSubsystem_DeviceDisconnected(handle, kDeviceIdCardboardHmd);
    cardboard_input_api_->PauseHeadTracker();
  }

  UnitySubsystemErrorCode Tick() {
    std::array<float, 4> out_orientation;
    std::array<float, 3> out_position;
    cardboard_input_api_->GetHeadTrackerPose(out_position.data(),
                                             out_orientation.data());
    // TODO(b/151817737): Compute pose position within SDK with custom rotation.
    head_pose_ =
        cardboard::unity::CardboardRotationToUnityPose(out_orientation);
    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode FillDeviceDefinition(
      UnityXRInternalInputDeviceId device_id,
      UnityXRInputDeviceDefinition* definition) {
    if (device_id != kDeviceIdCardboardHmd) {
      return kUnitySubsystemErrorCodeFailure;
    }

    input_->DeviceDefinition_SetName(definition, "Cardboard HMD");
    input_->DeviceDefinition_SetCharacteristics(definition,
                                                kHmdCharacteristics);
    input_->DeviceDefinition_AddFeatureWithUsage(
        definition, "Center Eye Position", kUnityXRInputFeatureTypeAxis3D,
        kUnityXRInputFeatureUsageCenterEyePosition);
    input_->DeviceDefinition_AddFeatureWithUsage(
        definition, "Center Eye Rotation", kUnityXRInputFeatureTypeRotation,
        kUnityXRInputFeatureUsageCenterEyeRotation);

    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode UpdateDeviceState(
      UnityXRInternalInputDeviceId device_id, UnityXRInputDeviceState* state) {
    if (device_id != kDeviceIdCardboardHmd) {
      return kUnitySubsystemErrorCodeFailure;
    }

    UnityXRInputFeatureIndex feature_index = 0;
    input_->DeviceState_SetAxis3DValue(state, feature_index++,
                                       head_pose_.position);
    input_->DeviceState_SetRotationValue(state, feature_index++,
                                         head_pose_.rotation);

    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode QueryTrackingOriginMode(
      UnityXRInputTrackingOriginModeFlags* tracking_origin_mode) {
    *tracking_origin_mode = kUnityXRInputTrackingOriginModeDevice;
    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode QuerySupportedTrackingOriginModes(
      UnityXRInputTrackingOriginModeFlags* supported_tracking_origin_modes) {
    *supported_tracking_origin_modes = kUnityXRInputTrackingOriginModeDevice;
    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode HandleSetTrackingOriginMode(
      UnityXRInputTrackingOriginModeFlags tracking_origin_mode) {
    return tracking_origin_mode == kUnityXRInputTrackingOriginModeDevice
               ? kUnitySubsystemErrorCodeSuccess
               : kUnitySubsystemErrorCodeFailure;
  }

 private:
  static constexpr int kDeviceIdCardboardHmd = 0;

  static constexpr UnityXRInputDeviceCharacteristics kHmdCharacteristics =
      static_cast<UnityXRInputDeviceCharacteristics>(
          kUnityXRInputDeviceCharacteristicsHeadMounted |
          kUnityXRInputDeviceCharacteristicsTrackedDevice);

  IUnityXRTrace* trace_ = nullptr;

  IUnityXRInputInterface* input_ = nullptr;

  UnityXRPose head_pose_;

  std::unique_ptr<cardboard::unity::CardboardInputApi> cardboard_input_api_;

  static std::unique_ptr<CardboardInputProvider> cardboard_input_provider_;
};

std::unique_ptr<CardboardInputProvider>
    CardboardInputProvider::cardboard_input_provider_;

std::unique_ptr<CardboardInputProvider>& CardboardInputProvider::GetInstance() {
  return cardboard_input_provider_;
}

}  // namespace

UnitySubsystemErrorCode LoadInput(IUnityInterfaces* xr_interfaces) {
  auto* input = xr_interfaces->Get<IUnityXRInputInterface>();
  if (input == NULL) {
    return kUnitySubsystemErrorCodeFailure;
  }
  auto* trace = xr_interfaces->Get<IUnityXRTrace>();
  if (trace == NULL) {
    return kUnitySubsystemErrorCodeFailure;
  }

  CardboardInputProvider::GetInstance().reset(
      new CardboardInputProvider(trace, input));

  UnityLifecycleProvider input_lifecycle_handler;
  input_lifecycle_handler.userData = NULL;
  input_lifecycle_handler.Initialize =
      [](UnitySubsystemHandle handle, void* data) -> UnitySubsystemErrorCode {
    return CardboardInputProvider::GetInstance()->Initialize(handle, data);
  };
  input_lifecycle_handler.Start = [](UnitySubsystemHandle handle, void*) {
    return CardboardInputProvider::GetInstance()->Start(handle);
  };
  input_lifecycle_handler.Stop = [](UnitySubsystemHandle handle, void*) {
    return CardboardInputProvider::GetInstance()->Stop(handle);
  };
  input_lifecycle_handler.Shutdown = [](UnitySubsystemHandle, void*) {
    CARDBOARD_INPUT_XR_TRACE_LOG(
        CardboardInputProvider::GetInstance()->GetTrace(),
        "Lifecycle finished");
  };
  return input->RegisterLifecycleProvider("Cardboard", "CardboardInput",
                                          &input_lifecycle_handler);
}

void UnloadInput() { CardboardInputProvider::GetInstance().reset(); }
