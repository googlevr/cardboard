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
#ifndef CARDBOARD_SDK_SENSORS_SENSOR_EVENT_PRODUCER_H_
#define CARDBOARD_SDK_SENSORS_SENSOR_EVENT_PRODUCER_H_

#include <functional>
#include <memory>

namespace cardboard {

// Stream publisher that reads sensor data from the device sensors.
// Sensor polling starts as soon as a subscriber is connected.
//
// For the system to be able to poll from a sensor one needs to connect a
// subscriber. You can stop and restart polling at anytime after you connected a
// subscriber.
template <typename DataType>
class SensorEventProducer {
 public:
  // Constructs a sensor publisher based on the sensor_name that is passed in.
  // It will fall back to the default sensor if the specified sensor cannot be
  // found.
  SensorEventProducer();

  ~SensorEventProducer();

  // Registers callback and starts polling from DeviceSensor if it is not
  // running yet. This is a no-op if the sensor is not supported by the
  // platform.
  void StartSensorPolling(
      const std::function<void(DataType)>* on_event_callback);

  // This stops DeviceSensor sensor polling if it is currently
  // running. This method blocks until the sensor capture thread is finished.
  void StopSensorPolling();

 private:
  // Internal function to start sensor polling with the assumption that the lock
  // has already been obtained. Not implemented for iOS.
  void StartSensorPollingLocked();

  // Internal function to stop sensor polling with the assumption that the lock
  // has already been obtained. Not implemented for iOS.
  void StopSensorPollingLocked();

  // Worker method that polls for sensor data and executes OnSensor. This may
  // bind to a thread or be used as a callback for a task loop depending on the
  // implementation.
  void WorkFn();

  // The implementation of device sensors differs between iOS and Android.
  struct EventProducer;
  std::unique_ptr<EventProducer> event_producer_;

  // Maximum waiting time for sensor events.
  static const int kMaxWaitMilliseconds = 100;

  // Callbacks to call when OnEvent() is called.
  const std::function<void(DataType)>* on_event_callback_;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_SENSORS_SENSOR_EVENT_PRODUCER_H_
