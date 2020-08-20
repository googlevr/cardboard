// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#if !UNITY
#include "IUnityInterface.h"
#endif
#include "IUnityXRDisplay.h"

/// @file IUnityXRDisplayShadingRateExt.h
/// @brief XR extension interface for texture allocation with shading rate image
/// @see IUnityXRDisplayShadingRateExt

/// @brief Exposes entry points related to shading rate
UNITY_DECLARE_INTERFACE(IUnityXRDisplayShadingRateExt)
{
    /// Provides the same functionality as IUnityXRDisplayInterface::CreateTexture, but also allows for supplying a shading rate image.
    /// This only works with the Vulkan graphics API if VK_EXT_fragment_density_map is available.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] colorDepthDesc Descriptor of the color/depth textures to be created.
    /// @param[in] shadingRateTexture Native texture pointer for a shading rate image.
    /// @param[out] outTexId Texture ID representing a unique instance of a texture.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully requested creation of texture.
    /// @return kUnitySubsystemErrorCodeInvalidArguments Invalid / null parameters
    /// @return kUnitySubsystemErrorCodeFailure Error
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * CreateTextureWithShadingRate)(UnitySubsystemHandle handle, const UnityXRRenderTextureDesc* colorDepthDesc, const UnityXRTextureData* shadingRateTexture, UnityXRRenderTextureId* outTexId);
};
UNITY_REGISTER_INTERFACE_GUID(0x1066FBD7A90E4A74ULL, 0xBCCBB730B26DE473ULL, IUnityXRDisplayShadingRateExt)
