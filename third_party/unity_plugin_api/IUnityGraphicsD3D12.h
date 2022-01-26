// Unity Native Plugin API copyright © 2015 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see[Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#include "IUnityInterface.h"
#ifndef __cplusplus
    #include <stdbool.h>
#endif

struct RenderSurfaceBase;
typedef struct RenderSurfaceBase* UnityRenderBuffer;

typedef struct UnityGraphicsD3D12ResourceState UnityGraphicsD3D12ResourceState;
struct UnityGraphicsD3D12ResourceState
{
    ID3D12Resource*       resource; // Resource to barrier.
    D3D12_RESOURCE_STATES expected; // Expected resource state before this command list is executed.
    D3D12_RESOURCE_STATES current;  // State this resource will be in after this command list is executed.
};

struct UnityGraphicsD3D12RecordingState
{
    ID3D12GraphicsCommandList* commandList;              // D3D12 command list that is currently recorded by Unity
};

enum UnityD3D12GraphicsQueueAccess
{
    // No queue acccess, no work must be submitted to UnityD3D12Instance::graphicsQueue from the plugin event callback
    kUnityD3D12GraphicsQueueAccess_DontCare,

    // Make sure that Unity worker threads don't access the D3D12 graphics queue
    // This disables access to the current Unity command buffer
    kUnityD3D12GraphicsQueueAccess_Allow,
};

enum UnityD3D12EventConfigFlagBits
{
    kUnityD3D12EventConfigFlag_EnsurePreviousFrameSubmission = (1 << 0), // default: (NOT SUPPORTED)
    kUnityD3D12EventConfigFlag_FlushCommandBuffers = (1 << 1),           // submit existing command buffers, default: not set
    kUnityD3D12EventConfigFlag_SyncWorkerThreads = (1 << 2),             // wait for worker threads to finish, default: not set
    kUnityD3D12EventConfigFlag_ModifiesCommandBuffersState = (1 << 3),   // should be set when descriptor set bindings, vertex buffer bindings, etc are changed (default: set)
};

struct UnityD3D12PluginEventConfig
{
    UnityD3D12GraphicsQueueAccess graphicsQueueAccess;
    UINT32 flags;                                           // UnityD3D12EventConfigFlagBits to be used when invoking a native plugin
    bool ensureActiveRenderTextureIsBound;                  // If true, the actively bound render texture will be bound prior the execution of the native plugin method.
};

typedef struct UnityGraphicsD3D12PhysicalVideoMemoryControlValues UnityGraphicsD3D12PhysicalVideoMemoryControlValues;
struct UnityGraphicsD3D12PhysicalVideoMemoryControlValues // all absolute values in bytes
{
    UINT64 reservation;           // Minimum required physical memory for an application [default = 64MB].
    UINT64 systemMemoryThreshold; // If free physical video memory drops below this threshold, resources will be allocated in system memory. [default = 64MB]
    UINT64 residencyHysteresisThreshold;    // Minimum free physical video memory needed to start bringing evicted resources back after shrunken video memory budget expands again. [default = 128MB]
    float nonEvictableRelativeThreshold;    // The relative proportion of the video memory budget that must be kept available for non-evictable resources. [default = 0.25]
};

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v6)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    //     Executes a given command list on a worker thread.
    //    [Optional] Declares expected and post-execution resource states.
    //     Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);

    void(UNITY_INTERFACE_API * SetPhysicalVideoMemoryControlValues)(const UnityGraphicsD3D12PhysicalVideoMemoryControlValues * memInfo);

    ID3D12CommandQueue* (UNITY_INTERFACE_API * GetCommandQueue)();

    ID3D12Resource* (UNITY_INTERFACE_API * TextureFromRenderBuffer)(UnityRenderBuffer rb);
    ID3D12Resource* (UNITY_INTERFACE_API * TextureFromNativeTexture)(UnityTextureID texture);

    // Change the precondition for a specific user-defined event
    // Should be called during initialization
    void(UNITY_INTERFACE_API * ConfigureEvent)(int eventID, const UnityD3D12PluginEventConfig * pluginEventConfig);

    bool(UNITY_INTERFACE_API * CommandRecordingState)(UnityGraphicsD3D12RecordingState* outCommandRecordingState);
};
UNITY_REGISTER_INTERFACE_GUID(0xA396DCE58CAC4D78ULL, 0xAFDD9B281F20B840ULL, IUnityGraphicsD3D12v6)

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v5)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    //     Executes a given command list on a worker thread.
    //    [Optional] Declares expected and post-execution resource states.
    //     Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);

    void(UNITY_INTERFACE_API * SetPhysicalVideoMemoryControlValues)(const UnityGraphicsD3D12PhysicalVideoMemoryControlValues * memInfo);

    ID3D12CommandQueue* (UNITY_INTERFACE_API * GetCommandQueue)();

    ID3D12Resource* (UNITY_INTERFACE_API * TextureFromRenderBuffer)(UnityRenderBuffer rb);
};
UNITY_REGISTER_INTERFACE_GUID(0xF5C8D8A37D37BC42ULL, 0xB02DFE93B5064A27ULL, IUnityGraphicsD3D12v5)

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v4)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Executes a given command list on a worker thread.
    // [Optional] Declares expected and post-execution resource states.
    // Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);

    void(UNITY_INTERFACE_API * SetPhysicalVideoMemoryControlValues)(const UnityGraphicsD3D12PhysicalVideoMemoryControlValues * memInfo);

    ID3D12CommandQueue* (UNITY_INTERFACE_API * GetCommandQueue)();
};
UNITY_REGISTER_INTERFACE_GUID(0X498FFCC13EC94006ULL, 0XB18F8B0FF67778C8ULL, IUnityGraphicsD3D12v4)

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v3)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Executes a given command list on a worker thread.
    // [Optional] Declares expected and post-execution resource states.
    // Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);

    void(UNITY_INTERFACE_API * SetPhysicalVideoMemoryControlValues)(const UnityGraphicsD3D12PhysicalVideoMemoryControlValues * memInfo);
};
UNITY_REGISTER_INTERFACE_GUID(0x57C3FAFE59E5E843ULL, 0xBF4F5998474BB600ULL, IUnityGraphicsD3D12v3)

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v2)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Executes a given command list on a worker thread.
    // [Optional] Declares expected and post-execution resource states.
    // Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);
};
UNITY_REGISTER_INTERFACE_GUID(0xEC39D2F18446C745ULL, 0xB1A2626641D6B11FULL, IUnityGraphicsD3D12v2)


// Obsolete
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();
    ID3D12CommandQueue* (UNITY_INTERFACE_API * GetCommandQueue)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Returns the state a resource will be in after the last command list is executed
    bool(UNITY_INTERFACE_API * GetResourceState)(ID3D12Resource * resource, D3D12_RESOURCE_STATES * outState);
    // Specifies the state a resource will be in after a plugin command list with resource barriers is executed
    void(UNITY_INTERFACE_API * SetResourceState)(ID3D12Resource * resource, D3D12_RESOURCE_STATES state);
};
UNITY_REGISTER_INTERFACE_GUID(0xEF4CEC88A45F4C4CULL, 0xBD295B6F2A38D9DEULL, IUnityGraphicsD3D12)
