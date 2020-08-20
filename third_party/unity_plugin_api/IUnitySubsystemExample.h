// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#if UNITY
#   include "Modules/Subsystems/ProviderInterface/UnityTypes.h"
#   include "Modules/Subsystems/ProviderInterface/UnitySubsystemTypes.h"
#else
#   include "UnityTypes.h"
#   include "UnitySubsystemTypes.h"
#endif

/// Sample data structure used for passing data between a Subsystem plugin and
/// the Unity runtime.
typedef struct UnitySubsystemExampleState
{
    /// Description of theBool
    bool theBool;

    /// Description of theFloat
    float theFloat;

    /// Description of theString
    const char* theString;

    /// Description of theDouble
    double theDouble;
} UnitySubsystemExampleState;

/// Legacy sample data structure used for passing data between a Subsystem plugin and the Unity runtime.
/// This version does not change between versions of the plugin.
typedef struct UnitySubsystemLegacyExampleState
{
    /// Description of theBool
    bool theBool;
} UnitySubsystemLegacyExampleState;

/// Example container illustrating the interface of functions for Unity to
/// call into a plugin.  A plugin should initialize all of the (relevant)
/// function pointers with function addresses local to the plugin library,
/// and then return the structure to Unity in the plugin's Initialize function
/// using the RegisterProviderFunctions function that's available as part
/// of the Unity plugin interface structure (below).
typedef struct SUBSYSTEM_PROVIDER UnitySubsystemExampleProvider
{
    /// Plugin-specific data pointer.  Every function in this structure will
    /// be passed this data by the Unity runtime.  The plugin should allocate
    /// an appropriate data container for any information that is needed when
    /// a callback function is called.
    void* userData;

    /// Example unity -> plugin batched update call, which is preferable to
    /// calling in to the plugin multiple times for different pieces of data.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param data Value of userData field when provider was registered.
    /// @param state State to be updated.
    /// @return kUnitySubsystemErrorCodeFailure if state was not updated.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * UpdateState)(UnitySubsystemHandle handle, void* userData, UnitySubsystemExampleState * state);

    /// Example non-adapted unity -> plugin call.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param userData Value of userData field when provider was registered.
    /// @param state State to be updated.
    /// @return kUnitySubsystemErrorCodeFailure if state was not updated.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * UpdateStateLegacy)(UnitySubsystemHandle handle, void* userData, UnitySubsystemLegacyExampleState * state);
} UnitySubsystemExampleProvider;

/// An example Subsystem interface showing how to create plugin -> unity call flows with complex data, and vice-versa.
/// Also showing how to deal with binary backwards compatibility of dylibs on a potentially ever-changing interface.
UNITY_DECLARE_INTERFACE(SUBSYSTEM_INTERFACE IUnitySubsystemExampleInterface)
{
    /// Entry-point for getting callbacks when the example subsystem is initialized / started / stopped / shutdown.
    ///
    /// @param[in] pluginName Name of the plugin which was registered in your xr.json
    /// @param[in] id Id of the subsystem that was registered in your xr.json
    /// @param[in] provider Callbacks to register.
    /// @return Status of function execution.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * RegisterLifecycleProvider)(const char* pluginName, const char* id, const UnityLifecycleProvider * provider);

    /// Register for example subsystem callbacks.
    ///
    /// @param[in] handle Handle obtained from IUnityLifecycleProvider callbacks.
    /// @param[in] provider Callbacks to register.
    /// @return kUnitySubsystemErrorCodeSuccess if successfully initialized, kUnitySubsystemErrorCodeFailure or kUnitySubsystemErrorCodeInvalidArguments on error conditions.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SUBSYSTEM_REGISTER_PROVIDER(UnitySubsystemExampleProvider) RegisterExampleProvider)(UnitySubsystemHandle handle, const UnitySubsystemExampleProvider * provider);

    /// Example plugin -> unity call.  Dumps some state about the current Example subsytem handle.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param header Value to be printed.
    /// @return Status of function execution.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DebugDumpState)(UnitySubsystemHandle handle, const char* header);

    /// Example non-adapted plugin -> unity call.  Print out a state struct.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param state State to be printed.
    /// @return Status of function execution.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DebugLegacyState)(UnitySubsystemHandle handle, UnitySubsystemLegacyExampleState * state);
};
UNITY_REGISTER_INTERFACE_GUID(0xAB695A1C94804266ULL, 0xBDB5A1B347AC54B8ULL, IUnitySubsystemExampleInterface)
