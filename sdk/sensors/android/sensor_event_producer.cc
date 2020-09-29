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
#include "sensors/sensor_event_producer.h"

#include <atomic>
#include <memory>
#include <mutex>   // NOLINT
#include <thread>  // NOLINT
#include <vector>

#include "sensors/accelerometer_data.h"
#include "sensors/device_accelerometer_sensor.h"
#include "sensors/device_gyroscope_sensor.h"
#include "sensors/gyroscope_data.h"

namespace cardboard {

template <typename DataType>
struct SensorEventProducer<DataType>::EventProducer {
  EventProducer() : run_thread(false) {}
  // Capture thread. This will be created when polling is started, and
  // destroyed when polling is stopped.
  std::unique_ptr<std::thread> thread;
  std::mutex mutex;
  // Flag indicating if the capture thread should run.
  std::atomic<bool> run_thread;
};

template <typename DataType>
SensorEventProducer<DataType>::SensorEventProducer()
    : event_producer_(new EventProducer()) {}

template <typename DataType>
SensorEventProducer<DataType>::~SensorEventProducer() {
  StopSensorPolling();
}

template <typename DataType>
void SensorEventProducer<DataType>::StartSensorPolling(
    const std::function<void(DataType)>* on_event_callback) {
  on_event_callback_ = on_event_callback;
  std::unique_lock<std::mutex> lock(event_producer_->mutex);
  StartSensorPollingLocked();
}

template <typename DataType>
void SensorEventProducer<DataType>::StopSensorPolling() {
  std::unique_lock<std::mutex> lock(event_producer_->mutex);
  StopSensorPollingLocked();
  on_event_callback_ = nullptr;
}

template <typename DataType>
void SensorEventProducer<DataType>::StartSensorPollingLocked() {
  // If the thread is started already there is nothing left to do.
  if (event_producer_->run_thread.exchange(true)) {
    return;
  }

  event_producer_->thread.reset(new std::thread([&]() { WorkFn(); }));
}

template <typename DataType>
void SensorEventProducer<DataType>::StopSensorPollingLocked() {
  // If the thread is already stop nothing needs to be done.
  if (!event_producer_->run_thread.exchange(false)) {
    return;
  }

  if (!event_producer_->thread || !event_producer_->thread->joinable()) {
    return;
  }
  event_producer_->thread->join();
  event_producer_->thread.reset();
}

template <>
void SensorEventProducer<AccelerometerData>::WorkFn() {
  DeviceAccelerometerSensor sensor;

  if (!sensor.Start()) {
    return;
  }

  std::vector<AccelerometerData> sensor_events_vec;

  // On other devices and platforms we estimate the clock bias.
  // TODO(b/135468657): Investigate clock conversion. Old cardboard doesn't have
  // this.
  while (event_producer_->run_thread) {
    sensor.PollForSensorData(kMaxWaitMilliseconds, &sensor_events_vec);
    for (AccelerometerData& event : sensor_events_vec) {
      event.system_timestamp = event.sensor_timestamp_ns;
      if (on_event_callback_) {
        (*on_event_callback_)(event);
      }
    }
  }
  sensor.Stop();
}

template <>
void SensorEventProducer<GyroscopeData>::WorkFn() {
  DeviceGyroscopeSensor sensor;

  if (!sensor.Start()) {
    return;
  }

  std::vector<GyroscopeData> sensor_events_vec;

  // On other devices and platforms we estimate the clock bias.
  // TODO(b/135468657): Investigate clock conversion. Old cardboard doesn't have
  // this.
  while (event_producer_->run_thread) {
    sensor.PollForSensorData(kMaxWaitMilliseconds, &sensor_events_vec);
    for (GyroscopeData& event : sensor_events_vec) {
      event.system_timestamp = event.sensor_timestamp_ns;
      if (on_event_callback_) {
        (*on_event_callback_)(event);
      }
    }
  }
  sensor.Stop();
}

// Forcing instantiation of SensorEventProducer for each sensor type.
template class SensorEventProducer<AccelerometerData>;
template class SensorEventProducer<GyroscopeData>;

}  // namespace cardboard
