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
#include "unity/xr_unity_plugin/cardboard_input_api.h"

#include <atomic>
#include <cstdint>

#include "include/cardboard.h"

// The following block makes log macros available for Android and iOS.
#if defined(__ANDROID__)
#include <android/log.h>
#define LOG_TAG "CardboardInputApi"
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

std::atomic<CardboardViewportOrientation>
    CardboardInputApi::selected_viewport_orientation_(kLandscapeLeft);

std::atomic<bool> CardboardInputApi::head_tracker_recenter_requested_(false);

std::atomic<int> CardboardInputApi::new_lowpass_filter_cutoff_frequency_(0);

void CardboardInputApi::InitHeadTracker() {
  if (head_tracker_ == nullptr) {
    head_tracker_.reset(CardboardHeadTracker_create());
  }
  CardboardHeadTracker_resume(head_tracker_.get());
}

void CardboardInputApi::PauseHeadTracker() {
  if (head_tracker_ == nullptr) {
    LOGW("Uninitialized head tracker was paused.");
    return;
  }
  CardboardHeadTracker_pause(head_tracker_.get());
}

void CardboardInputApi::ResumeHeadTracker() {
  if (head_tracker_ == nullptr) {
    LOGW("Uninitialized head tracker was resumed.");
    return;
  }
  CardboardHeadTracker_resume(head_tracker_.get());
}

void CardboardInputApi::GetHeadTrackerPose(float* position,
                                           float* orientation) {
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

  // Checks whether a head tracker set low-pass filter has been requested.
  // If it has been requested, the head tracker will reset with the velocity
  // filter.
  if (new_lowpass_filter_cutoff_frequency_ !=
      lowpass_filter_cutoff_frequency_) {
    lowpass_filter_cutoff_frequency_ = new_lowpass_filter_cutoff_frequency_;
    CardboardHeadTracker_setLowPassFilter(head_tracker_.get(),
                                          lowpass_filter_cutoff_frequency_);
  }

  // Checks whether a head tracker recentering has been requested.
  if (head_tracker_recenter_requested_) {
    CardboardHeadTracker_recenter(head_tracker_.get());
    head_tracker_recenter_requested_ = false;
  }

  CardboardHeadTracker_getPose(
      head_tracker_.get(), GetBootTimeNano() + kPredictionTimeWithoutVsyncNanos,
      selected_viewport_orientation_, position, orientation);
}

void CardboardInputApi::SetViewportOrientation(
    CardboardViewportOrientation viewport_orientation) {
  selected_viewport_orientation_ = viewport_orientation;
}

void CardboardInputApi::SetHeadTrackerRecenterRequested() {
  head_tracker_recenter_requested_ = true;
}

void CardboardInputApi::SetHeadTrackerLowPassFilterRequested(
    const int cutoff_frequency) {
  new_lowpass_filter_cutoff_frequency_ = cutoff_frequency;
}

int64_t CardboardInputApi::GetBootTimeNano() {
  struct timespec res;
#if defined(__ANDROID__)
  clock_gettime(CLOCK_BOOTTIME, &res);
#elif defined(__APPLE__)
  clock_gettime(CLOCK_UPTIME_RAW, &res);
#endif
  return (res.tv_sec * CardboardInputApi::kNanosInSeconds) + res.tv_nsec;
}

}  // namespace cardboard::unity

#ifdef __cplusplus
extern "C" {
#endif

void CardboardUnity_setViewportOrientation(
    CardboardViewportOrientation viewport_orientation) {
  switch (viewport_orientation) {
    case CardboardViewportOrientation::kLandscapeLeft:
      LOGD("Configured viewport orientation as landscape left.");
      cardboard::unity::CardboardInputApi::SetViewportOrientation(
          viewport_orientation);
      break;
    case CardboardViewportOrientation::kLandscapeRight:
      LOGD("Configured viewport orientation as landscape right.");
      cardboard::unity::CardboardInputApi::SetViewportOrientation(
          viewport_orientation);
      break;
    case CardboardViewportOrientation::kPortrait:
      LOGD("Configured viewport orientation as portrait.");
      cardboard::unity::CardboardInputApi::SetViewportOrientation(
          viewport_orientation);
      break;
    case CardboardViewportOrientation::kPortraitUpsideDown:
      LOGD("Configured viewport orientation as portrait upside down.");
      cardboard::unity::CardboardInputApi::SetViewportOrientation(
          viewport_orientation);
      break;
    default:
      LOGE(
          "Misconfigured viewport orientation. Neither landscape left nor "
          "lanscape right "
          "nor portrait, nor portrait upside down was selected.");
  }
}

void CardboardUnity_recenterHeadTracker() {
  cardboard::unity::CardboardInputApi::SetHeadTrackerRecenterRequested();
}

void CardboardUnity_setHeadTrackerLowPassFilter(
    const int velocity_filter_cutoff_frequency) {
  cardboard::unity::CardboardInputApi::SetHeadTrackerLowPassFilterRequested(
      velocity_filter_cutoff_frequency);
}

#ifdef __cplusplus
}
#endif
