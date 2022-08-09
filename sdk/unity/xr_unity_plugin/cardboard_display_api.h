/*
 * Copyright 2021 Google LLC
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
#ifndef CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_DISPLAY_API_H_
#define CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_DISPLAY_API_H_

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "include/cardboard.h"
#include "unity/xr_unity_plugin/renderer.h"
#include "IUnityInterface.h"

/// @brief Determines the supported graphics APIs.
typedef enum CardboardGraphicsApi {
  kOpenGlEs2 = 1,  ///< Uses OpenGL ES2.0
  kOpenGlEs3 = 2,  ///< Uses OpenGL ES3.0
  kMetal = 3,      ///< Uses Metal
  kVulkan = 4,     ///< Uses Vulkan
  kNone = -1,      ///< No graphic API is selected.
} CardboardGraphicsApi;

namespace cardboard::unity {

/// Minimalistic wrapper of Cardboard SDK with native and standard types which
/// hides Cardboard types and exposes enough functionality related to eyes
/// texture distortion to be consumed by the display provider of a XR Unity
/// plugin.
class CardboardDisplayApi {
 public:
  /// @brief Constructs a CardboardDisplayApi.
  /// @details Initializes the renderer based on the `selected_graphics_api_`
  /// variable.
  CardboardDisplayApi();

  /// @brief Destructor. Frees renderer resources.
  ~CardboardDisplayApi();

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

  /// @brief Runs commands needed before rendering.
  /// @pre It must be called before calling other rendering commands.
  void RunRenderingPreProcessing();

  /// @brief Runs commands needed after rendering.
  /// @pre It must be called after calling other rendering commands.
  void RunRenderingPostProcessing();

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

 private:
  // @brief Holds the screen and rendering area details.
  struct ScreenParams {
    // @brief The width of the screen in pixels.
    int width;
    // @brief The height of the screen in pixels.
    int height;
    // @brief x coordinate in pixels of the lower left corner of the rendering
    // area rectangle.
    int viewport_x;
    // @brief y coordinate in pixels of the lower left corner of the rendering
    // area rectangle.
    int viewport_y;
    // @brief The width of the rendering area rectangle in pixels.
    int viewport_width;
    // @brief The height of the rendering area rectangle in pixels.
    int viewport_height;
  };

  // @brief Holds eye information.
  struct EyeData {
    // @brief The eye-from-head homogeneous transformation for the eye.
    float eye_from_head_matrix[16];

    // @brief The field of view angles.
    // @details They are disposed as [left, right, bottom, top] and are in
    //          radians.
    float fov[4];

    // @brief Cardboard distortion mesh for the eye.
    CardboardMesh distortion_mesh;

    // @brief Cardboard texture description for the eye.
    CardboardEyeTextureDescription texture;
  };

  // @brief Custom deleter for DistortionRenderer.
  struct CardboardDistortionRendererDeleter {
    void operator()(CardboardDistortionRenderer* distortion_renderer) {
      CardboardDistortionRenderer_destroy(distortion_renderer);
    }
  };

  // @brief Configures rendering resources.
  void RenderingResourcesSetup();

  // @brief Frees rendering resources.
  void RenderingResourcesTeardown();

  /// @brief Creates a variable of type Renderer::ScreenParams with the
  /// information provided by a variable of type ScreenParams.
  /// @param[in] screen_params Variable with the input information.
  ///
  /// @return A Renderer::ScreenParams variable.
  Renderer::ScreenParams ScreenParamsToRendererScreenParams(
      const ScreenParams& screen_params) const;

  /// @brief Creates a variable of type Renderer::ScreenParams with the
  /// information provided by a variable of type ScreenParams.
  /// @param[in] screen_params Variable with the input information.
  ///
  /// @return A Renderer::ScreenParams variable.
  Renderer::ScreenParams ScreenParamsToRendererSreenParams(
      const ScreenParams& screen_params) const;

  // @brief Default z-axis coordinate for the near clipping plane.
  static constexpr float kZNear = 0.1f;

  // @brief Default z-axis coordinate for the far clipping plane.
  static constexpr float kZFar = 10.0f;

  // @brief DistortionRenderer native pointer.
  std::unique_ptr<CardboardDistortionRenderer,
                  CardboardDistortionRendererDeleter>
      distortion_renderer_;

  // @brief Screen parameters.
  // @details Must be used by rendering calls (or those to set up the pipeline).
  ScreenParams screen_params_;

  // @brief Eye data information.
  // @details `CardboardEye::kLeft` index holds left eye data and
  //          `CardboardEye::kRight` holds the right eye data.
  std::array<EyeData, 2> eye_data_;

  // @brief Holds the render texture information for each eye.
  std::array<Renderer::RenderTexture, 2> render_textures_;

  // @brief Manages the rendering elements lifecycle.
  std::unique_ptr<Renderer> renderer_;

  // @brief Store Unity reported screen params.
  static std::atomic<ScreenParams> unity_screen_params_;

  // @brief Unity-loaded widgets
  static std::vector<Renderer::WidgetParams> widget_params_;

  // @brief Mutex for widget_params_ access.
  static std::mutex widget_mutex_;

  // @brief Track changes to device parameters.
  static std::atomic<bool> device_params_changed_;

  // @brief Holds the selected graphics API.
  static std::atomic<CardboardGraphicsApi> selected_graphics_api_;

  // @brief Holds the Unity XR interfaces.
  static IUnityInterfaces* xr_interfaces_;
};

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Sets screen dimensions and rendering area rectangle in pixels.
/// @details It is expected to be called at
///          CardboardXRLoader::Initialize() from C# code when loading the
///          provider. Provided parameters will be returned by
///          CardboardDisplayApi::GetScreenParams().
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

#ifdef __cplusplus
}
#endif

}  // namespace cardboard::unity

#endif  // CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_DISPLAY_API_H_
