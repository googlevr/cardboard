/*
 * Copyright 2019 Google Inc. All Rights Reserved.
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

namespace cardboard {
namespace qrcode {

namespace {
JavaVM* vm_;
jobject context_;
}  // anonymous namespace

void initializeAndroid(JavaVM* vm, jobject context) {
  vm_ = vm;
  context_ = context;
}

std::vector<uint8_t> getCurrentSavedDeviceParams() {
  JNIEnv* env;
  vm_->GetEnv((void**)&env, JNI_VERSION_1_6);

  jclass cardboardParamsUtilsClass =
      env->FindClass("com/google/cardboard/qrcode/CardboardParamsUtils");
  jmethodID readDeviceParamsFromExternalStorage = env->GetStaticMethodID(
      cardboardParamsUtilsClass, "readDeviceParamsFromExternalStorage", "()[B");
  jbyteArray byteArray = (jbyteArray)env->CallStaticObjectMethod(
      cardboardParamsUtilsClass, readDeviceParamsFromExternalStorage);

  if (byteArray == nullptr) {
    return {};
  }

  int length = env->GetArrayLength(byteArray);
  std::vector<uint8_t> buffer;
  buffer.resize(length);
  env->GetByteArrayRegion(byteArray, 0, length,
                          reinterpret_cast<jbyte*>(&buffer[0]));
  return buffer;
}

void scanQrCodeAndSaveDeviceParams() {
  // Get JNI environment pointer
  JNIEnv* env;
  vm_->GetEnv((void**)&env, JNI_VERSION_1_6);

  // Get instance of Intent
  jclass intentClass = env->FindClass("android/content/Intent");
  jmethodID newIntent = env->GetMethodID(intentClass, "<init>", "()V");
  jobject intentObject = env->NewObject(intentClass, newIntent);

  // Get instance of ComponentName
  jclass componentNameClass = env->FindClass("android/content/ComponentName");
  jmethodID newComponentName = env->GetMethodID(
      componentNameClass, "<init>", "(Landroid/content/Context;Ljava/lang/String;)V");
  jstring className =
      env->NewStringUTF("com.google.cardboard.QrCodeCaptureActivity");
  jobject componentNameObject = env->NewObject(
      componentNameClass, newComponentName, context_, className);

  // Set component in intent
  jmethodID setComponent = env->GetMethodID(
      intentClass, "setComponent",
      "(Landroid/content/ComponentName;)Landroid/content/Intent;");
  env->CallObjectMethod(intentObject, setComponent, componentNameObject);

  // Start activity using intent
  jclass activityClass = env->GetObjectClass(context_);
  jmethodID startActivity = env->GetMethodID(activityClass, "startActivity",
                                             "(Landroid/content/Intent;)V");
  env->CallVoidMethod(context_, startActivity, intentObject);
}

}  // namespace qrcode
}  // namespace cardboard
