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

/// LogType definition.
typedef enum XRLogType
{
    /// LogType used for Errors.
    kXRLogTypeError = 0,
    /// LogType used for Asserts. (These indicate an error inside Unity itself.)
    kXRLogTypeAssert = 1,
    /// LogType used for Warnings.
    kXRLogTypeWarning = 2,
    /// LogType used for regular log messages.
    kXRLogTypeLog = 3,
    /// LogType used for Exceptions.
    kXRLogTypeException = 4,
    /// LogType used for Debug.
    kXRLogTypeDebug = 5,
    ///
    kXRLogTypeNumLevels
} XRLogType;


#define XR_TRACE_IMPL(ptrace, logType, ...) \
    do { \
        if (ptrace) \
        { \
            ptrace->Trace(logType, __VA_ARGS__); \
        } \
    } while (0)

/// Literal logging macros for each type of log entry desired.

#define XR_TRACE_ERROR(ptrace, ...) \
    XR_TRACE_IMPL(ptrace, kXRLogTypeError, __VA_ARGS__)

#define XR_TRACE_ASSERT(ptrace, ...) \
    XR_TRACE_IMPL(ptrace, kXRLogTypeAssert, __VA_ARGS__)

#define XR_TRACE_WARNING(ptrace, ...) \
    XR_TRACE_IMPL(ptrace, kXRLogTypeWarning, __VA_ARGS__)


#define XR_TRACE_LOG(ptrace, ...) \
    XR_TRACE_IMPL(ptrace, kXRLogTypeLog, __VA_ARGS__)

#define XR_TRACE_EXCEPTION(ptrace, ...) \
    XR_TRACE_IMPL(ptrace, kXRLogTypeException, __VA_ARGS__)

#define XR_TRACE_DEBUG(ptrace, ...) \
    XR_TRACE_IMPL(ptrace, kXRLogTypeDebug, __VA_ARGS__)


/// Simple wrapper macro that defaults to debug logging. Assumes that XR_TRACE_PTR is defined as something
/// In order to be useful
#define XR_TRACE(...) \
    XR_TRACE_DEBUG(XR_TRACE_PTR, __VA_ARGS__)

/// Interface to allow providers to  trace to the Unity console.
UNITY_DECLARE_INTERFACE(IUnityXRTrace)
{
    /// Log a message to the unity log.
    ///
    /// @param[in] logType type of log
    /// @param[in] message message to log
    void(UNITY_INTERFACE_API * Trace)(XRLogType logType, const char* message, ...);
};


UNITY_REGISTER_INTERFACE_GUID(0xC633A7C9398B4A95ULL, 0xC225399ED5A2328FULL, IUnityXRTrace);
