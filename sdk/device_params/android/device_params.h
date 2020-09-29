/*
 * Copyright 2020 Google LLC
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
#ifndef CARDBOARD_SDK_DEVICE_PARAMS_ANDROID_DEVICE_PARAMS_H_
#define CARDBOARD_SDK_DEVICE_PARAMS_ANDROID_DEVICE_PARAMS_H_

#include <jni.h>

#include <cstdint>

namespace cardboard {

// This class acts as a bridge between protobuf usage in C++ code to protobuf
// dependency in Java, by using JNI. The class and method names are equivalent
// to the ones present in protobuf generated source code, to make it transparent
// for the user.
class DeviceParams {
 public:
  enum VerticalAlignmentType { BOTTOM = 0, CENTER = 1, TOP = 2 };

  DeviceParams() : java_device_params_(nullptr) {}
  ~DeviceParams();

  // Initializes JavaVM and Android activity context.
  //
  // @param[in]      vm                      JavaVM pointer
  // @param[in]      context                 Android activity context
  static void initializeAndroid(JavaVM* vm, jobject context);

  // Parses device parameters from serialized buffer.
  //
  // @param[in]      encoded_device_params   Device parameters byte buffer.
  // @param[in]      size                    Buffer length in bytes.
  void ParseFromArray(const uint8_t* encoded_device_params, int size);

  // Device parameters getter methods.
  float screen_to_lens_distance() const;
  float inter_lens_distance() const;
  float tray_to_lens_distance() const;
  int vertical_alignment() const;
  float distortion_coefficients(int index) const;
  int distortion_coefficients_size() const;
  float left_eye_field_of_view_angles(int index) const;

 private:
  jobject java_device_params_;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_DEVICE_PARAMS_ANDROID_DEVICE_PARAMS_H_
