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
#include "jni_utils/android/jni_utils.h"

namespace cardboard {
namespace jni {

void CheckExceptionInJava(JNIEnv* env) {
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
}

void LoadJNIEnv(JavaVM* vm, JNIEnv** env) {
  switch (vm->GetEnv(reinterpret_cast<void**>(env), JNI_VERSION_1_6)) {
    case JNI_OK:
      break;
    case JNI_EDETACHED:
      if (vm->AttachCurrentThread(env, nullptr) != 0) {
        *env = nullptr;
      }
      break;
    default:
      *env = nullptr;
      break;
  }
}

jclass LoadJClass(JNIEnv* env, const char* class_name) {
  jclass local = env->FindClass(class_name);
  CheckExceptionInJava(env);
  return static_cast<jclass>(env->NewGlobalRef(local));
}

}  // namespace jni
}  // namespace cardboard
