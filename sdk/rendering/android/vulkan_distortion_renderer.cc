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
#include "rendering/android/shaders/distortion_frag.spv"
#include "rendering/android/shaders/distortion_vert.spv"
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
    texture_sampler_ = *reinterpret_cast<VkSampler*>(config->texture_sampler);
    swapchain_image_count_ = config->swapchain_image_count;
    image_width_ = config->image_width;
    image_height_ = config->image_height;

    CreateSharedVulkanObjects();
    CreatePerEyeVulkanObjects(kLeft);
    CreatePerEyeVulkanObjects(kRight);
  }

  ~VulkanDistortionRenderer() {
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
    CardboardVulkanDistortionRendererTarget* renderTarget =
        reinterpret_cast<CardboardVulkanDistortionRendererTarget*>(target);
    VkQueue queue = *reinterpret_cast<VkQueue*>(renderTarget->vk_queue);
    VkFence fence = *reinterpret_cast<VkFence*>(renderTarget->vk_fence);
    VkSubmitInfo* submitInfo =
        reinterpret_cast<VkSubmitInfo*>(renderTarget->vk_submits_info);
    VkPresentInfoKHR* presentInfo =
        reinterpret_cast<VkPresentInfoKHR*>(renderTarget->vk_present_info);

    image_width_ = width;
    image_height_ = height;

    viewport_[kLeft].x = x;
    viewport_[kLeft].y = y;
    viewport_[kLeft].width = image_width_ / 2;
    viewport_[kLeft].height = image_height_;
    scissor_[kLeft].offset = {.x = x, .y = y};
    scissor_[kLeft].extent = {.width = static_cast<uint32_t>(image_width_ / 2),
                              .height = static_cast<uint32_t>(image_height_)};
    RenderDistortionMesh(left_eye, kLeft, renderTarget->swapchain_image_index);

    viewport_[kRight].x = x + image_width_ / 2;
    viewport_[kRight].x = y;
    viewport_[kRight].width = image_width_ / 2;
    viewport_[kRight].height = height;
    scissor_[kRight].offset = {.x = static_cast<int32_t>(x + image_width_ / 2),
                               .y = y};
    scissor_[kRight].extent = {.width = static_cast<uint32_t>(image_width_ / 2),
                               .height = static_cast<uint32_t>(image_height_)};
    RenderDistortionMesh(right_eye, kRight,
                         renderTarget->swapchain_image_index);

    submitInfo->pCommandBuffers =
        &command_buffers_[renderTarget->swapchain_image_index];

    vkResetFences(logical_device_, 1, &fence);
    CALL_VK(vkQueueSubmit(queue, 1, submitInfo, fence));
    CALL_VK(vkQueuePresentKHR(queue, presentInfo));
  }

 private:
  VkShaderModule LoadShader(const uint32_t* const content, size_t size) const {
    VkShaderModule shader;
    VkShaderModuleCreateInfo shaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = size,
        .pCode = content,
    };
    CALL_VK(vkCreateShaderModule(logical_device_, &shaderModuleCreateInfo,
                                 nullptr, &shader));

    return shader;
  }

  uint32_t FindMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) ==
              properties) {
        return i;
      }
    }

    CARDBOARD_LOGE("Failed to find suitable memory type!");
    return 0;
  }

  void CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding bindings[2];

    VkDescriptorSetLayoutBinding samplerLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
    bindings[0] = samplerLayoutBinding;

    VkDescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };
    bindings[1] = uboLayoutBinding;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    CALL_VK(vkCreateDescriptorSetLayout(logical_device_, &layoutInfo, nullptr,
                                        &descriptor_set_layout_));
  }

  void CreatePipelineLayout() {
    // Create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout_,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    CALL_VK(vkCreatePipelineLayout(logical_device_, &pipelineLayoutCreateInfo,
                                   nullptr, &pipeline_layout_));
  }

  void CreateDescriptorPool(CardboardEye eye) {
    VkDescriptorPoolSize poolSizes[2];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount =
        static_cast<uint32_t>(swapchain_image_count_);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount =
        static_cast<uint32_t>(swapchain_image_count_);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = static_cast<uint32_t>(swapchain_image_count_);

    CALL_VK(vkCreateDescriptorPool(logical_device_, &poolInfo, nullptr,
                                   &descriptor_pool_[eye]));
  }

  void CreateDescriptorSets(CardboardEye eye) {
    std::vector<VkDescriptorSetLayout> layouts(swapchain_image_count_,
                                               descriptor_set_layout_);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_[eye];
    allocInfo.descriptorSetCount =
        static_cast<uint32_t>(swapchain_image_count_);
    allocInfo.pSetLayouts = layouts.data();

    descriptor_sets_[eye].resize(swapchain_image_count_);
    CALL_VK(vkAllocateDescriptorSets(logical_device_, &allocInfo,
                                     descriptor_sets_[eye].data()));
  }

  void CreateGraphicsPipeline(CardboardEye eye) {
    VkShaderModule vertexShader =
        LoadShader(distortion_vert, sizeof(distortion_vert));
    VkShaderModule fragmentShader =
        LoadShader(distortion_frag, sizeof(distortion_frag));

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2]{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        }};

    viewport_[eye].x = (float)(eye == kLeft ? 0 : image_width_ / 2);
    viewport_[eye].y = 0;
    viewport_[eye].width = (float)image_width_ / 2;
    viewport_[eye].height = (float)image_height_;
    viewport_[eye].minDepth = 0.0f;
    viewport_[eye].maxDepth = 1.0f;

    scissor_[eye].offset = {
        .x = static_cast<int32_t>(eye == kLeft ? 0 : image_width_ / 2), .y = 0};
    scissor_[eye].extent = {.width = image_width_ / 2, .height = image_height_};

    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &viewport_[eye],
        .scissorCount = 1,
        .pScissors = &scissor_[eye],
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
        .pSampleMask = &sampleMask,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state
    VkPipelineColorBlendAttachmentState attachmentStates{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &attachmentStates,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo{
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
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_input_bindings,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    VkDynamicState dynamicStateEnables[2];  // Viewport + Scissor
    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    dynamicStateEnables[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateEnables[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .dynamicStateCount = 0,
        .pDynamicStates = nullptr};

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pTessellationState = nullptr,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlendInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = pipeline_layout_,
        .renderPass = render_pass_,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    CALL_VK(vkCreateGraphicsPipelines(logical_device_, VK_NULL_HANDLE, 1,
                                      &pipelineCreateInfo, nullptr,
                                      &graphics_pipeline_[eye]));

    vkDestroyShaderModule(logical_device_, vertexShader, nullptr);
    vkDestroyShaderModule(logical_device_, fragmentShader, nullptr);
  }

  void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                  .size = size,
                                  .usage = usage,
                                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

    CALL_VK(vkCreateBuffer(logical_device_, &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logical_device_, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, properties)};

    CALL_VK(
        vkAllocateMemory(logical_device_, &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(logical_device_, buffer, bufferMemory, 0);
  }

  void CreateVertexBuffer(CardboardEye eye, std::vector<Vertex> vertices) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertex_buffers_[eye], vertex_buffers_memory_[eye]);

    void* data;
    CALL_VK(vkMapMemory(logical_device_, vertex_buffers_memory_[eye], 0,
                        bufferSize, 0, &data));
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(logical_device_, vertex_buffers_memory_[eye]);
  }

  void CreateIndexBuffer(CardboardEye eye, std::vector<uint16_t> indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 index_buffers_[eye], index_buffers_memory_[eye]);

    void* data;
    vkMapMemory(logical_device_, index_buffers_memory_[eye], 0, bufferSize, 0,
                &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(logical_device_, index_buffers_memory_[eye]);
  }

  void CreateUniformBuffers(CardboardEye eye) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniform_buffers_[eye], uniform_buffers_memory_[eye]);
  }

  void CreateSharedVulkanObjects() {
    CreateDescriptorSetLayout();
    CreatePipelineLayout();
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
                            VkImageView imageView) const {
    VkDescriptorBufferInfo bufferInfo{.buffer = uniform_buffers_[eye],
                                      .offset = 0,
                                      .range = sizeof(UniformBufferObject)};

    VkDescriptorImageInfo imageInfo{
        .sampler = texture_sampler_,
        .imageView = imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptorWrites[2];

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptor_sets_[eye][index];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptor_sets_[eye][index];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(logical_device_, 2, descriptorWrites, 0, nullptr);
  }

  void RenderDistortionMesh(
      const CardboardEyeTextureDescription* eye_description, CardboardEye eye,
      uint32_t imageIndex) const {
    UniformBufferObject ubo{
        .left_u = eye_description->left_u,
        .right_u = eye_description->right_u,
        .top_v = eye_description->top_v,
        .bottom_v = eye_description->bottom_v,
    };
    UpdateUniformBuffer(eye, ubo);

    VkImageView currentImageView =
        *reinterpret_cast<VkImageView*>(eye_description->texture);
    UpdateDescriptorSets(eye, imageIndex, currentImageView);
  }

  // Variables created externally.
  VkPhysicalDevice physical_device_;
  VkDevice logical_device_;
  VkRenderPass render_pass_;
  VkCommandBuffer* command_buffers_;
  VkSampler texture_sampler_;
  uint32_t swapchain_image_count_;
  uint32_t image_width_;
  uint32_t image_height_;

  // Variables created and maintained by distortion render.
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
