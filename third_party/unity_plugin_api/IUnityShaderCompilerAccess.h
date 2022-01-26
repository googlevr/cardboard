// Unity Native Plugin API copyright © 2015 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see[Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once


#include "IUnityGraphics.h"


/*
    Low-level Native Plugin Shader Compiler Access
    ==============================================

    On top of the Low-level native plugin interface, Unity also supports low level access to the shader compiler, allowing the user to inject different variants into a shader.
    It is also an event driven approach in which the plugin will receive callbacks when certain builtin events happen.
*/


enum UnityShaderCompilerExtCompilerPlatform
{
    kUnityShaderCompilerExtCompPlatformUnused0 = 0,
    kUnityShaderCompilerExtCompPlatformUnused1,
    kUnityShaderCompilerExtCompPlatformUnused2,
    kUnityShaderCompilerExtCompPlatformUnused3,
    kUnityShaderCompilerExtCompPlatformD3D11,           // Direct3D 11 (FL10.0 and up), compiled with MS D3DCompiler
    kUnityShaderCompilerExtCompPlatformGLES20,          // OpenGL ES 2.0 / WebGL 1.0, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformUnused6,
    kUnityShaderCompilerExtCompPlatformUnused7,
    kUnityShaderCompilerExtCompPlatformUnused8,
    kUnityShaderCompilerExtCompPlatformGLES3Plus,       // OpenGL ES 3.0+ / WebGL 2.0, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformUnused10,
    kUnityShaderCompilerExtCompPlatformPS4,             // Sony PS4
    kUnityShaderCompilerExtCompPlatformXboxOne,         // MS XboxOne
    kUnityShaderCompilerExtCompPlatformUnused13,
    kUnityShaderCompilerExtCompPlatformMetal,           // Apple Metal, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformOpenGLCore,      // Desktop OpenGL 3+, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformUnused16,
    kUnityShaderCompilerExtCompPlatformUnused17,
    kUnityShaderCompilerExtCompPlatformVulkan,          // Vulkan SPIR-V, compiled with MS D3DCompiler + HLSLcc
    kUnityShaderCompilerExtCompPlatformSwitch,          // Nintendo Switch (NVN)
    kUnityShaderCompilerExtCompPlatformXboxOneD3D12,    // Xbox One D3D12
    kUnityShaderCompilerExtCompPlatformGameCoreXboxOne, // GameCore XboxOne
    kUnityShaderCompilerExtCompPlatformGameCoreXboxSeries,// GameCore XboxSeries
    kUnityShaderCompilerExtCompPlatformPS5,
    kUnityShaderCompilerExtCompPlatformPS5NGGC,

    kUnityShaderCompilerExtCompPlatformCount
};


enum UnityShaderCompilerExtShaderType
{
    kUnityShaderCompilerExtShaderNone = 0,
    kUnityShaderCompilerExtShaderVertex = 1,
    kUnityShaderCompilerExtShaderFragment = 2,
    kUnityShaderCompilerExtShaderGeometry = 3,
    kUnityShaderCompilerExtShaderHull = 4,
    kUnityShaderCompilerExtShaderDomain = 5,
    kUnityShaderCompilerExtShaderRayTracing = 6,
    kUnityShaderCompilerExtShaderTypeCount // keep this last!
};


enum UnityShaderCompilerExtGPUProgramType
{
    kUnityShaderCompilerExtGPUProgramTargetUnknown = 0,

    kUnityShaderCompilerExtGPUProgramTargetGLLegacy = 1, // removed
    kUnityShaderCompilerExtGPUProgramTargetGLES31AEP = 2,
    kUnityShaderCompilerExtGPUProgramTargetGLES31 = 3,
    kUnityShaderCompilerExtGPUProgramTargetGLES3 = 4,
    kUnityShaderCompilerExtGPUProgramTargetGLES = 5,
    kUnityShaderCompilerExtGPUProgramTargetGLCore32 = 6,
    kUnityShaderCompilerExtGPUProgramTargetGLCore41 = 7,
    kUnityShaderCompilerExtGPUProgramTargetGLCore43 = 8,
    kUnityShaderCompilerExtGPUProgramTargetDX9VertexSM20 = 9, // removed
    kUnityShaderCompilerExtGPUProgramTargetDX9VertexSM30 = 10, // removed
    kUnityShaderCompilerExtGPUProgramTargetDX9PixelSM20 = 11, // removed
    kUnityShaderCompilerExtGPUProgramTargetDX9PixelSM30 = 12, // removed
    kUnityShaderCompilerExtGPUProgramTargetDX10Level9Vertex = 13,
    kUnityShaderCompilerExtGPUProgramTargetDX10Level9Pixel = 14,
    kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM40 = 15,
    kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM50 = 16,
    kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM40 = 17,
    kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM50 = 18,
    kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM40 = 19,
    kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM50 = 20,
    kUnityShaderCompilerExtGPUProgramTargetDX11HullSM50 = 21,
    kUnityShaderCompilerExtGPUProgramTargetDX11DomainSM50 = 22,
    kUnityShaderCompilerExtGPUProgramTargetMetalVS = 23,
    kUnityShaderCompilerExtGPUProgramTargetMetalFS = 24,
    kUnityShaderCompilerExtGPUProgramTargetSPIRV = 25,
    kUnityShaderCompilerExtGPUProgramTargetUnused1 = 26,
    kUnityShaderCompilerExtGPUProgramTargetUnused2 = 27,
    kUnityShaderCompilerExtGPUProgramTargetUnused3 = 28,
    kUnityShaderCompilerExtGPUProgramTargetUnused4 = 29,
    kUnityShaderCompilerExtGPUProgramTargetUnused5 = 30,
    kUnityShaderCompilerExtGPUProgramTargetRayTracing = 31,

    kUnityShaderCompilerExtGPUProgramTargetCount
};


enum UnityShaderCompilerExtGPUProgram
{
    kUnityShaderCompilerExtGPUProgramVS = 1 << kUnityShaderCompilerExtShaderVertex,
    kUnityShaderCompilerExtGPUProgramPS = 1 << kUnityShaderCompilerExtShaderFragment,
    kUnityShaderCompilerExtGPUProgramGS = 1 << kUnityShaderCompilerExtShaderGeometry,
    kUnityShaderCompilerExtGPUProgramHS = 1 << kUnityShaderCompilerExtShaderHull,
    kUnityShaderCompilerExtGPUProgramDS = 1 << kUnityShaderCompilerExtShaderDomain,
    kUnityShaderCompilerExtGPUProgramCustom = 1 << kUnityShaderCompilerExtShaderTypeCount
};


enum UnityShaderCompilerExtEventType
{
    kUnityShaderCompilerExtEventCreateCustomSourceVariant,          // The plugin can create a new variant based on the initial snippet. The callback will receive UnityShaderCompilerExtCustomSourceVariantParams as data.
    kUnityShaderCompilerExtEventCreateCustomSourceVariantCleanup,   // Typically received after the kUnityShaderCompilerExtEventCreateCustomVariant event so the plugin has a chance to cleanup after itself. (outputSnippet & outputKeyword)

    kUnityShaderCompilerExtEventCreateCustomBinaryVariant,          // The plugin can create a new variant based on the initial snippet. The callback will receive UnityShaderCompilerExtCustomBinaryVariantParams as data.
    kUnityShaderCompilerExtEventCreateCustomBinaryVariantCleanup,   // Typically received after the kUnityShaderCompilerExtEventCreateCustomVariant event so the plugin has a chance to cleanup after itself. (outputSnippet & outputKeyword)
    kUnityShaderCompilerExtEventPluginConfigure,                    // Event sent so the plugin can configure itself. It receives IUnityShaderCompilerExtPluginConfigure* as data

    // keep these last
    kUnityShaderCompilerExtEventCount,
    kUnityShaderCompilerExtUserEventsStart = kUnityShaderCompilerExtEventCount
};


struct UnityShaderCompilerExtCustomSourceVariantParams
{
    char* outputSnippet;                                    // snippet after the plugin has finished processing it.
    char* outputKeywords;                                   // keywords exported by the plugin for this specific variant
    const char* inputSnippet;                               // the source shader snippet
    bool vr;                                                // is VR enabled / supported ?
    UnityShaderCompilerExtCompilerPlatform platform;        // compiler platform
    UnityShaderCompilerExtShaderType shaderType;            // source code type
};


struct UnityShaderCompilerExtCustomBinaryVariantParams
{
    void**  outputBinaryShader;                             // output of the plugin generated binary shader (platform dependent)
    const unsigned char* inputByteCode;                     // the shader byteCode (platform dependent)
    unsigned int inputByteCodeSize;                         // shader bytecode size
    unsigned int programTypeMask;                           // a mask of UnityShaderCompilerExtGPUProgram values
    UnityShaderCompilerExtCompilerPlatform platform;        // compiler platform
};


/*
    This class is queried by unity to get information about the plugin.
    The plugin has to set all the relevant values during the kUnityShaderCompilerExtEventPluginConfigure event.

    <code>

        extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
        UnityShaderCompilerExtEvent(UnityShaderCompilerExtEventType event, void* data)
        {
            switch (event)
            {

                ...

                case kUnityShaderCompilerExtEventPluginConfigure:
                {
                    unsigned int GPUCompilerMask = (1 << kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM40)
                    | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11VertexSM50)
                    | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM40)
                    | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11PixelSM50)
                    | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM40)
                    | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11GeometrySM50)
                    | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11HullSM50)
                    | (1 << kUnityShaderCompilerExtGPUProgramTargetDX11DomainSM50);

                    unsigned int ShaderMask = kUnityShaderCompilerExtGPUProgramVS | kUnityShaderCompilerExtGPUProgramDS;

                    IUnityShaderCompilerExtPluginConfigure* pluginConfig = (IUnityShaderCompilerExtPluginConfigure*)data;
                    pluginConfig->ReserveKeyword(SHADER_KEYWORDS);
                    pluginConfig->SetGPUProgramCompilerMask(GPUCompilerMask);
                    pluginConfig->SetShaderProgramMask(ShaderMask);
                    break;
                }
            }
        }

    </code>
*/

class IUnityShaderCompilerExtPluginConfigure
{
public:
    virtual ~IUnityShaderCompilerExtPluginConfigure() {}
    virtual void ReserveKeyword(const char* keyword) = 0;           // Allow the plugin to reserve its keyword so unity can include it in calculating the variants
    virtual void SetGPUProgramCompilerMask(unsigned int mask) = 0;  // Specifies a bit mask of UnityShaderCompilerExtGPUProgramType programs the plugin supports compiling
    virtual void SetShaderProgramMask(unsigned int mask) = 0;       // Specifies a bit mask of UnityShaderCompilerExtGPUProgram programs the plugin supports compiling
};


// Interface to help setup / configure both unity and the plugin.


#ifdef __cplusplus
extern "C" {
#endif

// If exported by a plugin, this function will be called for all the events in UnityShaderCompilerExtEventType
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityShaderCompilerExtEvent(UnityShaderCompilerExtEventType event, void* data);

#ifdef __cplusplus
}
#endif
