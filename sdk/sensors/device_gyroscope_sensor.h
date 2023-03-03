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
#ifndef CARDBOARD_SDK_SENSORS_DEVICE_GYROSCOPE_SENSOR_H_
#define CARDBOARD_SDK_SENSORS_DEVICE_GYROSCOPE_SENSOR_H_

#include <memory>
#include <vector>

#include "sensors/gyroscope_data.h"
#include "util/vector.h"

namespace cardboard {

// Wrapper class that reads gyroscope sensor data from the native sensor
// framework.
class DeviceGyroscopeSensor {
 public:
  DeviceGyroscopeSensor();

  ~DeviceGyroscopeSensor();

  // Starts the sensor capture process.
  // This must be called successfully before calling PollForSensorData().
  //
  // @return false if the requested sensor is not supported.
  bool Start();

  // Actively waits up to timeout_ms and polls for sensor data. If
  // timeout_ms < 0, it waits indefinitely until sensor data is
  // available.
  // This must only be called after a successful call to Start() was made.
  //
  // @param timeout_ms timeout period in milliseconds.
  // @param results list of events emitted by the sensor.
  void PollForSensorData(int timeout_ms,
                         std::vector<GyroscopeData>* results) const;

  // Stops the sensor capture process.
  void Stop();

  // The implementation of device sensors differs between iOS and Android.
  struct SensorInfo;

 private:
  const std::unique_ptr<SensorInfo> sensor_info_;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_SENSORS_DEVICE_GYROSCOPE_SENSOR_H_
