// Unity Native Plugin API copyright © 2015 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see[Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once


#include "IUnityGraphics.h"

/*
    Low-level Native Plugin Rendering Extensions
    ============================================

    On top of the Low-level native plugin interface, Unity also supports low level rendering extensions that can receive callbacks when certain events happen.
    This is mostly used to implement and control low-level rendering in your plugin and enable it to work with Unity’s multithreaded rendering.

    Due to the low-level nature of this extension the plugin might need to be preloaded before the devices get created.
    Currently the convention is name-based namely the plugin name must be prefixed by “GfxPlugin”. Example: GfxPluginMyFancyNativePlugin.

    <code>
        // Native plugin code example

        enum PluginCustomCommands
        {
            kPluginCustomCommandDownscale = kUnityRenderingExtUserEventsStart,
            kPluginCustomCommandUpscale,

            // insert your own events here

            kPluginCustomCommandCount
        };

        static IUnityInterfaces* s_UnityInterfaces = NULL;

        extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
        UnityPluginLoad(IUnityInterfaces* unityInterfaces)
        {
            // initialization code here...
        }

        extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
        UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data)
        {
            switch (event)
            {
                case kUnityRenderingExtEventBeforeDrawCall:
                    // do some stuff
                    break;
                case kUnityRenderingExtEventAfterDrawCall:
                    // undo some stuff
                    break;
                case kPluginCustomCommandDownscale:
                    // downscale some stuff
                    break;
                case kPluginCustomCommandUpscale:
                    // upscale some stuff
                    break;
            }
        }
    </code>
*/


//     These events will be propagated to all plugins that implement void UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data);

typedef enum UnityRenderingExtEventType
{
    kUnityRenderingExtEventSetStereoTarget,                 // issued during SetStereoTarget and carrying the current 'eye' index as parameter
    kUnityRenderingExtEventSetStereoEye,                    // issued during stereo rendering at the beginning of each eye's rendering loop. It carries the current 'eye' index as parameter
    kUnityRenderingExtEventStereoRenderingDone,             // issued after the rendering has finished
    kUnityRenderingExtEventBeforeDrawCall,                  // issued during BeforeDrawCall and carrying UnityRenderingExtBeforeDrawCallParams as parameter
    kUnityRenderingExtEventAfterDrawCall,                   // issued during AfterDrawCall. This event doesn't carry any parameters
    kUnityRenderingExtEventCustomGrab,                      // issued during GrabIntoRenderTexture since we can't simply copy the resources
                                                            //      when custom rendering is used - we need to let plugin handle this. It carries over
                                                            //      a UnityRenderingExtCustomBlitParams params = { X, source, dest, 0, 0 } ( X means it's irrelevant )
    kUnityRenderingExtEventCustomBlit,                      // issued by plugin to insert custom blits. It carries over UnityRenderingExtCustomBlitParams as param.
    kUnityRenderingExtEventUpdateTextureBegin,                                                  // Deprecated.
    kUnityRenderingExtEventUpdateTextureEnd,                                                    // Deprecated.
    kUnityRenderingExtEventUpdateTextureBeginV1 = kUnityRenderingExtEventUpdateTextureBegin,    // Deprecated. Issued to update a texture. It carries over UnityRenderingExtTextureUpdateParamsV1
    kUnityRenderingExtEventUpdateTextureEndV1 = kUnityRenderingExtEventUpdateTextureEnd,        // Deprecated. Issued to signal the plugin that the texture update has finished. It carries over the same UnityRenderingExtTextureUpdateParamsV1 as kUnityRenderingExtEventUpdateTextureBeginV1
    kUnityRenderingExtEventUpdateTextureBeginV2,                                                // Issued to update a texture. It carries over UnityRenderingExtTextureUpdateParamsV2
    kUnityRenderingExtEventUpdateTextureEndV2,                                                  // Issued to signal the plugin that the texture update has finished. It carries over the same UnityRenderingExtTextureUpdateParamsV2 as kUnityRenderingExtEventUpdateTextureBeginV2

    // keep this last
    kUnityRenderingExtEventCount,
    kUnityRenderingExtUserEventsStart = kUnityRenderingExtEventCount
} UnityRenderingExtEventType;


typedef enum UnityRenderingExtCustomBlitCommands
{
    kUnityRenderingExtCustomBlitVRFlush,                    // This event is mostly used in multi GPU configurations ( SLI, etc ) in order to allow the plugin to flush all GPU's targets

    // keep this last
    kUnityRenderingExtCustomBlitCount,
    kUnityRenderingExtUserCustomBlitStart = kUnityRenderingExtCustomBlitCount
} UnityRenderingExtCustomBlitCommands;

/*
    This will be propagated to all plugins implementing UnityRenderingExtQuery.
*/
typedef enum UnityRenderingExtQueryType
{
    kUnityRenderingExtQueryOverrideViewport             = 1 << 0,           // The plugin handles setting up the viewport rects. Unity will skip its internal SetViewport calls
    kUnityRenderingExtQueryOverrideScissor              = 1 << 1,           // The plugin handles setting up the scissor rects. Unity will skip its internal SetScissor calls
    kUnityRenderingExtQueryOverrideVROcclussionMesh     = 1 << 2,           // The plugin handles its own VR occlusion mesh rendering. Unity will skip rendering its internal VR occlusion mask
    kUnityRenderingExtQueryOverrideVRSinglePass         = 1 << 3,           // The plugin uses its own single pass stereo technique. Unity will only traverse and render the render node graph once.
                                                                            //      and it will clear the whole render target not just per-eye on demand.
    kUnityRenderingExtQueryKeepOriginalDoubleWideWidth_DEPRECATED  = 1 << 4,           // Instructs unity to keep the original double wide width. By default unity will try and have a power-of-two width for mip-mapping requirements.
    kUnityRenderingExtQueryRequestVRFlushCallback       = 1 << 5,           // Instructs unity to provide callbacks when the VR eye textures need flushing. Useful for multi GPU synchronization.
} UnityRenderingExtQueryType;


typedef enum UnityRenderingExtTextureFormat
{
    kUnityRenderingExtFormatNone = 0, kUnityRenderingExtFormatFirst = kUnityRenderingExtFormatNone,

    // sRGB formats
    kUnityRenderingExtFormatR8_SRGB,
    kUnityRenderingExtFormatR8G8_SRGB,
    kUnityRenderingExtFormatR8G8B8_SRGB,
    kUnityRenderingExtFormatR8G8B8A8_SRGB,

    // 8 bit integer formats
    kUnityRenderingExtFormatR8_UNorm,
    kUnityRenderingExtFormatR8G8_UNorm,
    kUnityRenderingExtFormatR8G8B8_UNorm,
    kUnityRenderingExtFormatR8G8B8A8_UNorm,
    kUnityRenderingExtFormatR8_SNorm,
    kUnityRenderingExtFormatR8G8_SNorm,
    kUnityRenderingExtFormatR8G8B8_SNorm,
    kUnityRenderingExtFormatR8G8B8A8_SNorm,
    kUnityRenderingExtFormatR8_UInt,
    kUnityRenderingExtFormatR8G8_UInt,
    kUnityRenderingExtFormatR8G8B8_UInt,
    kUnityRenderingExtFormatR8G8B8A8_UInt,
    kUnityRenderingExtFormatR8_SInt,
    kUnityRenderingExtFormatR8G8_SInt,
    kUnityRenderingExtFormatR8G8B8_SInt,
    kUnityRenderingExtFormatR8G8B8A8_SInt,

    // 16 bit integer formats
    kUnityRenderingExtFormatR16_UNorm,
    kUnityRenderingExtFormatR16G16_UNorm,
    kUnityRenderingExtFormatR16G16B16_UNorm,
    kUnityRenderingExtFormatR16G16B16A16_UNorm,
    kUnityRenderingExtFormatR16_SNorm,
    kUnityRenderingExtFormatR16G16_SNorm,
    kUnityRenderingExtFormatR16G16B16_SNorm,
    kUnityRenderingExtFormatR16G16B16A16_SNorm,
    kUnityRenderingExtFormatR16_UInt,
    kUnityRenderingExtFormatR16G16_UInt,
    kUnityRenderingExtFormatR16G16B16_UInt,
    kUnityRenderingExtFormatR16G16B16A16_UInt,
    kUnityRenderingExtFormatR16_SInt,
    kUnityRenderingExtFormatR16G16_SInt,
    kUnityRenderingExtFormatR16G16B16_SInt,
    kUnityRenderingExtFormatR16G16B16A16_SInt,

    // 32 bit integer formats
    kUnityRenderingExtFormatR32_UInt,
    kUnityRenderingExtFormatR32G32_UInt,
    kUnityRenderingExtFormatR32G32B32_UInt,
    kUnityRenderingExtFormatR32G32B32A32_UInt,
    kUnityRenderingExtFormatR32_SInt,
    kUnityRenderingExtFormatR32G32_SInt,
    kUnityRenderingExtFormatR32G32B32_SInt,
    kUnityRenderingExtFormatR32G32B32A32_SInt,

    // HDR formats
    kUnityRenderingExtFormatR16_SFloat,
    kUnityRenderingExtFormatR16G16_SFloat,
    kUnityRenderingExtFormatR16G16B16_SFloat,
    kUnityRenderingExtFormatR16G16B16A16_SFloat,
    kUnityRenderingExtFormatR32_SFloat,
    kUnityRenderingExtFormatR32G32_SFloat,
    kUnityRenderingExtFormatR32G32B32_SFloat,
    kUnityRenderingExtFormatR32G32B32A32_SFloat,

    // Luminance and Alpha format
    kUnityRenderingExtFormatL8_UNorm,
    kUnityRenderingExtFormatA8_UNorm,
    kUnityRenderingExtFormatA16_UNorm,

    // BGR formats
    kUnityRenderingExtFormatB8G8R8_SRGB,
    kUnityRenderingExtFormatB8G8R8A8_SRGB,
    kUnityRenderingExtFormatB8G8R8_UNorm,
    kUnityRenderingExtFormatB8G8R8A8_UNorm,
    kUnityRenderingExtFormatB8G8R8_SNorm,
    kUnityRenderingExtFormatB8G8R8A8_SNorm,
    kUnityRenderingExtFormatB8G8R8_UInt,
    kUnityRenderingExtFormatB8G8R8A8_UInt,
    kUnityRenderingExtFormatB8G8R8_SInt,
    kUnityRenderingExtFormatB8G8R8A8_SInt,

    // 16 bit packed formats
    kUnityRenderingExtFormatR4G4B4A4_UNormPack16,
    kUnityRenderingExtFormatB4G4R4A4_UNormPack16,
    kUnityRenderingExtFormatR5G6B5_UNormPack16,
    kUnityRenderingExtFormatB5G6R5_UNormPack16,
    kUnityRenderingExtFormatR5G5B5A1_UNormPack16,
    kUnityRenderingExtFormatB5G5R5A1_UNormPack16,
    kUnityRenderingExtFormatA1R5G5B5_UNormPack16,

    // Packed formats
    kUnityRenderingExtFormatE5B9G9R9_UFloatPack32,
    kUnityRenderingExtFormatB10G11R11_UFloatPack32,

    kUnityRenderingExtFormatA2B10G10R10_UNormPack32,
    kUnityRenderingExtFormatA2B10G10R10_UIntPack32,
    kUnityRenderingExtFormatA2B10G10R10_SIntPack32,
    kUnityRenderingExtFormatA2R10G10B10_UNormPack32,
    kUnityRenderingExtFormatA2R10G10B10_UIntPack32,
    kUnityRenderingExtFormatA2R10G10B10_SIntPack32,
    kUnityRenderingExtFormatA2R10G10B10_XRSRGBPack32,
    kUnityRenderingExtFormatA2R10G10B10_XRUNormPack32,
    kUnityRenderingExtFormatR10G10B10_XRSRGBPack32,
    kUnityRenderingExtFormatR10G10B10_XRUNormPack32,
    kUnityRenderingExtFormatA10R10G10B10_XRSRGBPack32,
    kUnityRenderingExtFormatA10R10G10B10_XRUNormPack32,

    // ARGB formats... TextureFormat legacy
    kUnityRenderingExtFormatA8R8G8B8_SRGB,
    kUnityRenderingExtFormatA8R8G8B8_UNorm,
    kUnityRenderingExtFormatA32R32G32B32_SFloat,

    // Depth Stencil for formats
    kUnityRenderingExtFormatD16_UNorm,
    kUnityRenderingExtFormatD24_UNorm,
    kUnityRenderingExtFormatD24_UNorm_S8_UInt,
    kUnityRenderingExtFormatD32_SFloat,
    kUnityRenderingExtFormatD32_SFloat_S8_Uint,
    kUnityRenderingExtFormatS8_Uint,

    // Compression formats
    kUnityRenderingExtFormatRGBA_DXT1_SRGB,
    kUnityRenderingExtFormatRGBA_DXT1_UNorm,
    kUnityRenderingExtFormatRGBA_DXT3_SRGB,
    kUnityRenderingExtFormatRGBA_DXT3_UNorm,
    kUnityRenderingExtFormatRGBA_DXT5_SRGB,
    kUnityRenderingExtFormatRGBA_DXT5_UNorm,
    kUnityRenderingExtFormatR_BC4_UNorm,
    kUnityRenderingExtFormatR_BC4_SNorm,
    kUnityRenderingExtFormatRG_BC5_UNorm,
    kUnityRenderingExtFormatRG_BC5_SNorm,
    kUnityRenderingExtFormatRGB_BC6H_UFloat,
    kUnityRenderingExtFormatRGB_BC6H_SFloat,
    kUnityRenderingExtFormatRGBA_BC7_SRGB,
    kUnityRenderingExtFormatRGBA_BC7_UNorm,

    kUnityRenderingExtFormatRGB_PVRTC_2Bpp_SRGB,
    kUnityRenderingExtFormatRGB_PVRTC_2Bpp_UNorm,
    kUnityRenderingExtFormatRGB_PVRTC_4Bpp_SRGB,
    kUnityRenderingExtFormatRGB_PVRTC_4Bpp_UNorm,
    kUnityRenderingExtFormatRGBA_PVRTC_2Bpp_SRGB,
    kUnityRenderingExtFormatRGBA_PVRTC_2Bpp_UNorm,
    kUnityRenderingExtFormatRGBA_PVRTC_4Bpp_SRGB,
    kUnityRenderingExtFormatRGBA_PVRTC_4Bpp_UNorm,

    kUnityRenderingExtFormatRGB_ETC_UNorm,
    kUnityRenderingExtFormatRGB_ETC2_SRGB,
    kUnityRenderingExtFormatRGB_ETC2_UNorm,
    kUnityRenderingExtFormatRGB_A1_ETC2_SRGB,
    kUnityRenderingExtFormatRGB_A1_ETC2_UNorm,
    kUnityRenderingExtFormatRGBA_ETC2_SRGB,
    kUnityRenderingExtFormatRGBA_ETC2_UNorm,

    kUnityRenderingExtFormatR_EAC_UNorm,
    kUnityRenderingExtFormatR_EAC_SNorm,
    kUnityRenderingExtFormatRG_EAC_UNorm,
    kUnityRenderingExtFormatRG_EAC_SNorm,

    kUnityRenderingExtFormatRGBA_ASTC4X4_SRGB,
    kUnityRenderingExtFormatRGBA_ASTC4X4_UNorm,
    kUnityRenderingExtFormatRGBA_ASTC5X5_SRGB,
    kUnityRenderingExtFormatRGBA_ASTC5X5_UNorm,
    kUnityRenderingExtFormatRGBA_ASTC6X6_SRGB,
    kUnityRenderingExtFormatRGBA_ASTC6X6_UNorm,
    kUnityRenderingExtFormatRGBA_ASTC8X8_SRGB,
    kUnityRenderingExtFormatRGBA_ASTC8X8_UNorm,
    kUnityRenderingExtFormatRGBA_ASTC10X10_SRGB,
    kUnityRenderingExtFormatRGBA_ASTC10X10_UNorm,
    kUnityRenderingExtFormatRGBA_ASTC12X12_SRGB,
    kUnityRenderingExtFormatRGBA_ASTC12X12_UNorm,

    // Video formats
    kUnityRenderingExtFormatYUV2,

    // Automatic formats, back-end decides
    kUnityRenderingExtFormatDepthAuto,
    kUnityRenderingExtFormatShadowAuto,
    kUnityRenderingExtFormatVideoAuto,

    // ASTC hdr profile
    kUnityRenderingExtFormatRGBA_ASTC4X4_UFloat,
    kUnityRenderingExtFormatRGBA_ASTC5X5_UFloat,
    kUnityRenderingExtFormatRGBA_ASTC6X6_UFloat,
    kUnityRenderingExtFormatRGBA_ASTC8X8_UFloat,
    kUnityRenderingExtFormatRGBA_ASTC10X10_UFloat,
    kUnityRenderingExtFormatRGBA_ASTC12X12_UFloat,

    kUnityRenderingExtFormatLast = kUnityRenderingExtFormatRGBA_ASTC12X12_UFloat, // Remove?
} UnityRenderingExtTextureFormat;


typedef struct UnityRenderingExtBeforeDrawCallParams
{
    void*   vertexShader;                           // bound vertex shader (platform dependent)
    void*   fragmentShader;                         // bound fragment shader (platform dependent)
    void*   geometryShader;                         // bound geometry shader (platform dependent)
    void*   hullShader;                             // bound hull shader (platform dependent)
    void*   domainShader;                           // bound domain shader (platform dependent)
    int     eyeIndex;                               // the index of the current stereo "eye" being currently rendered.
} UnityRenderingExtBeforeDrawCallParams;


typedef struct UnityRenderingExtCustomBlitParams
{
    UnityTextureID source;                          // source texture
    UnityRenderBuffer destination;                  // destination surface
    unsigned int command;                           // command for the custom blit - could be any UnityRenderingExtCustomBlitCommands command or custom ones.
    unsigned int commandParam;                      // custom parameters for the command
    unsigned int commandFlags;                      // custom flags for the command
} UnityRenderingExtCustomBlitParams;

// Deprecated. Use UnityRenderingExtTextureUpdateParamsV2 and CommandBuffer.IssuePluginCustomTextureUpdateV2 instead.
// Only supports DX11, GLES, Metal
typedef struct UnityRenderingExtTextureUpdateParamsV1
{
    void*        texData;                           // source data for the texture update. Must be set by the plugin
    unsigned int userData;                          // user defined data. Set by the plugin

    unsigned int textureID;                         // texture ID of the texture to be updated.
    UnityRenderingExtTextureFormat format;          // format of the texture to be updated
    unsigned int width;                             // width of the texture
    unsigned int height;                            // height of the texture
    unsigned int bpp;                               // texture bytes per pixel.
} UnityRenderingExtTextureUpdateParamsV1;

// Deprecated. Use UnityRenderingExtTextureUpdateParamsV2 and CommandBuffer.IssuePluginCustomTextureUpdateV2 instead.
// Only supports DX11, GLES, Metal
typedef UnityRenderingExtTextureUpdateParamsV1 UnityRenderingExtTextureUpdateParams;

// Type of the "data" parameter passed when callbacks registered with CommandBuffer.IssuePluginCustomTextureUpdateV2 are called.
// Supports DX11, GLES, Metal, and Switch (also possibly PS4, PSVita in the future)
typedef struct UnityRenderingExtTextureUpdateParamsV2
{
    void*        texData;                           // source data for the texture update. Must be set by the plugin
    intptr_t     textureID;                         // texture ID of the texture to be updated.
    unsigned int userData;                          // user defined data. Set by the plugin
    UnityRenderingExtTextureFormat format;          // format of the texture to be updated
    unsigned int width;                             // width of the texture
    unsigned int height;                            // height of the texture
    unsigned int bpp;                               // texture bytes per pixel.
} UnityRenderingExtTextureUpdateParamsV2;


// Certain Unity APIs (GL.IssuePluginEventAndData, CommandBuffer.IssuePluginEventAndData) can callback into native plugins.
// Provide them with an address to a function of this signature.
typedef void (UNITY_INTERFACE_API * UnityRenderingEventAndData)(int eventId, void* data);


#ifdef __cplusplus
extern "C" {
#endif

// If exported by a plugin, this function will be called for all the events in UnityRenderingExtEventType
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityRenderingExtEvent(UnityRenderingExtEventType event, void* data);
// If exported by a plugin, this function will be called to query the plugin for the queries in UnityRenderingExtQueryType
bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityRenderingExtQuery(UnityRenderingExtQueryType query);

#ifdef __cplusplus
}
#endif
