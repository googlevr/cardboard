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
#include "qr_code.h"

#include <jni.h>

#include <atomic>

#include "jni_utils/android/jni_utils.h"

#define JNI_METHOD(return_type, clazz, method_name) \
  JNIEXPORT return_type JNICALL                     \
      Java_com_google_cardboard_sdk_##clazz##_##method_name

namespace cardboard {
namespace qrcode {

namespace {
JavaVM* vm_;
jobject context_;
jclass cardboard_params_utils_class_;
jclass intent_class_;
jclass component_name_class_;
std::atomic<int> qr_code_scan_count_(0);

void LoadJNIResources(JNIEnv* env) {
  cardboard_params_utils_class_ = cardboard::jni::LoadJClass(
      env, "com/google/cardboard/sdk/qrcode/CardboardParamsUtils");
  intent_class_ = cardboard::jni::LoadJClass(env, "android/content/Intent");
  component_name_class_ =
      cardboard::jni::LoadJClass(env, "android/content/ComponentName");
}

}  // anonymous namespace

void initializeAndroid(JavaVM* vm, jobject context) {
  vm_ = vm;
  context_ = context;

  JNIEnv* env;
  cardboard::jni::LoadJNIEnv(vm_, &env);
  LoadJNIResources(env);
}

std::vector<uint8_t> getCurrentSavedDeviceParams() {
  JNIEnv* env;
  cardboard::jni::LoadJNIEnv(vm_, &env);

  jmethodID readDeviceParams =
      env->GetStaticMethodID(cardboard_params_utils_class_, "readDeviceParams",
                             "(Landroid/content/Context;)[B");
  jbyteArray byteArray = static_cast<jbyteArray>(env->CallStaticObjectMethod(
      cardboard_params_utils_class_, readDeviceParams, context_));
  if (byteArray == nullptr) {
    return {};
  }

  const int length = env->GetArrayLength(byteArray);

  std::vector<uint8_t> buffer;
  buffer.resize(length);
  env->GetByteArrayRegion(byteArray, 0, length,
                          reinterpret_cast<jbyte*>(&buffer[0]));
  return buffer;
}

void scanQrCodeAndSaveDeviceParams() {
  // Get JNI environment pointer
  JNIEnv* env;
  cardboard::jni::LoadJNIEnv(vm_, &env);

  // Get instance of Intent
  jmethodID newIntent = env->GetMethodID(intent_class_, "<init>", "()V");
  jobject intentObject = env->NewObject(intent_class_, newIntent);

  // Get instance of ComponentName
  jmethodID newComponentName =
      env->GetMethodID(component_name_class_, "<init>",
                       "(Landroid/content/Context;Ljava/lang/String;)V");
  jstring className =
      env->NewStringUTF("com.google.cardboard.sdk.QrCodeCaptureActivity");
  jobject componentNameObject = env->NewObject(
      component_name_class_, newComponentName, context_, className);

  // Set component in intent
  jmethodID setComponent = env->GetMethodID(
      intent_class_, "setComponent",
      "(Landroid/content/ComponentName;)Landroid/content/Intent;");
  env->CallObjectMethod(intentObject, setComponent, componentNameObject);

  // Start activity using intent
  jclass activityClass = env->GetObjectClass(context_);
  jmethodID startActivity = env->GetMethodID(activityClass, "startActivity",
                                             "(Landroid/content/Intent;)V");
  env->CallVoidMethod(context_, startActivity, intentObject);
}

int getQrCodeScanCount() { return qr_code_scan_count_; }

}  // namespace qrcode
}  // namespace cardboard

extern "C" {

void IncrementQrCodeScanCount() { cardboard::qrcode::qr_code_scan_count_++; }

JNI_METHOD(void, QrCodeCaptureActivity, nativeIncrementQrCodeScanCount)
(JNIEnv* env, jobject obj) { IncrementQrCodeScanCount(); }

}  // extern "C"
