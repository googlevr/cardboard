// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#if UNITY
#include "Runtime/PluginInterface/Headers/IUnityInterface.h"
#include "Modules/XR/ProviderInterface/UnityXRSubsystemTypes.h"
#else
#include "IUnityInterface.h"
#include "UnitySubsystemTypes.h"
#endif

/// @cond undoc
#if !defined(UINT32_MAX)
#  define UINT32_MAX  4294967295UL
#endif
/// @endcond

/// @file IUnityXRStats.h
/// @brief XR Interface for recording and managing statistics data from XR subsystems

/// Flags that control various options and behaviors on registered stats.
typedef enum StatFlags
{
    /// Stat will have no special options or behaviors
    kUnityXRStatOptionNone = 0,
    /// Stat will clear to 0.0f at the beginning of every frame
    kUnityXRStatOptionClearOnUpdate = 1 << 0,
    /// Stat will have all special options and behaviors
    kUnityXRStatOptionAll = (1 << 1) - 1
} StatFlags;

/// An id used to identify individual statistics sourced from a specific Subsystem. They are unique for each stat registered with a subsystem.
typedef uint32_t UnityXRStatId;

/// @var kUnityInvalidXRStatId
/// Used to represent an invalid stat id
const UnityXRStatId kUnityInvalidXRStatId = UINT32_MAX;

/// @brief XR interface for statistics recording and stats data management.
///
/// Usage:
///
/// @code
/// #include "IUnityXRStats.h"
///
/// static IUnityXRStats* sXRStats = NULL;
///
/// extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
/// UnityPluginLoad(IUnityInterfaces* unityInterfaces)
/// {
///     sXRStats = (IUnityXRStats*)unityInterfaces->GetInterface(UNITY_GET_INTERFACE_GUID(IUnityXRStats));
///     // ... setup other subsystems
/// }
/// @endcode
UNITY_DECLARE_INTERFACE(IUnityXRStats)
{
    /// Used to register a subsystem that will be source statistics to the Unity stats interface. This is not thread safe and should be called from the main thread.
    ///
    /// @code
    /// int m_FrameCountStatId;
    /// int m_CpuTimeStatId;
    /// static UnitySubsystemErrorCode UNITY_INTERFACE_API Lifecycle_Initialize(UnitySubsystemHandle handle, void* userData)
    /// {
    ///      sXRStats->RegisterStatSource(handle);
    /// }
    /// @endcode
    /// @param[in] handle obtained from UnityLifecycleProvider callback
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * RegisterStatSource)(UnitySubsystemHandle handle);


    /// Used to register a stat definition with the stats interface. This is not thread safe and should be called from the main thread.
    ///
    /// @code
    /// int m_FrameCountStatId;
    /// int m_CpuTimeStatId;
    /// static UnitySubsystemErrorCode UNITY_INTERFACE_API Lifecycle_Initialize(UnitySubsystemHandle handle, void* userData)
    /// {
    ///      sXRStats->RegisterStatSource(handle);
    ///      m_FrameCountStatId = sXRStats->RegisterStatDefinition(handle, "FrameCount", kUnityXRStatOptionNone);
    /// }
    /// @endcode
    ///
    /// @param[in] handle Handle of a subsystem previously registered via RegisterStatProvider
    /// @param[in] statKey A key or tag used to identify the stat you are registering.
    /// @param[in] flags Flags that provide metadta about the stat you are registering.
    UnityXRStatId(UNITY_INTERFACE_API * RegisterStatDefinition)(UnitySubsystemHandle handle, const char* tag, unsigned int flags);

    /// Used to update a float type stat. This call is threadsafe.
    ///
    /// @code
    /// void EndOfFrame()
    /// {
    ///     sXRStats->SetStatFloat(CpuTimeStatId, MySystem::GetCPUTime());
    /// }
    /// @endcode
    ///
    /// @param[in] statID The statID obtained from calling RegisterStatDefinition.
    /// @param[in] value The value to set the stat to.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SetStatFloat)(UnityXRStatId statID, float value);

    /// Used to increment the frame from which stats will be recorded
    ///
    /// This is to be called at the beginning of a threads 'frame'
    ///
    /// Note: This is managed on the main and gfx thread automatically so it will NOT need to be called on either of those threads.
    ///
    /// @code
    /// void GfxThread_GetNextFrameDesc()
    /// {
    ///     // Do gfx things
    ///
    ///     sXRStats->SetStatFloat(GfxStatId, GetGPUTime());
    /// }
    ///
    /// void MainThread()
    /// {
    ///     // Do main thread things
    ///
    ///     sXRStats->SetStatFloat(MainThreadStatId, GetCPUTime());
    /// }
    ///
    /// void MyWorkerThread()
    /// {
    ///     IncrementStatFrame();
    ///
    ///     sXRStats->SetStatFloat(WorkerThreadStatId, WorkerThreadStat());
    /// }
    /// @endcode
    void(UNITY_INTERFACE_API * IncrementStatFrame)();

    /// Used to unregister the stats source from the stats interface. This is not thread safe and should be called from the main thread.
    ///
    /// @param[in] handle the handle obtained from the UnityLifecycleProvider callback of a previously registered Stats source
    UnitySubsystemErrorCode(UNITY_INTERFACE_API *  UnregisterStatSource)(UnitySubsystemHandle handle);
};
UNITY_REGISTER_INTERFACE_GUID(0xAB695A1C94114266ULL, 0xBDB5A1B3F7A54B8ULL, IUnityXRStats)
