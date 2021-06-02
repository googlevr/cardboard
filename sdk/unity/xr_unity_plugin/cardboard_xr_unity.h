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
#ifndef CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_XR_UNITY_H_
#define CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_XR_UNITY_H_

#include <memory>

#include "unity/xr_unity_plugin/renderer.h"
#include "IUnityInterface.h"

/// @brief Determines the supported graphics APIs.
typedef enum CardboardGraphicsApi {
  kOpenGlEs2 = 1,  ///< Uses OpenGL ES2.0
  kOpenGlEs3 = 2,  ///< Uses OpenGL ES3.0
  kMetal = 3,      ///< Uses Metal
  kNone = -1,      ///< No graphic API is selected.
} CardboardGraphicsApi;

namespace cardboard::unity {

/// Minimalistic wrapper of Cardboard SDK with native and standard types which
/// hides Cardboard types and exposes enough functionality to be consumed by a
/// XR Unity plugin.
class CardboardApi {
 public:
  /// @brief Constructs a CardboardApi.
  CardboardApi();

  /// @brief Destructor.
  /// @details Explicit declaration is required because of pImpl pattern.
  ~CardboardApi();

  /// @brief Initializes and resumes the HeadTracker module.
  void InitHeadTracker();

  /// @brief Pauses the HeadTracker module.
  void PauseHeadTracker();

  /// @brief Resumes the HeadTracker module.
  void ResumeHeadTracker();

  /// @brief Gets the pose of the HeadTracker module.
  /// @details When the HeadTracker has not been initialized, @p position and
  ///          @p rotation are zeroed.
  /// @param[out] position A pointer to an array with three floats to fill in
  ///             the position of the head.
  /// @param[out] orientation A pointer to an array with four floats to fill in
  ///             the quaternion that denotes the orientation of the head.
  // TODO(b/154305848): Move argument types to std::array*.
  void GetHeadTrackerPose(float* position, float* orientation);

  /// @brief Triggers a device parameters scan.
  /// @pre When using Android, the pointer to `JavaVM` must be previously set.
  void ScanDeviceParams();

  /// @brief Updates the DistortionRenderer configuration.
  /// @pre It must be called from the rendering thread.
  /// @details Reads device params from the storage, loads the eye matrices
  ///          (eye-from-head and perspective), and configures the distortion
  ///          meshes for both eyes. If no device parameters are provided,
  ///          Cardboard V1 device parameters is used.
  void UpdateDeviceParams();

  /// @brief Gets the eye perspective matrix and field of view.
  /// @pre UpdateDeviceParams() must have been successfully called.
  /// @param[in] eye The eye to retrieve the information. It must be one of {0 ,
  ///            1}.
  /// @param[out] eye_from_head A pointer to 4x4 float matrix represented as an
  ///             array to store the eye-from-head transformation.
  /// @param[out] fov A pointer to 4x1 float vector to store the [left, right,
  ///             bottom, top] field of view angles in radians.
  void GetEyeMatrices(int eye, float* eye_from_head, float* fov);

  /// @brief Renders both distortion meshes to the screen.
  /// @pre It must be called from the rendering thread.
  void RenderEyesToDisplay();

  /// @brief Render the Cardboard widgets (X, Gear, divider line) to the screen.
  /// @pre It must be called from the rendering thread.
  void RenderWidgets();

  /// @brief Gets the left eye texture color buffer ID.
  /// @pre UpdateDeviceParams() must have been successfully called.
  ///
  /// @return The left eye texture color buffer ID. When using OpenGL ES 2.x and
  ///     OpenGL ES 3.x, the returned value holds a GLuint variable. When using
  ///     Metal, the returned value holds an IOSurfaceRef variable.
  uint64_t GetLeftTextureColorBufferId();

  /// @brief Gets the right eye texture color buffer ID.
  /// @pre UpdateDeviceParams() must have been successfully called.
  ///
  /// @return The right eye texture color buffer ID. When using OpenGL ES 2.x
  ///     and OpenGL ES 3.x, the returned value holds a GLuint variable. When
  ///     using Metal, the returned value holds an IOSurfaceRef variable.
  uint64_t GetRightTextureColorBufferId();

  /// @brief Gets the left eye texture depth buffer ID.
  /// @pre UpdateDeviceParams() must have been successfully called.
  ///
  /// @return The left eye texture depth buffer ID. When using OpenGL ES 2.x and
  ///     OpenGL ES 3.x, the returned value holds a GLuint variable. When using
  ///     Metal, the returned value is zero.
  uint64_t GetLeftTextureDepthBufferId();

  /// @brief Gets the right eye texture depth buffer ID.
  /// @pre UpdateDeviceParams() must have been successfully called.
  ///
  /// @return The right eye texture depth buffer ID. When using OpenGL ES 2.x
  ///     and OpenGL ES 3.x, the returned value holds a GLuint variable. When
  ///     using Metal, the returned value is zero.
  uint64_t GetRightTextureDepthBufferId();

  /// @brief Gets the rectangle size in pixels to draw into.
  ///
  /// @param[out] width Pointer to an int to load the width in pixels of the
  ///             rectangle to draw into.
  /// @param[out] height Pointer to an int to load the height in pixels of the
  ///             rectangle to draw into.
  static void GetScreenParams(int* width, int* height);

  /// @brief Sets screen dimensions and rendering area rectangle in pixels.
  ///
  /// @param[in] screen_width The width of the screen in pixels.
  /// @param[in] screen_height The height of the screen in pixels.
  /// @param[in] viewport_x x coordinate in pixels of the lower left corner of
  ///            the rendering area rectangle.
  /// @param[in] viewport_y y coordinate in pixels of the lower left corner of
  ///            the rendering area rectangle.
  /// @param[in] viewport_width The width of the rendering area rectangle in
  ///            pixels.
  /// @param[in] viewport_height The height of the rendering area rectangle in
  ///            pixels.
  static void SetUnityScreenParams(int screen_width, int screen_height,
                                   int viewport_x, int viewport_y,
                                   int viewport_width, int viewport_height);

  /// @brief Sets the total number of widgets to draw.
  ///
  /// @param[in] count The number of widgets that will be drawn.
  static void SetWidgetCount(int count);

  /// @brief Sets the the parameters of how to draw a specific widget.
  ///
  /// @param[in] i The widget index to set.
  /// @param[in] params The parameters to set.
  static void SetWidgetParams(int i, const Renderer::WidgetParams& params);

  /// @brief Flags a change in device parameters configuration.
  static void SetDeviceParametersChanged();

  /// @brief Gets whether device parameters changed or not.
  /// @details It will return true even though SetDeviceParametersChanged() has
  ///          not been called. A successful call to UpdateDeviceParams() will
  ///          set the return value to false.
  /// @return true When device parameters changed.
  static bool GetDeviceParametersChanged();

  /// @brief Sets the Graphics API that should be used.
  /// @param graphics_api One of the possible CardboardGraphicsApi
  ///        implementations.
  static void SetGraphicsApi(CardboardGraphicsApi graphics_api);

  /// @brief Gets the graphics API being used.
  /// @return Graphics API being used.
  static CardboardGraphicsApi GetGraphicsApi();

  /// @brief Sets Unity XR interface provider.
  /// @param xr_interfaces Pointer to Unity XR interface provider.
  static void SetUnityInterfaces(IUnityInterfaces* xr_interfaces);

  /// @brief Flags a head tracker recentering request.
  static void SetHeadTrackerRecenterRequested();

 private:
  // Forward declaration of the api implementation.
  class CardboardApiImpl;

  // @brief Implementation of this class that hides Cardboard types.
  std::unique_ptr<CardboardApiImpl> p_impl_;
};

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Sets screen dimensions and rendering area rectangle in pixels.
/// @details It is expected to be called at
///          CardboardXRLoader::Initialize() from C# code when loading the
///          provider. Provided parameters will be returned by
///          CardboardApi::GetScreenParams().
/// @param[in] screen_width The width of the screen in pixels.
/// @param[in] screen_height The height of the screen in pixels.
/// @param[in] viewport_x x coordinate in pixels of the lower left corner of the
///            rendering area rectangle.
/// @param[in] viewport_y y coordinate in pixels of the lower left corner of the
///            rendering area rectangle.
/// @param[in] viewport_width The width of the rendering area rectangle in
///            pixels.
/// @param[in] viewport_height The height of the rendering area rectangle in
///            pixels.
void CardboardUnity_setScreenParams(int screen_width, int screen_height,
                                    int viewport_x, int viewport_y,
                                    int viewport_width, int viewport_height);

/// @brief Sets the total number of widgets to draw.
///
/// @param[in] count The number of widgets that will be drawn.
void CardboardUnity_setWidgetCount(int count);

/// @brief Sets the the parameters of how to draw a specific widget.
///
/// @param[in] i The widget index to set.
/// @param[in] texture The widget texture as a Texture.GetNativeTexturePtr, @see
///            https://docs.unity3d.com/ScriptReference/Texture.GetNativeTexturePtr.html.
/// @param[in] x x coordinate in pixels of the lower left corner of the
///            rectangle.
/// @param[in] y y coordinate in pixels of the lower left corner of the
///            rectangle.
/// @param[in] width The width in pixels of the rectangle.
/// @param[in] height The height in pixels of the rectangle.
void CardboardUnity_setWidgetParams(int i, void* texture, int x, int y,
                                    int width, int height);

/// @brief Flags a change in device parameters configuration.
void CardboardUnity_setDeviceParametersChanged();

/// @brief Sets the graphics API to use.
/// @param graphics_api The graphics API to use.
void CardboardUnity_setGraphicsApi(CardboardGraphicsApi graphics_api);

/// @brief Flags a head tracker recentering request.
void CardboardUnity_recenterHeadTracker();

#ifdef __cplusplus
}
#endif

}  // namespace cardboard::unity

#endif  // CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_XR_UNITY_H_
