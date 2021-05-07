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

#include <memory>
#include <vector>

#include "include/cardboard.h"
#include "util/is_arg_null.h"
#include "util/logging.h"
#include "unity/xr_unity_plugin/renderer.h"
#include "IUnityGraphicsMetal.h"

// TODO(b/151087873) Convert into single line namespace declaration.
namespace cardboard {
namespace unity {
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

  void TeardownWidgets() override {
    CARDBOARD_LOGD("TeardownWidgets is a no-op method when using Metal.");
  }

  void CreateRenderTexture(RenderTexture* render_texture, int screen_width,
                           int screen_height) override {
    id<MTLDevice> mtl_device = metal_interface_->MetalDevice();

      // Create texture color buffer.
    NSDictionary* color_surface_attribs = @{
      // (NSString*)kIOSurfaceIsGlobal : @YES, // Seems to not be necessary and less safe, maybe doesn't work with earlier versions of Unity?
      (NSString*)kIOSurfaceWidth : @(screen_width / 2),
      (NSString*)kIOSurfaceHeight : @(screen_height),
      (NSString*)kIOSurfaceBytesPerElement : @4u
    };
        
      MTLTextureDescriptor* texture_color_buffer_descriptor = [MTLTextureDescriptorClass new];
      texture_color_buffer_descriptor.textureType = MTLTextureType2D;
      texture_color_buffer_descriptor.width = screen_width / 2;
      texture_color_buffer_descriptor.height = screen_height;
      texture_color_buffer_descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
      texture_color_buffer_descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderWrite; // MTLTextureUsageShaderRead is less optimized.
        
      IOSurfaceRef color_surface = IOSurfaceCreate((CFDictionaryRef)color_surface_attribs);
        
      render_texture->color_buffer = reinterpret_cast<uint64_t>(color_surface);
        
      if (eyeIndex == 0) {
        color_texture_left_ = [mtl_device newTextureWithDescriptor:texture_color_buffer_descriptor
                                                    iosurface:color_surface
                                                        plane:0];
        eyeIndex = 1;
      } else {
        color_texture_right_ = [mtl_device newTextureWithDescriptor:texture_color_buffer_descriptor
                                                    iosurface:color_surface
                                                        plane:0];
        eyeIndex = 0;
      }

      // When using Metal, texture depth buffer is unused.
      render_texture->depth_buffer = 0;
  }

  void DestroyRenderTexture(RenderTexture* render_texture) override {
    render_texture->color_buffer = 0;
    render_texture->depth_buffer = 0;
  }

  void RenderEyesToDisplay(CardboardDistortionRenderer* renderer, const ScreenParams& screen_params,
                           const CardboardEyeTextureDescription* left_eye,
                           const CardboardEyeTextureDescription* right_eye) override {

    const CardboardDistortionRendererTargetConfig target_config{
      reinterpret_cast<uint64_t>(CFBridgingRetain(metal_interface_->CurrentCommandEncoder())),
      screen_params.width, screen_params.height};

    CardboardEyeTextureDescription left_eye_description = *left_eye;
    CardboardEyeTextureDescription right_eye_description = *right_eye;

    left_eye_description.texture = reinterpret_cast<uint64_t>(color_texture_left_);
    right_eye_description.texture = reinterpret_cast<uint64_t>(color_texture_right_);
    
    CardboardDistortionRenderer_renderEyeToDisplay(
      renderer, reinterpret_cast<uint64_t>(&target_config), screen_params.viewport_x,
      screen_params.viewport_y, screen_params.viewport_width, screen_params.viewport_height,
      &left_eye_description, &right_eye_description);

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

  id<MTLTexture> color_texture_left_;
  id<MTLTexture> color_texture_right_;

  id<MTLTexture> black_texture_;
  id<MTLBuffer> black_texture_vertices_buffer_;
  id<MTLBuffer> black_texture_uvs_buffer_;

  bool are_widgets_setup_{false};
    
  int eyeIndex = 0;
};

}  // namespace

std::unique_ptr<Renderer> MakeMetalRenderer(IUnityInterfaces* xr_interfaces) {
  return std::make_unique<MetalRenderer>(xr_interfaces);
}

CardboardDistortionRenderer* MakeCardboardMetalDistortionRenderer(IUnityInterfaces* xr_interfaces) {
  IUnityGraphicsMetalV1* metal_interface = xr_interfaces->Get<IUnityGraphicsMetalV1>();
  const CardboardMetalDistortionRendererConfig config{
      reinterpret_cast<uint64_t>(CFBridgingRetain(metal_interface->MetalDevice())),
      MTLPixelFormatRGBA8Unorm, MTLPixelFormatDepth32Float_Stencil8,
      MTLPixelFormatDepth32Float_Stencil8};

  CardboardDistortionRenderer* distortion_renderer =
      CardboardMetalDistortionRenderer_create(&config);

  CFBridgingRelease(reinterpret_cast<CFTypeRef>(config.mtl_device));
  return distortion_renderer;
}

}  // namespace unity
}  // namespace cardboard
