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
#include "screen_params.h"

#include <jni.h>

namespace cardboard {
namespace screen_params {

namespace {
JavaVM* vm_;
jobject context_;

struct DisplayMetrics {
  float xdpi;
  float ydpi;
};

DisplayMetrics getDisplayMetrics() {
  JNIEnv* env;
  vm_->GetEnv((void**)&env, JNI_VERSION_1_6);

  jclass display_metrics_cls = env->FindClass("android/util/DisplayMetrics");
  jmethodID display_metrics_constructor =
      env->GetMethodID(display_metrics_cls, "<init>", "()V");
  jclass activity_cls = env->FindClass("android/app/Activity");
  jmethodID get_window_manager = env->GetMethodID(
      activity_cls, "getWindowManager", "()Landroid/view/WindowManager;");
  jclass window_manager_cls = env->FindClass("android/view/WindowManager");
  jmethodID get_default_display = env->GetMethodID(
      window_manager_cls, "getDefaultDisplay", "()Landroid/view/Display;");
  jclass display_cls = env->FindClass("android/view/Display");
  jmethodID get_metrics = env->GetMethodID(display_cls, "getRealMetrics",
                                           "(Landroid/util/DisplayMetrics;)V");

  jobject display_metrics =
      env->NewObject(display_metrics_cls, display_metrics_constructor);
  jobject window_manager = env->CallObjectMethod(context_, get_window_manager);
  jobject default_display =
      env->CallObjectMethod(window_manager, get_default_display);
  env->CallVoidMethod(default_display, get_metrics, display_metrics);

  jfieldID xdpi_id = env->GetFieldID(display_metrics_cls, "xdpi", "F");
  jfieldID ydpi_id = env->GetFieldID(display_metrics_cls, "ydpi", "F");

  float xdpi = env->GetFloatField(display_metrics, xdpi_id);
  float ydpi = env->GetFloatField(display_metrics, ydpi_id);

  return {xdpi, ydpi};
}

}  // anonymous namespace

void initializeAndroid(JavaVM* vm, jobject context) {
  vm_ = vm;
  context_ = context;
}

void getScreenSizeInMeters(int width_pixels, int height_pixels,
                           float* out_width_meters, float* out_height_meters) {
  DisplayMetrics display_metrics = getDisplayMetrics();

  *out_width_meters = (width_pixels / display_metrics.xdpi) * kMetersPerInch;
  *out_height_meters = (height_pixels / display_metrics.ydpi) * kMetersPerInch;
}

}  // namespace screen_params
}  // namespace cardboard
