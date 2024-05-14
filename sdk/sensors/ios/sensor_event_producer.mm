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
#import "sensors/sensor_event_producer.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include <atomic>
#include <vector>

#import "sensors/accelerometer_data.h"
#import "sensors/device_accelerometer_sensor.h"
#import "sensors/device_gyroscope_sensor.h"
#import "sensors/gyroscope_data.h"
#import "sensors/ios/sensor_helper.h"

namespace cardboard {

template <typename T>
struct DeviceSensor {};

template <>
struct DeviceSensor<AccelerometerData> {
  std::unique_ptr<DeviceAccelerometerSensor> value;
};

template <>
struct DeviceSensor<GyroscopeData> {
  std::unique_ptr<DeviceGyroscopeSensor> value;
};

template <typename DataType>
struct SensorEventProducer<DataType>::EventProducer {
  EventProducer() : run_thread(false) {}

  // Sensor to poll for data.
  DeviceSensor<DataType> sensor;
  // Data obtained from each poll of the sensor.
  std::vector<DataType> sensor_events_vec;
  // Flag indicating if the capture thread should run.
  std::atomic<bool> run_thread;
  // Block that invokes the WorkFn.
  void (^workfn_block)(void);
};

template <typename DataType>

SensorEventProducer<DataType>::SensorEventProducer() : event_producer_(new EventProducer()) {}

template <typename DataType>
SensorEventProducer<DataType>::~SensorEventProducer() {
  StopSensorPolling();
}

template <>
void SensorEventProducer<AccelerometerData>::StartSensorPolling(
    const std::function<void(AccelerometerData)>* on_event_callback) {
  on_event_callback_ = on_event_callback;

  // If the thread is started already there is nothing left to do.
  if (event_producer_->run_thread.exchange(true)) {
    return;
  }

  event_producer_->sensor.value.reset(new DeviceAccelerometerSensor());
  if (!event_producer_->sensor.value->Start()) {
    event_producer_->run_thread = false;
    return;
  }

  event_producer_->workfn_block = ^{
    WorkFn();
  };

  [[CardboardSensorHelper sharedSensorHelper] start:SensorHelperTypeAccelerometer
                                           callback:event_producer_->workfn_block];
}

template <>
void SensorEventProducer<GyroscopeData>::StartSensorPolling(
    const std::function<void(GyroscopeData)>* on_event_callback) {
  on_event_callback_ = on_event_callback;
  // If the thread is started already there is nothing left to do.
  if (event_producer_->run_thread.exchange(true)) {
    return;
  }

  event_producer_->sensor.value.reset(new DeviceGyroscopeSensor());
  if (!event_producer_->sensor.value->Start()) {
    event_producer_->run_thread = false;
    return;
  }

  event_producer_->workfn_block = ^{
    WorkFn();
  };

  [[CardboardSensorHelper sharedSensorHelper] start:SensorHelperTypeGyro
                                           callback:event_producer_->workfn_block];
}

template <>
void SensorEventProducer<AccelerometerData>::StopSensorPolling() {
  // If the thread is already stop nothing needs to be done.
  if (!event_producer_->run_thread.exchange(false)) {
    return;
  }

  [[CardboardSensorHelper sharedSensorHelper] stop:SensorHelperTypeAccelerometer
                                          callback:event_producer_->workfn_block];
}

template <>
void SensorEventProducer<GyroscopeData>::StopSensorPolling() {
  // If the thread is already stop nothing needs to be done.
  if (!event_producer_->run_thread.exchange(false)) {
    return;
  }

  [[CardboardSensorHelper sharedSensorHelper] stop:SensorHelperTypeGyro
                                          callback:event_producer_->workfn_block];
}

template <typename DataType>
void SensorEventProducer<DataType>::WorkFn() {
  event_producer_->sensor.value->PollForSensorData(kMaxWaitMilliseconds,
                                                   &event_producer_->sensor_events_vec);

  for (DataType& event : event_producer_->sensor_events_vec) {
    // iOS hardware timestamps are already in system time.
    event.system_timestamp = event.sensor_timestamp_ns;
    if (on_event_callback_) {
      (*on_event_callback_)(event);
    }
  }
}

// Forcing instantiation of SensorEventProducer for each sensor type.
template class SensorEventProducer<AccelerometerData>;
template class SensorEventProducer<GyroscopeData>;

}  // namespace cardboard
