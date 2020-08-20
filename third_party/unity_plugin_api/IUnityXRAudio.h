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


/// Interface to allow providers to set up audio input and output devices.
UNITY_DECLARE_INTERFACE(IUnityXRAudio)
{
    /// Set the audio output endpoint.
    ///
    /// @param[in] guid A pointer to the 16-byte GUID of the audio endpoint to use for audio output.  The GUID value is copied if it is needed beyond the lifetime of this call.
    /// @return true on success, false on failure.
    bool(UNITY_INTERFACE_API * SetOutput)(const void* guid);

    /// Set the audio input endpoint.
    ///
    /// @param[in] guid A pointer to the 16-byte GUID of the audio endpoint to use for audio input.  The GUID value is copied if it is needed beyond the lifetime of this call.
    /// @return true on success, false on failure.
    bool(UNITY_INTERFACE_API * SetInput)(const void* guid);
};


UNITY_REGISTER_INTERFACE_GUID(0x62a4ec89e8d54f6bULL, 0x92ae9e5c46bc6e55ULL, IUnityXRAudio);
