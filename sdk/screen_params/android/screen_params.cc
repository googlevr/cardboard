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
#include "screen_params.h"

#include <jni.h>

#include "jni_utils/android/jni_utils.h"

namespace cardboard::screen_params {

namespace {
JavaVM* vm_;
jobject context_;

jclass screen_pixel_density_class_;
jclass screen_params_utils_class_;

struct DisplayMetrics {
  float xdpi;
  float ydpi;
};

// TODO(b/180938531): Release these global references.
void LoadJNIResources(JNIEnv* env) {
  screen_params_utils_class_ =
      reinterpret_cast<jclass>(env->NewGlobalRef(cardboard::jni::LoadJClass(
          env, "com/google/cardboard/sdk/screenparams/ScreenParamsUtils")));
  screen_pixel_density_class_ = reinterpret_cast<jclass>(env->NewGlobalRef(
      cardboard::jni::LoadJClass(env,
                                 "com/google/cardboard/sdk/screenparams/"
                                 "ScreenParamsUtils$ScreenPixelDensity")));
}

DisplayMetrics getDisplayMetrics() {
  JNIEnv* env;
  cardboard::jni::LoadJNIEnv(vm_, &env);

  const jmethodID get_screen_pixel_density_method = env->GetStaticMethodID(
      screen_params_utils_class_, "getScreenPixelDensity",
      "(Landroid/content/Context;)Lcom/google/cardboard/sdk/screenparams/"
      "ScreenParamsUtils$ScreenPixelDensity;");
  const jobject screen_pixel_density = env->CallStaticObjectMethod(
      screen_params_utils_class_, get_screen_pixel_density_method, context_);
  const jfieldID xdpi_id =
      env->GetFieldID(screen_pixel_density_class_, "xdpi", "F");
  const jfieldID ydpi_id =
      env->GetFieldID(screen_pixel_density_class_, "ydpi", "F");

  const float xdpi = env->GetFloatField(screen_pixel_density, xdpi_id);
  const float ydpi = env->GetFloatField(screen_pixel_density, ydpi_id);
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
  const DisplayMetrics display_metrics = getDisplayMetrics();

  *out_width_meters = (width_pixels / display_metrics.xdpi) * kMetersPerInch;
  *out_height_meters = (height_pixels / display_metrics.ydpi) * kMetersPerInch;
}

}  // namespace cardboard::screen_params
