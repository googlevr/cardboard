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
#ifndef CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_RENDERER_H_
#define CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_RENDERER_H_

#include <stdint.h>

#include <memory>
#include <mutex>
#include <vector>

#include "include/cardboard.h"
#include "IUnityInterface.h"

namespace cardboard::unity {

/// Manages the interaction of the Cardboard XR Plugin with different rendering
/// APIs. The supported rendering APIs are expected to extend this class and
/// provide a custom method to construct an instance of it.
class Renderer {
 public:
  /// @brief Data about drawing a custom widget.
  struct WidgetParams {
    /// @brief Texture ID. This field holds a Texture.GetNativeTexturePtr.
    /// @see https://docs.unity3d.com/ScriptReference/Texture.GetNativeTexturePtr.html
    intptr_t texture;
    /// @brief x Widget X coordinate in pixels.
    int x;
    /// @brief y Widget Y coordinate in pixels.
    int y;
    /// @brief width Widget width in pixels.
    int width;
    /// @brief height Widget height in pixels.
    int height;
  };

  /// @brief Holds the screen and rendering area details.
  struct ScreenParams {
    /// @brief The width of the screen in pixels.
    int width;
    /// @brief The height of the screen in pixels.
    int height;
    /// @brief x coordinate in pixels of the lower left corner of the rendering
    /// area rectangle.
    int viewport_x;
    /// @brief y coordinate in pixels of the lower left corner of the rendering
    /// area rectangle.
    int viewport_y;
    /// @brief The width of the rendering area rectangle in pixels.
    int viewport_width;
    /// @brief The height of the rendering area rectangle in pixels.
    int viewport_height;
  };

  /// @brief Holds the texture and depth buffer for each eye.
  struct RenderTexture {
    /// @brief Texture color buffer ID. When using OpenGL ES 2.x and OpenGL
    ///     ES 3.x, this field holds a GLuint variable. When using Metal, this
    ///     field holds an IOSurfaceRef variable.
    uint64_t color_buffer = 0;
    /// @brief Texture depth buffer ID. When using OpenGL ES 2.x and OpenGL
    ///     ES 3.x, this field holds a GLuint variable. When using Metal, this
    ///     field is unused.
    uint64_t depth_buffer = 0;
  };

  virtual ~Renderer() = default;

  /// @brief Initializes resources.
  /// @pre It must be called from the rendering thread.
  virtual void SetupWidgets() = 0;

  /// @brief Render the Cardboard widgets (X, Gear, divider line) to the screen.
  /// @pre It must be called from the rendering thread.
  ///
  /// @param screen_params The screen and rendering area details.
  /// @param widgets The list of widgets to rendered.
  virtual void RenderWidgets(const ScreenParams& screen_params,
                             const std::vector<WidgetParams>& widgets) = 0;

  /// @brief Deinitializes taken resources.
  /// @pre It must be called from the rendering thread.
  virtual void TeardownWidgets() = 0;

  /// @brief Creates and configures resources in a RenderTexture.
  ///
  /// @param render_texture A RenderTexture to load its resources.
  /// @param screen_width The width in pixels of the rectangle.
  /// @param screen_height The height in pixels of the rectangle.
  virtual void CreateRenderTexture(RenderTexture* render_texture,
                                   int screen_width, int screen_height) = 0;

  /// @brief Releases resources in a RenderTexture.
  ///
  /// @param render_texture A RenderTexture to release its resources.
  virtual void DestroyRenderTexture(RenderTexture* render_texture) = 0;

  /// @brief Renders eye textures onto the display.
  ///
  /// @param[in] renderer Distortion renderer object pointer.
  /// @param[in] screen_params The screen and rendering area details.
  /// @param[in] left_eye Left eye texture description.
  /// @param[in] right_eye Right eye texture description.
  virtual void RenderEyesToDisplay(
      CardboardDistortionRenderer* renderer, const ScreenParams& screen_params,
      const CardboardEyeTextureDescription* left_eye,
      const CardboardEyeTextureDescription* right_eye) = 0;

  /// @brief Runs commands needed before rendering.
  ///
  /// @param[in] screen_params The screen and rendering area details.
  virtual void RunRenderingPreProcessing(const ScreenParams& screen_params) = 0;

  /// @brief Runs commands needed after rendering.
  virtual void RunRenderingPostProcessing() = 0;

 protected:
  // Constructs a Renderer.
  //
  // @details Each rendering API should have its own implementation. Use the
  // appropriate functions in this file to get an instance of this class.
  Renderer() = default;
};

/// Constructs a Renderer implementation for OpenGL ES2.
///
/// @return A pointer to a OpenGL ES2 based Renderer implementation.
std::unique_ptr<Renderer> MakeOpenGlEs2Renderer();

/// Constructs a Renderer implementation for OpenGL ES3.
///
/// @return A pointer to a OpenGL ES3 based Renderer implementation.
std::unique_ptr<Renderer> MakeOpenGlEs3Renderer();

#if defined(__APPLE__)
/// Constructs a Renderer implementation for Metal.
///
/// @param xr_interfaces Pointer to Unity XR interface provider to obtain the
///        Metal context.
/// @return A pointer to a Metal based Renderer implementation.
std::unique_ptr<Renderer> MakeMetalRenderer(IUnityInterfaces* xr_interfaces);

/// Constructs a Cardboard Distortion Renderer implementation for Metal.
///
/// @param xr_interfaces Pointer to Unity XR interface provider to obtain the
///     Metal context.
/// @return A pointer to a Metal based Cardboard Distortion Renderer
///     implementation.
CardboardDistortionRenderer* MakeCardboardMetalDistortionRenderer(
    IUnityInterfaces* xr_interfaces);
#endif
#if defined(__ANDROID__)
/// Constructs a Renderer implementation for Vulkan.
///
/// @param xr_interfaces Pointer to Unity XR interface provider to obtain the
///        Vulkan context.
/// @return A pointer to a Vulkan based Renderer implementation.
std::unique_ptr<Renderer> MakeVulkanRenderer(IUnityInterfaces* xr_interfaces);

/// Constructs a Cardboard Distortion Renderer implementation for Vulkan.
///
/// @param xr_interfaces Pointer to Unity XR interface provider to obtain the
///     Vulkan context.
/// @return A pointer to a Vulkan based Cardboard Distortion Renderer
///     implementation.
CardboardDistortionRenderer* MakeCardboardVulkanDistortionRenderer(
    IUnityInterfaces* xr_interfaces);
#endif

}  // namespace cardboard::unity

#endif  // CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_RENDERER_H_
