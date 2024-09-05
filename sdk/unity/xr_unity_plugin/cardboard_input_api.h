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
#ifndef CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_INPUT_API_H_
#define CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_INPUT_API_H_

#include <atomic>
#include <cstdint>
#include <memory>

#include "include/cardboard.h"

namespace cardboard::unity {

/// Minimalistic wrapper of Cardboard SDK with native and standard types which
/// hides Cardboard types and exposes enough functionality of the head tracker
/// to be consumed by the InputProvider of a XR Unity plugin.
class CardboardInputApi {
 public:
  CardboardInputApi() = default;
  ~CardboardInputApi() = default;

  /// @brief Initializes and resumes the HeadTracker module.
  /// @pre Requires a prior call to @c Cardboard_initializeAndroid on Android
  ///      devices.
  void InitHeadTracker();

  /// @brief Pauses the HeadTracker module.
  void PauseHeadTracker();

  /// @brief Resumes the HeadTracker module.
  void ResumeHeadTracker();

  /// @brief Gets the pose of the HeadTracker module.
  /// @details When the HeadTracker has not been initialized, @p position and
  ///          @p rotation are zeroed.
  /// @param[out] position A pointer to an array with three floats to fill in
  ///             the position of the head.
  /// @param[out] orientation A pointer to an array with four floats to fill in
  ///             the quaternion that denotes the orientation of the head.
  // TODO(b/154305848): Move argument types to std::array*.
  void GetHeadTrackerPose(float* position, float* orientation);

  /// @brief Sets the viewport orientation that will be used.
  /// @param viewport_orientation one of the possible orientations of the
  /// viewport.
  static void SetViewportOrientation(
      CardboardViewportOrientation viewport_orientation);

  /// @brief Flags a head tracker recentering request.
  static void SetHeadTrackerRecenterRequested();

  /// @brief Flags a set head tracker low pass filter request.
  static void SetHeadTrackerLowPassFilterRequested(int cutoff_frequency);

 private:
  // @brief Custom deleter for HeadTracker.
  struct CardboardHeadTrackerDeleter {
    void operator()(CardboardHeadTracker* head_tracker) {
      CardboardHeadTracker_destroy(head_tracker);
    }
  };

  // @brief Computes the system boot time in nanoseconds.
  // @return The system boot time count in nanoseconds.
  static int64_t GetBootTimeNano();

  // @brief Default prediction excess time in nano seconds.
  static constexpr int64_t kPredictionTimeWithoutVsyncNanos = 50000000;

  // @brief Constant to convert seconds into nano seconds.
  static constexpr int64_t kNanosInSeconds = 1000000000;

  // @brief HeadTracker native pointer.
  std::unique_ptr<CardboardHeadTracker, CardboardHeadTrackerDeleter>
      head_tracker_;

  // @brief Holds the selected viewport orientation.
  static std::atomic<CardboardViewportOrientation>
      selected_viewport_orientation_;

  // @brief Tracks head tracker recentering requests.
  static std::atomic<bool> head_tracker_recenter_requested_;

  // @brief Current cut-off Frequency for low-pass filter.
  int lowpass_filter_cutoff_frequency_;

  // @brief New cut-off Frequency for low-pass filter.
  static std::atomic<int> new_lowpass_filter_cutoff_frequency_;
};

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Sets the orientation of the device viewport to use.
/// @param viewport_orientation The orientation of the viewport to use.
void CardboardUnity_setViewportOrientation(
    CardboardViewportOrientation viewport_orientation);

/// @brief Flags a head tracker recentering request.
void CardboardUnity_recenterHeadTracker();

/// @brief Resets the head tracker with a low pass filter to stabilize the pose.
void CardboardUnity_setHeadTrackerLowPassFilter(
    int velocity_filter_cutoff_frequency);

#ifdef __cplusplus
}
#endif

}  // namespace cardboard::unity

#endif  // CARDBOARD_SDK_UNITY_XR_UNITY_PLUGIN_CARDBOARD_INPUT_API_H_
