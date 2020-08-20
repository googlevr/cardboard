// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#if !UNITY
#include "UnityXRTypes.h"
#include "UnityXRSubsystemTypes.h"
#else
#include "Modules/XR/ProviderInterface/UnityXRTypes.h"
#include "Modules/XR/ProviderInterface/UnityXRSubsystemTypes.h"
#endif

/// @file IUnityXRDisplay.h
/// @brief XR interface for texture allocation, lifecycle of a frame, blocking for cadence and frame submission.
/// @see IUnityXRDisplayInterface

/// Handle to a texture obtained from IUnityXRDisplayInterface::CreateTexture.
typedef uint32_t UnityXRRenderTextureId;

/// Id to an OcclusionMesh from IUnityXRDisplayInterface::CreateOcclusionMesh.
typedef uint32_t UnityXROcclusionMeshId;

/// Flags that can be set on an UnityXRRenderTextureDesc before creation to modify behavior.
typedef enum UnityXRRenderTextureFlags
{
    /// By default, Unity expects texture coordinates in OpenGL mapping with (0,0) in lower left hand corner.  This flag specifies that (0,0) is
    /// in the upper left hand corner for this texture.  Unity will flip the texture at the appropriate time.
    kUnityXRRenderTextureFlagsUVDirectionTopToBottom = 1 << 0,

    /// This texture will be auto resolved upon sampling.  Tile-based archictures can benefit with reduced bandwidth by utilizing auto resolved textures.
    kUnityXRRenderTextureFlagsAutoResolve = 1 << 1,

    /// Specifies that the resources backing this texture cannot be resized.  No control over width / height of texture.
    /// Unity may allocate a separate texture to render to that is a more convenient size and blit into this one.
    /// Ex. HoloLens backbuffer size cannot be changed.
    kUnityXRRenderTextureFlagsLockedWidthHeight = 1 << 2,

    /// Texture can only be written to and cannot be read from.  Unity will need to create intermediate textures in order to do post process work.
    kUnityXRRenderTextureFlagsWriteOnly = 1 << 3,

    /// Use sRGB texture formats if possible.
    kUnityXRRenderTextureFlagsSRGB = 1 << 4,
} UnityXRRenderTextureFlags;

/// @var kUnityXRRenderTextureIdDontCare
/// Unity will allocate the texture if needed.  #kUnityXRRenderTextureIdDontCare can be set on UnityXRRenderTextureDesc::nativeColorTexPtr or UnityXRRenderTextureDesc::nativeDepthTexPtr.
enum { kUnityXRRenderTextureIdDontCare = 0 };

/// Format for color texture.
typedef enum UnityXRRenderTextureFormat
{
    /// R8 G8 B8 A8
    kUnityXRRenderTextureFormatRGBA32,

    /// B8 G8 R8 A8
    kUnityXRRenderTextureFormatBGRA32,

    /// R5 G6 B5
    kUnityXRRenderTextureFormatRGB565,

    /// Don't create a color texture, instead create a reference to another color texture that's already been created.  Must fill out UnityTextureData::referenceTextureId.
    kUnityXRRenderTextureFormatReference,

    /// No color texture.
    kUnityXRRenderTextureFormatNone
} UnityXRRenderTextureFormat;

/// Precision of depth texture.
typedef enum UnityXRDepthTextureFormat
{
    /// 24-bit or greater depth texture.  Unity will prefer 32 bit floating point Z buffer if available on the platform.
    kUnityXRDepthTextureFormat24bitOrGreater,

    /// If possible, use a 16-bit texture format for bandwidth savings.
    kUnityXRDepthTextureFormat16bit,

    /// Don't create a depth texture, instead create a reference to another depth texture that's already been created.  Must fill out UnityTextureData::referenceTextureId.
    /// This is useful for sharing a single depth texture between double/triple buffered color textures (of the same width/height).
    kUnityXRDepthTextureFormatReference,

    /// No depth texture.
    kUnityXRDepthTextureFormatNone
} UnityXRDepthTextureFormat;

/// @brief const ids for reserved blit modes
///
/// Supporting reserved blit mode is optional. kUnityXRMirrorViewBlitNone is always supported.
/// Must be in sync with its c# declaration: XRMirrorViewBlitMode.
/// Negative numbers and 0 are being reserved. Users can start at 1 for the custom blit modes.
/// All supported blit modes besides kUnityXRMirrorViewBlitNone should be specified in display provider's manifest json file. For example:
/// "displays": [{
///    "id": "Display Sample",
///        "supportedMirrorBlitReservedModes" : ["leftEye","rightEye", "sideBySide", "occlusionMeshSideBySide", "distort"],
///        "supportedMirrorBlitCustomModes" : [{
///            "blitModeId" : 1,
///            "blitModeDesc" : "Custom Eye Blit 1"
///        },
///        {
///            "blitModeId" : 2,
///            "blitModeDesc" : "Custom Eye Blit 2"
///        }]
/// }],
/// No blit should be performed
const static int kUnityXRMirrorBlitNone = 0;
/// Left eye blit
const static int kUnityXRMirrorBlitLeftEye = -1;
/// Right eye blit
const static int kUnityXRMirrorBlitRightEye = -2;
/// Both eye blit
const static int kUnityXRMirrorBlitSideBySide = -3;
/// Both eye show occlusion blit
const static int kUnityXRMirrorBlitSideBySideOcclusionMesh = -4;
/// Distort blit
const static int kUnityXRMirrorBlitDistort = -5;

/// Container for different ways of representing texture data.
/// If the format (#UnityXRRenderTextureDesc::colorFormat or #UnityXRRenderTextureDesc::depthFormat) is a 'Reference' format, referenceTextureId must be set.
/// Otherwise, nativePtr will be used.
typedef union UnityXRTextureData
{
    /// @brief Native texture ID if you've allocated it yourself.  The texture ID varies by graphics API.
    ///
    ///  - GL: texture name that comes from glGenTextures
    ///  - DX11: ID3D11Texture2D*
    ///  - etc.
    ///
    /// You can pass in #kUnityXRRenderTextureIdDontCare and have Unity allocate one for you.
    void* nativePtr;

    /// Texture id to share color / depth with in the case of reference color / depth format.
    UnityXRRenderTextureId referenceTextureId;
} UnityXRTextureData; ///< Contains all supported ways of representing texture data.

/// Description of a texture that the plugin can request to be allocated via IUnityXRDisplayInterface::CreateTexture.  Encapsulates both color and depth surfaces.
typedef struct UnityXRRenderTextureDesc
{
    /// Color format of the texture.  Format will be sRGB if kUnityXRRenderTextureFlagsSRGB is set and there is an equivalent sRGB native format.
    UnityXRRenderTextureFormat colorFormat;

    /// Data for color texture.
    UnityXRTextureData color;

    /// Depth format of the texture.
    UnityXRDepthTextureFormat depthFormat;

    /// Data for depth texture.
    UnityXRTextureData depth;

    /// Width of the texture in pixels.
    uint32_t width;

    /// Height of the texture in pixels.
    uint32_t height;

    /// If requesting a texture array, the length of the texture array.
    uint32_t textureArrayLength;

    /// Combination of #UnityXRRenderTextureFlags.
    uint32_t flags;
} UnityXRRenderTextureDesc;

/// @var kUnityXRMaxNumRenderPasses
/// Constant for the max number of RenderPasses.
enum { kUnityXRMaxNumRenderPasses = 4 };

/// @var kUnityXRMaxNumUnityXRRenderParams
/// Constant for the max number of UnityXRRenderParams in a RenderPass.
enum { kUnityXRMaxNumUnityXRRenderParams = 2 };

/// @var kUnityXRMaxNumUnityXRBlitParams
/// Constant for the max number of kUnityXRMaxNumUnityXRBlitParams in a MirrorViewBlitDescriptor
enum { kUnityXRMaxNumUnityXRBlitParams = 6 };


/// @brief Structure that the plugin fills out during UnityXRDisplayGraphicsThreadProvider::PopulateNextFrameDesc.
///
/// All information in this struct must describe the frame that is about to be rendered.
typedef struct UnityXRNextFrameDesc
{
    /// @brief An UnityXRRenderPass potentially involves a culling pass and a scene graph traversal.
    ///
    /// It's an expensive operation and we should try to limit the number of them via tricks like single-pass rendering.
    ///
    /// Each UnityXRRenderPass contains an output texture (which may be a texture array),
    /// and output UnityXRRenderParams such as the rect to render to or the texture array slice.
    ///
    /// Here are some use cases for XRRenderPass:
    ///  - Two pass stereo rendering (2 RenderPass x 1 RenderParams)
    ///  - Single pass stereo rendering (1 RenderPass x 2 RenderParams)
    /// and we'd like to support these:
    ///  - Quad pass wide FOV stereo rendering (4 RenderPass x 1 RenderParams)
    ///  - Single pass + wide FOV stereo rendering (1 RenderPass x 2 RenderParams + 2 RenderPass x 1 RenderParams)
    ///  - Foveated rendering
    ///    - (two separate textures for each eye [inner, outer])
    ///    - (one texture, UV fitting)
    ///  - External view scenario (HoloLens BEV, Mayo 3rd eye) (extra RenderPass)
    ///  - Near / Far rendering for compositing objects / people in scene (2 RenderPass, different projections, different targets)
    ///
    /// We can make these assumptions:
    ///  - one texture per "pass"
    ///    - (singlepass will be one texture-array)
    ///  - If single-pass, two pose / projections per pass
    ///    - (or foveated rendering)
    struct UnityXRRenderPass
    {
        /// A Unity texture ID returned by CreateTexture.
        UnityXRRenderTextureId textureId;

        /// Index into cullingPasses array, pointing at the culling data for this render pass.
        /// You can combine culling passes by pointing to the same culling data.
        uint32_t cullingPassIndex;

        /// Parameters for rendering. In the case of single-pass there may be multiple sets of rendering params.
        struct UnityXRRenderParams
        {
            /// Eye pose in device anchor space.  This is usually a static transform that may change for IPD or foveated rendering.
            /// Head tracking is applied through the input subsystem by filling out kUnityXRInputFeatureUsageCenterEyePosition.
            /// Pose is in Unity coordinates (forward z is positive).
            UnityXRPose deviceAnchorToEyePose;

            /// Projection to render with.
            UnityXRProjection projection;

            /// Id for the OcclusionMesh to render with.
            UnityXROcclusionMeshId occlusionMeshId;

            // PLANNED: gameWindowViewportRect

            /// Slice of the texture array to render to.  Only valid if single-pass rendering mode is active.
            int32_t textureArraySlice;

            /// Normalized viewport from zero (bottom/left) to one (top/right).
            UnityXRRectf viewportRect;
        } renderParams[kUnityXRMaxNumUnityXRRenderParams]; ///< Parameters for rendering.

        /// Number of entries in #UnityXRNextFrameDesc::UnityXRRenderPass::UnityXRRenderParams.
        int32_t renderParamsCount;
    } renderPasses[kUnityXRMaxNumRenderPasses]; ///< Render Passes that unity should execute next frame.

    /// Data for a culling pass.
    struct UnityXRCullingPass
    {
        /// Culling pose in device anchor space.  This is usually a static transform that may change for IPD or foveated rendering.
        /// Head tracking is applied through the input subsystem by filling out kUnityXRInputFeatureUsageCenterEyePosition.
        /// Pose is in Unity coordinates (forward z is positive).
        UnityXRPose deviceAnchorToCullingPose;

        /// Viewing frustum.
        UnityXRProjection projection;

        /// Diameter of a sphere of visibility.  Anything in the frustum and visible to this sphere won't be culled.
        /// Useful if combining culling passes.
        float separation;
    } cullingPasses[kUnityXRMaxNumRenderPasses]; ///< Collection of culling pass data for render passes.  Each render pass will call out which culling pass to use via index.

    /// Number of entries in #renderPasses.
    int32_t renderPassesCount;

    /// Preferred mirror blit mode to render with.
    int mirrorBlitMode;
} UnityXRNextFrameDesc;

/// @brief Structure that the plugin fills out during UnityXRDisplayProvider::QueryMirrorViewBlitDesc.
///
/// All information in this struct describes the mirror view blit operation.
///
typedef struct UnityXRMirrorViewBlitDesc
{
    /// Is the provider configured to support the native mirror view blit?
    /// Marking this value as true will suggest the engine to use the registered native blit callback: UnityXRDisplayGraphicsThreadProvider::BlitToMirrorViewRenderTarget
    /// The rendering pipeline might not call the native blit function even when it is available.
    bool nativeBlitAvailable;

    /// If your plugin changes any graphics state in UnityXRDisplayGraphicsThreadProvider::BlitToMirrorViewRenderTarget, set this bool to true.
    bool nativeBlitInvalidStates;

    /// Number of entries in #blitParams
    int32_t blitParamsCount;

    /// Data for the desired blit operations
    struct UnityXRBlitParams
    {
        /// Source texture Id. This Id represents the source texture resource that the blit operation will blit from.
        UnityXRRenderTextureId srcTexId;

        /// Source texture's array slice.
        int32_t srcTexArraySlice;

        /// Source texture rect area. Should be the area your plugin wants to blit from
        UnityXRRectf srcRect;

        /// Target mirror view rect area. Should be the area your plugin wants to blit to
        UnityXRRectf destRect;
    } blitParams[kUnityXRMaxNumUnityXRBlitParams]; ///< Parameters for blit operations.
} UnityXRMirrorViewBlitDesc;

/// Reprojection mode
typedef enum UnityXRReprojectionMode
{
    /// The provider has not specified the type of reprojection mode it is using.
    kUnityXRReprojectionModeUnspecified,

    /// Reprojection will be based on both the users head position and orientation.
    kUnityXRReprojectionModePositionAndOrientation,

    /// Reprojection will be based only on the users head orientation.
    kUnityXRReprojectionModeOrientationOnly,

    /// No stabilization based on user head information will be used.
    kUnityXRReprojectionModeNone,
} UnityXRReprojectionMode;

/// Flags corresponding to fields in #UnityXRFrameSetupHints.
typedef enum UnityXRFrameSetupHintsChanged
{
    /// The engine has either turned singlePassRendering on or off.  Check the #UnityXRFrameSetupHints::UnityXRAppSetup `singlePassRendering` field.
    kUnityXRFrameSetupHintsChangedSinglePassRendering = 1 << 0,

    /// The engine has requested rendering to a different viewport.  Check the #UnityXRFrameSetupHints::UnityXRAppSetup `renderViewport` field.
    kUnityXRFrameSetupHintsChangedRenderViewport = 1 << 1,

    /// The engine has requested that the provider reallocates its textures at a different resolution.  Check the #UnityXRFrameSetupHints::UnityXRAppSetup `textureResolutionScale` field.
    kUnityXRFrameSetupHintsChangedTextureResolutionScale = 1 << 2,

    /// The engine has changed the content protection state.  Check the #UnityXRFrameSetupHints::UnityXRAppSetup `contentProtectionEnabled` field.
    kUnityXRFrameSetuphintsChangedContentProtectionState = 1 << 4,

    /// The engine has changed the reprojection mode state.  Check the #UnityXRFrameSetupHints::UnityXRAppSetup `reprojectionMode` field.
    kUnityXRFrameSetuphintsChangedReprojectionMode = 1 << 5,

    /// The engine has changed the focus plane state.  Check the #UnityXRFrameSetupHints::UnityXRAppSetup `UnityXRFocusPlane` field.
    kUnityXRFrameSetuphintsChangedFocusPlane = 1 << 6,

    /// No changes in #UnityXRFrameSetupHints since last frame.
    kUnityXRFrameSetupHintsChangedNone = 0
} UnityXRFrameSetupHintsChanged;

/// The focus plane struct used to pass data to the provider.
typedef struct UnityXRFocusPlane
{
    /// The location in world space that should be set as the main point of focus.
    UnityXRVector3 point;

    /// The normal of the plane the user should focus on. Normally should be
    /// set to the vector from the point back to the users head. If set to (0,0,0)
    /// then setting the normal is up to the provider.
    UnityXRVector3 normal;

    /// The velocity of the plane the user should focus on.
    UnityXRVector3 velocity;
} UnityXRFocusPlane;

/// Info about how the unity developer configured the frame to be rendered.
typedef struct UnityXRFrameSetupHints
{
    /// Options that are configured directly by the application.  These may change on a per-frame basis.
    struct UnityXRAppSetup
    {
        /// Is the scene + user's shaders configured to use single-pass rendering?
        bool singlePassRendering;

        /// The rect which unity rendered the last frame to.
        UnityXRRectf renderViewport;

        /// Distance of near plane in meters from main camera.
        float zNear;

        /// Distance of far plane in meters from main camera.
        float zFar;

        /// App requests to render to sRGB textures.
        bool sRGB;

        /// App requests to render with 16 bit color buffers if possible (ex: RGB565)
        bool use16BitColorBuffers;

        /// App requests texture sizes to be scaled.  If this field changes, the provider should reallocate relevant textures with this scale.
        float textureResolutionScale;

        /// App is requesting that the provider enabled or disabled content protection.
        bool contentProtectionEnabled;

        /// Tells the provider the reprojection mode it should be using for stabilization.
        UnityXRReprojectionMode reprojectionMode;

        /// Tells the provider the focus plane to use. If not set on every frame, the
        /// provider may switch back to a default focus plane.
        UnityXRFocusPlane focusPlane;
    } appSetup; ///< Options that are configured directly by the application.

    /// Bitfield representing which, if any, of the hint fields changed last frame.  Combination of #UnityXRFrameSetupHintsChanged.
    uint64_t changedFlags;
} UnityXRFrameSetupHints;

/// Info about graphics capabilities
typedef struct UnityXRRenderingCapabilities
{
    /// If the hardware or SDK we're running on cannot support single-pass rendering, set this bit.
    bool noSinglePassRenderingSupport;

    /// Invalidate graphics state after each graphics thread callback.  If your plugin changes any graphics state, set this bit.
    bool invalidateRenderStateAfterEachCallback;

    /// Don't present to the main screen (eglSwapBuffers, etc).
    bool skipPresentToMainScreen;
} UnityXRRenderingCapabilities;

/// Structure that describes the mirror view's render target's specification.
typedef struct UnityXRMirrorViewRenderTargetDescriptor
{
    /// Mirror view render target's width when it is first created.
    uint16_t             rtOriginalWidth;

    /// Mirror view render target's height when it is first created.
    uint16_t             rtOriginalHeight;

    /// Mirror view render target's current scaled width. ScaledWidth should be used for the current render target width.
    uint16_t             rtScaledWidth;

    /// Mirror view render target's current scaled height. ScaledHeight should be used for the current render target height.
    uint16_t             rtScaledHeight;

    /// Mirror view render target's available array size or depth.
    uint16_t             rtDepthOrArraySize;

    /// Mirror view render view render target's sample
    uint8_t              rtSamples;

    /// Mirror view render target's mips
    uint8_t              rtMipCount;

    /// Gfx thread native texture, will be NULL on game thread
    UnityRenderBuffer    rtNative;
} UnityXRMirrorViewRenderTargetDescriptor;

/// Infos that are used for performing mirror view blit
typedef struct UnityXRMirrorViewBlitInfo
{
    /// Mirror view render target descriptor that describes mirror view's render target information
    UnityXRMirrorViewRenderTargetDescriptor *mirrorRtDesc;

    /// Selected mirrorBlit Mode that provider should perform
    int mirrorBlitMode;
} UnityXRMirrorViewBlitInfo;

/// @brief Event handler implemented by a plugin for display subsystem graphics-thread specific lifecycle events.
///
/// All event callbacks will be executed on the graphics thread.
typedef struct SUBSYSTEM_PROVIDER UnityXRDisplayGraphicsThreadProvider
{
    /// Plugin-specific data pointer.  Every function in this structure will
    /// be passed this data by the Unity runtime.  The plugin should allocate
    /// an appropriate data container for any information that is needed when
    /// a callback function is called.
    void* userData;

    /// Called on the graphics thread when display subsystem is started and before PopulateNextFrameDesc.  Do any one time setup here.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified userData that was filled out on #UnityXRDisplayGraphicsThreadProvider
    /// @param[out] renderingCaps Modify if your SDK or the hardware we're running on needs or supports specific rendering features.  Ex. single-pass rendering.
    ///
    /// @return kUnitySubsystemErrorCodeSuccess Successfully started.  Continue startup.
    /// @return kUnitySubsystemErrorCodeFailure Startup failed.  Provider will be shutdown.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * Start)(UnitySubsystemHandle handle, void* userData, UnityXRRenderingCapabilities * renderingCaps);

    /// Submit the previous frame to the compositor (eye textures, etc).
    /// This call is scheduled just before the native graphics API call to submit / swap buffers for the main window (if applicable).
    /// This call may optionally block for cadence if #PopulateNextFrameDesc doesn't block.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified userData that was filled out on #UnityXRDisplayGraphicsThreadProvider
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SubmitCurrentFrame)(UnitySubsystemHandle handle, void* userData);

    /// PopulateNextFrameDesc will be called after simulation and pending graphics calls have completed
    /// PopulateNextFrameDesc is responsible for:
    ///  - waiting for cadence (waiting until unity should start submitting rendering commands again) (if #SubmitCurrentFrame didn't block)
    ///  - acquiring the next image and telling Unity what texture id to render to next via `nextFrame`.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified userData that was filled out on #UnityXRDisplayGraphicsThreadProvider
    /// @param[in] frameHints Information about the frame that was just rendered.
    /// @param[out] nextFrame Information that must be filled out about the next frame to be rendered.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * PopulateNextFrameDesc)(UnitySubsystemHandle handle, void* userData, const UnityXRFrameSetupHints * frameHints, UnityXRNextFrameDesc * nextFrame);

    /// Called on the graphics thread when the display subsystem is stopped.  PopulateNextFrameDesc won't be called again until after Start.  Do tear down here.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified userData that was filled out on #UnityXRDisplayGraphicsThreadProvider
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * Stop)(UnitySubsystemHandle handle, void* userData);

    /// Called on the graphics thread when the display provider is responsible for the blit. Do native API blit here.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified userData that wass filled out on #UnityXRDisplayGraphicsThreadProvider
    /// @param[in] mirrorViewRenderTargetDesc A render target descriptor that describs the target render surface.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * BlitToMirrorViewRenderTarget)(UnitySubsystemHandle handle, void* userData, const UnityXRMirrorViewBlitInfo mirrorBlitInfo);
} UnityXRDisplayGraphicsThreadProvider;

/// State of the display subsystem.
typedef struct UnityXRDisplayState
{
    /// Set this value to true when the display is not active or is not rendering the application content.
    /// Return false when the display becomes active and it is rendering content.
    bool focusLost;

    /// Set this to true if the display is transparent (i.e. AR device display).
    bool displayIsTransparent;

    /// Set this to report to the application if content protection is currently enabled.
    bool contentProtectionEnabled;

    /// The current mode being used by the provider to stabilize rendering.
    UnityXRReprojectionMode reprojectionMode;

    /// Optional legacy native pointer which will be exposed via XRDevice.GetNativePtr() c# api.
    /// Note that this is only to be used in legacy cases, it's better to pinvoke into your dll from c#
    /// to do implementation specific things.
    void* nativePtr;

    // PLANNED: UserPresence
} UnityXRDisplayState;

/// @brief Event handler implemented by a plugin for display subsystem main-thread specific events.
///
/// All event callbacks will be executed on the main thread.
typedef struct SUBSYSTEM_PROVIDER UnityXRDisplayProvider
{
    /// Pointer which will be passed to every callback as the userData parameter.
    void* userData;

    /// Called at the beginning of a frame to request the state of the display provider.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[out] state State of the display
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * UpdateDisplayState)(UnitySubsystemHandle handle, void* userData, UnityXRDisplayState * state);

    /// Called by the engine to request the descriptor for mirror view blit operation.
    /// If provider doesn't implement this callback, default blit behavior will be used.
    /// If returns 0 blit params, no blit will happen.
    /// If returns an error code, no blit will happen.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data
    /// @param[in] mirrorViewRenderTargetDesc A render target descriptor that describs the target render surface.
    /// @param[out] blitDescriptor Information that describes mirror view blit operation.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully retrieved UnityXRMirrorViewBlitDesc
    /// @return kUnitySubsystemErrorCodeFailure Error
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * QueryMirrorViewBlitDesc)(UnitySubsystemHandle handle, void* userData, const UnityXRMirrorViewBlitInfo mirrorBlitInfo, UnityXRMirrorViewBlitDesc * blitDescriptor);
    // PLANNED: OnPause
} UnityXRDisplayProvider;

/// @brief XR interface for texture allocation, lifecycle of a frame, blocking for cadence and frame submission.
UNITY_DECLARE_INTERFACE(SUBSYSTEM_INTERFACE IUnityXRDisplayInterface)
{
    /// Entry-point for getting callbacks when the display subsystem is initialized / started / stopped / shutdown.
    ///
    /// Example usage:
    /// @code
    /// #include "IUnityXRDisplay.h"
    ///
    /// static IUnityXRDisplayInterface* s_XrDisplay = NULL;
    ///
    /// extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    /// UnityPluginLoad(IUnityInterfaces* unityInterfaces)
    /// {
    ///     s_XrDisplay = unityInterfaces->Get<IUnityXRDisplayInterface>();
    ///     UnityLifecycleProvider displayLifecycleHandler =
    ///     {
    ///         NULL,  // This can be any object you want to be passed as userData to the following functions
    ///         &Lifecycle_Initialize,
    ///         &Lifecycle_Start,
    ///         &Lifecycle_Stop,
    ///         &Lifecycle_Shutdown
    ///     };
    ///     s_XrDisplay->RegisterLifecycleProvider("pluginName", "pc display", &displayLifecycleHandler);
    /// }
    /// @endcode
    ///
    /// @param[in] pluginName Name of the plugin which was registered in UnitySubsystemsManifest.json
    /// @param[in] id Id of the subsystem that was registered in UnitySubsystemsManifest.json
    /// @param[in] provider Lifecycle callbacks to register.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully registered
    /// @return kUnitySubsystemErrorCodeFailure No UnitySubsystemsManifest.json with matching pluginName and id
    /// @return kUnitySubsystemErrorCodeInvalidArguments pluginName or id too long / not valid.
    /// @return kUnitySubsystemErrorCodeInvalidArguments Required fields on provider not filled out
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * RegisterLifecycleProvider)(const char* pluginName, const char* id, const UnityLifecycleProvider * provider);

    /// Register for main thread callbacks.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] provider Callbacks to register.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully initialized
    /// @return kUnitySubsystemErrorCodeFailure Error
    /// @return kUnitySubsystemErrorCodeInvalidArguments Error
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SUBSYSTEM_REGISTER_PROVIDER(UnityXRDisplayProvider) RegisterProvider)(UnitySubsystemHandle handle, const UnityXRDisplayProvider * provider);

    /// Register for graphics thread callbacks.  All callbacks will be executed on the graphics thread and only APIs that are marked as thread-safe can be called.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] provider Callbacks to register.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully initialized
    /// @return kUnitySubsystemErrorCodeFailure Error
    /// @return kUnitySubsystemErrorCodeInvalidArguments Error
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SUBSYSTEM_REGISTER_PROVIDER(UnityXRDisplayGraphicsThreadProvider) RegisterProviderForGraphicsThread)(UnitySubsystemHandle handle, const UnityXRDisplayGraphicsThreadProvider * provider);

    /// Create an UnityXRRenderTextureId given an UnityXRRenderTextureDesc.  Safe to be called on main thread and gfx thread.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] desc Descriptor of the texture to be created.
    /// @param[out] outTexId Texture ID representing a unique instance of a texture.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully initialized
    /// @return kUnitySubsystemErrorCodeFailure Error
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * CreateTexture)(UnitySubsystemHandle handle, const UnityXRRenderTextureDesc * desc, UnityXRRenderTextureId * outTexId);

    /// Converts an UnityXRRenderTextureId to an UnityXRRenderTextureDesc.  Useful in order to obtain the platform specific texture information.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] texId Texture ID to query.
    /// @param[out] outDesc UnityXRRenderTextureDesc to be filled out.
    /// @return kUnitySubsystemErrorCodeSuccess Texture was successfully queried
    /// @return kUnitySubsystemErrorCodeFailure Texture was not found
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * QueryTextureDesc)(UnitySubsystemHandle handle, UnityXRRenderTextureId texId, UnityXRRenderTextureDesc * outDesc);

    /// Destroy an UnityXRRenderTextureId.  Safe to be called on main thread and gfx thread.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] texId Texture ID to destroy.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully initialized
    /// @return kUnitySubsystemErrorCodeFailure Error
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DestroyTexture)(UnitySubsystemHandle handle, UnityXRRenderTextureId texId);

    /// TEMPORARY: Returns native pointer to some sdk specific resources of the active legacy VR SDK.  The format of the data changes depending on the active legacy SDK.
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully initialized
    /// @return kUnitySubsystemErrorCodeFailure Error
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * GetPlatformData)(UnitySubsystemHandle handle, void** platformData);

    /// Create a UnityXROcclusionMeshHandle with the specified number of vertices and indices available. Meshes are automatically cleaned up by the Display subsystem on shutdown.
    /// Safe to be called during PopulateNextFrameDesc.
    ///
    /// @param[in] subsystemHandle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] numVertices The number of vertices the Occlusion mesh contains. Cannot be more than UINT16_MAX.
    /// @param[in] numIndices The number of indices in the Occlusion mesh. Cannot be more than UINT16_MAX
    /// @param[out] outOcclusionMeshHandle The handle to the Occlusion Mesh created. Used for setting the OcclusionMesh or modifying it between frames.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully created
    /// @return kUnitySubsystemErrorCodeFailure Out of memory
    /// @return kUnitySubsystemErrorCodeInvalidArguments numVertices or numIndices is 0
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * CreateOcclusionMesh)(UnitySubsystemHandle subsystemHandle, uint32_t numVertices, uint32_t numIndices, UnityXROcclusionMeshId* outOcclusionMeshId);

    /// Destroy the OcclusionMesh with a given Id that was created using CreateOcclusionMesh. Safe to be called during PopulateNextFrameDesc.
    ///
    /// @param[in] subsystemHandle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] occlusionMeshHandle The handle to destroy.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully destroyed
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DestroyOcclusionMesh)(UnitySubsystemHandle subsystemHandle, UnityXROcclusionMeshId occlusionMeshId);

    /// Set the UnityXROclusionMeshHandle's vertices and indices. Vertices and indices can change between frames so long as the total vertex and index count remains the same.
    ///
    /// @param[in] subsystemHandle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] occlusionMeshHandle The handle to modify.
    /// @param[in] vertices The list of vertices for the OcclusionMesh.
    /// @param[in] numVertices The number of vertices the OcclusionMesh contains. This value can be less than the number used to create the OcclusionMesh.
    /// @param[in] indices The list of indices for the OcclusionMesh.
    /// @param[in] numIndices The number of indices the OcclusionMesh contains. This value can be less than the number used to create the OcclusionMesh.
    /// @return kUnitySubsystemErrorCodeSuccess Successfully set
    /// @return kUnitySubsystemErrorCodeInvalidArguments vertices or indices is null, numVertices or numIndices is > size used to Create or <= 0
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SetOcclusionMesh)(UnitySubsystemHandle subsystemHandle, UnityXROcclusionMeshId occlusionMeshId, UnityXRVector2* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices);
};

UNITY_REGISTER_INTERFACE_GUID(0x940e64d2e52243ecULL, 0xa348f3026b1b1193ULL, IUnityXRDisplayInterface)
