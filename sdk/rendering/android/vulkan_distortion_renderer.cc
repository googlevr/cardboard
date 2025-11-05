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
#include <sys/types.h>

#include <array>
#include <cstdint>
#include <vector>

#include "distortion_renderer.h"
#include "include/cardboard.h"
#include "rendering/android/shaders/distortion_frag.spv.h"
#include "rendering/android/shaders/distortion_vert.spv.h"
#include "rendering/android/vulkan/android_vulkan_loader.h"
#include "util/is_arg_null.h"
#include "util/is_initialized.h"
#include "util/logging.h"
#include <vulkan/vulkan.h>

// Vulkan call wrapper
#define CALL_VK(func)                                                    \
  {                                                                      \
    VkResult vkResult = (func);                                          \
    if (VK_SUCCESS != vkResult) {                                        \
      CARDBOARD_LOGE("Vulkan error. Error Code[%d], File[%s], line[%d]", \
                     vkResult, __FILE__, __LINE__);                      \
    }                                                                    \
  }

namespace cardboard::rendering {

struct PushConstantsObject {
  float left_u;
  float right_u;
  float top_v;
  float bottom_v;
};

struct Vertex {
  float pos_x;
  float pos_y;
  float tex_u;
  float tex_v;
};

class VulkanDistortionRenderer : public DistortionRenderer {
 public:
  explicit VulkanDistortionRenderer(
      const CardboardVulkanDistortionRendererConfig* config) {
    if (!LoadVulkan()) {
      CARDBOARD_LOGE("Failed to load vulkan lib in cardboard!");
      return;
    }

    physical_device_ =
        *reinterpret_cast<VkPhysicalDevice*>(config->physical_device);
    logical_device_ = *reinterpret_cast<VkDevice*>(config->logical_device);
    swapchain_ = *reinterpret_cast<VkSwapchainKHR*>(config->vk_swapchain);
    CALL_VK(vkGetSwapchainImagesKHR(logical_device_, swapchain_,
                                    &swapchain_image_count_,
                                    nullptr /* pSwapchainImages */));

    CreateSharedVulkanObjects();
    CreatePerEyeVulkanObjects(kLeft);
    CreatePerEyeVulkanObjects(kRight);
  }

  ~VulkanDistortionRenderer() {
    for (uint32_t i = 0; i < swapchain_image_count_; i++) {
      CleanTextureImageView(kLeft, i);
      CleanTextureImageView(kRight, i);
    }

    vkDestroySampler(logical_device_, texture_sampler_, nullptr);
    vkDestroyPipelineLayout(logical_device_, pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(logical_device_, descriptor_set_layout_,
                                 nullptr);

    vkDestroyDescriptorPool(logical_device_, descriptor_pool_[kLeft], nullptr);
    vkDestroyDescriptorPool(logical_device_, descriptor_pool_[kRight], nullptr);

    CleanPipeline(kLeft);
    CleanPipeline(kRight);

    vkDestroyBuffer(logical_device_, index_buffers_[kLeft], nullptr);
    vkFreeMemory(logical_device_, index_buffers_memory_[kLeft], nullptr);
    vkDestroyBuffer(logical_device_, index_buffers_[kRight], nullptr);
    vkFreeMemory(logical_device_, index_buffers_memory_[kRight], nullptr);

    vkDestroyBuffer(logical_device_, vertex_buffers_[kLeft], nullptr);
    vkFreeMemory(logical_device_, vertex_buffers_memory_[kLeft], nullptr);
    vkDestroyBuffer(logical_device_, vertex_buffers_[kRight], nullptr);
    vkFreeMemory(logical_device_, vertex_buffers_memory_[kRight], nullptr);
  }

  void SetMesh(const CardboardMesh* mesh, CardboardEye eye) override {
    // Create Vertex buffer
    std::vector<Vertex> vertices;
    vertices.resize(mesh->n_vertices);
    for (int i = 0; i < mesh->n_vertices; i++) {
      vertices[i].pos_x = mesh->vertices[2 * i];
      vertices[i].pos_y = mesh->vertices[2 * i + 1];
      vertices[i].tex_u = mesh->uvs[2 * i];
      vertices[i].tex_v = mesh->uvs[2 * i + 1];
    }

    VkDeviceSize vertex_buffer_size = sizeof(vertices[0]) * vertices.size();
    CreateBuffer(vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertex_buffers_[eye], vertex_buffers_memory_[eye]);

    void* vertex_data;
    CALL_VK(vkMapMemory(logical_device_, vertex_buffers_memory_[eye], 0,
                        vertex_buffer_size, 0, &vertex_data));
    memcpy(vertex_data, vertices.data(), vertex_buffer_size);
    vkUnmapMemory(logical_device_, vertex_buffers_memory_[eye]);

    // Create Index Buffer
    std::vector<uint16_t> indices;
    indices.resize(mesh->n_indices);
    for (int i = 0; i < mesh->n_indices; i++) {
      indices[i] = mesh->indices[i];
    }

    VkDeviceSize index_buffer_size = sizeof(indices[0]) * indices.size();
    CreateBuffer(index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 index_buffers_[eye], index_buffers_memory_[eye]);

    void* index_data;
    vkMapMemory(logical_device_, index_buffers_memory_[eye], 0,
                index_buffer_size, 0, &index_data);
    memcpy(index_data, indices.data(), index_buffer_size);
    vkUnmapMemory(logical_device_, index_buffers_memory_[eye]);

    indices_count_ = mesh->n_indices;
  }

  void RenderEyeToDisplay(
      uint64_t target, int x, int y, int width, int height,
      const CardboardEyeTextureDescription* left_eye,
      const CardboardEyeTextureDescription* right_eye) override {
    CardboardVulkanDistortionRendererTarget* render_target =
        reinterpret_cast<CardboardVulkanDistortionRendererTarget*>(target);
    VkCommandBuffer command_buffer =
        *reinterpret_cast<VkCommandBuffer*>(render_target->vk_command_buffer);
    VkRenderPass render_pass =
        *reinterpret_cast<VkRenderPass*>(render_target->vk_render_pass);
    uint32_t image_index = render_target->swapchain_image_index;

    if (image_index >= swapchain_image_count_) {
      CARDBOARD_LOGE(
          "Input swapchain image index is above the swapchain length");
      return;
    }

    if (render_pass != current_render_pass_) {
      current_render_pass_ = render_pass;
      CreateGraphicsPipeline(kLeft);
      CreateGraphicsPipeline(kRight);
    }

    RenderDistortionMesh(left_eye, kLeft, command_buffer, image_index, x, y,
                         width, height);
    RenderDistortionMesh(right_eye, kRight, command_buffer, image_index, x, y,
                         width, height);
  }

 private:
  void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& buffer_memory) {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CALL_VK(vkCreateBuffer(logical_device_, &buffer_info, nullptr, &buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(logical_device_, buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    alloc_info.allocationSize = mem_requirements.size,
    alloc_info.memoryTypeIndex =
        FindMemoryType(mem_requirements.memoryTypeBits, properties);

    CALL_VK(vkAllocateMemory(logical_device_, &alloc_info, nullptr,
                             &buffer_memory));

    vkBindBufferMemory(logical_device_, buffer, buffer_memory, 0);
  }

  /**
   * Create shared vulkan objects for two eyes.
   */
  void CreateSharedVulkanObjects() {
    // Create DescriptorSet Layout
    VkDescriptorSetLayoutBinding bindings[1];

    VkDescriptorSetLayoutBinding sampler_layout_binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
    bindings[0] = sampler_layout_binding;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = bindings;

    CALL_VK(vkCreateDescriptorSetLayout(logical_device_, &layout_info, nullptr,
                                        &descriptor_set_layout_));

    // Setup push constants.
    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstantsObject),
    };

    // Create Pipeline Layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout_;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
    CALL_VK(vkCreatePipelineLayout(logical_device_,
                                   &pipeline_layout_create_info, nullptr,
                                   &pipeline_layout_));

    // Create Texture Sampler
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device_, &properties);

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.pNext = nullptr;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = 0.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.unnormalizedCoordinates = VK_FALSE;

    CALL_VK(
        vkCreateSampler(logical_device_, &sampler, nullptr, &texture_sampler_));
  }

  /**
   * Create required vulkan objects for the given eye.
   *
   * @param eye CardboardEye input.
   */
  void CreatePerEyeVulkanObjects(CardboardEye eye) {
    // Create Descriptor Pool
    VkDescriptorPoolSize pool_sizes[1];
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount =
        static_cast<uint32_t>(swapchain_image_count_);

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = static_cast<uint32_t>(swapchain_image_count_);

    CALL_VK(vkCreateDescriptorPool(logical_device_, &pool_info, nullptr,
                                   &descriptor_pool_[eye]));

    // Create Descriptor Sets
    std::vector<VkDescriptorSetLayout> layouts(swapchain_image_count_,
                                               descriptor_set_layout_);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_[eye];
    alloc_info.descriptorSetCount =
        static_cast<uint32_t>(swapchain_image_count_);
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets_[eye].resize(swapchain_image_count_);
    CALL_VK(vkAllocateDescriptorSets(logical_device_, &alloc_info,
                                     descriptor_sets_[eye].data()));

    // Set the size of image view array to the swapchain length.
    image_views_[eye].resize(swapchain_image_count_);
  }

  /**
   * Create the graphics pipeline for the given eye.
   * It cleans the previous pipeline if it exists.
   *
   * @param eye CardboardEye input.
   *
   * @return VkPipeline the graphics pipeline output.
   */
  void CreateGraphicsPipeline(CardboardEye eye) {
    CleanPipeline(eye);

    VkShaderModule vertex_shader =
        LoadShader(distortion_vert, sizeof(distortion_vert));
    VkShaderModule fragment_shader =
        LoadShader(distortion_frag, sizeof(distortion_frag));

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo vertex_shader_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_shader,
        .pName = "main",
        .pSpecializationInfo = nullptr,
    };
    VkPipelineShaderStageCreateInfo fragment_shader_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_shader,
        .pName = "main",
        .pSpecializationInfo = nullptr,
    };

    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewport_info{};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.pNext = nullptr;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports = nullptr;
    viewport_info.scissorCount = 1;
    viewport_info.pScissors = nullptr;

    // Specify multisample info
    VkSampleMask sample_mask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisample_info{};
    multisample_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_info.pNext = nullptr;
    multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_info.sampleShadingEnable = VK_FALSE;
    multisample_info.minSampleShading = 0;
    multisample_info.pSampleMask = &sample_mask;
    multisample_info.alphaToCoverageEnable = VK_FALSE;
    multisample_info.alphaToOneEnable = VK_FALSE;

    // Specify color blend state
    VkPipelineColorBlendAttachmentState attachment_states{};
    attachment_states.blendEnable = VK_FALSE,
    attachment_states.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.pNext = nullptr;
    color_blend_info.flags = 0;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &attachment_states;

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo raster_info{};
    raster_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.pNext = nullptr;
    raster_info.depthClampEnable = VK_FALSE;
    raster_info.rasterizerDiscardEnable = VK_FALSE;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.cullMode = VK_CULL_MODE_NONE;
    raster_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster_info.depthBiasEnable = VK_FALSE;
    raster_info.lineWidth = 1;

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings = {
        .binding = 0,
        .stride = 4 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription vertex_input_attributes[2] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0,
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = sizeof(float) * 2,
        }};

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_bindings;
    vertex_input_info.vertexAttributeDescriptionCount = 2;
    vertex_input_info.pVertexAttributeDescriptions = vertex_input_attributes;

    VkDynamicState dynamic_state_enables[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_state_enables;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    // Create the pipeline
    VkPipelineShaderStageCreateInfo shader_stages[2] = {vertex_shader_state,
                                                        fragment_shader_state};
    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_info,
        .pMultisampleState = &multisample_info,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = &dynamic_state_info,
        .layout = pipeline_layout_,
        .renderPass = current_render_pass_,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };
    CALL_VK(vkCreateGraphicsPipelines(logical_device_, VK_NULL_HANDLE, 1,
                                      &pipeline_create_info, nullptr,
                                      &graphics_pipeline_[eye]));

    vkDestroyShaderModule(logical_device_, vertex_shader, nullptr);
    vkDestroyShaderModule(logical_device_, fragment_shader, nullptr);
  }

  VkShaderModule LoadShader(const uint32_t* const content, size_t size) const {
    VkShaderModule shader;
    VkShaderModuleCreateInfo shader_module_create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = size,
        .pCode = content,
    };
    CALL_VK(vkCreateShaderModule(logical_device_, &shader_module_create_info,
                                 nullptr, &shader));

    return shader;
  }

  /**
   * Find the memory type of the physical device.
   *
   * @param type_filter required memory type shift.
   * @param properties required memory flag bits.
   *
   * @return memory type or 0 if not found.
   */
  uint32_t FindMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (mem_properties.memoryTypes[i].propertyFlags & properties) ==
              properties) {
        return i;
      }
    }

    CARDBOARD_LOGE("Failed to find suitable memory type!");
    return 0;
  }

  /**
   * Set up render distortion mesh and bind them to the command buffer.
   *
   * @param eye_description Texture for the eye.
   * @param eye CardboardEye input.
   * @param command_buffer VkCommandBuffer to be bond.
   * @param image_index index of current image in the swapchain.
   * @param x x of the rendering area.
   * @param y y of the rendering area.
   * @param width width of the rendering area.
   * @param height height of the rendering area.
   */
  void RenderDistortionMesh(
      const CardboardEyeTextureDescription* eye_description, CardboardEye eye,
      VkCommandBuffer command_buffer, uint32_t image_index, int x, int y,
      int width, int height) {
    // Update Push constants.
    PushConstantsObject push_constants{
        .left_u = eye_description->left_u,
        .right_u = eye_description->right_u,
        .top_v = eye_description->bottom_v,  // Swap top and bottom to match the
                                            // texture coordinates.
        .bottom_v = eye_description->top_v,
    };
    vkCmdPushConstants(command_buffer, pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(PushConstantsObject), &push_constants);

    // Update image and view
    VkImage current_image = reinterpret_cast<VkImage>(eye_description->texture);
    CleanTextureImageView(eye, image_index);
    const VkImageViewCreateInfo view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = current_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    CALL_VK(vkCreateImageView(logical_device_, &view_create_info,
                              nullptr /* pAllocator */,
                              &image_views_[eye][image_index]));

    // Update Descriptor Sets
    VkDescriptorImageInfo image_info{
        .sampler = texture_sampler_,
        .imageView = image_views_[eye][image_index],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    VkWriteDescriptorSet descriptor_writes[1];

    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet = descriptor_sets_[eye][image_index];
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pImageInfo = &image_info;
    descriptor_writes[0].pNext = nullptr;

    vkUpdateDescriptorSets(logical_device_, 1, descriptor_writes, 0, nullptr);

    // Update Viewport and scissor
    // Swap the height to make the viewport bottom-left origin as expected by
    // the distortion mesh.
    VkViewport viewport = {.x = static_cast<float>(x),
                           .y = static_cast<float>(y + height),
                           .width = static_cast<float>(width),
                           .height = -static_cast<float>(height),
                           .minDepth = 0.0,
                           .maxDepth = 1.0};

    VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = {.width = static_cast<uint32_t>(width / 2),
                   .height = static_cast<uint32_t>(height)},
    };
    if (eye == kLeft) {
      scissor.offset = {.x = x, .y = y};
    } else {
      scissor.offset = {.x = static_cast<int32_t>(x + width / 2), .y = y};
    }

    // Bind to the command buffer.
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline_[eye]);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffers_[eye],
                           &offset);

    vkCmdBindIndexBuffer(command_buffer, index_buffers_[eye], 0,
                         VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout_, 0, 1,
                            &descriptor_sets_[eye][image_index], 0, nullptr);
    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices_count_), 1,
                     0, 0, 0);
  }

  /**
   * Clean the graphics pipeline of the given eye.
   *
   * @param eye CardboardEye input.
   */
  void CleanPipeline(CardboardEye eye) {
    if (graphics_pipeline_[eye] != VK_NULL_HANDLE) {
      vkDestroyPipeline(logical_device_, graphics_pipeline_[eye], nullptr);
      graphics_pipeline_[eye] = VK_NULL_HANDLE;
    }
  }

  /**
   * Clean the image view of the given eye and swapchain image index.
   *
   * @param eye CardboardEye input.
   * @param index The index of the image in the swapchain.
   */
  void CleanTextureImageView(CardboardEye eye, int index) {
    if (image_views_[eye][index] != VK_NULL_HANDLE) {
      vkDestroyImageView(logical_device_, image_views_[eye][index],
                         nullptr /* vkDestroyImageView */);
      image_views_[eye][index] = VK_NULL_HANDLE;
    }
  }

  // Variables created externally.
  VkPhysicalDevice physical_device_;
  VkDevice logical_device_;
  VkSwapchainKHR swapchain_;
  VkRenderPass current_render_pass_;
  int indices_count_;

  // Variables created and maintained by the distortion renderer.
  uint32_t swapchain_image_count_;
  VkSampler texture_sampler_;
  VkDescriptorSetLayout descriptor_set_layout_;
  VkPipelineLayout pipeline_layout_;
  VkPipeline graphics_pipeline_[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
  VkBuffer vertex_buffers_[2];
  VkDeviceMemory vertex_buffers_memory_[2];
  VkBuffer index_buffers_[2];
  VkDeviceMemory index_buffers_memory_[2];
  VkDescriptorPool descriptor_pool_[2];
  std::vector<VkDescriptorSet> descriptor_sets_[2];
  std::vector<VkImageView> image_views_[2];
};

}  // namespace cardboard::rendering

extern "C" {

CardboardDistortionRenderer* CardboardVulkanDistortionRenderer_create(
    const CardboardVulkanDistortionRendererConfig* config) {
  if (CARDBOARD_IS_NOT_INITIALIZED() || CARDBOARD_IS_ARG_NULL(config)) {
    return nullptr;
  }

  return reinterpret_cast<CardboardDistortionRenderer*>(
      new cardboard::rendering::VulkanDistortionRenderer(config));
}

}  // extern "C"
