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
#include <stdint.h>

#include <array>
#include <memory>
#include <vector>

#include "include/cardboard.h"
#include "rendering/android/vulkan/android_vulkan_loader.h"
#include "util/is_arg_null.h"
#include "util/logging.h"
#include "unity/xr_unity_plugin/cardboard_display_api.h"
#include "unity/xr_unity_plugin/renderer.h"
#include "unity/xr_unity_plugin/vulkan/vulkan_widgets_renderer.h"
#include <vulkan/vulkan.h>
// clang-format off
#include "IUnityRenderingExtensions.h"
#include "IUnityGraphicsVulkan.h"
// clang-format on

namespace cardboard::unity {
namespace {
const uint64_t kFenceTimeoutNs = 100000000;

/**
 * Holds and manages the version of the swapchain.
 * Dependent code on the swapchain handle must own both a copy of the handle and
 * its version.
 * In case Vulkan entities have been created with an old swapchain handle and
 * it has been torn down, those entities must no be used as it could produce
 * unexpected crashes.
 */
class VkSwapchainCache final {
 public:
  /**
   * Updates the swapchain handle and increases its version.
   */
  static void Update(VkSwapchainKHR swapchain) {
    swapchain_ = swapchain;
    version_++;
  }

  /** Gets the swapchain handle. */
  static VkSwapchainKHR& Get() { return swapchain_; }

  /** Gets the swapchain handle version. */
  static int GetVersion() { return version_; }

  /** Tells whether or not the version is up to date. */
  static bool IsCacheUpToDate(int version) { return version_ == version; }

 private:
  VkSwapchainCache() = default;
  static VkSwapchainKHR swapchain_;
  static int version_;
};

VkSwapchainKHR VkSwapchainCache::swapchain_ = VK_NULL_HANDLE;
int VkSwapchainCache::version_ = 0;

PFN_vkGetInstanceProcAddr Orig_GetInstanceProcAddr;
PFN_vkCreateSwapchainKHR Orig_vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR Orig_vkDestroySwapchainKHR;
PFN_vkAcquireNextImageKHR Orig_vkAcquireNextImageKHR;
uint32_t image_index;

/**
 * Function registerd to intercept the vulkan function `vkCreateSwapchainKHR`.
 * Through this function we could get the swapchain that Unity created.
 */
static VKAPI_ATTR void VKAPI_CALL Hook_vkCreateSwapchainKHR(
    VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
  cardboard::rendering::vkCreateSwapchainKHR(device, pCreateInfo, pAllocator,
                                             pSwapchain);
  VkSwapchainCache::Update(*pSwapchain);
  CardboardDisplayApi::SetDeviceParametersChanged();
}

/**
 * Function registerd to intercept the vulkan function `vkDestroySwapchainKHR`.
 * Through this function we could destroy and invalidate the swapchain.
 */
static VKAPI_ATTR void VKAPI_CALL
Hook_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
                           const VkAllocationCallbacks* pAllocator) {
  cardboard::rendering::vkDestroySwapchainKHR(device, swapchain, pAllocator);
  VkSwapchainCache::Update(VK_NULL_HANDLE);
}

/**
 * Function registerd to intercept the vulkan function `vkAcquireNextImageKHR`.
 * Through this function we could get the image index in the swapchain for
 * each frame.
 */
static VKAPI_ATTR void VKAPI_CALL Hook_vkAcquireNextImageKHR(
    VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
    VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) {
  cardboard::rendering::vkAcquireNextImageKHR(device, swapchain, timeout,
                                              semaphore, fence, pImageIndex);
  image_index = *pImageIndex;
}

/**
 * Function used to register the Vulkan interception functions.
 */
PFN_vkVoidFunction VKAPI_PTR MyGetInstanceProcAddr(VkInstance instance,
                                                   const char* pName) {
  if (strcmp(pName, "vkCreateSwapchainKHR") == 0) {
    Orig_vkCreateSwapchainKHR =
        (PFN_vkCreateSwapchainKHR)Orig_GetInstanceProcAddr(instance, pName);
    return (PFN_vkVoidFunction)&Hook_vkCreateSwapchainKHR;
  }

  if (strcmp(pName, "vkDestroySwapchainKHR") == 0) {
    Orig_vkDestroySwapchainKHR =
        (PFN_vkDestroySwapchainKHR)Orig_GetInstanceProcAddr(instance, pName);
    return (PFN_vkVoidFunction)&Hook_vkDestroySwapchainKHR;
  }

  if (strcmp(pName, "vkAcquireNextImageKHR") == 0) {
    Orig_vkAcquireNextImageKHR =
        (PFN_vkAcquireNextImageKHR)Orig_GetInstanceProcAddr(instance, pName);
    return (PFN_vkVoidFunction)&Hook_vkAcquireNextImageKHR;
  }

  return Orig_GetInstanceProcAddr(instance, pName);
}

/**
 * Register the interception function during the initialization.
 */
PFN_vkGetInstanceProcAddr InterceptVulkanInitialization(
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr, void* /*userdata*/) {
  Orig_GetInstanceProcAddr = GetInstanceProcAddr;
  return &MyGetInstanceProcAddr;
}

/**
 * This function is exported so the plugin could call it during loading.
 */
extern "C" void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces* interfaces) {
  IUnityGraphicsVulkanV2* vulkan_interface =
      interfaces->Get<IUnityGraphicsVulkanV2>();

  vulkan_interface->AddInterceptInitialization(InterceptVulkanInitialization,
                                               NULL, 2);
  cardboard::rendering::LoadVulkan();
}

class VulkanRenderer : public Renderer {
 public:
  explicit VulkanRenderer(IUnityInterfaces* xr_interfaces)
      : current_rendering_width_(0), current_rendering_height_(0) {
    vulkan_interface_ = xr_interfaces->Get<IUnityGraphicsVulkanV2>();
    if (CARDBOARD_IS_ARG_NULL(vulkan_interface_)) {
      return;
    }

    UnityVulkanInstance vulkanInstance = vulkan_interface_->Instance();
    logical_device_ = vulkanInstance.device;
    physical_device_ = vulkanInstance.physicalDevice;
    swapchain_ = VkSwapchainCache::Get();
    swapchain_version_ = VkSwapchainCache::GetVersion();

    cardboard::rendering::vkGetSwapchainImagesKHR(
        logical_device_, swapchain_, &swapchain_image_count_, nullptr);
    swapchain_images_.resize(swapchain_image_count_);
    swapchain_views_.resize(swapchain_image_count_);
    frame_buffers_.resize(swapchain_image_count_);

    // Create command pool.
    const VkCommandPoolCreateInfo cmd_pool_create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vulkanInstance.queueFamilyIndex,
    };
    cardboard::rendering::vkCreateCommandPool(
        logical_device_, &cmd_pool_create_info, nullptr, &command_pool_);

    // Create command buffers.
    command_buffers_.resize(swapchain_image_count_);
    const VkCommandBufferAllocateInfo cmd_buffer_create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = swapchain_image_count_,
    };
    cardboard::rendering::vkAllocateCommandBuffers(
        logical_device_, &cmd_buffer_create_info, command_buffers_.data());

    // Create fences.
    fences_.resize(swapchain_image_count_);

    // We need a fence in the main thread so the frame buffer won't be swapped
    // until drawing commands are completed.
    const VkFenceCreateInfo fence_create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    for (auto& fence : fences_) {
      cardboard::rendering::vkCreateFence(logical_device_, &fence_create_info,
                                          nullptr, &fence);
    }

    // Create semaphores.
    semaphores_.resize(swapchain_image_count_);

    // We need a semaphore in the main thread so it could wait until the frame
    // buffer is available.
    const VkSemaphoreCreateInfo semaphore_create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    for (auto& semaphore : semaphores_) {
      cardboard::rendering::vkCreateSemaphore(
          logical_device_, &semaphore_create_info, nullptr, &semaphore);
    }

    // Get the images from the swapchain and wrap it into a image view.
    cardboard::rendering::vkGetSwapchainImagesKHR(logical_device_, swapchain_,
                                                  &swapchain_image_count_,
                                                  swapchain_images_.data());

    for (size_t i = 0; i < swapchain_images_.size(); i++) {
      const VkImageViewCreateInfo view_create_info = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .image = swapchain_images_[i],
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

      cardboard::rendering::vkCreateImageView(
          logical_device_, &view_create_info, nullptr /* pAllocator */,
          &swapchain_views_[i]);
    }

    // Create RenderPass
    const VkAttachmentDescription attachment_descriptions{
        .flags = 0,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference colour_reference = {
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    const VkSubpassDescription subpass_description{
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colour_reference,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };
    const VkRenderPassCreateInfo render_pass_create_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .attachmentCount = 1,
        .pAttachments = &attachment_descriptions,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = 0,
        .pDependencies = nullptr,
    };
    cardboard::rendering::vkCreateRenderPass(
        logical_device_, &render_pass_create_info, nullptr /* pAllocator */,
        &render_pass_);
  }

  ~VulkanRenderer() {
    WaitForAllFences(kFenceTimeoutNs);

    TeardownWidgets();

    // Remove the Vulkan resources created by this VulkanRenderer.
    for (uint32_t i = 0; i < swapchain_image_count_; i++) {
      cardboard::rendering::vkDestroyFramebuffer(
          logical_device_, frame_buffers_[i], nullptr /* pAllocator */);
      cardboard::rendering::vkDestroyImageView(
          logical_device_, swapchain_views_[i],
          nullptr /* vkDestroyImageView */);
    }

    cardboard::rendering::vkDestroyRenderPass(logical_device_, render_pass_,
                                              nullptr);
    // Clean semaphores.
    for (auto& semaphore : semaphores_) {
      if (semaphore != VK_NULL_HANDLE) {
        cardboard::rendering::vkDestroySemaphore(logical_device_, semaphore,
                                                 nullptr);
        semaphore = VK_NULL_HANDLE;
      }
    }

    // Clean fences.
    for (auto& fence : fences_) {
      if (fence != VK_NULL_HANDLE) {
        cardboard::rendering::vkDestroyFence(logical_device_, fence, nullptr);
        fence = VK_NULL_HANDLE;
      }
    }

    // Delete command buffers.
    cardboard::rendering::vkFreeCommandBuffers(logical_device_, command_pool_,
                                               swapchain_image_count_,
                                               command_buffers_.data());

    cardboard::rendering::vkDestroyCommandPool(logical_device_, command_pool_,
                                               nullptr);
  }

  void SetupWidgets() override {
    widget_renderer_ = std::make_unique<VulkanWidgetsRenderer>(
        physical_device_, logical_device_, swapchain_image_count_);
  }

  void RenderWidgets(const ScreenParams& screen_params,
                     const std::vector<WidgetParams>& widget_params) override {
    if (skip_frame_) {
      return;
    }

    if (!VkSwapchainCache::IsCacheUpToDate(swapchain_version_)) {
      return;
    }

    widget_renderer_->RenderWidgets(screen_params, widget_params,
                                    command_buffers_[image_index], image_index,
                                    render_pass_);
  }

  void TeardownWidgets() override {
    WaitForAllFences(kFenceTimeoutNs);

    if (widget_renderer_ != nullptr) {
      widget_renderer_.reset(nullptr);
    }
  }

  void CreateRenderTexture(RenderTexture* render_texture, int screen_width,
                           int screen_height) override {
    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent =
      {
          .width = static_cast<uint32_t>(screen_width / 2),
          .height = static_cast<uint32_t>(screen_height),
          .depth = 1,
      },
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage =
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image;
    cardboard::rendering::vkCreateImage(logical_device_, &imageInfo, nullptr,
                                        &image);

    VkMemoryRequirements memRequirements;
    cardboard::rendering::vkGetImageMemoryRequirements(logical_device_, image,
                                                       &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory texture_image_memory;
    cardboard::rendering::vkAllocateMemory(logical_device_, &allocInfo, nullptr,
                                           &texture_image_memory);
    cardboard::rendering::vkBindImageMemory(logical_device_, image,
                                            texture_image_memory, 0);

    // Unity requires an VkImage in order to draw the scene.
    render_texture->color_buffer = reinterpret_cast<uint64_t>(image);

    // When using Vulkan, texture depth buffer is unused.
    render_texture->depth_buffer = 0;
  }

  void DestroyRenderTexture(RenderTexture* render_texture) override {
    render_texture->color_buffer = 0;
    render_texture->depth_buffer = 0;
  }

  void RenderEyesToDisplay(
      CardboardDistortionRenderer* renderer, const ScreenParams& screen_params,
      const CardboardEyeTextureDescription* left_eye,
      const CardboardEyeTextureDescription* right_eye) override {
    if (skip_frame_) {
      return;
    }

    if (!VkSwapchainCache::IsCacheUpToDate(swapchain_version_)) {
      return;
    }

    // Setup rendering content
    CardboardVulkanDistortionRendererTarget target_config{
        .vk_render_pass = reinterpret_cast<uint64_t>(&render_pass_),
        .vk_command_buffer =
            reinterpret_cast<uint64_t>(&command_buffers_[image_index]),
        .swapchain_image_index = image_index,
    };

    current_image_left_ = reinterpret_cast<VkImage>(left_eye->texture);
    current_image_right_ = reinterpret_cast<VkImage>(right_eye->texture);
    TransitionEyeImagesLayoutFromUnityToDistortionRenderer(
        current_image_left_, current_image_right_);

    CardboardDistortionRenderer_renderEyeToDisplay(
        renderer, reinterpret_cast<uint64_t>(&target_config),
        screen_params.viewport_x, screen_params.viewport_y,
        screen_params.viewport_width, screen_params.viewport_height, left_eye,
        right_eye);
  }

  void RunRenderingPreProcessing(const ScreenParams& screen_params) override {
    skip_frame_ = (prev_image_index_ == image_index);
    if (skip_frame_) {
      return;
    }

    if (!VkSwapchainCache::IsCacheUpToDate(swapchain_version_)) {
      return;
    }

    UnityVulkanRecordingState vulkanRecordingState;
    vulkan_interface_->EnsureOutsideRenderPass();
    vulkan_interface_->CommandRecordingState(
        &vulkanRecordingState, kUnityVulkanGraphicsQueueAccess_DontCare);

    // If width or height of the rendering area changes, then we need to
    // recreate all frame buffers.
    if (screen_params.viewport_width != current_rendering_width_ ||
        screen_params.viewport_height != current_rendering_height_) {
      frames_to_update_count_ = swapchain_image_count_;
      current_rendering_width_ = screen_params.viewport_width;
      current_rendering_height_ = screen_params.viewport_height;
    }

    if (frames_to_update_count_ > 0) {
      if (frame_buffers_[image_index] != VK_NULL_HANDLE) {
        cardboard::rendering::vkDestroyFramebuffer(
            logical_device_, frame_buffers_[image_index], nullptr);
      }

      VkImageView attachments[] = {swapchain_views_[image_index]};
      VkFramebufferCreateInfo fb_create_info{
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .renderPass = render_pass_,
          .attachmentCount = 1,
          .pAttachments = attachments,
          .width = static_cast<uint32_t>(screen_params.width),
          .height = static_cast<uint32_t>(screen_params.height),
          .layers = 1,
      };

      cardboard::rendering::vkCreateFramebuffer(
          logical_device_, &fb_create_info, nullptr /* pAllocator */,
          &frame_buffers_[image_index]);
      frames_to_update_count_--;
    }

    // Begin recording command buffers.
    cardboard::rendering::vkWaitForFences(logical_device_, 1 /* fenceCount */,
                                          &fences_[image_index], VK_TRUE,
                                          kFenceTimeoutNs);

    // We start by creating and declaring the "beginning" of our command buffer
    VkCommandBufferBeginInfo cmd_buffer_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    cardboard::rendering::vkBeginCommandBuffer(command_buffers_[image_index],
                                               &cmd_buffer_begin_info);

    // Begin RenderPass
    const VkClearValue clear_vals = {
        .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};
    const VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = render_pass_,
        .framebuffer = frame_buffers_[image_index],
        .renderArea = {.offset =
                           {
                               .x = 0,
                               .y = 0,
                           },
                       .extent =
                           {
                               .width =
                                   static_cast<uint32_t>(screen_params.width),
                               .height =
                                   static_cast<uint32_t>(screen_params.height),
                           }},
        .clearValueCount = 1,
        .pClearValues = &clear_vals};
    cardboard::rendering::vkCmdBeginRenderPass(command_buffers_[image_index],
                                               &render_pass_begin_info,
                                               VK_SUBPASS_CONTENTS_INLINE);

    prev_image_index_ = image_index;
  }

  void RunRenderingPostProcessing() override {
    if (skip_frame_) {
      return;
    }

    if (!VkSwapchainCache::IsCacheUpToDate(swapchain_version_)) {
      return;
    }

    cardboard::rendering::vkCmdEndRenderPass(command_buffers_[image_index]);
    cardboard::rendering::vkEndCommandBuffer(command_buffers_[image_index]);

    // Submit recording command buffer.
    cardboard::rendering::vkResetFences(logical_device_, 1,
                                        &fences_[image_index]);

    VkSemaphore wait_semaphores[] = {semaphores_[image_index]};
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffers_[image_index],
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    UnityVulkanInstance vulkanInstance = vulkan_interface_->Instance();

    VkResult result = cardboard::rendering::vkQueueSubmit(
        vulkanInstance.graphicsQueue, 1, &submit_info, fences_[image_index]);
    if (result != VK_SUCCESS) {
      CARDBOARD_LOGE("Failed to submit command buffer due to error code %d",
                     result);
    }

    // Once the commands have been submited, set the layout that Unity uses to
    // draw on the images.
    TransitionEyeImagesLayoutFromDistortionRendererToUnity(
        current_image_left_, current_image_right_);
  }

  void WaitForAllFences(uint64_t timeout_ns) {
    cardboard::rendering::vkWaitForFences(logical_device_,
                                          fences_.size() /* fenceCount */,
                                          fences_.data(), VK_TRUE, timeout_ns);
  }

 private:
  // @{ The distortion renderer needs the VkImages for both eyes to have
  // VK_IMAGE_LAYOUT_GENERAL in order to use them as image samplers.
  //
  // However, after Unity processes the VkImages
  // for both eyes to get undistorted pictures of the world, it returns the
  // images with different layouts:
  // - Left: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
  // - Right: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.
  //
  // This set of methods and variables is a workaround for this unexpected
  // behavior setting the layout that the. distortion renderer requires before
  // passing the images to it and changing them to what. Unity requires after
  // the command buffers are submitted to the queue.
  static const VkImageLayout kUnityLeftEyeImageLayout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  static const VkImageLayout kUnityRightEyeImageLayout =
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  static const VkImageLayout kDistortionRendererEyeImagesLayout =
      VK_IMAGE_LAYOUT_GENERAL;

  /**
   * Changes the image layout of the received images from the layout used by
   * Unity to draw on them to the layout used by the distortion renderer as an
   * image sampler.
   *
   * @param left_eye_image Image used for the left eye.
   * @param right_eye_image Image used for the right eye.
   */
  void TransitionEyeImagesLayoutFromUnityToDistortionRenderer(
      VkImage left_eye_image, VkImage right_eye_image) {
    TransitionImageLayout(left_eye_image, kUnityLeftEyeImageLayout,
                          kDistortionRendererEyeImagesLayout);
    TransitionImageLayout(right_eye_image, kUnityRightEyeImageLayout,
                          kDistortionRendererEyeImagesLayout);
  }

  /**
   * Changes the image layout of the received images from the layout used by the
   * distortion renderer as an image sampler to the layout used by Unity to draw
   * on them.
   *
   * @param left_eye_image Image used for the left eye.
   * @param right_eye_image Image used for the right eye.
   */
  void TransitionEyeImagesLayoutFromDistortionRendererToUnity(
      VkImage left_eye_image, VkImage right_eye_image) {
    TransitionImageLayout(left_eye_image, kDistortionRendererEyeImagesLayout,
                          kUnityLeftEyeImageLayout);
    TransitionImageLayout(right_eye_image, kDistortionRendererEyeImagesLayout,
                          kUnityRightEyeImageLayout);
  }
  // @}

  /**
   * Find the memory type of the physical device.
   *
   * @param type_filter required memory type shift.
   * @param properties required memory flag bits.
   *
   * @return memory type or 0 if not found.
   */
  uint32_t FindMemoryType(uint32_t type_filter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    cardboard::rendering::vkGetPhysicalDeviceMemoryProperties(physical_device_,
                                                              &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((type_filter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) ==
              properties) {
        return i;
      }
    }

    CARDBOARD_LOGE("failed to find suitable memory type!");
    return 0;
  }

  /**
   * Changes the layout of the received image.
   *
   * @param image The image whose layout will be changed.
   * @param old_layout Current layout of the image.
   * @param new_layout New layout for the image.
   */
  void TransitionImageLayout(VkImage image, VkImageLayout old_layout,
                             VkImageLayout new_layout) {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

    VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destination_stage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    cardboard::rendering::vkCmdPipelineBarrier(command_buffer, source_stage,
                                               destination_stage, 0, 0, nullptr,
                                               0, nullptr, 1, &barrier);
    EndSingleTimeCommands(command_buffer);
  }

  /**
   * Creates a command buffer that is supossed to be used once and returns it.
   *
   * @return The command buffer.
   */
  VkCommandBuffer BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    cardboard::rendering::vkAllocateCommandBuffers(logical_device_, &alloc_info,
                                                   &command_buffer);

    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    cardboard::rendering::vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
  }

  /**
   * Submits a single timme command buffer to the current queue and frees its
   * memory.
   *
   * @param The command buffer.
   */
  void EndSingleTimeCommands(VkCommandBuffer command_buffer) {
    cardboard::rendering::vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    UnityVulkanInstance vulkan_instance = vulkan_interface_->Instance();

    cardboard::rendering::vkQueueSubmit(vulkan_instance.graphicsQueue, 1,
                                        &submit_info, VK_NULL_HANDLE);
    cardboard::rendering::vkQueueWaitIdle(vulkan_instance.graphicsQueue);

    cardboard::rendering::vkFreeCommandBuffers(logical_device_, command_pool_,
                                               1, &command_buffer);
  }

  // Variables created externally.
  int current_rendering_width_;
  int current_rendering_height_;
  IUnityGraphicsVulkanV2* vulkan_interface_{nullptr};
  VkPhysicalDevice physical_device_;
  VkDevice logical_device_;
  std::vector<VkImage> swapchain_images_;
  VkSwapchainKHR swapchain_;
  int swapchain_version_;
  VkImage current_image_left_;
  VkImage current_image_right_;

  // Variables created and maintained by the vulkan renderer.
  uint32_t swapchain_image_count_;
  uint32_t frames_to_update_count_;
  uint32_t prev_image_index_{UINT32_MAX};
  bool skip_frame_{false};
  VkRenderPass render_pass_;
  VkCommandPool command_pool_;
  std::vector<VkFence> fences_;
  std::vector<VkSemaphore> semaphores_;
  std::vector<VkCommandBuffer> command_buffers_;
  std::vector<VkImageView> swapchain_views_;
  std::vector<VkFramebuffer> frame_buffers_;
  std::unique_ptr<VulkanWidgetsRenderer> widget_renderer_;
};

}  // namespace

std::unique_ptr<Renderer> MakeVulkanRenderer(IUnityInterfaces* xr_interfaces) {
  return std::make_unique<VulkanRenderer>(xr_interfaces);
}

CardboardDistortionRenderer* MakeCardboardVulkanDistortionRenderer(
    IUnityInterfaces* xr_interfaces) {
  IUnityGraphicsVulkanV2* vulkan_interface =
      xr_interfaces->Get<IUnityGraphicsVulkanV2>();
  UnityVulkanInstance vulkan_instance = vulkan_interface->Instance();
  const CardboardVulkanDistortionRendererConfig config{
      .physical_device =
          reinterpret_cast<uint64_t>(&vulkan_instance.physicalDevice),
      .logical_device = reinterpret_cast<uint64_t>(&vulkan_instance.device),
      .vk_swapchain = reinterpret_cast<uint64_t>(&VkSwapchainCache::Get()),
  };

  CardboardDistortionRenderer* distortion_renderer =
      CardboardVulkanDistortionRenderer_create(&config);
  return distortion_renderer;
}

}  // namespace cardboard::unity
