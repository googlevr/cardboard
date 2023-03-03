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
#import "sensors/device_gyroscope_sensor.h"

#import <CoreMotion/CoreMotion.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

#import <sys/types.h>
#import <thread>

#import "sensors/gyroscope_data.h"
#import "sensors/ios/sensor_helper.h"
#import "util/vector.h"

namespace cardboard {
static const int64_t kNsecPerSec = 1000000000;

// This struct holds gyroscope specific sensor information.
struct DeviceGyroscopeSensor::SensorInfo {
};

DeviceGyroscopeSensor::DeviceGyroscopeSensor() : sensor_info_(new SensorInfo()) {}

DeviceGyroscopeSensor::~DeviceGyroscopeSensor() {}

void DeviceGyroscopeSensor::PollForSensorData(int /*timeout_ms*/,
                                              std::vector<GyroscopeData>* results) const {
  results->clear();
  @autoreleasepool {
    CardboardSensorHelper* helper = [CardboardSensorHelper sharedSensorHelper];
    const float x = static_cast<float>(helper.deviceMotion.rotationRate.x);
    const float y = static_cast<float>(helper.deviceMotion.rotationRate.y);
    const float z = static_cast<float>(helper.deviceMotion.rotationRate.z);

    GyroscopeData sample;
    uint64_t nstime = helper.deviceMotion.timestamp * kNsecPerSec;
    sample.sensor_timestamp_ns = nstime;
    sample.system_timestamp = nstime;
    sample.data.Set(x, y, z);
    results->push_back(sample);
  }
}

bool DeviceGyroscopeSensor::Start() {
  return [[CardboardSensorHelper sharedSensorHelper] isGyroAvailable];
}

void DeviceGyroscopeSensor::Stop() {
  // This should never be called on iOS.
}

}  // namespace cardboard
