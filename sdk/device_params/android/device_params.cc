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
#include "device_params/android/device_params.h"

#include <jni.h>

#include <cstdint>

#include "jni_utils/android/jni_utils.h"
#include "qrcode/cardboard_v1/cardboard_v1.h"
#include "util/logging.h"

namespace cardboard {

namespace {

JavaVM* vm_;
jobject context_;
jclass device_params_utils_class_;

// TODO(b/180938531): Release this global reference.
void LoadJNIResources(JNIEnv* env) {
  device_params_utils_class_ =
      reinterpret_cast<jclass>(env->NewGlobalRef(jni::LoadJClass(
          env, "com/google/cardboard/sdk/deviceparams/DeviceParamsUtils")));
}

}  // anonymous namespace

// TODO(b/181575962): Add C++ unit tests.
DeviceParams::~DeviceParams() {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);
  env->DeleteGlobalRef(java_device_params_);
}

void DeviceParams::initializeAndroid(JavaVM* vm, jobject context) {
  vm_ = vm;
  context_ = context;

  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);
  LoadJNIResources(env);
}

void DeviceParams::ParseFromArray(const uint8_t* encoded_device_params,
                                  int size) {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jmethodID mid = env->GetStaticMethodID(
      device_params_utils_class_, "parseCardboardDeviceParams",
      "([B)Lcom/google/cardboard/proto/CardboardDevice$DeviceParams;");

  jbyteArray encoded_device_params_array = env->NewByteArray(size);
  env->SetByteArrayRegion(encoded_device_params_array, 0, size,
                          const_cast<jbyte*>(reinterpret_cast<const jbyte*>(
                              encoded_device_params)));

  jobject device_params_obj = env->CallStaticObjectMethod(
      device_params_utils_class_, mid, encoded_device_params_array);

  if (java_device_params_ != nullptr) {
    env->DeleteGlobalRef(java_device_params_);
  }
  java_device_params_ = env->NewGlobalRef(device_params_obj);
}

// TODO(b/181658993): Check if JNI "execution + exception check" could be
// wrapped within a reusable function or macro.
float DeviceParams::screen_to_lens_distance() const {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jclass cls = env->GetObjectClass(java_device_params_);
  jni::CheckExceptionInJava(env);
  jmethodID mid = env->GetMethodID(cls, "getScreenToLensDistance", "()F");
  jni::CheckExceptionInJava(env);
  const float screen_to_lens_distance =
      env->CallFloatMethod(java_device_params_, mid);
  if (jni::CheckExceptionInJava(env)) {
    CARDBOARD_LOGE(
        "Cannot retrieve ScreenToLensDistance from device parameters. Using "
        "Cardboard Viewer v1 parameter.");
    return qrcode::kCardboardV1ScreenToLensDistance;
  }
  return screen_to_lens_distance;
}

float DeviceParams::inter_lens_distance() const {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jclass cls = env->GetObjectClass(java_device_params_);
  jni::CheckExceptionInJava(env);
  jmethodID mid = env->GetMethodID(cls, "getInterLensDistance", "()F");
  jni::CheckExceptionInJava(env);
  const float inter_lens_distance =
      env->CallFloatMethod(java_device_params_, mid);
  if (jni::CheckExceptionInJava(env)) {
    CARDBOARD_LOGE(
        "Cannot retrieve InterLensDistance from device parameters. Using "
        "Cardboard Viewer v1 parameter.");
    return qrcode::kCardboardV1InterLensDistance;
  }
  return inter_lens_distance;
}

float DeviceParams::tray_to_lens_distance() const {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jclass cls = env->GetObjectClass(java_device_params_);
  jni::CheckExceptionInJava(env);
  jmethodID mid = env->GetMethodID(cls, "getTrayToLensDistance", "()F");
  jni::CheckExceptionInJava(env);
  const float tray_to_lens_distance =
      env->CallFloatMethod(java_device_params_, mid);
  if (jni::CheckExceptionInJava(env)) {
    CARDBOARD_LOGE(
        "Cannot retrieve TrayToLensDistance from device parameters. Using "
        "Cardboard Viewer v1 parameter.");
    return qrcode::kCardboardV1TrayToLensDistance;
  }
  return tray_to_lens_distance;
}

int DeviceParams::vertical_alignment() const {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jclass cls = env->GetObjectClass(java_device_params_);
  jni::CheckExceptionInJava(env);
  jmethodID mid =
      env->GetMethodID(cls, "getVerticalAlignment",
                       "()Lcom/google/cardboard/proto/"
                       "CardboardDevice$DeviceParams$VerticalAlignmentType;");
  jni::CheckExceptionInJava(env);
  jobject vertical_alignment = env->CallObjectMethod(java_device_params_, mid);
  jni::CheckExceptionInJava(env);
  jmethodID get_enum_value_mid = env->GetMethodID(
      env->GetObjectClass(vertical_alignment), "ordinal", "()I");
  jni::CheckExceptionInJava(env);
  const int vertical_alignment_type =
      env->CallIntMethod(vertical_alignment, get_enum_value_mid);
  if (jni::CheckExceptionInJava(env)) {
    CARDBOARD_LOGE(
        "Cannot retrieve VerticalAlignmentType from device parameters. Using "
        "Cardboard Viewer v1 parameter.");
    return qrcode::kCardboardV1VerticalAlignmentType;
  }
  return vertical_alignment_type;
}

float DeviceParams::distortion_coefficients(int index) const {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jclass cls = env->GetObjectClass(java_device_params_);
  jni::CheckExceptionInJava(env);
  jmethodID mid = env->GetMethodID(cls, "getDistortionCoefficients", "(I)F");
  jni::CheckExceptionInJava(env);
  const float distortion_coefficient =
      env->CallFloatMethod(java_device_params_, mid, index);
  if (jni::CheckExceptionInJava(env)) {
    CARDBOARD_LOGE(
        "Cannot retrieve DistortionCoefficient from device parameters. Using "
        "Cardboard Viewer v1 parameter.");
    return qrcode::kCardboardV1DistortionCoeffs[index];
  }
  return distortion_coefficient;
}

int DeviceParams::distortion_coefficients_size() const {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jclass cls = env->GetObjectClass(java_device_params_);
  jni::CheckExceptionInJava(env);
  jmethodID mid =
      env->GetMethodID(cls, "getDistortionCoefficientsCount", "()I");
  jni::CheckExceptionInJava(env);
  const int distortion_coefficients_size =
      env->CallIntMethod(java_device_params_, mid);
  if (jni::CheckExceptionInJava(env)) {
    CARDBOARD_LOGE(
        "Cannot retrieve DistortionCoefficientsCount from device parameters. "
        "Using Cardboard Viewer v1 parameter.");
    return qrcode::kCardboardV1DistortionCoeffsSize;
  }
  return distortion_coefficients_size;
}

float DeviceParams::left_eye_field_of_view_angles(int index) const {
  JNIEnv* env;
  jni::LoadJNIEnv(vm_, &env);

  jclass cls = env->GetObjectClass(java_device_params_);
  jni::CheckExceptionInJava(env);
  jmethodID mid = env->GetMethodID(cls, "getLeftEyeFieldOfViewAngles", "(I)F");
  jni::CheckExceptionInJava(env);
  const float left_eye_field_of_view_angle =
      env->CallFloatMethod(java_device_params_, mid, index);
  if (jni::CheckExceptionInJava(env)) {
    CARDBOARD_LOGE(
        "Cannot retrieve LeftEyeFieldOfViewAngle from device parameters. "
        "Using Cardboard Viewer v1 parameter.");
    return qrcode::kCardboardV1FovHalfDegrees[index];
  }
  return left_eye_field_of_view_angle;
}

}  // namespace cardboard
