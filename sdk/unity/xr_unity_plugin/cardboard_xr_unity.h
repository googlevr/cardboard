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

/// @brief Determines the supported graphics APIs.
typedef enum CardboardGraphicsApi {
    kOpenGlEs2 = 1,         ///< Uses OpenGL ES2.0
    kOpenGlEs3 = 2,        ///< Uses OpenGL ES3.0
    kNone = -1,             ///< No graphic API is selected.
} CardboardGraphicsApi;

// TODO(b/151087873) Convert into single line namespace declaration.
namespace cardboard {
namespace unity {

/// Minimalistic wrapper of Cardboard SDK with native and standard types which
/// hides Cardboard types and exposes enough functionality to be consumed by a
/// XR Unity plugin.
///
/// Cardboard SDK interface has C-structs and native types but its use requires
/// OpenGL calls. Unity XR display provider has a custom build system with
/// limitations to link against OpenGL ES 2/3 for both Android and iOS. This
/// wrapper enables the provider configuration rely on this class for OpenGL
/// specific calls and Cardboard SDK configurations reducing them to a minimum.
class CardboardApi {
 public:
  // @brief Data about drawing a custom widget.
  struct WidgetParams {
    // @brief GL texture ID.
    uint32_t texture;

    // @brief x Widget X coordinate in pixels.
    int x;

    // @brief y Widget Y coordinate in pixels.
    int y;

    // @brief width Widget width in pixels.
    int width;

    // @brief height Widget height in pixels.
    int height;
  };

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
  ///
  /// @param gl_framebuffer_id The GL framebuffer ID to render the eyes to.
  void RenderEyesToDisplay(int gl_framebuffer_id);

  /// @brief Render the Cardboard widgets (X, Gear, divider line) to the screen.
  /// @pre It must be called from the rendering thread.
  void RenderWidgets();

  /// @brief Gets the left eye color texture.
  /// @pre UpdateDeviceParams() must have been successfully called.
  /// @return The left eye color texture.
  int GetLeftTextureId();

  /// @brief Gets the right eye color texture.
  /// @pre UpdateDeviceParams() must have been successfully called.
  /// @return The right eye color texture.
  int GetRightTextureId();

  /// @brief Gets the left depth buffer ID.
  /// @pre UpdateDeviceParams() must have been successfully called.
  /// @return The left depth buffer ID.
  int GetLeftDepthBufferId();

  /// @brief Gets the right depth buffer ID.
  /// @pre UpdateDeviceParams() must have been successfully called.
  /// @return The right depth buffer ID.
  int GetRightDepthBufferId();

  /// @brief Gets the OpenGL bound framebuffer.
  /// @return The bound framebuffer.
  static int GetBoundFramebuffer();

  /// @brief Gets the rectangle size in pixels to draw into.
  ///
  /// @param[out] width Pointer to an int to load the width in pixels of the
  ///             rectangle to draw into.
  /// @param[out] height Pointer to an int to load the height in pixels of the
  ///             rectangle to draw into.
  static void GetScreenParams(int* width, int* height);

  /// @brief Sets Unity reported rectangle lower left corner and size in pixels
  ///        to draw into.
  ///
  /// @param[in] x x coordinate in pixels of the lower left corner of the
  ///            rectangle.
  /// @param[in] y y coordinate in pixels of the lower left corner of the
  ///            rectangle.
  /// @param[in] width The width in pixels of the rectangle.
  /// @param[in] height The height in pixels of the rectangle.
  static void SetUnityScreenParams(int x, int y, int width, int height);

  /// @brief Sets the total number of widgets to draw.
  ///
  /// @param[in] count The number of widgets that will be drawn.
  static void SetWidgetCount(int count);

  /// @brief Sets the the parameters of how to draw a specific widget.
  ///
  /// @param[in] i The widget index to set.
  /// @param[in] params The parameters to set.
  static void SetWidgetParams(int i, const WidgetParams& params);

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

 private:
  // Forward declaration of the api implementation.
  class CardboardApiImpl;

  // @brief Implementation of this class that hides Cardboard types.
  std::unique_ptr<CardboardApiImpl> p_impl_;
};

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Hook to set the screen parameters: lower left corner coordinate and
///        size in pixels of the rectangle to draw into by Unity.
/// @details It is expected to be called at
///          CardboardXRLoader::Initialize() from C# code when loading the
///          provider. Provided parameters will be returned by
///          CardboardApi::GetScreenParams().
/// @param[in] x x coordinate in pixels of the lower left corner of the
///            rectangle.
/// @param[in] y y coordinate in pixels of the lower left corner of the
///            rectangle.
/// @param[in] width The width in pixels of the rectangle.
/// @param[in] height The height in pixels of the rectangle.
void CardboardUnity_setScreenParams(int x, int y, int width, int height);

/// @brief Sets the total number of widgets to draw.
///
/// @param[in] count The number of widgets that will be drawn.
void CardboardUnity_setWidgetCount(int count);

/// @brief Sets the the parameters of how to draw a specific widget.
///
/// @param[in] i The widget index to set.
/// @param[in] params The parameters to set.
void CardboardUnity_setWidgetParams(int i, void* texture, int x, int y,
                                    int width, int height);

/// @brief Flags a change in device parameters configuration.
void CardboardUnity_setDeviceParametersChanged();

/// @brief Sets the graphics API to use.
/// @param graphics_api The graphics API to use.
void CardboardUnity_setGraphicsApi(CardboardGraphicsApi graphics_api);

#ifdef __cplusplus
}
#endif

}  // namespace unity
}  // namespace cardboard

#endif  // CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_XR_UNITY_H_
