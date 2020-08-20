// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#if UNITY
#include "Runtime/PluginInterface/Headers/IUnityInterface.h"
#else
#include "IUnityInterface.h"
#endif


/// Flags used for graphics device creation, etc.
typedef enum UnityXRPreInitFlags
{
    /// Initialize EGL with pbuffer support
    kUnityXRPreInitFlagsEGLUsePBuffer = (1 << 0),

    /// Plugin requests the EGL Context is created with NO_ERROR
    kUnityXRPreInitFlagsEGLUseNoErrorContext = (1 << 1),

    /// Plugin handles it's own framebuffer and swapchain, request Unity to save memory by not allocating redundant buffers
    kUnityXRPreInitFlagsDisableMainDisplayFramebuffer = (1 << 2)
} UnityXRPreInitFlags;


/// Graphics device types
typedef enum UnityXRPreInitRenderer
{
    kUnityXRPreInitRendererNull,
    kUnityXRPreInitRendererD3D11,
    kUnityXRPreInitRendererD3D12,
    kUnityXRPreInitRendererGLES20,
    kUnityXRPreInitRendererGLES3x,
    kUnityXRPreInitRendererMetal,
    kUnityXRPreInitRendererGLCore,
    kUnityXRPreInitRendererVulkan
} UnityXRPreInitRenderer;


/// @brief Callbacks for setting up graphics devices before the XR subsystems are able to be initialized.
///
/// All callbacks should be thread safe unless marked otherwise.
/// Callbacks can be NULL if they aren't needed by a provider.
typedef struct UnityXRPreInitProvider
{
    /// Pointer which will be passed to every callback as the userData parameter.
    void* userData;

    /// Called to get a bitfield of UnityXRPreInitFlags flags used for early initialization.
    ///
    /// @param[in] userData User specified data.
    /// @param[out] flags A 64-bit bitfield of UnityXRPreInitFlags.
    /// @return true on success, false on failure.
    bool(UNITY_INTERFACE_API * GetPreInitFlags)(void* userData, uint64_t* flags);

    /// Get the adapter LUID, etc. for the adapter connected to the preferred XR SDK plugin.
    ///
    /// @param[in] userData User specified data.
    /// @param[in] renderer The graphics renderer type that is being initialized.
    /// @param[in] rendererData Graphics renderer specific information needed for initialization.
    /// @param[out] adapterId Returns the LUID/VkPhysicalDevice/id[MTLDevice] for the connected graphics adapter.
    /// @return true on success, false on failure.
    bool(UNITY_INTERFACE_API * GetGraphicsAdapterId)(void* userData, UnityXRPreInitRenderer renderer, uint64_t rendererData, uint64_t* adapterId);

    /// Get the Vulkan instance extensions for the preferred XR SDK plugin.
    /// Called twice: once to get capacity, once to get the extension strings.  If the required capacity is 0, there are no extensions required.
    ///
    /// @param[in] userData User specified data.
    /// @param[in] namesCapacityIn Capacity of the namesString array, or 0 to request the required capacity.
    /// @param[out] namesCountOut Number of characters written (including null terminator), or the buffer size required if namesCapacityIn is 0.
    /// @param[out] namesString Pointer to char buffer for extensions, can be nullptr if namesCapacityIn is 0.
    ///                         Output is a single space (0x20) delimited string of extension names.
    /// @return true on success, false on failure.
    bool(UNITY_INTERFACE_API * GetVulkanInstanceExtensions)(void* userData, uint32_t namesCapacityIn, uint32_t* namesCountOut, char* namesString);

    /// Get the Vulkan device extensions for the preferred XR SDK plugin.
    /// Called twice: once to get capacity, once to get the extension strings.  If the required capacity is 0, there are no extensions required.
    ///
    /// @param[in] userData User specified data.
    /// @param[in] namesCapacityIn Capacity of the namesString array, or 0 to request the required capacity.
    /// @param[out] namesCountOut Number of characters written (including null terminator), or the buffer size required if namesCapacityIn is 0.
    /// @param[out] namesString Pointer to char buffer for extensions, can be nullptr if namesCapacityIn is 0.
    ///                         Output is a single space (0x20) delimited string of extension names.
    /// @return true on success, false on failure.
    bool(UNITY_INTERFACE_API * GetVulkanDeviceExtensions)(void* userData, uint32_t namesCapacityIn, uint32_t* namesCountOut, char* namesString);
} UnityXRPreInitProvider;


/// Interface to allow early pre-initialization of a preferred XR SDK plugin during graphics device initialization, etc.
UNITY_DECLARE_INTERFACE(IUnityXRPreInit)
{
    /// Register callbacks for doing early engine setup before the XR subsystems start up.
    /// There can only ever be one PreInit provider registered, so future calls to register
    /// will overwrite the previous provider entirely.
    ///
    /// @param[in] provider Callbacks to register.
    void(UNITY_INTERFACE_API * RegisterPreInitProvider)(UnityXRPreInitProvider * provider);
};


UNITY_REGISTER_INTERFACE_GUID(0x4E5EB567159F4848ULL, 0x9969601F505A455EULL, IUnityXRPreInit);
