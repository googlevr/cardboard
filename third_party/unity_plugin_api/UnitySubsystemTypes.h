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

#include "stddef.h"

/// Error codes for Subsystem operations
typedef enum UnitySubsystemErrorCode
{
    /// Indicates a successful operation
    kUnitySubsystemErrorCodeSuccess,

    /// Indicates the operation failed
    kUnitySubsystemErrorCodeFailure,

    /// Indicates invalid arguments were passed to the method
    kUnitySubsystemErrorCodeInvalidArguments,

    /// Indicates this feature is not supported
    kUnitySubsystemErrorCodeNotSupported,

    // A request to allocate memory by a subsystem has failed or can not be completed.
    kUnitySubsystemErrorCodeOutOfMemory
} UnitySubsystemErrorCode;

/// An UnitySubsystemHandle is an opaque type that's passed between the plugin and Unity. (C-API)
typedef void* UnitySubsystemHandle;

/// Event handler implemented by the plugin for subsystem specific lifecycle events (C-API).
typedef struct UnityLifecycleProvider
{
    /// Plugin-specific data pointer.  Every function in this structure will
    /// be passed this data by the Unity runtime.  The plugin should allocate
    /// an appropriate data container for any information that is needed when
    /// a callback function is called.
    void* userData;

    /// Initialize the subsystem.
    ///
    /// @param handle Handle for the current subsystem which can be passed to methods related to that subsystem.
    /// @param data Value of userData field when provider was registered.
    /// @return Status of function execution.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * Initialize)(UnitySubsystemHandle handle, void* data);

    /// Start the subsystem.
    ///
    /// @param handle Handle for the current subsystem which can be passed to methods related to that subsystem.
    /// @param data Value of userData field when provider was registered.
    /// @return Status of function execution.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * Start)(UnitySubsystemHandle handle, void* data);

    /// Stop the subsystem.
    ///
    /// @param handle Handle for the current subsystem which can be passed to methods related to that subsystem.
    /// @param data Value of userData field when provider was registered.
    void(UNITY_INTERFACE_API * Stop)(UnitySubsystemHandle handle, void* data);

    /// Shutdown the subsystem.
    ///
    /// @param handle Handle for the current subsystem which can be passed to methods related to that subsystem.
    /// @param data Value of userData field when provider was registered.
    void(UNITY_INTERFACE_API * Shutdown)(UnitySubsystemHandle handle, void* data);
} UnityLifecycleProvider;

// Macros and defines for back-compatibility code-gen (#defined to no-op when code-gen not running)
#if SUBSYSTEM_CODE_GEN
#define SUBSYSTEM_PROVIDER __attribute__((annotate("provider")))
#define SUBSYSTEM_INTERFACE __attribute__((annotate("interface")))
#define SUBSYSTEM_ADAPT_FROM(FUNC_NAME) __attribute__((annotate("adapt_from_" #FUNC_NAME)))
#define SUBSYSTEM_REGISTER_PROVIDER(PROVIDER) __attribute__((annotate("register_provider_" #PROVIDER)))
#define SUBSYSTEM_ADAPT_IN_PROVIDER_REGISTRATION __attribute__((annotate("in_provider_registration")))
#define UNITY_DECLARE_INTERFACE(NAME) struct NAME
#define UNITY_INTERFACE_API
#else
#define SUBSYSTEM_PROVIDER
#define SUBSYSTEM_INTERFACE
#define SUBSYSTEM_ADAPT_FROM(FUNC_NAME)
#define SUBSYSTEM_REGISTER_PROVIDER(PROVIDER)
#define SUBSYSTEM_ADAPT_IN_PROVIDER_REGISTRATION
#endif
