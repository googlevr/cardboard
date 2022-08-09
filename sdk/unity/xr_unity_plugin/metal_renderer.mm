/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>

#include <array>
#include <memory>
#include <vector>

#include "include/cardboard.h"
#include "util/is_arg_null.h"
#include "util/logging.h"
#include "unity/xr_unity_plugin/renderer.h"
#include "IUnityGraphicsMetal.h"

namespace cardboard::unity {
namespace {

/// @note This enum must be kept in sync with the shader counterpart.
typedef enum VertexInputIndex {
  VertexInputIndexPosition = 0,
  VertexInputIndexTexCoords,
} VertexInputIndex;

/// @note This enum must be kept in sync with the shader counterpart.
typedef enum FragmentInputIndex {
  FragmentInputIndexTexture = 0,
} FragmentInputIndex;

// TODO(b/178125083): Revisit Metal shader approach.
/// @brief Vertex and fragment shaders for rendering the widgets when using Metal.
constexpr const char* kMetalShaders =
    R"msl(#include <metal_stdlib>
    #include <simd/simd.h>

    using namespace metal;

    typedef enum VertexInputIndex {
      VertexInputIndexPosition = 0,
      VertexInputIndexTexCoords,
    } VertexInputIndex;

    typedef enum FragmentInputIndex {
      FragmentInputIndexTexture = 0,
    } FragmentInputIndex;

    struct VertexOut {
      float4 position [[position]];
      float2 tex_coords;
    };

    vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                                  constant vector_float2 *position [[buffer(VertexInputIndexPosition)]],
                                  constant vector_float2 *tex_coords [[buffer(VertexInputIndexTexCoords)]]) {
      VertexOut out;
      out.position = vector_float4(position[vertexID], 0.0, 1.0);
      out.tex_coords = tex_coords[vertexID];
      return out;
    }

    fragment float4 fragmentShader(VertexOut in [[stage_in]],
                                   texture2d<half> colorTexture [[texture(0)]]) {
      constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
      return float4(colorTexture.sample(textureSampler, in.tex_coords));
    })msl";

// These classes need to be loaded on runtime, otherwise a Unity project using a rendering API
// different than Metal won't be able to be built due to linking errors.
static Class MTLRenderPipelineDescriptorClass;
static Class MTLTextureDescriptorClass;

class MetalRenderer : public Renderer {
 public:
  explicit MetalRenderer(IUnityInterfaces* xr_interfaces) {
    metal_interface_ = xr_interfaces->Get<IUnityGraphicsMetalV1>();
    if (CARDBOARD_IS_ARG_NULL(metal_interface_)) {
      return;
    }

    MTLRenderPipelineDescriptorClass = NSClassFromString(@"MTLRenderPipelineDescriptor");
    MTLTextureDescriptorClass = NSClassFromString(@"MTLTextureDescriptor");
  }

  ~MetalRenderer() { TeardownWidgets(); }

  void SetupWidgets() override {
    id<MTLDevice> mtl_device = metal_interface_->MetalDevice();

    // Compile metal library.
    id<MTLLibrary> mtl_library =
        [mtl_device newLibraryWithSource:[NSString stringWithUTF8String:kMetalShaders]
                                 options:nil
                                   error:nil];
    if (mtl_library == nil) {
      CARDBOARD_LOGE("Failed to compile Metal library.");
      return;
    }

    id<MTLFunction> vertex_function = [mtl_library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragment_function = [mtl_library newFunctionWithName:@"fragmentShader"];

    // Create pipeline.
    MTLRenderPipelineDescriptor* mtl_render_pipeline_descriptor =
        [[MTLRenderPipelineDescriptorClass alloc] init];
    mtl_render_pipeline_descriptor.vertexFunction = vertex_function;
    mtl_render_pipeline_descriptor.fragmentFunction = fragment_function;
    mtl_render_pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    mtl_render_pipeline_descriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    mtl_render_pipeline_descriptor.stencilAttachmentPixelFormat =
        MTLPixelFormatDepth32Float_Stencil8;
    mtl_render_pipeline_descriptor.sampleCount = 1;
    mtl_render_pipeline_descriptor.colorAttachments[0].blendingEnabled = YES;
    mtl_render_pipeline_descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    mtl_render_pipeline_descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    mtl_render_pipeline_descriptor.colorAttachments[0].sourceRGBBlendFactor =
        MTLBlendFactorSourceAlpha;
    mtl_render_pipeline_descriptor.colorAttachments[0].sourceAlphaBlendFactor =
        MTLBlendFactorSourceAlpha;
    mtl_render_pipeline_descriptor.colorAttachments[0].destinationRGBBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    mtl_render_pipeline_descriptor.colorAttachments[0].destinationAlphaBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;

    mtl_render_pipeline_state_ =
        [mtl_device newRenderPipelineStateWithDescriptor:mtl_render_pipeline_descriptor error:nil];
    if (mtl_render_pipeline_state_ == nil) {
      CARDBOARD_LOGE("Failed to create Metal render pipeline.");
      return;
    }

    are_widgets_setup_ = true;
  }

  void RenderWidgets(const ScreenParams& screen_params,
                     const std::vector<WidgetParams>& widget_params) override {
    if (!are_widgets_setup_) {
      CARDBOARD_LOGF(
          "RenderWidgets called before setting them up. Please call SetupWidgets first.");
      return;
    }

    id<MTLRenderCommandEncoder> mtl_render_command_encoder =
        (id<MTLRenderCommandEncoder>)metal_interface_->CurrentCommandEncoder();

    [mtl_render_command_encoder setRenderPipelineState:mtl_render_pipeline_state_];

    // Translate y coordinate of the rectangle since in Metal the (0,0) coordinate is
    // located on the top-left corner instead of the bottom-left corner.
    int mtl_viewport_y =
        screen_params.height - screen_params.viewport_height - screen_params.viewport_y;

    [mtl_render_command_encoder
        setViewport:(MTLViewport){(double)screen_params.viewport_x, (double)mtl_viewport_y,
                                  (double)screen_params.viewport_width,
                                  (double)screen_params.viewport_height, 0.0, 1.0}];

    for (const WidgetParams& widget_param : widget_params) {
      RenderWidget(mtl_render_command_encoder, screen_params.viewport_width,
                   screen_params.viewport_height, widget_param);
    }
  }

  void RunRenderingPreProcessing(
      const ScreenParams& /* screen_params */) override {
    // Nothing to do.
  }

  void RunRenderingPostProcessing() override {
    // Nothing to do.
  }

  void TeardownWidgets() override {
    CARDBOARD_LOGD("TeardownWidgets is a no-op method when using Metal.");
  }

  void CreateRenderTexture(RenderTexture* render_texture, int screen_width,
                           int screen_height) override {
    id<MTLDevice> mtl_device = metal_interface_->MetalDevice();

    // Create texture color buffer.
    NSDictionary* color_surface_attribs = @{
      (NSString*)kIOSurfaceWidth : @(screen_width / 2),
      (NSString*)kIOSurfaceHeight : @(screen_height),
      (NSString*)kIOSurfaceBytesPerElement : @4u
    };
    IOSurfaceRef color_surface = IOSurfaceCreate((CFDictionaryRef)color_surface_attribs);

    MTLTextureDescriptor* texture_color_buffer_descriptor = [MTLTextureDescriptorClass new];
    texture_color_buffer_descriptor.textureType = MTLTextureType2D;
    texture_color_buffer_descriptor.width = screen_width / 2;
    texture_color_buffer_descriptor.height = screen_height;
    texture_color_buffer_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    texture_color_buffer_descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    id<MTLTexture> color_texture =
        [mtl_device newTextureWithDescriptor:texture_color_buffer_descriptor
                                   iosurface:color_surface
                                       plane:0];

    // Unity requires an IOSurfaceRef in order to draw the scene.
    render_texture->color_buffer = reinterpret_cast<uint64_t>(color_surface);
    // When using Metal, texture depth buffer is unused.
    render_texture->depth_buffer = 0;

    // Store created buffer elements.
    color_buffer_[eye_] = {color_surface, color_texture};

    // Switch eye for the next CreateRenderTexture call.
    eye_ = eye_ == kLeft ? kRight : kLeft;

    // Create a black texture. It is used to hide a rendering previously performed by Unity.
    // TODO(b/185478026): Prevent Unity from drawing a monocular scene when using Metal.
    MTLTextureDescriptor* black_texture_descriptor = [MTLTextureDescriptorClass new];
    black_texture_descriptor.textureType = MTLTextureType2D;
    black_texture_descriptor.width = screen_width;
    black_texture_descriptor.height = screen_height;
    black_texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
    black_texture_descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    black_texture_ = [mtl_device newTextureWithDescriptor:black_texture_descriptor];

    std::vector<uint32_t> black_texture_data(screen_width * screen_height, 0xFF000000);
    MTLRegion region = MTLRegionMake2D(0, 0, screen_width, screen_height);
    [black_texture_ replaceRegion:region
                      mipmapLevel:0
                        withBytes:reinterpret_cast<uint8_t*>(black_texture_data.data())
                      bytesPerRow:4 * screen_width];

    black_texture_vertices_buffer_ = [mtl_device newBufferWithBytes:vertices
                                                             length:sizeof(vertices)
                                                            options:MTLResourceStorageModeShared];
    black_texture_uvs_buffer_ = [mtl_device newBufferWithBytes:uvs
                                                        length:sizeof(uvs)
                                                       options:MTLResourceStorageModeShared];
  }

  void DestroyRenderTexture(RenderTexture* render_texture) override {
    render_texture->color_buffer = 0;
    render_texture->depth_buffer = 0;
  }

  void RenderEyesToDisplay(CardboardDistortionRenderer* renderer, const ScreenParams& screen_params,
                           const CardboardEyeTextureDescription* left_eye,
                           const CardboardEyeTextureDescription* right_eye) override {
    // Render black texture. It is used to hide a rendering previously performed by Unity.
    // TODO(b/185478026): Prevent Unity from drawing a monocular scene when using Metal.
    RenderBlackTexture(screen_params.width, screen_params.height);

    const CardboardMetalDistortionRendererTargetConfig target_config{
        reinterpret_cast<uint64_t>(CFBridgingRetain(metal_interface_->CurrentCommandEncoder())),
        screen_params.width, screen_params.height};

    // An IOSurfaceRef was passed to Unity for drawing, but a reference to an id<MTLTexture> using
    // it must be passed to the SDK.
    CardboardEyeTextureDescription left_eye_description = *left_eye;
    CFTypeRef left_color_texture = CFBridgingRetain(color_buffer_[kLeft].texture);
    left_eye_description.texture = reinterpret_cast<uint64_t>(left_color_texture);
    CardboardEyeTextureDescription right_eye_description = *right_eye;
    CFTypeRef right_color_texture = CFBridgingRetain(color_buffer_[kRight].texture);
    right_eye_description.texture = reinterpret_cast<uint64_t>(right_color_texture);

    CardboardDistortionRenderer_renderEyeToDisplay(
        renderer, reinterpret_cast<uint64_t>(&target_config), screen_params.viewport_x,
        screen_params.viewport_y, screen_params.viewport_width, screen_params.viewport_height,
        &left_eye_description, &right_eye_description);

    CFBridgingRelease(left_color_texture);
    CFBridgingRelease(right_color_texture);
    CFBridgingRelease(reinterpret_cast<CFTypeRef>(target_config.render_command_encoder));
  }

 private:
  static constexpr float Lerp(float start, float end, float val) {
    return start + (end - start) * val;
  }

  void RenderWidget(id<MTLRenderCommandEncoder> mtl_render_command_encoder, int screen_width,
                    int screen_height, const WidgetParams& params) {
    // Convert coordinates to normalized space (-1,-1 - +1,+1).
    const float x = Lerp(-1, +1, static_cast<float>(params.x) / screen_width);
    const float y = Lerp(-1, +1, static_cast<float>(params.y) / screen_height);
    const float width = params.width * 2.0f / screen_width;
    const float height = params.height * 2.0f / screen_height;

    const float vertices[] = {x, y, x + width, y, x, y + height, x + width, y + height};
    [mtl_render_command_encoder setVertexBytes:vertices
                                        length:sizeof(vertices)
                                       atIndex:VertexInputIndexPosition];

    [mtl_render_command_encoder setVertexBytes:uvs
                                        length:sizeof(uvs)
                                       atIndex:VertexInputIndexTexCoords];

    [mtl_render_command_encoder
        setFragmentTexture:(__bridge id<MTLTexture>)reinterpret_cast<CFTypeRef>(params.texture)
                   atIndex:FragmentInputIndexTexture];

    [mtl_render_command_encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                                   vertexStart:0
                                   vertexCount:4];
  }

  void RenderBlackTexture(int screen_width, int screen_height) {
    // Get Metal current render command encoder.
    id<MTLRenderCommandEncoder> mtl_render_command_encoder_ =
        static_cast<id<MTLRenderCommandEncoder>>(metal_interface_->CurrentCommandEncoder());

    [mtl_render_command_encoder_ setRenderPipelineState:mtl_render_pipeline_state_];

    [mtl_render_command_encoder_
        setViewport:(MTLViewport){0.0, 0.0, static_cast<double>(screen_width),
                                  static_cast<double>(screen_height), 0.0, 1.0}];

    [mtl_render_command_encoder_ setVertexBuffer:black_texture_vertices_buffer_
                                          offset:0
                                         atIndex:VertexInputIndexPosition];

    [mtl_render_command_encoder_ setVertexBuffer:black_texture_uvs_buffer_
                                          offset:0
                                         atIndex:VertexInputIndexTexCoords];

    [mtl_render_command_encoder_ setFragmentTexture:black_texture_
                                            atIndex:FragmentInputIndexTexture];

    [mtl_render_command_encoder_ drawPrimitives:MTLPrimitiveTypeTriangleStrip
                                    vertexStart:0
                                    vertexCount:4];
  }

  constexpr static float vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};
  constexpr static float uvs[] = {0, 0, 1, 0, 0, 1, 1, 1};

  IUnityGraphicsMetalV1* metal_interface_{nullptr};
  id<MTLRenderPipelineState> mtl_render_pipeline_state_;

  struct ColorBuffer {
    IOSurfaceRef surface;
    id<MTLTexture> texture;
  };

  std::array<ColorBuffer, 2> color_buffer_;
  CardboardEye eye_{kLeft};

  id<MTLTexture> black_texture_;
  id<MTLBuffer> black_texture_vertices_buffer_;
  id<MTLBuffer> black_texture_uvs_buffer_;

  bool are_widgets_setup_{false};
};

}  // namespace

std::unique_ptr<Renderer> MakeMetalRenderer(IUnityInterfaces* xr_interfaces) {
  return std::make_unique<MetalRenderer>(xr_interfaces);
}

CardboardDistortionRenderer* MakeCardboardMetalDistortionRenderer(IUnityInterfaces* xr_interfaces) {
  IUnityGraphicsMetalV1* metal_interface = xr_interfaces->Get<IUnityGraphicsMetalV1>();
  const CardboardMetalDistortionRendererConfig config{
      reinterpret_cast<uint64_t>(CFBridgingRetain(metal_interface->MetalDevice())),
      MTLPixelFormatBGRA8Unorm, MTLPixelFormatDepth32Float_Stencil8,
      MTLPixelFormatDepth32Float_Stencil8};

  CardboardDistortionRenderer* distortion_renderer =
      CardboardMetalDistortionRenderer_create(&config);

  CFBridgingRelease(reinterpret_cast<CFTypeRef>(config.mtl_device));
  return distortion_renderer;
}

}  // namespace cardboard::unity
