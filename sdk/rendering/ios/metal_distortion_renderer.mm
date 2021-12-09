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

#include "distortion_renderer.h"
#include "include/cardboard.h"
#include "util/is_arg_null.h"
#include "util/is_initialized.h"
#include "util/logging.h"

namespace {

/// @note This enum must be kept in sync with the shader counterpart.
typedef enum VertexInputIndex {
  VertexInputIndexPosition = 0,
  VertexInputIndexTexCoords,
} VertexInputIndex;

/// @note This enum must be kept in sync with the shader counterpart.
typedef enum FragmentInputIndex {
  FragmentInputIndexTexture = 0,
  FragmentInputIndexStart,
  FragmentInputIndexEnd,
} FragmentInputIndex;

/// @note This struct must be kept in sync with the shader counterpart.
typedef struct Vertex {
  vector_float2 position;
  vector_float2 tex_coords;
} Vertex;

// TODO(b/178125083): Revisit Metal shader approach.
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
      FragmentInputIndexStart,
      FragmentInputIndexEnd,
    } FragmentInputIndex;

    typedef struct Vertex {
      vector_float2 position;
      vector_float2 tex_coords;
    } Vertex;

    struct VertexOut {
      float4 position [[position]];
      float2 tex_coords;
    };

    vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                                  constant vector_float2 *position [[buffer(VertexInputIndexPosition)]],
                                  constant vector_float2 *tex_coords [[buffer(VertexInputIndexTexCoords)]]) {
      VertexOut out;
      out.position = vector_float4(position[vertexID], 0.0, 1.0);
      // The v coordinate of the distortion mesh is reversed compared to what Metal expects, so we invert it.
      out.tex_coords = vector_float2(tex_coords[vertexID].x, 1.0 - tex_coords[vertexID].y);
      return out;
    }

    fragment float4 fragmentShader(VertexOut in [[stage_in]],
                                   texture2d<half> colorTexture [[texture(FragmentInputIndexTexture)]],
                                   constant vector_float2 *start [[buffer(FragmentInputIndexStart)]],
                                   constant vector_float2 *end [[buffer(FragmentInputIndexEnd)]]) {
      constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
      float2 coords = *start + in.tex_coords * (*end - *start);
      return float4(colorTexture.sample(textureSampler, coords));
    })msl";

}  // namespace

// This class needs to be loaded on runtime, otherwise a Unity project using a rendering API
// different than Metal won't be able to be built due to linking errors.
static Class MTLRenderPipelineDescriptorClass;

namespace cardboard {
namespace rendering {

// @brief Metal concrete implementation of DistortionRenderer.
class MetalDistortionRenderer : public DistortionRenderer {
 public:
  MetalDistortionRenderer(const CardboardMetalDistortionRendererConfig* config) {
    mtl_device_ = (__bridge id<MTLDevice>)reinterpret_cast<CFTypeRef>(config->mtl_device);

    // Compile metal library.
    id<MTLLibrary> mtl_library =
        [mtl_device_ newLibraryWithSource:[NSString stringWithUTF8String:kMetalShaders]
                                  options:nil
                                    error:nil];
    if (mtl_library == nil) {
      CARDBOARD_LOGE("Failed to compile Metal library.");
      return;
    }

    id<MTLFunction> vertex_function = [mtl_library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragment_function = [mtl_library newFunctionWithName:@"fragmentShader"];

    // Create pipeline.
    MTLRenderPipelineDescriptorClass = NSClassFromString(@"MTLRenderPipelineDescriptor");
    MTLRenderPipelineDescriptor* mtl_render_pipeline_descriptor =
        [[MTLRenderPipelineDescriptorClass alloc] init];
    mtl_render_pipeline_descriptor.vertexFunction = vertex_function;
    mtl_render_pipeline_descriptor.fragmentFunction = fragment_function;
    mtl_render_pipeline_descriptor.colorAttachments[0].pixelFormat =
        static_cast<MTLPixelFormat>(config->color_attachment_pixel_format);
    mtl_render_pipeline_descriptor.depthAttachmentPixelFormat =
        static_cast<MTLPixelFormat>(config->depth_attachment_pixel_format);
    mtl_render_pipeline_descriptor.stencilAttachmentPixelFormat =
        static_cast<MTLPixelFormat>(config->stencil_attachment_pixel_format);
    mtl_render_pipeline_state_ =
        [mtl_device_ newRenderPipelineStateWithDescriptor:mtl_render_pipeline_descriptor error:nil];
    if (mtl_render_pipeline_state_ == nil) {
      CARDBOARD_LOGE("Failed to create Metal render pipeline.");
      return;
    }

    is_initialized_ = true;
  }

  ~MetalDistortionRenderer() {}

  void SetMesh(const CardboardMesh* mesh, CardboardEye eye) override {
    vertices_buffer_[eye] = [mtl_device_
        newBufferWithBytes:mesh->vertices
                    length:(mesh->n_vertices * sizeof(float) * 2)  // Two components per vertex
                   options:MTLResourceStorageModeShared];
    uvs_buffer_[eye] = [mtl_device_
        newBufferWithBytes:mesh->uvs
                    length:(mesh->n_vertices * sizeof(float) * 2)  // Two components per uv
                   options:MTLResourceStorageModeShared];
    indices_buffer_[eye] = [mtl_device_ newBufferWithBytes:mesh->indices
                                                    length:(mesh->n_indices * sizeof(int))
                                                   options:MTLResourceStorageModeShared];
    indices_count_[eye] = mesh->n_indices;
  }

  void RenderEyeToDisplay(uint64_t target, int x, int y, int width, int height,
                          const CardboardEyeTextureDescription* left_eye,
                          const CardboardEyeTextureDescription* right_eye) override {
    if (!is_initialized_) {
      return;
    }

    if (indices_count_[0] == 0 || indices_count_[1] == 0) {
      CARDBOARD_LOGE("Distortion mesh is empty. MetalDistortionRenderer::SetMesh was "
                     "not called yet.");
      return;
    }

    CardboardMetalDistortionRendererTargetConfig* target_config =
        reinterpret_cast<CardboardMetalDistortionRendererTargetConfig*>(target);
    if (CARDBOARD_IS_ARG_NULL(target_config)) {
      return;
    }

    // Get Metal current render command encoder.
    id<MTLRenderCommandEncoder> mtl_render_command_encoder =
        (__bridge id<MTLRenderCommandEncoder>)reinterpret_cast<CFTypeRef>(
            target_config->render_command_encoder);

    [mtl_render_command_encoder setRenderPipelineState:mtl_render_pipeline_state_];

    // Translate y coordinate of the rectangle since in Metal the (0,0) coordinate is
    // located on the top-left corner instead of the bottom-left corner.
    const int mtl_viewport_y = target_config->screen_height - height - y;

    [mtl_render_command_encoder
        setViewport:(MTLViewport){static_cast<double>(x), static_cast<double>(mtl_viewport_y),
                                  static_cast<double>(width), static_cast<double>(height), 0.0,
                                  1.0}];

    RenderDistortionMesh(mtl_render_command_encoder, left_eye, kLeft);
    RenderDistortionMesh(mtl_render_command_encoder, right_eye, kRight);
  }

 private:
  void RenderDistortionMesh(id<MTLRenderCommandEncoder> mtl_render_command_encoder,
                            const CardboardEyeTextureDescription* eye_description,
                            CardboardEye eye) const {
    [mtl_render_command_encoder setVertexBuffer:vertices_buffer_[eye]
                                         offset:0
                                        atIndex:VertexInputIndexPosition];

    [mtl_render_command_encoder setVertexBuffer:uvs_buffer_[eye]
                                         offset:0
                                        atIndex:VertexInputIndexTexCoords];

    [mtl_render_command_encoder
        setFragmentTexture:(__bridge id<MTLTexture>)reinterpret_cast<CFTypeRef>(
                               eye_description->texture)
                   atIndex:FragmentInputIndexTexture];

    simd::float2 start = {eye_description->left_u, eye_description->bottom_v};
    [mtl_render_command_encoder setFragmentBytes:&start
                                          length:sizeof(start)
                                         atIndex:FragmentInputIndexStart];

    simd::float2 end = {eye_description->right_u, eye_description->top_v};
    [mtl_render_command_encoder setFragmentBytes:&end
                                          length:sizeof(end)
                                         atIndex:FragmentInputIndexEnd];

    [mtl_render_command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangleStrip
                                           indexCount:indices_count_[eye]
                                            indexType:MTLIndexTypeUInt32
                                          indexBuffer:indices_buffer_[eye]
                                    indexBufferOffset:0];
  }

  id<MTLDevice> mtl_device_;
  id<MTLRenderPipelineState> mtl_render_pipeline_state_;

  // Mesh buffers. One per eye.
  std::array<id<MTLBuffer>, 2> vertices_buffer_;
  std::array<id<MTLBuffer>, 2> uvs_buffer_;
  std::array<id<MTLBuffer>, 2> indices_buffer_;
  std::array<int, 2> indices_count_{0, 0};

  bool is_initialized_{false};
};

}  // namespace rendering
}  // namespace cardboard

extern "C" {

CardboardDistortionRenderer* CardboardMetalDistortionRenderer_create(
    const CardboardMetalDistortionRendererConfig* config) {
  if (CARDBOARD_IS_NOT_INITIALIZED() || CARDBOARD_IS_ARG_NULL(config)) {
    return nullptr;
  }
  if (config->mtl_device == 0) {
    return nullptr;
  }
  return reinterpret_cast<CardboardDistortionRenderer*>(
      new cardboard::rendering::MetalDistortionRenderer(config));
}

}  // extern "C"
