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
#include <jni.h>

#include "include/cardboard.h"
#include "jni_utils/android/jni_utils.h"

namespace {
JavaVM* vm_ = nullptr;
}  // anonymous namespace

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
  vm_ = vm;
  return JNI_VERSION_1_6;
}

JavaVM* getJavaVm() { return vm_; }

/// @brief Forwards @p context to Cardboard_initializeAndroid() but as a global
///        reference.
/// @details Calls Cardboard_initializeAndroid() with the JavaVM pointer
///          gathered by JNI_OnLoad() and creates a global reference from
///          @p context. This function is expected to be called by C# from
///          Unity's main thread, but it could be used by the SDK from other
///          threads, so the global reference is required.
/// @param context The Android context to pass to Cardboard SDK.
void CardboardUnity_initializeAndroid(jobject context) {
  JNIEnv* env = nullptr;
  cardboard::jni::LoadJNIEnv(vm_, &env);
  Cardboard_initializeAndroid(vm_, env->NewGlobalRef(context));
}
}
