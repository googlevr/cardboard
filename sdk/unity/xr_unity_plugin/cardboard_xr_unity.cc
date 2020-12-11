/*
 * Copyright 2020 Google LLC
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
#include "unity/xr_unity_plugin/cardboard_xr_unity.h"

#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

#ifdef __ANDROID__
#include <GLES2/gl2.h>
#endif
#ifdef __APPLE__
#include <OpenGLES/ES2/gl.h>
#endif
#ifdef __ANDROID__
#include <GLES3/gl3.h>
#endif
#ifdef __APPLE__
#include <OpenGLES/ES3/gl.h>
#endif
#include "include/cardboard.h"

// The following block makes log macros available for Android and iOS.
#if __ANDROID__
#include <android/log.h>
#define LOG_TAG "CardboardXRUnity"
#define LOGW(fmt, ...)                                                       \
  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%s : %d] " fmt, __FILE__, \
                      __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...)                                                        \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s : %d] " fmt, __FILE__, \
                      __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...)                                                        \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s : %d] " fmt, __FILE__, \
                      __LINE__, ##__VA_ARGS__)
#define LOGF(fmt, ...)                                                        \
  __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "[%s : %d] " fmt, __FILE__, \
                      __LINE__, ##__VA_ARGS__)
#elif __APPLE__
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CoreFoundation.h>
extern "C" {
void NSLog(CFStringRef format, ...);
}
#define LOGW(fmt, ...) \
  NSLog(CFSTR("[%s : %d] " fmt), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...) \
  NSLog(CFSTR("[%s : %d] " fmt), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...) \
  NSLog(CFSTR("[%s : %d] " fmt), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGF(fmt, ...) \
  NSLog(CFSTR("[%s : %d] " fmt), __FILE__, __LINE__, ##__VA_ARGS__)
#elif
#define LOGW(...)
#define LOGD(...)
#define LOGE(...)
#define LOGF(...)
#endif

// @def Forwards the call to CheckGlError().
#define CHECKGLERROR(label) CheckGlError(__FILE__, __LINE__)

namespace {
/**
 * Checks for OpenGL errors, and crashes if one has occurred.  Note that this
 * can be an expensive call, so real applications should call this rarely.
 *
 * @param file File name
 * @param line Line number
 * @param label Error label
 */
void CheckGlError(const char* file, int line) {
  int gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) {
    LOGF("[%s : %d] GL error: %d", file, line, gl_error);
    // Crash immediately to make OpenGL errors obvious.
    abort();
  }
}

// TODO(b/155457703): De-dupe GL utility function here and in
// distortion_renderer.cc
GLuint LoadShader(GLenum shader_type, const char* source) {
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  CHECKGLERROR("glCompileShader");
  GLint result = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
  if (result == GL_FALSE) {
    int log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length == 0) {
      return 0;
    }

    std::vector<char> log_string(log_length);
    glGetShaderInfoLog(shader, log_length, nullptr, log_string.data());
    LOGE("Could not compile shader of type %d: %s", shader_type,
         log_string.data());

    shader = 0;
  }

  return shader;
}

// TODO(b/155457703): De-dupe GL utility function here and in
// distortion_renderer.cc
GLuint CreateProgram(const char* vertex, const char* fragment) {
  GLuint vertex_shader = LoadShader(GL_VERTEX_SHADER, vertex);
  if (vertex_shader == 0) {
    return 0;
  }

  GLuint fragment_shader = LoadShader(GL_FRAGMENT_SHADER, fragment);
  if (fragment_shader == 0) {
    return 0;
  }

  GLuint program = glCreateProgram();

  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  CHECKGLERROR("glLinkProgram");

  GLint result = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &result);
  if (result == GL_FALSE) {
    int log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length == 0) {
      return 0;
    }

    std::vector<char> log_string(log_length);
    glGetShaderInfoLog(program, log_length, nullptr, log_string.data());
    LOGE("Could not compile program: %s", log_string.data());

    return 0;
  }

  glDetachShader(program, vertex_shader);
  glDetachShader(program, fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  CHECKGLERROR("GlCreateProgram");

  return program;
}

/// @brief Vertex shader for RenderWidget when using OpenGL ES2.0.
const char kWidgetVertexShaderOpenGlEs2[] =
    R"glsl(
  attribute vec2 a_Position;
  attribute vec2 a_TexCoords;
  varying vec2 v_TexCoords;
  void main() {
    gl_Position = vec4(a_Position, 0, 1);
    v_TexCoords = a_TexCoords;
  }
  )glsl";

/// @brief Fragment shader for RenderWidget when using OpenGL ES2.0.
const char kWidgetFragmentShaderOpenGlEs2[] =
    R"glsl(
  precision mediump float;
  uniform sampler2D u_Texture;
  varying vec2 v_TexCoords;
  void main() {
    gl_FragColor = texture2D(u_Texture, v_TexCoords);
  }
  )glsl";

/// @brief Vertex shader for RenderWidget when using OpenGL ES3.0.
const char kWidgetVertexShaderOpenGlEs3[] =
    R"glsl(#version 300 es
  layout (location = 0) in vec2 a_Position;
  layout (location = 1) in vec2 a_TexCoords;
  out vec2 v_TexCoords;
  void main() {
    gl_Position = vec4(a_Position, 0, 1);
    v_TexCoords = a_TexCoords;
  }
  )glsl";

/// @brief Fragment shader for RenderWidget when using OpenGL ES3.0.
const char kWidgetFragmentShaderOpenGlEs3[] =
    R"glsl(#version 300 es
  precision mediump float;
  uniform sampler2D u_Texture;
  in vec2 v_TexCoords;
  out vec4 o_FragColor;
  void main() {
    o_FragColor = texture(u_Texture, v_TexCoords);
  }
  )glsl";

}  // namespace

// TODO(b/151087873) Convert into single line namespace declaration.
namespace cardboard {
namespace unity {

// @brief It provides the implementation of the PImpl pattern for the Cardboard
//        SDK C-API.
class CardboardApi::CardboardApiImpl {
 public:
  // @brief Default contructor. See attributes for default initialization.
  CardboardApiImpl() = default;

  // @brief Destructor.
  // @details Frees GL resources, HeadTracker module and Distortion Renderer
  //          module.
  ~CardboardApiImpl() { GlTeardown(); }

  void InitHeadTracker() {
    if (head_tracker_ == nullptr) {
      head_tracker_.reset(CardboardHeadTracker_create());
    }
    CardboardHeadTracker_resume(head_tracker_.get());
  }

  void PauseHeadTracker() {
    if (head_tracker_ == nullptr) {
      LOGW("Uninitialized head tracker was paused.");
      return;
    }
    CardboardHeadTracker_pause(head_tracker_.get());
  }

  void ResumeHeadTracker() {
    if (head_tracker_ == nullptr) {
      LOGW("Uninitialized head tracker was resumed.");
      return;
    }
    CardboardHeadTracker_resume(head_tracker_.get());
  }

  void GetHeadTrackerPose(float* position, float* orientation) {
    if (head_tracker_ == nullptr) {
      LOGW("Uninitialized head tracker was queried for the pose.");
      position[0] = 0.0f;
      position[1] = 0.0f;
      position[2] = 0.0f;
      orientation[0] = 0.0f;
      orientation[1] = 0.0f;
      orientation[2] = 0.0f;
      orientation[3] = 1.0f;
      return;
    }
    CardboardHeadTracker_getPose(head_tracker_.get(),
                                 CardboardApiImpl::GetMonotonicTimeNano() +
                                     kPredictionTimeWithoutVsyncNanos,
                                 position, orientation);
  }

  static void ScanDeviceParams() {
    CardboardQrCode_scanQrCodeAndSaveDeviceParams();
  }

  void UpdateDeviceParams() {
    if (selected_graphics_api_ == kNone) {
      LOGE(
          "Misconfigured Graphics API. Neither OpenGl ES2.0 nor OpenGl ES3.0 "
          "was selected.");
      return;
    }

    // Updates the screen size.
    screen_params_ = unity_screen_params_;

    // Get saved device parameters
    uint8_t* data;
    int size;
    CardboardQrCode_getSavedDeviceParams(&data, &size);
    CardboardLensDistortion* lens_distortion;
    if (size == 0) {
      // Loads Cardboard V1 device parameters when no device parameters are
      // available.
      CardboardQrCode_getCardboardV1DeviceParams(&data, &size);
      lens_distortion = CardboardLensDistortion_create(
          data, size, screen_params_.width, screen_params_.height);
    } else {
      lens_distortion = CardboardLensDistortion_create(
          data, size, screen_params_.width, screen_params_.height);
      CardboardQrCode_destroy(data);
    }
    device_params_changed_ = false;

    GlSetup();

    if (selected_graphics_api_ == kOpenGlEs2) {
      distortion_renderer_.reset(CardboardOpenGlEs2DistortionRenderer_create());
    } else {
      // #gles3 - This call is only needed if OpenGL ES 3.0 support is desired.
      // Remove the following line if OpenGL ES 3.0 is not needed.
      distortion_renderer_.reset(CardboardOpenGlEs3DistortionRenderer_create());
    }

    CardboardLensDistortion_getDistortionMesh(
        lens_distortion, CardboardEye::kLeft,
        &eye_data_[CardboardEye::kLeft].distortion_mesh);
    CardboardLensDistortion_getDistortionMesh(
        lens_distortion, CardboardEye::kRight,
        &eye_data_[CardboardEye::kRight].distortion_mesh);

    CardboardDistortionRenderer_setMesh(
        distortion_renderer_.get(),
        &eye_data_[CardboardEye::kLeft].distortion_mesh, CardboardEye::kLeft);
    CardboardDistortionRenderer_setMesh(
        distortion_renderer_.get(),
        &eye_data_[CardboardEye::kRight].distortion_mesh, CardboardEye::kRight);

    // Get eye matrices
    CardboardLensDistortion_getEyeFromHeadMatrix(
        lens_distortion, CardboardEye::kLeft,
        eye_data_[CardboardEye::kLeft].eye_from_head_matrix);
    CardboardLensDistortion_getEyeFromHeadMatrix(
        lens_distortion, CardboardEye::kRight,
        eye_data_[CardboardEye::kRight].eye_from_head_matrix);
    CardboardLensDistortion_getFieldOfView(lens_distortion, CardboardEye::kLeft,
                                           eye_data_[CardboardEye::kLeft].fov);
    CardboardLensDistortion_getFieldOfView(lens_distortion,
                                           CardboardEye::kRight,
                                           eye_data_[CardboardEye::kRight].fov);

    CardboardLensDistortion_destroy(lens_distortion);

    CHECKGLERROR("UpdateDeviceParams");
  }

  void GetEyeMatrices(int eye, float* eye_from_head, float* fov) {
    std::memcpy(eye_from_head, eye_data_[eye].eye_from_head_matrix,
                sizeof(float) * 16);
    std::memcpy(fov, eye_data_[eye].fov, sizeof(float) * 4);
  }

  void RenderEyesToDisplay(int gl_framebuffer_id) {
    CardboardDistortionRenderer_renderEyeToDisplay(
        distortion_renderer_.get(), gl_framebuffer_id, screen_params_.x,
        screen_params_.y, screen_params_.width, screen_params_.height,
        &eye_data_[CardboardEye::kLeft].texture,
        &eye_data_[CardboardEye::kRight].texture);
  }

  void RenderWidgets() {
    std::lock_guard<std::mutex> l(widget_mutex_);
    for (WidgetParams widget_param : widget_params_) {
      RenderWidget(widget_param);
    }
  }

  int GetLeftTextureId() {
    return gl_render_textures_[CardboardEye::kLeft].color_texture;
  }

  int GetRightTextureId() {
    return gl_render_textures_[CardboardEye::kRight].color_texture;
  }

  int GetLeftDepthBufferId() {
    return gl_render_textures_[CardboardEye::kLeft].depth_render_buffer;
  }

  int GetRightDepthBufferId() {
    return gl_render_textures_[CardboardEye::kRight].depth_render_buffer;
  }

  static void SetUnityScreenParams(int x, int y, int width, int height) {
    unity_screen_params_ = ScreenParams{x, y, width, height};
  }

  static void GetUnityScreenParams(int* width, int* height) {
    const ScreenParams screen_params = unity_screen_params_;
    *width = screen_params.width;
    *height = screen_params.height;
  }

  static void SetWidgetCount(int count) {
    std::lock_guard<std::mutex> l(widget_mutex_);
    widget_params_.resize(count);
  }

  static void SetWidgetParams(int i, const WidgetParams& params) {
    std::lock_guard<std::mutex> l(widget_mutex_);
    if (i < 0 || i >= widget_params_.size()) {
      LOGE("SetWidgetParams parameter i=%d, out of bounds (size=%d)", i,
           static_cast<int>(widget_params_.size()));
      return;
    }

    widget_params_[i] = params;
  }

  static void SetDeviceParametersChanged() { device_params_changed_ = true; }

  static bool GetDeviceParametersChanged() { return device_params_changed_; }

  static void SetGraphicsApi(CardboardGraphicsApi graphics_api) {
    selected_graphics_api_ = graphics_api;
  }

 private:
  // @brief Holds the rectangle information to draw into the screen.
  struct ScreenParams {
    // @brief x coordinate in pixels of the lower left corner of the rectangle.
    int x;

    // @brief y coordinate in pixels of the lower left corner of the rectangle.
    int y;

    // @brief The width of the rectangle in pixels.
    int width;

    // @brief The height of the rectangle in pixels.
    int height;
  };

  // @brief Holds eye information.
  struct EyeData {
    // @brief The eye-from-head homogeneous transformation for the eye.
    float eye_from_head_matrix[16];

    // @brief The field of view angles.
    // @details They are disposed as [left, right, bottom, top] and are in
    //          radians.
    float fov[4];

    // @brief Cardboard distortion mesh for the eye.
    CardboardMesh distortion_mesh;

    // @brief Cardboard texture description for the eye.
    CardboardEyeTextureDescription texture;
  };

  // @brief Holds the OpenGl texture and depth buffers for each eye.
  struct GlRenderTexture {
    unsigned int color_texture = 0;

    unsigned int depth_render_buffer = 0;
  };

  // @brief Custom deleter for HeadTracker.
  struct CardboardHeadTrackerDeleter {
    void operator()(CardboardHeadTracker* head_tracker) {
      CardboardHeadTracker_destroy(head_tracker);
    }
  };

  // @brief Custom deleter for DistortionRenderer.
  struct CardboardDistortionRendererDeleter {
    void operator()(CardboardDistortionRenderer* distortion_renderer) {
      CardboardDistortionRenderer_destroy(distortion_renderer);
    }
  };

  // @brief Computes the monotonic time in nano seconds.
  // @return The monotonic time count in nano seconds.
  static int64_t GetMonotonicTimeNano() {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (res.tv_sec * CardboardApi::CardboardApiImpl::kNanosInSeconds) +
           res.tv_nsec;
  }

  // @brief Creates and configures a texture and depth buffer for an eye.
  //
  // @details Loads a color texture and then a depth buffer for an eye.
  // @param gl_render_texture A GlRenderTexture to load its resources.
  void CreateGlRenderTexture(GlRenderTexture* gl_render_texture) {
    // Create color texture.
    glGenTextures(1, &gl_render_texture->color_texture);
    glBindTexture(GL_TEXTURE_2D, gl_render_texture->color_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screen_params_.width / 2,
                 screen_params_.height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    CHECKGLERROR("Create a color texture.");

    // Create depth buffer.
    glGenRenderbuffers(1, &gl_render_texture->depth_render_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, gl_render_texture->depth_render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          screen_params_.width / 2, screen_params_.height);
    CHECKGLERROR("Create depth render buffer.");
  }

  // @brief Configures GL resources.
  void GlSetup() {
    if (selected_graphics_api_ == kNone) {
      LOGE(
          "Misconfigured Graphics API. Neither OpenGL ES 2.0 nor OpenGL ES 3.0 "
          "was selected.");
      return;
    }

    if (gl_render_textures_[0].color_texture != 0) {
      GlTeardown();
    }

    // Create render texture and depth buffer for both eyes.
    CreateGlRenderTexture(&gl_render_textures_[CardboardEye::kLeft]);
    CreateGlRenderTexture(&gl_render_textures_[CardboardEye::kRight]);

    eye_data_[CardboardEye::kLeft].texture.texture =
        gl_render_textures_[CardboardEye::kLeft].color_texture;
    eye_data_[CardboardEye::kLeft].texture.left_u = 0;
    eye_data_[CardboardEye::kLeft].texture.right_u = 1;
    eye_data_[CardboardEye::kLeft].texture.top_v = 1;
    eye_data_[CardboardEye::kLeft].texture.bottom_v = 0;

    eye_data_[CardboardEye::kRight].texture.texture =
        gl_render_textures_[CardboardEye::kRight].color_texture;
    eye_data_[CardboardEye::kRight].texture.left_u = 0;
    eye_data_[CardboardEye::kRight].texture.right_u = 1;
    eye_data_[CardboardEye::kRight].texture.top_v = 1;
    eye_data_[CardboardEye::kRight].texture.bottom_v = 0;

    // Load widget state
    if (selected_graphics_api_ == kOpenGlEs2) {
      widget_program_ = CreateProgram(kWidgetVertexShaderOpenGlEs2,
                                      kWidgetFragmentShaderOpenGlEs2);
    } else {
      // #gles3 - This call is only needed if OpenGL ES 3.0 support is desired.
      // Remove the following line if OpenGL ES 3.0 is not needed.
      widget_program_ = CreateProgram(kWidgetVertexShaderOpenGlEs3,
                                      kWidgetFragmentShaderOpenGlEs3);
    }
    widget_attrib_position_ =
        glGetAttribLocation(widget_program_, "a_Position");
    widget_attrib_tex_coords_ =
        glGetAttribLocation(widget_program_, "a_TexCoords");
    widget_uniform_texture_ =
        glGetUniformLocation(widget_program_, "u_Texture");
  }

  // @brief Releases Gl resources in a GlRenderTexture.
  //
  // @param gl_render_texture A GlRenderTexture to release its resources.
  void DestroyGlRenderTexture(GlRenderTexture* gl_render_texture) {
    glDeleteRenderbuffers(1, &gl_render_texture->depth_render_buffer);
    gl_render_texture->depth_render_buffer = 0;

    glDeleteTextures(1, &gl_render_texture->color_texture);
    gl_render_texture->color_texture = 0;
  }

  // @brief Frees GL resources.
  void GlTeardown() {
    if (gl_render_textures_[0].color_texture == 0) {
      return;
    }
    DestroyGlRenderTexture(&gl_render_textures_[CardboardEye::kLeft]);
    DestroyGlRenderTexture(&gl_render_textures_[CardboardEye::kRight]);
    CHECKGLERROR("GlTeardown");
  }

  static constexpr float Lerp(float start, float end, float val) {
    return start + (end - start) * val;
  }

  void RenderWidget(WidgetParams params) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // Convert coordinates to normalized space (-1,-1 - +1,+1)
    float x = Lerp(-1, +1, static_cast<float>(params.x) / screen_params_.width);
    float y =
        Lerp(-1, +1, static_cast<float>(params.y) / screen_params_.height);
    float width = params.width * 2.0f / screen_params_.width;
    float height = params.height * 2.0f / screen_params_.height;
    const float position[] = {x, y,          x + width, y,
                              x, y + height, x + width, y + height};

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(widget_attrib_position_);
    glVertexAttribPointer(
        widget_attrib_position_, /*size=*/2, /*type=*/GL_FLOAT,
        /*normalized=*/GL_FALSE, /*stride=*/0, /*pointer=*/position);
    CHECKGLERROR("RenderWidget-7");

    const float uv[] = {0, 0, 1, 0, 0, 1, 1, 1};
    glEnableVertexAttribArray(widget_attrib_tex_coords_);
    glVertexAttribPointer(
        widget_attrib_tex_coords_, /*size=*/2, /*type=*/GL_FLOAT,
        /*normalized=*/GL_FALSE, /*stride=*/0, /*pointer=*/uv);

    glUseProgram(widget_program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, params.texture);
    glUniform1i(widget_uniform_texture_, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    CHECKGLERROR("RenderWidget");
  }

  // @brief Default prediction excess time in nano seconds.
  static constexpr int64_t kPredictionTimeWithoutVsyncNanos = 50000000;

  // @brief Default z-axis coordinate for the near clipping plane.
  static constexpr float kZNear = 0.1f;

  // @brief Default z-axis coordinate for the far clipping plane.
  static constexpr float kZFar = 10.0f;

  // @brief Constant to convert seconds into nano seconds.
  static constexpr int64_t kNanosInSeconds = 1000000000;

  // @brief HeadTracker native pointer.
  std::unique_ptr<CardboardHeadTracker, CardboardHeadTrackerDeleter>
      head_tracker_;

  // @brief DistortionRenderer native pointer.
  std::unique_ptr<CardboardDistortionRenderer,
                  CardboardDistortionRendererDeleter>
      distortion_renderer_;

  // @brief Screen parameters.
  ScreenParams screen_params_;

  // @brief Eye data information.
  // @details `CardboardEye::kLeft` index holds left eye data and
  //          `CardboardEye::kRight` holds the right eye data.
  std::array<EyeData, 2> eye_data_;

  // @brief Holds the OpenGL render texture information for each eye.
  std::array<GlRenderTexture, 2> gl_render_textures_;

  // @brief Store Unity reported screen params.
  static std::atomic<ScreenParams> unity_screen_params_;

  // @brief Unity-loaded widgets
  static std::vector<WidgetParams> widget_params_;

  // @brief Mutex for widget_params_ access.
  static std::mutex widget_mutex_;

  // @brief RenderWidget GL program.
  GLuint widget_program_;

  // @brief RenderWidget "a_Position" attrib location.
  GLint widget_attrib_position_;

  // @brief RenderWidget "a_TexCoords" attrib location.
  GLint widget_attrib_tex_coords_;

  // @brief RenderWidget "u_Texture" uniform location.
  GLint widget_uniform_texture_;

  // @brief Track changes to device parameters.
  static std::atomic<bool> device_params_changed_;

  // @brief Holds the selected graphics API.
  static std::atomic<CardboardGraphicsApi> selected_graphics_api_;
};

std::atomic<CardboardApi::CardboardApiImpl::ScreenParams>
    CardboardApi::CardboardApiImpl::unity_screen_params_({0, 0});

std::vector<CardboardApi::WidgetParams>
    CardboardApi::CardboardApiImpl::widget_params_;

std::mutex CardboardApi::CardboardApiImpl::widget_mutex_;

std::atomic<bool> CardboardApi::CardboardApiImpl::device_params_changed_(true);

std::atomic<CardboardGraphicsApi>
    CardboardApi::CardboardApiImpl::selected_graphics_api_(kNone);

CardboardApi::CardboardApi() { p_impl_.reset(new CardboardApiImpl()); }

CardboardApi::~CardboardApi() = default;

void CardboardApi::InitHeadTracker() { p_impl_->InitHeadTracker(); }

void CardboardApi::PauseHeadTracker() { p_impl_->PauseHeadTracker(); }

void CardboardApi::ResumeHeadTracker() { p_impl_->ResumeHeadTracker(); }

void CardboardApi::GetHeadTrackerPose(float* position, float* orientation) {
  p_impl_->GetHeadTrackerPose(position, orientation);
}

void CardboardApi::ScanDeviceParams() { CardboardApiImpl::ScanDeviceParams(); }

void CardboardApi::UpdateDeviceParams() { p_impl_->UpdateDeviceParams(); }

void CardboardApi::GetEyeMatrices(int eye, float* eye_from_head, float* fov) {
  return p_impl_->GetEyeMatrices(eye, eye_from_head, fov);
}

void CardboardApi::RenderEyesToDisplay(int gl_framebuffer_id) {
  p_impl_->RenderEyesToDisplay(gl_framebuffer_id);
}

void CardboardApi::RenderWidgets() { p_impl_->RenderWidgets(); }

int CardboardApi::GetLeftTextureId() { return p_impl_->GetLeftTextureId(); }

int CardboardApi::GetRightTextureId() { return p_impl_->GetRightTextureId(); }

int CardboardApi::GetLeftDepthBufferId() {
  return p_impl_->GetLeftDepthBufferId();
}

int CardboardApi::GetRightDepthBufferId() {
  return p_impl_->GetRightDepthBufferId();
}

int CardboardApi::GetBoundFramebuffer() {
  int bound_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_framebuffer);
  return bound_framebuffer;
}

void CardboardApi::GetScreenParams(int* width, int* height) {
  CardboardApi::CardboardApiImpl::GetUnityScreenParams(width, height);
}

void CardboardApi::SetUnityScreenParams(int x, int y, int width, int height) {
  CardboardApi::CardboardApiImpl::SetUnityScreenParams(x, y, width, height);
}

void CardboardApi::SetDeviceParametersChanged() {
  CardboardApi::CardboardApiImpl::SetDeviceParametersChanged();
}

bool CardboardApi::GetDeviceParametersChanged() {
  return CardboardApi::CardboardApiImpl::GetDeviceParametersChanged();
}

void CardboardApi::SetWidgetCount(int count) {
  CardboardApi::CardboardApiImpl::SetWidgetCount(count);
}

void CardboardApi::SetWidgetParams(int i, const WidgetParams& params) {
  CardboardApi::CardboardApiImpl::SetWidgetParams(i, params);
}

void CardboardApi::SetGraphicsApi(CardboardGraphicsApi graphics_api) {
  CardboardApi::CardboardApiImpl::SetGraphicsApi(graphics_api);
}

}  // namespace unity
}  // namespace cardboard

#ifdef __cplusplus
extern "C" {
#endif

void CardboardUnity_setScreenParams(int x, int y, int width, int height) {
  cardboard::unity::CardboardApi::SetUnityScreenParams(x, y, width, height);
}

void CardboardUnity_setDeviceParametersChanged() {
  cardboard::unity::CardboardApi::SetDeviceParametersChanged();
}

void CardboardUnity_setWidgetCount(int count) {
  cardboard::unity::CardboardApi::SetWidgetCount(count);
}

void CardboardUnity_setWidgetParams(int i, void* texture, int x, int y,
                                    int width, int height) {
  cardboard::unity::CardboardApi::WidgetParams params;

  params.texture = static_cast<int>(reinterpret_cast<intptr_t>(texture));
  params.x = x;
  params.y = y;
  params.width = width;
  params.height = height;
  cardboard::unity::CardboardApi::SetWidgetParams(i, params);
}

void CardboardUnity_setGraphicsApi(CardboardGraphicsApi graphics_api) {
  switch (graphics_api) {
    case CardboardGraphicsApi::kOpenGlEs2:
      LOGD("Configured OpenGL ES2.0 as Graphics API.");
      cardboard::unity::CardboardApi::SetGraphicsApi(graphics_api);
      break;
    case CardboardGraphicsApi::kOpenGlEs3:
      LOGD("Configured OpenGL ES3.0 as Graphics API.");
      cardboard::unity::CardboardApi::SetGraphicsApi(graphics_api);
      break;
    default:
      LOGE(
          "Misconfigured Graphics API. Neither OpenGL ES 2.0 nor OpenGL ES 3.0 "
          "was selected.");
  }
}

#ifdef __cplusplus
}
#endif
