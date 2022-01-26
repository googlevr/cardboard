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

struct UniformBufferObject {
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
    render_pass_ = *reinterpret_cast<VkRenderPass*>(config->render_pass);
    command_buffers_ =
        reinterpret_cast<VkCommandBuffer*>(config->command_buffers);
    swapchain_image_count_ = config->swapchain_image_count;
    image_width_ = config->image_width;
    image_height_ = config->image_height;

    CreateSharedVulkanObjects();
    CreatePerEyeVulkanObjects(kLeft);
    CreatePerEyeVulkanObjects(kRight);
  }

  ~VulkanDistortionRenderer() {
    vkDestroySampler(logical_device_, texture_sampler_, nullptr);
    vkDestroyPipelineLayout(logical_device_, pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(logical_device_, descriptor_set_layout_,
                                 nullptr);

    vkDestroyDescriptorPool(logical_device_, descriptor_pool_[0], nullptr);
    vkDestroyDescriptorPool(logical_device_, descriptor_pool_[1], nullptr);

    vkDestroyPipeline(logical_device_, graphics_pipeline_[0], nullptr);
    vkDestroyPipeline(logical_device_, graphics_pipeline_[1], nullptr);

    vkDestroyBuffer(logical_device_, index_buffers_[0], nullptr);
    vkFreeMemory(logical_device_, index_buffers_memory_[0], nullptr);
    vkDestroyBuffer(logical_device_, index_buffers_[1], nullptr);
    vkFreeMemory(logical_device_, index_buffers_memory_[1], nullptr);

    vkDestroyBuffer(logical_device_, vertex_buffers_[0], nullptr);
    vkFreeMemory(logical_device_, vertex_buffers_memory_[0], nullptr);
    vkDestroyBuffer(logical_device_, vertex_buffers_[1], nullptr);
    vkFreeMemory(logical_device_, vertex_buffers_memory_[1], nullptr);

    vkDestroyBuffer(logical_device_, uniform_buffers_[0], nullptr);
    vkFreeMemory(logical_device_, uniform_buffers_memory_[0], nullptr);
    vkDestroyBuffer(logical_device_, uniform_buffers_[1], nullptr);
    vkFreeMemory(logical_device_, uniform_buffers_memory_[1], nullptr);
  }

  void SetMesh(const CardboardMesh* mesh, CardboardEye eye) override {
    std::vector<Vertex> vertices;
    vertices.resize(mesh->n_vertices);
    for (int i = 0; i < mesh->n_vertices; i++) {
      vertices[i].pos_x = mesh->vertices[2 * i];
      vertices[i].pos_y = mesh->vertices[2 * i + 1];
      vertices[i].tex_u = mesh->uvs[2 * i];
      vertices[i].tex_v = mesh->uvs[2 * i + 1];
    }
    CreateVertexBuffer(eye, vertices);

    std::vector<uint16_t> indices;
    indices.resize(mesh->n_indices);
    for (int i = 0; i < mesh->n_indices; i++) {
      indices[i] = mesh->indices[i];
    }
    CreateIndexBuffer(eye, indices);

    BindCommandBuffers(eye, mesh->n_indices);
  }

  void RenderEyeToDisplay(
      uint64_t target, int x, int y, int width, int height,
      const CardboardEyeTextureDescription* left_eye,
      const CardboardEyeTextureDescription* right_eye) override {
    CardboardVulkanDistortionRendererTarget* render_target =
        reinterpret_cast<CardboardVulkanDistortionRendererTarget*>(target);
    VkQueue queue = *reinterpret_cast<VkQueue*>(render_target->vk_queue);
    VkFence fence = *reinterpret_cast<VkFence*>(render_target->vk_fence);
    VkSemaphore* semaphore =
        reinterpret_cast<VkSemaphore*>(render_target->vk_semaphore);
    VkSwapchainKHR* swapchain =
        reinterpret_cast<VkSwapchainKHR*>(render_target->vk_swapchain);

    image_width_ = width;
    image_height_ = height;

    viewport_[kLeft].x = x;
    viewport_[kLeft].y = y;
    viewport_[kLeft].width = image_width_ / 2;
    viewport_[kLeft].height = image_height_;
    scissor_[kLeft].offset = {.x = x, .y = y};
    scissor_[kLeft].extent = {.width = static_cast<uint32_t>(image_width_ / 2),
                              .height = static_cast<uint32_t>(image_height_)};
    RenderDistortionMesh(left_eye, kLeft, render_target->swapchain_image_index);

    viewport_[kRight].x = x + image_width_ / 2;
    viewport_[kRight].x = y;
    viewport_[kRight].width = image_width_ / 2;
    viewport_[kRight].height = height;
    scissor_[kRight].offset = {.x = static_cast<int32_t>(x + image_width_ / 2),
                               .y = y};
    scissor_[kRight].extent = {.width = static_cast<uint32_t>(image_width_ / 2),
                               .height = static_cast<uint32_t>(image_height_)};
    RenderDistortionMesh(right_eye, kRight,
                         render_target->swapchain_image_index);

    vkResetFences(logical_device_, 1, &fence);

    const VkPipelineStageFlags wait_stage_mask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = semaphore,
        .pWaitDstStageMask = &wait_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers =
            &command_buffers_[render_target->swapchain_image_index],
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr};

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .swapchainCount = 1,
        .pSwapchains = swapchain,
        .pImageIndices = &render_target->swapchain_image_index,
    };

    CALL_VK(vkQueueSubmit(queue, 1, &submit_info, fence));
    CALL_VK(vkWaitForFences(logical_device_, 1 /* fenceCount */, &fence,
                            VK_TRUE, 100000000));

    // TODO(b/209801806): Fix error message once the integration with Unity is
    // finished.
    CALL_VK(vkQueuePresentKHR(queue, &present_info));
  }

 private:
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

  void CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding bindings[2];

    VkDescriptorSetLayoutBinding sampler_layout_binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
    bindings[0] = sampler_layout_binding;

    VkDescriptorSetLayoutBinding ubo_layout_binding{
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };
    bindings[1] = ubo_layout_binding;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    CALL_VK(vkCreateDescriptorSetLayout(logical_device_, &layout_info, nullptr,
                                        &descriptor_set_layout_));
  }

  void CreatePipelineLayout() {
    // Create the pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout_,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    CALL_VK(vkCreatePipelineLayout(logical_device_, &pipeline_layout_create_info,
                                   nullptr, &pipeline_layout_));
  }

  void CreateTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device_, &properties);

    VkSamplerCreateInfo sampler = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    };

    CALL_VK(
        vkCreateSampler(logical_device_, &sampler, nullptr, &texture_sampler_));
  }

  void CreateDescriptorPool(CardboardEye eye) {
    VkDescriptorPoolSize pool_sizes[2];
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount =
        static_cast<uint32_t>(swapchain_image_count_);
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[1].descriptorCount =
        static_cast<uint32_t>(swapchain_image_count_);

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = static_cast<uint32_t>(swapchain_image_count_);

    CALL_VK(vkCreateDescriptorPool(logical_device_, &pool_info, nullptr,
                                   &descriptor_pool_[eye]));
  }

  void CreateDescriptorSets(CardboardEye eye) {
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
  }

  void CreateGraphicsPipeline(CardboardEye eye) {
    VkShaderModule vertex_shader =
        LoadShader(distortion_vert, sizeof(distortion_vert));
    VkShaderModule fragment_shader =
        LoadShader(distortion_frag, sizeof(distortion_frag));

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shader_stages[2]{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        }};

    viewport_[eye].x = 0;
    viewport_[eye].y = 0;
    viewport_[eye].width = (float)image_width_;
    viewport_[eye].height = (float)image_height_;
    viewport_[eye].minDepth = 0.0f;
    viewport_[eye].maxDepth = 1.0f;

    scissor_[eye].offset = {
        .x = static_cast<int32_t>(eye == kLeft ? 0 : image_width_ / 2), .y = 0};
    scissor_[eye].extent = {.width = image_width_ / 2, .height = image_height_};

    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewport_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &viewport_[eye],
        .scissorCount = 1,
        .pScissors = &scissor_[eye],
    };

    // Specify multisample info
    VkSampleMask sample_mask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisample_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
        .pSampleMask = &sample_mask,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state
    VkPipelineColorBlendAttachmentState attachment_states{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &attachment_states,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo raster_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings{
        .binding = 0,
        .stride = 4 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription vertex_input_attributes[2]{
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

    VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_input_bindings,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    VkDynamicState dynamic_state_enables[2];  // Viewport + Scissor
    memset(dynamic_state_enables, 0, sizeof dynamic_state_enables);
    dynamic_state_enables[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_state_enables[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo dynamic_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .dynamicStateCount = 0,
        .pDynamicStates = nullptr};

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipeline_create_info{
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
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = &dynamic_state_info,
        .layout = pipeline_layout_,
        .renderPass = render_pass_,
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

  void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& buffer_memory) {
    VkBufferCreateInfo buffer_info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                  .size = size,
                                  .usage = usage,
                                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

    CALL_VK(vkCreateBuffer(logical_device_, &buffer_info, nullptr, &buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(logical_device_, buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex =
            FindMemoryType(mem_requirements.memoryTypeBits, properties)};

    CALL_VK(
        vkAllocateMemory(logical_device_, &alloc_info, nullptr, &buffer_memory));

    vkBindBufferMemory(logical_device_, buffer, buffer_memory, 0);
  }

  void CreateVertexBuffer(CardboardEye eye, std::vector<Vertex> vertices) {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    CreateBuffer(buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertex_buffers_[eye], vertex_buffers_memory_[eye]);

    void* data;
    CALL_VK(vkMapMemory(logical_device_, vertex_buffers_memory_[eye], 0,
                        buffer_size, 0, &data));
    memcpy(data, vertices.data(), buffer_size);
    vkUnmapMemory(logical_device_, vertex_buffers_memory_[eye]);
  }

  void CreateIndexBuffer(CardboardEye eye, std::vector<uint16_t> indices) {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    CreateBuffer(buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 index_buffers_[eye], index_buffers_memory_[eye]);

    void* data;
    vkMapMemory(logical_device_, index_buffers_memory_[eye], 0, buffer_size, 0,
                &data);
    memcpy(data, indices.data(), buffer_size);
    vkUnmapMemory(logical_device_, index_buffers_memory_[eye]);
  }

  void CreateUniformBuffers(CardboardEye eye) {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniform_buffers_[eye], uniform_buffers_memory_[eye]);
  }

  void CreateSharedVulkanObjects() {
    CreateDescriptorSetLayout();
    CreatePipelineLayout();
    CreateTextureSampler();
  }

  void CreatePerEyeVulkanObjects(CardboardEye eye) {
    CreateDescriptorPool(eye);
    CreateUniformBuffers(eye);
    CreateDescriptorSets(eye);
    CreateGraphicsPipeline(eye);
  }

  void BindCommandBuffers(CardboardEye eye, int indicesCount) {
    for (uint32_t i = 0; i < swapchain_image_count_; i++) {
      vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        graphics_pipeline_[eye]);
      vkCmdSetViewport(command_buffers_[i], 0, 1, &viewport_[eye]);
      vkCmdSetScissor(command_buffers_[i], 0, 1, &scissor_[eye]);

      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, &vertex_buffers_[eye],
                             &offset);

      vkCmdBindIndexBuffer(command_buffers_[i], index_buffers_[eye], 0,
                           VK_INDEX_TYPE_UINT16);

      vkCmdBindDescriptorSets(command_buffers_[i],
                              VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_,
                              0, 1, &descriptor_sets_[eye][i], 0, nullptr);
      vkCmdDrawIndexed(command_buffers_[i], static_cast<uint32_t>(indicesCount),
                       1, 0, 0, 0);
    }
  }

  void UpdateUniformBuffer(CardboardEye eye, UniformBufferObject ubo) const {
    void* data;
    vkMapMemory(logical_device_, uniform_buffers_memory_[eye], 0, sizeof(ubo),
                0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(logical_device_, uniform_buffers_memory_[eye]);
  }

  void UpdateDescriptorSets(CardboardEye eye, int index,
                            VkImageView image_view) const {
    VkDescriptorBufferInfo buffer_info{.buffer = uniform_buffers_[eye],
                                      .offset = 0,
                                      .range = sizeof(UniformBufferObject)};

    VkDescriptorImageInfo image_info{
        .sampler = texture_sampler_,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptor_writes[2];

    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet = descriptor_sets_[eye][index];
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pImageInfo = &image_info;

    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet = descriptor_sets_[eye][index];
    descriptor_writes[1].dstBinding = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(logical_device_, 2, descriptor_writes, 0, nullptr);
  }

  void RenderDistortionMesh(
      const CardboardEyeTextureDescription* eye_description, CardboardEye eye,
      uint32_t image_index) const {
    UniformBufferObject ubo{
        .left_u = eye_description->left_u,
        .right_u = eye_description->right_u,
        .top_v = eye_description->top_v,
        .bottom_v = eye_description->bottom_v,
    };
    UpdateUniformBuffer(eye, ubo);

    VkImageView current_image_view =
        *reinterpret_cast<VkImageView*>(eye_description->texture);
    UpdateDescriptorSets(eye, image_index, current_image_view);
  }

  // Variables created externally.
  VkPhysicalDevice physical_device_;
  VkDevice logical_device_;
  VkRenderPass render_pass_;
  VkCommandBuffer* command_buffers_;
  uint32_t swapchain_image_count_;
  uint32_t image_width_;
  uint32_t image_height_;

  // Variables created and maintained by the distortion renderer.
  VkSampler texture_sampler_;
  VkDescriptorSetLayout descriptor_set_layout_;
  VkPipelineLayout pipeline_layout_;
  VkViewport viewport_[2];
  VkRect2D scissor_[2];
  VkPipeline graphics_pipeline_[2];
  VkBuffer vertex_buffers_[2];
  VkDeviceMemory vertex_buffers_memory_[2];
  VkBuffer index_buffers_[2];
  VkDeviceMemory index_buffers_memory_[2];
  VkBuffer uniform_buffers_[2];
  VkDeviceMemory uniform_buffers_memory_[2];
  VkDescriptorPool descriptor_pool_[2];
  std::vector<VkDescriptorSet> descriptor_sets_[2];
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
