/*
 * Copyright 2019 Google LLC
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
#ifndef CARDBOARD_SDK_SENSORS_SENSOR_FUSION_EKF_H_
#define CARDBOARD_SDK_SENSORS_SENSOR_FUSION_EKF_H_

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>  // NOLINT

#include "sensors/accelerometer_data.h"
#include "sensors/gyroscope_bias_estimator.h"
#include "sensors/gyroscope_data.h"
#include "sensors/lowpass_filter.h"
#include "sensors/rotation_state.h"
#include "util/matrix_3x3.h"
#include "util/rotation.h"
#include "util/vector.h"

namespace cardboard {

// Sensor fusion class that implements an Extended Kalman Filter (EKF) to
// estimate a 3D rotation from a gyroscope and an accelerometer.
// This system only has one state, the rotation. It does not estimate any
// velocity or acceleration.
//
// To learn more about Kalman filtering one can read this article which is a
// good introduction: https://en.wikipedia.org/wiki/Kalman_filter
class SensorFusionEkf {
 public:
  SensorFusionEkf();

  // Resets the state of the sensor fusion. It sets the velocity for
  // prediction to zero. The reset will happen with the next
  // accelerometer sample. Gyroscope sample will be discarded until a new
  // accelerometer sample arrives.
  void Reset();

  // Gets the RotationState representing the latest rotation and angular
  // velocity at a particular timestamp as estimated by SensorFusion.
  RotationState GetLatestRotationState() const;

  // Gets a predicted rotation for a given time in the future (e.g. rendering
  // time) based on a linear prediction model (this EKF implementation). It uses
  // the system current rotation state (position, velocity, etc.) from the past
  // to extrapolate a position in the future.
  //
  // @param requested_timestamp time at which you want the rotation.
  // @return If the requested timestamp is equal to zero, it returns the current
  //         rotation. Otherwise, it returns the rotation from Start to Sensor
  //         Space.
  Rotation PredictRotation(int64_t requested_timestamp) const;

  // Processes one gyroscope sample event. This updates the rotation of the
  // system and the prediction model. The gyroscope data is assumed to be in
  // axis angle form. Angle = ||v|| and Axis = v / ||v||, with
  // v = [v_x, v_y, v_z]^T.
  //
  // @param sample gyroscope sample data.
  void ProcessGyroscopeSample(const GyroscopeData& sample);

  // Processes one accelerometer sample event. This updates the rotation of the
  // system. If the Accelerometer norm changes too much between sample it is not
  // trusted as much.
  //
  // @param sample accelerometer sample data.
  void ProcessAccelerometerSample(const AccelerometerData& sample);

  // Rotates the current transformation from Sensor Space to Start Space.
  //
  // @details The current state space rotation is post-multiplied by
  //          @p rotation.
  //          Typically used when a viewport orientation changes.
  //
  // @param rotation The Rotation that maps from the Sensor Space
  //                 frame to Start Space.
  void RotateSensorSpaceToStartSpaceTransformation(const Rotation& rotation);

  // Sets the low pass filter of the head tracker with the given cut-off
  // frequency.
  //
  // @param cutoff_frequency Cutoff frequency for the low-pass filter of the
  // head tracker.
  void SetLowPassFilter(int velocity_filter_cutoff_frequency);

 private:
  // Estimates the average timestep between gyroscope event.
  void FilterGyroscopeTimestep(double gyroscope_timestep);

  // Updates the state covariance with an incremental motion. It changes the
  // space of the quadric.
  void UpdateStateCovariance(const Matrix3x3& motion_update);

  // Computes the innovation vector of the Kalman based on the input rotation.
  // It uses the latest measurement vector (i.e. accelerometer data), which must
  // be set prior to calling this function.
  Vector3 ComputeInnovation(const Rotation& rotation_in);

  // This computes the measurement_jacobian_ via numerical differentiation based
  // on the current value of sensor_from_start_rotation_.
  void ComputeMeasurementJacobian();

  // Updates the accelerometer covariance matrix.
  //
  // This looks at the norm of recent accelerometer readings. If it has changed
  // significantly, it means the phone receives additional acceleration than
  // just gravity, and so the down vector information gravity signal is noisier.
  void UpdateMeasurementCovariance();

  // Reset all internal states. This is not thread safe. Lock should be acquired
  // outside of it. This function is called in ProcessAccelerometerSample.
  void ResetState();

  // Current transformation from Sensor Space to Start Space.
  // x_sensor = sensor_from_start_rotation_ * x_start;
  RotationState current_state_;

  // Filtering of the gyroscope timestep started?
  bool is_timestep_filter_initialized_;
  // Filtered gyroscope timestep valid?
  bool is_gyroscope_filter_valid_;
  // Sensor fusion currently aligned with gravity? After initialization
  // it will requires a couple of accelerometer data for the system to get
  // aligned.
  std::atomic<bool> is_aligned_with_gravity_;

  // Covariance of Kalman filter state (P in common formulation).
  Matrix3x3 state_covariance_;
  // Covariance of the process noise (Q in common formulation).
  Matrix3x3 process_covariance_;
  // Covariance of the accelerometer measurement (R in common formulation).
  Matrix3x3 accelerometer_measurement_covariance_;
  // Covariance of innovation (S in common formulation).
  Matrix3x3 innovation_covariance_;
  // Jacobian of the measurements (H in common formulation).
  Matrix3x3 accelerometer_measurement_jacobian_;
  // Gain of the Kalman filter (K in common formulation).
  Matrix3x3 kalman_gain_;
  // Parameter update a.k.a. innovation vector. (\nu in common formulation).
  Vector3 innovation_;
  // Measurement vector (z in common formulation).
  Vector3 accelerometer_measurement_;
  // Current prediction vector (g in common formulation).
  Vector3 prediction_;
  // Control input, currently this is only the gyroscope data (\mu in common
  // formulation).
  Vector3 control_input_;
  // Update of the state vector. (x in common formulation).
  Vector3 state_update_;

  // Sensor time of the last gyroscope processed event.
  uint64_t current_gyroscope_sensor_timestamp_ns_;
  // Sensor time of the last accelerometer processed event.
  uint64_t current_accelerometer_sensor_timestamp_ns_;

  // Estimates of the timestep between gyroscope event in seconds.
  double filtered_gyroscope_timestep_s_;
  // Number of timestep samples processed so far by the filter.
  uint32_t num_gyroscope_timestep_samples_;
  // Norm of the accelerometer for the previous measurement.
  double previous_accelerometer_norm_;
  // Moving average of the accelerometer norm changes. It is computed for every
  // sensor datum.
  double moving_average_accelerometer_norm_change_;

  // Flag indicating if a state reset should be executed with the next
  // accelerometer sample.
  std::atomic<bool> execute_reset_with_next_accelerometer_sample_;

  mutable std::mutex mutex_;

  // Bias estimator and static device detector.
  GyroscopeBiasEstimator gyroscope_bias_estimator_;

  // Current bias estimate_;
  Vector3 gyroscope_bias_estimate_;

  // Filter to smooth velocity vector
  std::unique_ptr<LowpassFilter> velocity_filter_;

  SensorFusionEkf(const SensorFusionEkf&) = delete;
  SensorFusionEkf& operator=(const SensorFusionEkf&) = delete;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_SENSORS_SENSOR_FUSION_EKF_H_
