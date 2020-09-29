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
#ifndef CARDBOARD_SDK_JNI_UTILS_JNI_UTILS_H_
#define CARDBOARD_SDK_JNI_UTILS_JNI_UTILS_H_

#include <jni.h>

namespace cardboard {
namespace jni {

/// @brief Logs a Java exception and clears JNI flag if any exception occured.
/// @param java_env The pointer to the JNI Environmnent.
void CheckExceptionInJava(JNIEnv* env);

/// @brief Retrieves the JNI environment.
/// @details JavaVM::GetEnv() might return JNI_OK, JNI_EDETACHED or other value.
///          When JNI_OK is returned, the obtained value is returned as is. When
///          JNI_EDETACHED is returned, JavaVM::AttachCurrentThread() is called.
///          Upon failure or any other result, @p *java_env is set to nullptr.
/// @param env The pointer to the JNI Environmnent.
void LoadJNIEnv(JavaVM* vm, JNIEnv** env);

/// @brief Loads a class by its @p class_name as a global reference.
/// @param env The JNI Environment context.
/// @param class_name A char pointer holding the Java full class name.
/// @return A global referenced jclass pointing to the @p class_name Java class.
jclass LoadJClass(JNIEnv* env, const char* class_name);

}  // namespace jni
}  // namespace cardboard

#endif  // CARDBOARD_SDK_JNI_UTILS_JNI_UTILS_H_
