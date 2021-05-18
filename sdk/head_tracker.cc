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

#include "sensors/neck_model.h"
#include "util/logging.h"
#include "util/vector.h"
#include "util/vectorutils.h"
#include "screen_params.h"

namespace cardboard {

// Aryzon 6DoF
const int rotation_samples = 10;
const int position_samples = 6;
const int64_t max6DoFTimeDifference = 200000000; // Maximum time difference between last pose state timestamp and last 6DoF timestamp, if it takes longer than this the last known location of sixdof will be used
const float reduceBiasRate = 0.05;

HeadTracker::HeadTracker()
    : is_tracking_(false),
      sensor_fusion_(new SensorFusionEkf()),
      latest_gyroscope_data_({0, 0, Vector3::Zero()}),
      accel_sensor_(new SensorEventProducer<AccelerometerData>()),
      gyro_sensor_(new SensorEventProducer<GyroscopeData>()),
      // Aryzon 6DoF
      rotation_data_(new RotationData(rotation_samples)),
      position_data_(new PositionData(position_samples)) {

  on_accel_callback_ = [&](const AccelerometerData& event) {
    OnAccelerometerData(event);
  };
  on_gyro_callback_ = [&](const GyroscopeData& event) {
    OnGyroscopeData(event);
  };
                 
  switch(screen_params::getScreenOrientation()) {
    case kLandscapeLeft:
        ekf_to_head_tracker_ = Rotation::FromYawPitchRoll(-M_PI / 2.0, 0, -M_PI / 2.0);
      break;
    case kLandscapeRight:
        ekf_to_head_tracker_ = Rotation::FromYawPitchRoll(M_PI / 2.0, 0, M_PI / 2.0);
      break;
    default: // Portrait and PortraitUpsideDown
        ekf_to_head_tracker_ = Rotation::FromYawPitchRoll(M_PI / 2.0, M_PI / 2.0, M_PI / 2.0);
      break;
  }
  difference_to_6DoF_ = Rotation::Identity();
  y_bias_ = 0;
}

HeadTracker::~HeadTracker() { UnregisterCallbacks(); }

void HeadTracker::Pause() {
  if (!is_tracking_) {
    return;
  }

  /*UnregisterCallbacks();

  // Create a gyro event with zero velocity. This effectively stops the
  // prediction.
  GyroscopeData event = latest_gyroscope_data_;
  event.data = Vector3::Zero();

  OnGyroscopeData(event);

  is_tracking_ = false;
  y_bias_ = 0;
  initialised_6dof_ = false;*/
}

void HeadTracker::Resume() {
  if (!is_tracking_) {
    RegisterCallbacks();
  }
  is_tracking_ = true;
}

void HeadTracker::GetPose(int64_t timestamp_ns,
                          std::array<float, 3>& out_position,
                          std::array<float, 4>& out_orientation) const {
    
    Rotation sensor_to_display;

    switch(screen_params::getScreenOrientation()) {
      case kLandscapeLeft:
        sensor_to_display = Rotation::FromAxisAndAngle(Vector3(0, 0, 1), M_PI / 2.0);
        break;
      case kLandscapeRight:
        sensor_to_display = Rotation::FromAxisAndAngle(Vector3(0, 0, 1), -M_PI / 2.0);
        break;
      default: // Portrait and PortraitUpsideDown
        sensor_to_display = Rotation::FromAxisAndAngle(Vector3(0, 0, 1), 0.);
        break;
    }
    
  Rotation predicted_rotation = sensor_fusion_->PredictRotation(timestamp_ns);

  const RotationState rotation_state = sensor_fusion_->GetLatestRotationState();
    // Save rotation sample with timestamp to be used in AddSixDoFData()
    rotation_data_->AddSample((sensor_to_display * predicted_rotation *
                              ekf_to_head_tracker_).GetQuaternion(), timestamp_ns);
    
  if (position_data_->IsValid() && rotation_state.timestamp - position_data_->GetLatestTimestamp() < max6DoFTimeDifference) {
      
    // 6DoF is recently updated
    Vector3 p = position_data_->GetExtrapolatedForTimeStamp(timestamp_ns);
      //printf("GetPose\t%llu\t%f\t%f\t%f\n",timestamp_ns, p[0], p[1], p[2]);
    std::array<float, 3> predicted_position_ = {(float)p[0], (float)p[1], (float)p[2]};
    const Rotation orientationRotation = (sensor_to_display * predicted_rotation *
                                            ekf_to_head_tracker_);
       
    const Vector4 orientation = (orientationRotation * -difference_to_6DoF_).GetQuaternion();
      
    out_orientation[0] = static_cast<float>(orientation[0]);
    out_orientation[1] = static_cast<float>(orientation[1]);
    out_orientation[2] = static_cast<float>(orientation[2]);
    out_orientation[3] = static_cast<float>(orientation[3]);
        
    out_position = predicted_position_;
  } else {
    // 6DoF is not recently updated
    const Vector4 orientation = (sensor_to_display * predicted_rotation *
                                 ekf_to_head_tracker_)
                                    .GetQuaternion();
      
    out_orientation[0] = static_cast<float>(orientation[0]);
    out_orientation[1] = static_cast<float>(orientation[1]);
    out_orientation[2] = static_cast<float>(orientation[2]);
    out_orientation[3] = static_cast<float>(orientation[3]);
        
    out_position = ApplyNeckModel(out_orientation, 1.0);
    if (position_data_->IsValid()) {
        // Apply last known 6DoF position if 6DoF data was previously added, while still applying neckmodel.
          
        Vector3 last_known_position_ = position_data_->GetLatestData();
        out_position[0] += (float)last_known_position_[0];
        out_position[1] += (float)last_known_position_[1];
        out_position[2] += (float)last_known_position_[2];
    }
  }
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

// Aryzon 6DoF
void HeadTracker::AddSixDoFData(int64_t timestamp_ns, float* pos, float* orientation) {
  if (!is_tracking_) {
    return;
  }
    if (position_data_->GetLatestTimestamp() != timestamp_ns) {
        position_data_->AddSample(Vector3(pos[0], pos[1], pos[2]), timestamp_ns);
    }
    
    // Do not add data if no new data is available!
    if (position_data_->IsValid() && rotation_data_->IsValid()) {
        // 6DoF timestamp should be before the latest rotation_data timestamp
        const Rotation gyroAtTimeOfSixDoF = Rotation::FromQuaternion(rotation_data_->GetInterpolatedForTimeStamp(timestamp_ns));
        const Rotation sixDoFRotation = Rotation::FromQuaternion(Vector4(orientation[0], orientation[1], orientation[2], orientation[3]));
        
        Vector4 difference = (gyroAtTimeOfSixDoF * -sixDoFRotation).GetQuaternion();
        double yDifference = 0;
        
        // Extract y euler angle from quaternion
        const double sinp = 2 * (difference[3] * difference[1] - difference[2] * difference[0]);
        if (std::abs(sinp) >= 1) {
            yDifference = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
        } else {
            yDifference = std::asin(sinp);
        }
        
        if (yDifference > M_PI_2) {
            yDifference -= M_PI;
        } else if (yDifference < -M_PI_2) {
            yDifference += M_PI;
        }
        
        y_bias_ += (yDifference - y_bias_) * reduceBiasRate;
        difference_to_6DoF_ = Rotation::FromRollPitchYaw(0, 0, y_bias_);
    }
}
}  // namespace cardboard
