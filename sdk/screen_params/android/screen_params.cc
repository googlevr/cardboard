/*
 * Copyright 2019 Google LLC. All Rights Reserved.
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
#include "screen_params.h"

#include <jni.h>

#include "jni_utils/android/jni_utils.h"

namespace cardboard {
namespace screen_params {

namespace {
JavaVM* vm_;
jobject context_;
jclass display_metrics_class_;
jclass activity_class_;
jclass window_manager_class_;
jclass display_class_;

struct DisplayMetrics {
  float xdpi;
  float ydpi;
};

void LoadJNIResources(JNIEnv* env) {
  display_metrics_class_ = cardboard::jni::LoadJClass(env, "android/util/DisplayMetrics");
  activity_class_ = cardboard::jni::LoadJClass(env, "android/app/Activity");
  window_manager_class_ = cardboard::jni::LoadJClass(env, "android/view/WindowManager");
  display_class_ = cardboard::jni::LoadJClass(env, "android/view/Display");
}

DisplayMetrics getDisplayMetrics() {
  JNIEnv* env;
  cardboard::jni::LoadJNIEnv(vm_, &env);

  jmethodID display_metrics_constructor =
      env->GetMethodID(display_metrics_class_, "<init>", "()V");
  jmethodID get_window_manager = env->GetMethodID(
      activity_class_, "getWindowManager", "()Landroid/view/WindowManager;");
  jmethodID get_default_display = env->GetMethodID(
      window_manager_class_, "getDefaultDisplay", "()Landroid/view/Display;");
  jmethodID get_metrics = env->GetMethodID(display_class_, "getRealMetrics",
                                           "(Landroid/util/DisplayMetrics;)V");

  jobject display_metrics =
      env->NewObject(display_metrics_class_, display_metrics_constructor);
  jobject window_manager = env->CallObjectMethod(context_, get_window_manager);
  jobject default_display =
      env->CallObjectMethod(window_manager, get_default_display);
  env->CallVoidMethod(default_display, get_metrics, display_metrics);

  jfieldID xdpi_id = env->GetFieldID(display_metrics_class_, "xdpi", "F");
  jfieldID ydpi_id = env->GetFieldID(display_metrics_class_, "ydpi", "F");

  float xdpi = env->GetFloatField(display_metrics, xdpi_id);
  float ydpi = env->GetFloatField(display_metrics, ydpi_id);

  return {xdpi, ydpi};
}

}  // anonymous namespace

void initializeAndroid(JavaVM* vm, jobject context) {
  vm_ = vm;
  context_ = context;

  JNIEnv* env;
  cardboard::jni::LoadJNIEnv(vm_, &env);
  LoadJNIResources(env);
}

void getScreenSizeInMeters(int width_pixels, int height_pixels,
                           float* out_width_meters, float* out_height_meters) {
  DisplayMetrics display_metrics = getDisplayMetrics();

  *out_width_meters = (width_pixels / display_metrics.xdpi) * kMetersPerInch;
  *out_height_meters = (height_pixels / display_metrics.ydpi) * kMetersPerInch;
}

}  // namespace screen_params
}  // namespace cardboard
