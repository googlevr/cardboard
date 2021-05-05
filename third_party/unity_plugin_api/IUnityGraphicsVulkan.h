// Unity Native Plugin API copyright © 2015 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see[Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#include "IUnityInterface.h"

#ifndef UNITY_VULKAN_HEADER
#define UNITY_VULKAN_HEADER <vulkan/vulkan.h>
#endif

#include UNITY_VULKAN_HEADER

struct UnityVulkanInstance
{
    VkPipelineCache pipelineCache; // Unity's pipeline cache is serialized to disk
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    PFN_vkGetInstanceProcAddr getInstanceProcAddr; // vkGetInstanceProcAddr of the Vulkan loader, same as the one passed to UnityVulkanInitCallback
    unsigned int queueFamilyIndex;

    void* reserved[8];
};

struct UnityVulkanMemory
{
    VkDeviceMemory memory;        // Vulkan memory handle
    VkDeviceSize offset;          // offset within memory
    VkDeviceSize size;            // size in bytes, may be less than the total size of memory;
    void* mapped;                 // pointer to mapped memory block, NULL if not mappable, offset is already applied, remaining block still has at least the given size.
    VkMemoryPropertyFlags flags;  // Vulkan memory properties
    unsigned int memoryTypeIndex; // index into VkPhysicalDeviceMemoryProperties::memoryTypes

    void* reserved[4];
};

enum UnityVulkanResourceAccessMode
{
    // Does not imply any pipeline barriers, should only be used to query resource attributes
    kUnityVulkanResourceAccess_ObserveOnly,

    // Handles layout transition and barriers
    kUnityVulkanResourceAccess_PipelineBarrier,

    // Recreates the backing resource (VkBuffer/VkImage) but keeps the previous one alive if it's in use
    kUnityVulkanResourceAccess_Recreate,
};

struct UnityVulkanImage
{
    UnityVulkanMemory memory;   // memory that backs the image
    VkImage image;              // Vulkan image handle
    VkImageLayout layout;       // current layout, may change resource access
    VkImageAspectFlags aspect;
    VkImageUsageFlags usage;
    VkFormat format;
    VkExtent3D extent;
    VkImageTiling tiling;
    VkImageType type;
    VkSampleCountFlagBits samples;
    int layers;
    int mipCount;

    void* reserved[4];
};

struct UnityVulkanBuffer
{
    UnityVulkanMemory memory;   // memory that backs the buffer
    VkBuffer buffer;            // Vulkan buffer handle
    size_t sizeInBytes;         // size of the buffer in bytes, may be less than memory size
    VkBufferUsageFlags usage;

    void* reserved[4];
};

struct UnityVulkanRecordingState
{
    VkCommandBuffer commandBuffer;              // Vulkan command buffer that is currently recorded by Unity
    VkCommandBufferLevel commandBufferLevel;
    VkRenderPass renderPass;                    // Current render pass, a compatible one or VK_NULL_HANDLE
    VkFramebuffer framebuffer;                  // Current framebuffer or VK_NULL_HANDLE
    int subPassIndex;                           // index of the current sub pass, -1 if not inside a render pass

    // Resource life-time tracking counters, only relevant for resources allocated by the plugin
    unsigned long long currentFrameNumber;      // can be used to track lifetime of own resources
    unsigned long long safeFrameNumber;         // all resources that were used in this frame (or before) are safe to be released

    void* reserved[4];
};

enum UnityVulkanEventRenderPassPreCondition
{
    // Don't care about the state on Unity's current command buffer
    // This is the default precondition
    kUnityVulkanRenderPass_DontCare,

    // Make sure that there is currently no RenderPass in progress.
    // This allows e.g. resource uploads.
    // There are no guarantees about the currently bound descriptor sets, vertex buffers, index buffers and pipeline objects
    // Unity does however set dynamic pipeline set VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR based on the current settings
    // If used in combination with the SRP RenderPass API the resuls is undefined
    kUnityVulkanRenderPass_EnsureInside,

    // Make sure that there is currently no RenderPass in progress.
    // Ends the current render pass (and resumes it afterwards if needed)
    // If used in combination with the SRP RenderPass API the resuls is undefined.
    kUnityVulkanRenderPass_EnsureOutside
};

enum UnityVulkanGraphicsQueueAccess
{
    // No queue acccess, no work must be submitted to UnityVulkanInstance::graphicsQueue from the plugin event callback
    kUnityVulkanGraphicsQueueAccess_DontCare,

    // Make sure that Unity worker threads don't access the Vulkan graphics queue
    // This disables access to the current Unity command buffer
    kUnityVulkanGraphicsQueueAccess_Allow,
};

enum UnityVulkanEventConfigFlagBits
{
    kUnityVulkanEventConfigFlag_EnsurePreviousFrameSubmission = (1 << 0), // default: set
    kUnityVulkanEventConfigFlag_FlushCommandBuffers = (1 << 1),           // submit existing command buffers, default: not set
    kUnityVulkanEventConfigFlag_SyncWorkerThreads = (1 << 2),             // wait for worker threads to finish, default: not set
    kUnityVulkanEventConfigFlag_ModifiesCommandBuffersState = (1 << 3),   // should be set when descriptor set bindings, vertex buffer bindings, etc are changed (default: set)
};

struct UnityVulkanPluginEventConfig
{
    UnityVulkanEventRenderPassPreCondition renderPassPrecondition;
    UnityVulkanGraphicsQueueAccess graphicsQueueAccess;
    uint32_t flags;
};

// Constant that can be used to reference the whole image
const VkImageSubresource* const UnityVulkanWholeImage = NULL;

// callback function, see InterceptInitialization
typedef PFN_vkGetInstanceProcAddr(UNITY_INTERFACE_API * UnityVulkanInitCallback)(PFN_vkGetInstanceProcAddr getInstanceProcAddr, void* userdata);

enum UnityVulkanSwapchainMode
{
    kUnityVulkanSwapchainMode_Default,
    kUnityVulkanSwapchainMode_Offscreen
};

struct UnityVulkanSwapchainConfiguration
{
    UnityVulkanSwapchainMode mode;
};

UNITY_DECLARE_INTERFACE(IUnityGraphicsVulkan)
{
    // Vulkan API hooks
    //
    // Must be called before kUnityGfxDeviceEventInitialize (preload plugin)
    // Unity will call 'func' when initializing the Vulkan API
    // The 'getInstanceProcAddr' passed to the callback is the function pointer from the Vulkan Loader
    // The function pointer returned from UnityVulkanInitCallback may be a different implementation
    // This allows intercepting all Vulkan API calls
    //
    // Most rules/restrictions for implementing a Vulkan layer apply
    // Returns true on success, false on failure (typically because it is used too late)
    bool(UNITY_INTERFACE_API * InterceptInitialization)(UnityVulkanInitCallback func, void* userdata);

    // Intercept Vulkan API function of the given name with the given function
    // In contrast to InterceptInitialization this interface can be used at any time
    // The user must handle all synchronization
    // Generally this cannot be used to wrap Vulkan object because there might because there may already be non-wrapped instances
    // returns the previous function pointer
    PFN_vkVoidFunction(UNITY_INTERFACE_API * InterceptVulkanAPI)(const char* name, PFN_vkVoidFunction func);

    // Change the precondition for a specific user-defined event
    // Should be called during initialization
    void(UNITY_INTERFACE_API * ConfigureEvent)(int eventID, const UnityVulkanPluginEventConfig * pluginEventConfig);

    // Access the Vulkan instance and render queue created by Unity
    // UnityVulkanInstance does not change between kUnityGfxDeviceEventInitialize and kUnityGfxDeviceEventShutdown
    UnityVulkanInstance(UNITY_INTERFACE_API * Instance)();

    // Access the current command buffer
    //
    // outCommandRecordingState is invalidated by any resource access calls.
    // queueAccess must be kUnityVulkanGraphicsQueueAccess_Allow when called from from a AccessQueue callback or from a event that is configured for queue access.
    // Otherwise queueAccess must be kUnityVulkanGraphicsQueueAccess_DontCare.
    bool(UNITY_INTERFACE_API * CommandRecordingState)(UnityVulkanRecordingState * outCommandRecordingState, UnityVulkanGraphicsQueueAccess queueAccess);

    // Resource access
    //
    // Using the following resource query APIs will mark the resources as used for the current frame.
    // Pipeline barriers will be inserted when needed.
    //
    // Resource access APIs may record commands, so the current UnityVulkanRecordingState is invalidated
    // Must not be called from event callbacks configured for queue access (UnityVulkanGraphicsQueueAccess_Allow)
    // or from a AccessQueue callback of an event
    bool(UNITY_INTERFACE_API * AccessTexture)(void* nativeTexture, const VkImageSubresource * subResource, VkImageLayout layout,
        VkPipelineStageFlags pipelineStageFlags, VkAccessFlags accessFlags, UnityVulkanResourceAccessMode accessMode, UnityVulkanImage * outImage);

    bool(UNITY_INTERFACE_API * AccessRenderBufferTexture)(UnityRenderBuffer nativeRenderBuffer, const VkImageSubresource * subResource, VkImageLayout layout,
        VkPipelineStageFlags pipelineStageFlags, VkAccessFlags accessFlags, UnityVulkanResourceAccessMode accessMode, UnityVulkanImage * outImage);

    bool(UNITY_INTERFACE_API * AccessRenderBufferResolveTexture)(UnityRenderBuffer nativeRenderBuffer, const VkImageSubresource * subResource, VkImageLayout layout,
        VkPipelineStageFlags pipelineStageFlags, VkAccessFlags accessFlags, UnityVulkanResourceAccessMode accessMode, UnityVulkanImage * outImage);

    bool(UNITY_INTERFACE_API * AccessBuffer)(void* nativeBuffer, VkPipelineStageFlags pipelineStageFlags, VkAccessFlags accessFlags, UnityVulkanResourceAccessMode accessMode, UnityVulkanBuffer * outBuffer);

    // Control current state of render pass
    //
    // Must not be called from event callbacks configured for queue access (UnityVulkanGraphicsQueueAccess_Allow, UnityVulkanGraphicsQueueAccess_FlushAndAllow)
    // or from a AccessQueue callback of an event
    // See kUnityVulkanRenderPass_EnsureInside, kUnityVulkanRenderPass_EnsureOutside
    void(UNITY_INTERFACE_API * EnsureOutsideRenderPass)();
    void(UNITY_INTERFACE_API * EnsureInsideRenderPass)();

    // Allow command buffer submission to the the Vulkan graphics queue from the given UnityRenderingEventAndData callback.
    // This is an alternative to using ConfigureEvent with kUnityVulkanGraphicsQueueAccess_Allow.
    //
    // eventId and userdata are passed to the callback
    // This may or may not be called synchronously or from the submission thread.
    // If flush is true then all Unity command buffers of this frame are submitted before UnityQueueAccessCallback
    void(UNITY_INTERFACE_API * AccessQueue)(UnityRenderingEventAndData, int eventId, void* userData, bool flush);

    // Configure swapchains that are created by Unity.
    // Must be called before kUnityGfxDeviceEventInitialize (preload plugin)
    bool(UNITY_INTERFACE_API * ConfigureSwapchain)(const UnityVulkanSwapchainConfiguration * swapChainConfig);

    // see AccessTexture
    // Accepts UnityTextureID (UnityRenderingExtTextureUpdateParamsV2::textureID, UnityRenderingExtCustomBlitParams::source)
    bool(UNITY_INTERFACE_API * AccessTextureByID)(UnityTextureID textureID, const VkImageSubresource * subResource, VkImageLayout layout,
        VkPipelineStageFlags pipelineStageFlags, VkAccessFlags accessFlags, UnityVulkanResourceAccessMode accessMode, UnityVulkanImage * outImage);
};
UNITY_REGISTER_INTERFACE_GUID(0x95355348d4ef4e11ULL, 0x9789313dfcffcc87ULL, IUnityGraphicsVulkan)
