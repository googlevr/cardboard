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
#include "head_tracker.h"

#include "include/cardboard.h"
#include "sensors/neck_model.h"
#include "util/logging.h"
#include "util/rotation.h"
#include "util/vector.h"
#include "util/vectorutils.h"

namespace cardboard {

const std::array<Rotation, 4> HeadTracker::kEkfToHeadTrackerRotations{
    // LandscapeLeft: This is the same than initializing the rotation from
    // Rotation::FromYawPitchRoll(-M_PI / 2., 0, -M_PI / 2.).
    Rotation::FromQuaternion(Rotation::QuaternionType(0.5, -0.5, -0.5, 0.5)),
    // LandscapeRight: This is the same than initializing the rotation from
    // Rotation::FromYawPitchRoll(M_PI / 2., 0, M_PI / 2.).
    Rotation::FromQuaternion(Rotation::QuaternionType(0.5, 0.5, 0.5, 0.5)),
    // Portrait: This is the same than initializing the rotation from
    // Rotation::FromYawPitchRoll(M_PI / 2., M_PI / 2., M_PI / 2.).
    Rotation::FromQuaternion(Rotation::QuaternionType(0.7071067811865476, 0.,
                                                      0., 0.7071067811865476)),
    // Portrait upside down: This is the same than initializing the rotation
    // from Rotation::FromYawPitchRoll(-M_PI / 2., -M_PI / 2., -M_PI / 2.).
    Rotation::FromQuaternion(Rotation::QuaternionType(
        0., -0.7071067811865476, -0.7071067811865476, 0.))};

const std::array<Rotation, 4> HeadTracker::kSensorToDisplayRotations{
    // LandscapeLeft: This is the same than initializing the rotation from
    // Rotation::FromAxisAndAngle(Vector3(0., 0., 1.), M_PI / 2.).
    Rotation::FromQuaternion(Rotation::QuaternionType(
        0., 0., 0.7071067811865476, 0.7071067811865476)),
    // LandscapeRight: This is the same than initializing the rotation from
    // Rotation::FromAxisAndAngle(Vector3(0., 0., 1.), -M_PI / 2.).
    Rotation::FromQuaternion(Rotation::QuaternionType(
        0., 0., -0.7071067811865476, 0.7071067811865476)),
    // Portrait: This is the same than initializing the rotation from
    // Rotation::FromAxisAndAngle(Vector3(0., 0., 1.), 0.).
    Rotation::FromQuaternion(Rotation::QuaternionType(0., 0., 0., 1.)),
    // PortaitUpsideDown: This is the same than initializing the rotation from
    // Rotation::FromAxisAndAngle(Vector3(0., 0., 1.), M_PI).
    Rotation::FromQuaternion(Rotation::QuaternionType(0., 0., 1., 0.))};

const std::array<std::array<Rotation, 4>, 4>
    HeadTracker::kViewportChangeRotationCompensation{{
        // Landscape left.
        {Rotation::Identity(), Rotation::FromYawPitchRoll(0, 0, M_PI),
         Rotation::FromYawPitchRoll(0, 0, -M_PI / 2),
         Rotation::FromYawPitchRoll(0, 0, M_PI / 2)},
        // Landscape Right.
        {Rotation::FromYawPitchRoll(0, 0, M_PI), Rotation::Identity(),
         Rotation::FromYawPitchRoll(0, 0, M_PI / 2),
         Rotation::FromYawPitchRoll(0, 0, -M_PI / 2)},
        // Portrait.
        {Rotation::FromYawPitchRoll(0, 0, M_PI / 2),
         Rotation::FromYawPitchRoll(0, 0, -M_PI / 2), Rotation::Identity(),
         Rotation::FromYawPitchRoll(0, 0, M_PI)},
        // Portrait Upside Down.
        {Rotation::FromYawPitchRoll(0, 0, -M_PI / 2),
         Rotation::FromYawPitchRoll(0, 0, M_PI / 2),
         Rotation::FromYawPitchRoll(0, 0, M_PI), Rotation::Identity()},
    }};

HeadTracker::HeadTracker()
    : is_tracking_(false),
      sensor_fusion_(new SensorFusionEkf()),
      latest_gyroscope_data_({0, 0, Vector3::Zero()}),
      accel_sensor_(new SensorEventProducer<AccelerometerData>()),
      gyro_sensor_(new SensorEventProducer<GyroscopeData>()),
      is_viewport_orientation_initialized_(false) {
  on_accel_callback_ = [&](const AccelerometerData& event) {
    OnAccelerometerData(event);
  };
  on_gyro_callback_ = [&](const GyroscopeData& event) {
    OnGyroscopeData(event);
  };
}

HeadTracker::~HeadTracker() { UnregisterCallbacks(); }

void HeadTracker::Pause() {
  if (!is_tracking_) {
    return;
  }

  UnregisterCallbacks();

  // Create a gyro event with zero velocity. This effectively stops the
  // prediction.
  GyroscopeData event = latest_gyroscope_data_;
  event.data = Vector3::Zero();

  OnGyroscopeData(event);

  is_tracking_ = false;
}

void HeadTracker::Resume() {
  is_tracking_ = true;
  RegisterCallbacks();
}

void HeadTracker::GetPose(int64_t timestamp_ns,
                          CardboardViewportOrientation viewport_orientation,
                          std::array<float, 3>& out_position,
                          std::array<float, 4>& out_orientation) {
  const Vector4 orientation =
      GetRotation(viewport_orientation, timestamp_ns).GetQuaternion();

  if (is_viewport_orientation_initialized_ &&
      viewport_orientation != viewport_orientation_) {
    sensor_fusion_->RotateSensorSpaceToStartSpaceTransformation(
        kViewportChangeRotationCompensation[viewport_orientation_]
                                           [viewport_orientation]);
  }
  viewport_orientation_ = viewport_orientation;
  is_viewport_orientation_initialized_ = true;

  out_orientation[0] = static_cast<float>(orientation[0]);
  out_orientation[1] = static_cast<float>(orientation[1]);
  out_orientation[2] = static_cast<float>(orientation[2]);
  out_orientation[3] = static_cast<float>(orientation[3]);

  out_position = ApplyNeckModel(out_orientation, 1.0);
}

void HeadTracker::Recenter() {
  sensor_fusion_->Reset();
}

void HeadTracker::RegisterCallbacks() {
  accel_sensor_->StartSensorPolling(&on_accel_callback_);
  gyro_sensor_->StartSensorPolling(&on_gyro_callback_);
}

void HeadTracker::UnregisterCallbacks() {
  accel_sensor_->StopSensorPolling();
  gyro_sensor_->StopSensorPolling();
}

void HeadTracker::OnAccelerometerData(const AccelerometerData& event) {
  if (!is_tracking_) {
    return;
  }
  sensor_fusion_->ProcessAccelerometerSample(event);
}

void HeadTracker::OnGyroscopeData(const GyroscopeData& event) {
  if (!is_tracking_) {
    return;
  }
  latest_gyroscope_data_ = event;
  sensor_fusion_->ProcessGyroscopeSample(event);
}

Rotation HeadTracker::GetRotation(
    CardboardViewportOrientation viewport_orientation,
    int64_t timestamp_ns) const {
  const Rotation predicted_rotation =
      sensor_fusion_->PredictRotation(timestamp_ns);

  // In order to update our pose as the sensor changes, we begin with the
  // inverse default orientation (the orientation returned by a reset sensor,
  // i.e. since the last Reset() call), apply the current sensor transformation,
  // and then transform into display space.
  return kSensorToDisplayRotations[viewport_orientation] * predicted_rotation *
         kEkfToHeadTrackerRotations[viewport_orientation];
}

}  // namespace cardboard
