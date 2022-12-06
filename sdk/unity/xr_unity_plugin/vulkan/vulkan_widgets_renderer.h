/*
 * Copyright 2022 Google LLC
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
#ifndef CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_VULKAN_VULKAN_WIDGETS_RENDERER_H_
#define CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_VULKAN_VULKAN_WIDGETS_RENDERER_H_

#include <cstdint>
#include <vector>

#include "rendering/android/vulkan/android_vulkan_loader.h"
#include "unity/xr_unity_plugin/renderer.h"

namespace cardboard::unity {

/**
 * Helper Class to render widgets with Vulkan. Receives images and renders them
 * as square textures.
 */
class VulkanWidgetsRenderer {
 public:
  /**
   * Constructs a VulkanWidgetsRenderer.
   *
   * @param physical_device Vulkan physical device.
   * @param logical_device Vulkan logical device.
   * @param swapchain_image_count Number of images available in the swapchain.
   */
  VulkanWidgetsRenderer(VkPhysicalDevice physical_device,
                        VkDevice logical_device,
                        const int swapchain_image_count);

  /**
   * Destructor. Frees renderer resources.
   */
  ~VulkanWidgetsRenderer();

  /**
   * Attaches the widgets pipelines to the command buffer.
   *
   * @param screen_params Screen parameters of the rendering area.
   * @param widgets_params Params for each widget. This includes position to
   * render and texture.
   * @param command_buffer VkCommandBuffer to be bond.
   * @param swapchain_image_index Swapchain image to be rendered.
   * @param render_pass Render pass used.
   */
  void RenderWidgets(const Renderer::ScreenParams& screen_params,
                     const std::vector<Renderer::WidgetParams>& widgets_params,
                     const VkCommandBuffer command_buffer,
                     const uint32_t swapchain_image_index,
                     const VkRenderPass render_pass);

 private:
  /**
   * @struct Vertex Information to be processed in the vertex shader.
   */
  struct Vertex {
    // pos_x: x-axis position in the screen where the vertex of the image will
    // be rendered. Normalized between -1 and 1.
    float pos_x;
    // pos_y: y-axis position in the screen where the vertex of the image will
    // be rendered. Normalized between -1 and 1. Take into account that in
    // Vulkan, the y axis is upside down. So -1 is the top of the screen.
    float pos_y;
    // tex_u: x-axis position of the texture to be renderer. Normalized between
    // 0 and 1.
    float tex_u;
    // tex_v: y-axis position of the texture to be renderer. Normalized between
    // 0 and 1.
    float tex_v;
  };

  /**
   * @struct Data required for each widget.
   */
  struct PerWidgetData {
    VkDescriptorPool descriptor_pool;
    // Size should be the size of the swapchain.
    std::vector<VkDescriptorSet> descriptor_sets;
    // Size should be the size of the swapchain.
    std::vector<VkImageView> image_views;
  };

  static constexpr float Lerp(float start, float end, float val) {
    return start + (end - start) * val;
  }

  /**
   * Updates the vertex buffers with the information given by the widgets
   * params.
   *
   * @param widgets_params Params for each widget with the position to
   * render.
   * @param screen_params Screen parameters of the rendering area.
   */
  void UpdateVertexBuffers(
      const std::vector<unity::Renderer::WidgetParams>& widgets_params,
      const unity::Renderer::ScreenParams& screen_params);

  /**
   * Updates the vertex buffer of the given index woth the information given by
   * the widget params.
   *
   * @param widget_params Params for a widget with the position to
   * render.
   * @param screen_params Screen parameters of the rendering area.
   * @param index Index of the buffer to be updated.
   */
  void UpdateVertexBuffer(const unity::Renderer::WidgetParams& widget_params,
                          const unity::Renderer::ScreenParams& screen_params,
                          const uint32_t index);

  /**
   * Creates a Vulkan buffer.
   * @param size Size of the buffer in bytes.
   * @param usage Bitmask of VkBufferUsageFlagBits specifying allowed usages of
   * the buffer.
   * @param properties Required memory flag bits.
   * @param buffer Handle in which the resulting buffer object is stored.
   * @param buffer_memory Memory for the buffer allocated in the graphics card.
   */
  void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& buffer_memory);

  /**
   * Creates shared vulkan objects for every widget.
   */
  void CreateSharedVulkanObjects();

  /**
   * Creates required vulkan objects for the given widget.
   *
   * @param widget Number of the widget.
   */
  void SetWidgetImageCount(const uint32_t widget);

  /**
   * Creates the graphics pipeline.
   * It cleans the previous pipeline if it exists.
   */
  void CreateGraphicsPipeline();

  /**
   * Loads a shader module.
   *
   * @param content Content of the shader.
   * @param size Sizeof the shader content.
   *
   * @return A shader module.
   */
  VkShaderModule LoadShader(const uint32_t* const content, size_t size) const;

  /**
   * Finds the memory type of the physical device.
   *
   * @param type_filter Required memory type shift.
   * @param properties Required memory flag bits.
   *
   * @return Memory type or 0 if not found.
   */
  uint32_t FindMemoryType(uint32_t type_filter,
                          VkMemoryPropertyFlags properties);

  /**
   * Set up render widgets and bind them to the command buffer.
   *
   * @param widget_params Texture for the widget.
   * @param command_buffer VkCommandBuffer to be bond.
   * @param swapchain_image_index Index of current image in the image views
   * array.
   * @param screen_params Screen parameters of the rendering area.
   */
  void RenderWidget(const unity::Renderer::WidgetParams& widget_params,
                    VkCommandBuffer command_buffer, const uint32_t widget_index,
                    const uint32_t swapchain_image_index,
                    const unity::Renderer::ScreenParams& screen_params);

  /**
   * Cleans the graphics pipeline.
   */
  void CleanPipeline();

  /**
   * Cleans the image view of the given widget and swapchain image index.
   *
   * @param widget_index The index of the widget.
   * @param swapchain_image_index The index of the image in the swapchain.
   */
  void CleanTextureImageView(const int widget_index,
                             const int swapchain_image_index);

  /**
   * Creates a vertex buffer and store it internally.
   *
   * @param vertices Content of the vertex buffer.
   * @param widget_index The index of the widget related to the buffer.
   */
  void CreateVertexBuffer(std::vector<Vertex> vertices,
                          const uint32_t widget_index);

  /**
   * Cleans a vertex buffer.
   *
   * @param widget_index The index of the widget related to the buffer.
   */
  void CleanVertexBuffer(const uint32_t widget_index);

  /**
   * Creates an index buffer and store it internally.
   *
   * @param indices Content of the index buffer.
   */
  void CreateIndexBuffer(std::vector<uint16_t> indices);

  // Variables created externally.
  VkPhysicalDevice physical_device_;
  VkDevice logical_device_;
  VkRenderPass current_render_pass_;

  // Variables created and maintained by the widget renderer.
  uint32_t swapchain_image_count_;
  int indices_count_;
  VkSampler texture_sampler_;
  VkDescriptorSetLayout descriptor_set_layout_;
  VkPipelineLayout pipeline_layout_;
  VkPipeline graphics_pipeline_ = {VK_NULL_HANDLE};
  std::vector<VkBuffer> vertex_buffers_;
  std::vector<VkDeviceMemory> vertex_buffers_memory_;
  VkBuffer index_buffers_;
  VkDeviceMemory index_buffers_memory_;
  std::vector<PerWidgetData> widgets_data_;
  std::vector<Renderer::WidgetParams> current_widget_params_;
};

}  // namespace cardboard::unity

#endif  // CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_VULKAN_VULKAN_WIDGETS_RENDERER_H_