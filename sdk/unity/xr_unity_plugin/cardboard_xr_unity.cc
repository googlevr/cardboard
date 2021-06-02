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
#include <mutex>  // NOLINT(build/c++11)
#include <vector>

#include "include/cardboard.h"

// The following block makes log macros available for Android and iOS.
#if defined(__ANDROID__)
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
#elif defined(__APPLE__)
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

namespace cardboard::unity {

// @brief It provides the implementation of the PImpl pattern for the Cardboard
//        SDK C-API.
class CardboardApi::CardboardApiImpl {
 public:
  // @brief CardboardApiImpl constructor.
  // @details Initializes the renderer based on the `selected_graphics_api_`
  // variable. The rest of the instance variables would use default
  // initialization values.
  CardboardApiImpl() {
    switch (selected_graphics_api_) {
      case CardboardGraphicsApi::kOpenGlEs2:
        renderer_ = MakeOpenGlEs2Renderer();
        break;
      case CardboardGraphicsApi::kOpenGlEs3:
        renderer_ = MakeOpenGlEs3Renderer();
        break;
#if defined(__APPLE__)
      case CardboardGraphicsApi::kMetal:
        renderer_ = MakeMetalRenderer(xr_interfaces_);
        break;
#endif
      default:
        LOGF("Unsupported API: %d", static_cast<int>(selected_graphics_api_));
        break;
    }
  }

  // @brief CardboardApiImpl destructor.
  // @details Frees renderer resources.
  ~CardboardApiImpl() { RenderingResourcesTeardown(); }

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

    // Checks whether a head tracker recentering has been requested.
    if (head_tracker_recenter_requested_) {
      CardboardHeadTracker_recenter(head_tracker_.get());
      head_tracker_recenter_requested_ = false;
    }

    CardboardHeadTracker_getPose(
        head_tracker_.get(),
        CardboardApiImpl::GetBootTimeNano() + kPredictionTimeWithoutVsyncNanos,
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
          data, size, screen_params_.viewport_width,
          screen_params_.viewport_height);
    } else {
      lens_distortion = CardboardLensDistortion_create(
          data, size, screen_params_.viewport_width,
          screen_params_.viewport_height);
      CardboardQrCode_destroy(data);
    }
    device_params_changed_ = false;

    RenderingResourcesSetup();

    switch (selected_graphics_api_) {
      case CardboardGraphicsApi::kOpenGlEs2:
        distortion_renderer_.reset(
            CardboardOpenGlEs2DistortionRenderer_create());
        break;
      case CardboardGraphicsApi::kOpenGlEs3:
        // #gles3 - This call is only needed if OpenGL ES 3.0 support is
        // desired. Remove the following line if OpenGL ES 3.0 is not needed.
        distortion_renderer_.reset(
            CardboardOpenGlEs3DistortionRenderer_create());
        break;
#if defined(__APPLE__)
      case CardboardGraphicsApi::kMetal:
        distortion_renderer_.reset(
            MakeCardboardMetalDistortionRenderer(xr_interfaces_));
        break;
#endif
      default:
        LOGF("Unsupported API: %d", static_cast<int>(selected_graphics_api_));
        break;
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
  }

  void GetEyeMatrices(int eye, float* eye_from_head, float* fov) {
    std::memcpy(eye_from_head, eye_data_[eye].eye_from_head_matrix,
                sizeof(float) * 16);
    std::memcpy(fov, eye_data_[eye].fov, sizeof(float) * 4);
  }

  void RenderEyesToDisplay() {
    const Renderer::ScreenParams screen_params{
        screen_params_.width,          screen_params_.height,
        screen_params_.viewport_x,     screen_params_.viewport_y,
        screen_params_.viewport_width, screen_params_.viewport_height};
    renderer_->RenderEyesToDisplay(distortion_renderer_.get(), screen_params,
                                   &eye_data_[CardboardEye::kLeft].texture,
                                   &eye_data_[CardboardEye::kRight].texture);
  }

  void RenderWidgets() {
    std::lock_guard<std::mutex> l(widget_mutex_);
    Renderer::ScreenParams screen_params{
        screen_params_.width,          screen_params_.height,
        screen_params_.viewport_x,     screen_params_.viewport_y,
        screen_params_.viewport_width, screen_params_.viewport_height};
    renderer_->RenderWidgets(screen_params, widget_params_);
  }

  uint64_t GetLeftTextureColorBufferId() {
    return render_textures_[CardboardEye::kLeft].color_buffer;
  }

  uint64_t GetRightTextureColorBufferId() {
    return render_textures_[CardboardEye::kRight].color_buffer;
  }

  uint64_t GetLeftTextureDepthBufferId() {
    return render_textures_[CardboardEye::kLeft].depth_buffer;
  }

  uint64_t GetRightTextureDepthBufferId() {
    return render_textures_[CardboardEye::kRight].depth_buffer;
  }

  static void SetUnityScreenParams(int width, int height, int viewport_x,
                                   int viewport_y, int viewport_width,
                                   int viewport_height) {
    unity_screen_params_ = ScreenParams{
        width, height, viewport_x, viewport_y, viewport_width, viewport_height};
  }

  static void GetUnityScreenParams(int* width, int* height) {
    const ScreenParams screen_params = unity_screen_params_;
    *width = screen_params.viewport_width;
    *height = screen_params.viewport_height;
  }

  static void SetWidgetCount(int count) {
    std::lock_guard<std::mutex> l(widget_mutex_);
    widget_params_.resize(count);
  }

  static void SetWidgetParams(int i, const Renderer::WidgetParams& params) {
    std::lock_guard<std::mutex> l(widget_mutex_);
    if (i < 0 || i >= static_cast<int>(widget_params_.size())) {
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

  static CardboardGraphicsApi GetGraphicsApi() {
    return selected_graphics_api_;
  }

  static void SetUnityInterfaces(IUnityInterfaces* xr_interfaces) {
    xr_interfaces_ = xr_interfaces;
  }

  static void SetHeadTrackerRecenterRequested() {
    head_tracker_recenter_requested_ = true;
  }

 private:
  // @brief Holds the screen and rendering area details.
  struct ScreenParams {
    // @brief The width of the screen in pixels.
    int width;
    // @brief The height of the screen in pixels.
    int height;
    // @brief x coordinate in pixels of the lower left corner of the rendering
    // area rectangle.
    int viewport_x;
    // @brief y coordinate in pixels of the lower left corner of the rendering
    // area rectangle.
    int viewport_y;
    // @brief The width of the rendering area rectangle in pixels.
    int viewport_width;
    // @brief The height of the rendering area rectangle in pixels.
    int viewport_height;
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

  // @brief Computes the system boot time in nanoseconds.
  // @return The system boot time count in nanoseconds.
  static int64_t GetBootTimeNano() {
    struct timespec res;
#if defined(__ANDROID__)
    clock_gettime(CLOCK_BOOTTIME, &res);
#elif defined(__APPLE__)
    clock_gettime(CLOCK_UPTIME_RAW, &res);
#endif
    return (res.tv_sec * CardboardApi::CardboardApiImpl::kNanosInSeconds) +
           res.tv_nsec;
  }

  // @brief Configures rendering resources.
  void RenderingResourcesSetup() {
    if (selected_graphics_api_ == kNone) {
      LOGE(
          "Misconfigured Graphics API. Neither OpenGL ES 2.0 nor OpenGL ES 3.0 "
          "was selected.");
      return;
    }

    if (render_textures_[0].color_buffer != 0) {
      RenderingResourcesTeardown();
    }

    // Create render texture, depth buffer for both eyes and setup widgets.
    renderer_->CreateRenderTexture(&render_textures_[CardboardEye::kLeft],
                                   screen_params_.viewport_width,
                                   screen_params_.viewport_height);
    renderer_->CreateRenderTexture(&render_textures_[CardboardEye::kRight],
                                   screen_params_.viewport_width,
                                   screen_params_.viewport_height);
    renderer_->SetupWidgets();

    // Set texture description structures.
    eye_data_[CardboardEye::kLeft].texture.texture =
        render_textures_[CardboardEye::kLeft].color_buffer;
    eye_data_[CardboardEye::kLeft].texture.left_u = 0;
    eye_data_[CardboardEye::kLeft].texture.right_u = 1;
    eye_data_[CardboardEye::kLeft].texture.top_v = 1;
    eye_data_[CardboardEye::kLeft].texture.bottom_v = 0;

    eye_data_[CardboardEye::kRight].texture.texture =
        render_textures_[CardboardEye::kRight].color_buffer;
    eye_data_[CardboardEye::kRight].texture.left_u = 0;
    eye_data_[CardboardEye::kRight].texture.right_u = 1;
    eye_data_[CardboardEye::kRight].texture.top_v = 1;
    eye_data_[CardboardEye::kRight].texture.bottom_v = 0;
  }

  // @brief Frees rendering resources.
  void RenderingResourcesTeardown() {
    if (render_textures_[0].color_buffer == 0) {
      return;
    }
    renderer_->DestroyRenderTexture(&render_textures_[CardboardEye::kLeft]);
    renderer_->DestroyRenderTexture(&render_textures_[CardboardEye::kRight]);
    renderer_->TeardownWidgets();
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
  // @details Must be used by rendering calls (or those to set up the pipeline).
  ScreenParams screen_params_;

  // @brief Eye data information.
  // @details `CardboardEye::kLeft` index holds left eye data and
  //          `CardboardEye::kRight` holds the right eye data.
  std::array<EyeData, 2> eye_data_;

  // @brief Holds the render texture information for each eye.
  std::array<Renderer::RenderTexture, 2> render_textures_;

  // @brief Manages the rendering elements lifecycle.
  std::unique_ptr<Renderer> renderer_;

  // @brief Store Unity reported screen params.
  static std::atomic<ScreenParams> unity_screen_params_;

  // @brief Unity-loaded widgets
  static std::vector<Renderer::WidgetParams> widget_params_;

  // @brief Mutex for widget_params_ access.
  static std::mutex widget_mutex_;

  // @brief Track changes to device parameters.
  static std::atomic<bool> device_params_changed_;

  // @brief Holds the selected graphics API.
  static std::atomic<CardboardGraphicsApi> selected_graphics_api_;

  // @brief Holds the Unity XR interfaces.
  static IUnityInterfaces* xr_interfaces_;

  // @brief Tracks head tracker recentering requests.
  static std::atomic<bool> head_tracker_recenter_requested_;
};

std::atomic<CardboardApi::CardboardApiImpl::ScreenParams>
    CardboardApi::CardboardApiImpl::unity_screen_params_({0, 0, 0, 0, 0, 0});

std::vector<Renderer::WidgetParams>
    CardboardApi::CardboardApiImpl::widget_params_;

std::mutex CardboardApi::CardboardApiImpl::widget_mutex_;

std::atomic<bool> CardboardApi::CardboardApiImpl::device_params_changed_(true);

std::atomic<CardboardGraphicsApi>
    CardboardApi::CardboardApiImpl::selected_graphics_api_(kNone);

IUnityInterfaces* CardboardApi::CardboardApiImpl::xr_interfaces_{nullptr};

std::atomic<bool>
    CardboardApi::CardboardApiImpl::head_tracker_recenter_requested_(false);

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

void CardboardApi::RenderEyesToDisplay() { p_impl_->RenderEyesToDisplay(); }

void CardboardApi::RenderWidgets() { p_impl_->RenderWidgets(); }

uint64_t CardboardApi::GetLeftTextureColorBufferId() {
  return p_impl_->GetLeftTextureColorBufferId();
}

uint64_t CardboardApi::GetRightTextureColorBufferId() {
  return p_impl_->GetRightTextureColorBufferId();
}

uint64_t CardboardApi::GetLeftTextureDepthBufferId() {
  return p_impl_->GetLeftTextureDepthBufferId();
}

uint64_t CardboardApi::GetRightTextureDepthBufferId() {
  return p_impl_->GetRightTextureDepthBufferId();
}

void CardboardApi::GetScreenParams(int* width, int* height) {
  CardboardApi::CardboardApiImpl::GetUnityScreenParams(width, height);
}

void CardboardApi::SetUnityScreenParams(int screen_width, int screen_height,
                                        int viewport_x, int viewport_y,
                                        int viewport_width,
                                        int viewport_height) {
  CardboardApi::CardboardApiImpl::SetUnityScreenParams(
      screen_width, screen_height, viewport_x, viewport_y, viewport_width,
      viewport_height);
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

void CardboardApi::SetWidgetParams(int i,
                                   const Renderer::WidgetParams& params) {
  CardboardApi::CardboardApiImpl::SetWidgetParams(i, params);
}

void CardboardApi::SetGraphicsApi(CardboardGraphicsApi graphics_api) {
  CardboardApi::CardboardApiImpl::SetGraphicsApi(graphics_api);
}

CardboardGraphicsApi CardboardApi::GetGraphicsApi() {
  return CardboardApi::CardboardApiImpl::GetGraphicsApi();
}

void CardboardApi::SetUnityInterfaces(IUnityInterfaces* xr_interfaces) {
  CardboardApi::CardboardApiImpl::SetUnityInterfaces(xr_interfaces);
}

void CardboardApi::SetHeadTrackerRecenterRequested() {
  CardboardApi::CardboardApiImpl::SetHeadTrackerRecenterRequested();
}

}  // namespace cardboard::unity

#ifdef __cplusplus
extern "C" {
#endif

void CardboardUnity_setScreenParams(int screen_width, int screen_height,
                                    int viewport_x, int viewport_y,
                                    int viewport_width, int viewport_height) {
  cardboard::unity::CardboardApi::SetUnityScreenParams(
      screen_width, screen_height, viewport_x, viewport_y, viewport_width,
      viewport_height);
}

void CardboardUnity_setDeviceParametersChanged() {
  cardboard::unity::CardboardApi::SetDeviceParametersChanged();
}

void CardboardUnity_setWidgetCount(int count) {
  cardboard::unity::CardboardApi::SetWidgetCount(count);
}

void CardboardUnity_setWidgetParams(int i, void* texture, int x, int y,
                                    int width, int height) {
  cardboard::unity::Renderer::WidgetParams params;

  params.texture = reinterpret_cast<intptr_t>(texture);
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
#if defined(__APPLE__)
    case CardboardGraphicsApi::kMetal:
      LOGD("Configured Metal as Graphics API.");
      cardboard::unity::CardboardApi::SetGraphicsApi(graphics_api);
      break;
#endif
    default:
      LOGE(
          "Misconfigured Graphics API. Neither OpenGL ES 2.0 nor OpenGL ES 3.0 "
          "nor Metal was selected.");
  }
}

void CardboardUnity_recenterHeadTracker() {
  cardboard::unity::CardboardApi::SetHeadTrackerRecenterRequested();
}

#ifdef __cplusplus
}
#endif
