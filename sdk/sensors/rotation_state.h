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
#ifndef CARDBOARD_SDK_SENSORS_ROTATION_STATE_H_
#define CARDBOARD_SDK_SENSORS_ROTATION_STATE_H_

#include "util/rotation.h"
#include "util/vector.h"

namespace cardboard {

// Stores a rotation and the angular velocity measured in the sensor space.
// It can be used for prediction.
struct RotationState {
  // System wall time. It is measured in nanoseconds.
  int64_t timestamp;

  // Rotation from Sensor Space to Start Space. It is measured in radians (rad).
  Rotation sensor_from_start_rotation;

  // First derivative of the rotation. It is measured in radians per second
  // (rad/s).
  Vector3 sensor_from_start_rotation_velocity;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_SENSORS_ROTATION_STATE_H_
