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
#include <array>
#include <cassert>
#include <map>
#include <memory>

#include "unity/xr_unity_plugin/cardboard_xr_unity.h"
#include "unity/xr_provider/load.h"
#include "unity/xr_provider/math_tools.h"
#include "IUnityInterface.h"
#include "IUnityXRDisplay.h"
#include "IUnityXRTrace.h"
#include "UnitySubsystemTypes.h"

// @def Logs to Unity XR Trace interface @p message.
#define CARDBOARD_DISPLAY_XR_TRACE_LOG(trace, message, ...)                \
  XR_TRACE_LOG(trace, "[CardboardXrDisplayProvider]: " message "\n", \
               ##__VA_ARGS__)

namespace {
// @brief Holds the implementation methods of UnityLifecycleProvider and
//        UnityXRDisplayGraphicsThreadProvider
class CardboardDisplayProvider {
 public:
  CardboardDisplayProvider(IUnityXRTrace* trace,
                           IUnityXRDisplayInterface* display)
      : trace_(trace), display_(display) {}

  IUnityXRDisplayInterface* GetDisplay() { return display_; }

  IUnityXRTrace* GetTrace() { return trace_; }

  void SetHandle(UnitySubsystemHandle handle) { handle_ = handle; }

  UnitySubsystemErrorCode Initialize() const {
    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode Start() const {
    return kUnitySubsystemErrorCodeSuccess;
  }

  void Stop() const {}

  void Shutdown() const {}

  UnitySubsystemErrorCode GfxThread_Start(
      UnityXRRenderingCapabilities* rendering_caps) const {
    // The display provider uses multipass redering.
    rendering_caps->noSinglePassRenderingSupport = true;
    rendering_caps->invalidateRenderStateAfterEachCallback = true;
    // Unity will swap buffers for us after GfxThread_SubmitCurrentFrame() is
    // executed.
    rendering_caps->skipPresentToMainScreen = false;
    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode GfxThread_SubmitCurrentFrame() {
    if (!is_initialized_) {
      CARDBOARD_DISPLAY_XR_TRACE_LOG(
          trace_, "Skip the rendering because Cardboard SDK is uninitialized.");
      return kUnitySubsystemErrorCodeFailure;
    }
    cardboard_api_->RenderEyesToDisplay(cardboard_api_->GetBoundFramebuffer());
    cardboard_api_->RenderWidgets();
    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode GfxThread_PopulateNextFrameDesc(
      const UnityXRFrameSetupHints* frame_hints,
      UnityXRNextFrameDesc* next_frame) {
    // Allocate new color texture descriptors if needed and update device
    // parameters in Cardboard SDK.
    if ((frame_hints->changedFlags &
         kUnityXRFrameSetupHintsChangedTextureResolutionScale) != 0 ||
        !is_initialized_ ||
        cardboard::unity::CardboardApi::GetDeviceParametersChanged()) {
      // Create a new Cardboard SDK to clear previous truncated initializations
      // or just do it for the first time.
      CARDBOARD_DISPLAY_XR_TRACE_LOG(trace_, "Initializes Cardboard API.");
      cardboard_api_.reset(new cardboard::unity::CardboardApi());
      // Deallocate old textures since we're completely reallocating new
      // textures for Cardboard SDK.
      for (auto&& tex : tex_map_) {
        display_->DestroyTexture(handle_, tex.second);
      }
      tex_map_.clear();

      cardboard::unity::CardboardApi::GetScreenParams(&width_, &height_);
      cardboard_api_->UpdateDeviceParams();
      is_initialized_ = true;

      // Initialize texture descriptors.
      for (int i = 0; i < texture_descriptors_.size(); ++i) {
        texture_descriptors_[i] = {};
        texture_descriptors_[i].width = width_ / 2;
        texture_descriptors_[i].height = height_;
        texture_descriptors_[i].depthFormat = kUnityXRDepthTextureFormatNone;
        texture_descriptors_[i].flags = 0;
        texture_descriptors_[i].depthFormat = kUnityXRDepthTextureFormat16bit;
      }
    }

    // Setup render passes + texture ids for eye textures and layers.
    for (int i = 0; i < texture_descriptors_.size(); ++i) {
      // Sets the color texture ID to Unity texture descriptors.
      const int gl_colorname = i == 0 ? cardboard_api_->GetLeftTextureId()
                                      : cardboard_api_->GetRightTextureId();
      const int gl_depthname = i == 0 ? cardboard_api_->GetLeftDepthBufferId()
                                      : cardboard_api_->GetRightDepthBufferId();

      UnityXRRenderTextureId unity_texture_id = 0;
      auto found = tex_map_.find(gl_colorname);
      if (found == tex_map_.end()) {
        UnityXRRenderTextureDesc texture_descriptor = texture_descriptors_[i];
        texture_descriptor.color.nativePtr = ConvertInt(gl_colorname);
        texture_descriptor.depth.nativePtr = ConvertInt(gl_depthname);
        display_->CreateTexture(handle_, &texture_descriptor,
                                &unity_texture_id);
        tex_map_[gl_colorname] = unity_texture_id;
      } else {
        unity_texture_id = found->second;
      }
      next_frame->renderPasses[i].textureId = unity_texture_id;
    }

    {
      auto* left_eye_params = &next_frame->renderPasses[0].renderParams[0];
      auto* right_eye_params = &next_frame->renderPasses[1].renderParams[0];

      for (int i = 0; i < 2; ++i) {
        std::array<float, 4> fov;
        std::array<float, 16> eye_from_head;
        cardboard_api_->GetEyeMatrices(i, eye_from_head.data(), fov.data());

        auto* eye_params = i == 0 ? left_eye_params : right_eye_params;
        // Update pose for rendering.
        eye_params->deviceAnchorToEyePose =
            cardboard::unity::CardboardTransformToUnityPose(eye_from_head);
        // Field of view and viewport.
        eye_params->viewportRect = frame_hints->appSetup.renderViewport;
        ConfigureFieldOfView(fov, &eye_params->projection);
      }

      // Configure the culling pass for the right eye (index == 1) to be the
      // same as the left eye (index == 0).
      next_frame->renderPasses[0].cullingPassIndex = 0;
      next_frame->renderPasses[1].cullingPassIndex = 0;
      next_frame->cullingPasses[0].deviceAnchorToCullingPose =
          left_eye_params->deviceAnchorToEyePose;
      next_frame->cullingPasses[0].projection = left_eye_params->projection;
      // TODO(b/155084408): Properly document this constant.
      next_frame->cullingPasses[0].separation = 0.064f;
    }

    // Configure multipass rendering with one pass for each eye.
    next_frame->renderPassesCount = 2;
    next_frame->renderPasses[0].renderParamsCount = 1;
    next_frame->renderPasses[1].renderParamsCount = 1;

    return kUnitySubsystemErrorCodeSuccess;
  }

  UnitySubsystemErrorCode GfxThread_Stop() {
    cardboard_api_.reset();
    is_initialized_ = false;
    return kUnitySubsystemErrorCodeSuccess;
  }

 private:
  /// @brief Converts @p i to a void*
  /// @param i An integer to convert to void*.
  /// @return A void* whose value is @p i.
  static void* ConvertInt(int i) {
    return reinterpret_cast<void*>(static_cast<intptr_t>(i));
  }

  /// @brief Loads Unity @p projection eye params from Cardboard field of view.
  /// @details Sets Unity @p projection to use half angles as Cardboard reported
  ///          field of view angles.
  /// @param[in] cardboard_fov A float vector containing
  ///            [left, right, bottom, top] angles in radians.
  /// @param[out] project A Unity projection structure pointer to load.
  static void ConfigureFieldOfView(const std::array<float, 4>& cardboard_fov,
                                   UnityXRProjection* projection) {
    projection->type = kUnityXRProjectionTypeHalfAngles;
    projection->data.halfAngles.bottom = -std::abs(tan(cardboard_fov[2]));
    projection->data.halfAngles.top = std::abs(tan(cardboard_fov[3]));
    projection->data.halfAngles.left = -std::abs(tan(cardboard_fov[0]));
    projection->data.halfAngles.right = std::abs(tan(cardboard_fov[1]));
  }

  /// @brief Points to Unity XR Trace interface.
  IUnityXRTrace* trace_ = nullptr;

  /// @brief Points to Unity XR Display interface.
  IUnityXRDisplayInterface* display_ = nullptr;

  /// @brief Opaque Unity pointer type passed between plugins.
  UnitySubsystemHandle handle_;

  /// @brief Tracks Cardboard API initialization status. It is set to true once
  /// the CardboardApi::UpdateDeviceParams() is called and returns true.
  bool is_initialized_ = false;

  /// @brief Screen width in pixels.
  int width_;

  /// @brief Screen height in pixels.
  int height_;

  /// @brief Cardboard SDK API wrapper.
  std::unique_ptr<cardboard::unity::CardboardApi> cardboard_api_;

  /// @brief Unity XR texture descriptors.
  std::array<UnityXRRenderTextureDesc, 2> texture_descriptors_{};

  /// @brief Map to link Cardboard API and Unity XR texture IDs.
  std::map<int, UnityXRRenderTextureId> tex_map_{};
};

std::unique_ptr<CardboardDisplayProvider> display_provider;

}  // namespace

/// @brief Initializes the display subsystem.
///
/// @details Loads and configures a UnityXRDisplayGraphicsThreadProvider and
///          UnityXRDisplayProvider with pointers to `display_provider`'s
///          methods.
/// @param handle Opaque Unity pointer type passed between plugins.
/// @return kUnitySubsystemErrorCodeSuccess when the registration is successful.
///         Otherwise, a value in UnitySubsystemErrorCode flagging the error.
static UnitySubsystemErrorCode UNITY_INTERFACE_API
DisplayInitialize(UnitySubsystemHandle handle, void*) {
  display_provider->SetHandle(handle);

  // Register for callbacks on the graphics thread.
  UnityXRDisplayGraphicsThreadProvider gfx_thread_provider{};
  gfx_thread_provider.userData = NULL;
  gfx_thread_provider.Start = [](UnitySubsystemHandle, void*,
                                 UnityXRRenderingCapabilities* rendering_caps)
      -> UnitySubsystemErrorCode {
    return display_provider->GfxThread_Start(rendering_caps);
  };
  gfx_thread_provider.SubmitCurrentFrame =
      [](UnitySubsystemHandle, void*) -> UnitySubsystemErrorCode {
    return display_provider->GfxThread_SubmitCurrentFrame();
  };
  gfx_thread_provider.PopulateNextFrameDesc =
      [](UnitySubsystemHandle, void*, const UnityXRFrameSetupHints* frame_hints,
         UnityXRNextFrameDesc* next_frame) -> UnitySubsystemErrorCode {
    return display_provider->GfxThread_PopulateNextFrameDesc(frame_hints,
                                                             next_frame);
  };
  gfx_thread_provider.Stop = [](UnitySubsystemHandle,
                                void*) -> UnitySubsystemErrorCode {
    return display_provider->GfxThread_Stop();
  };
  display_provider->GetDisplay()->RegisterProviderForGraphicsThread(
      handle, &gfx_thread_provider);

  UnityXRDisplayProvider provider{NULL, NULL, NULL};
  display_provider->GetDisplay()->RegisterProvider(handle, &provider);

  return display_provider->Initialize();
}

/// @brief Loads a UnityLifecycleProvider for the display provider.
///
/// @details Gets the trace and display interfaces from @p xr_interfaces and
///          initializes the UnityLifecycleProvider's callbacks with references
///          to `display_provider`'s methods. The subsystem is "Display", and
///          the plugin is "Cardboard".
/// @param xr_interfaces Unity XR interface provider to create the display
///          subsystem.
/// @return kUnitySubsystemErrorCodeSuccess when the registration is successful.
///         Otherwise, a value in UnitySubsystemErrorCode flagging the error.
UnitySubsystemErrorCode LoadDisplay(IUnityInterfaces* xr_interfaces) {
  auto* display = xr_interfaces->Get<IUnityXRDisplayInterface>();
  if (display == NULL) {
    return kUnitySubsystemErrorCodeFailure;
  }
  auto* trace = xr_interfaces->Get<IUnityXRTrace>();
  if (trace == NULL) {
    return kUnitySubsystemErrorCodeFailure;
  }
  display_provider.reset(new CardboardDisplayProvider(trace, display));

  UnityLifecycleProvider display_lifecycle_handler;
  display_lifecycle_handler.userData = NULL;
  display_lifecycle_handler.Initialize = &DisplayInitialize;
  display_lifecycle_handler.Start = [](UnitySubsystemHandle,
                                       void*) -> UnitySubsystemErrorCode {
    return display_provider->Start();
  };
  display_lifecycle_handler.Stop = [](UnitySubsystemHandle, void*) -> void {
    display_provider->Stop();
  };
  display_lifecycle_handler.Shutdown = [](UnitySubsystemHandle, void*) -> void {
    display_provider->Shutdown();
  };

  return display_provider->GetDisplay()->RegisterLifecycleProvider(
      "Cardboard", "Display", &display_lifecycle_handler);
}
