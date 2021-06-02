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
#ifndef CARDBOARD_SDK_SCREEN_PARAMS_H_
#define CARDBOARD_SDK_SCREEN_PARAMS_H_

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace cardboard::screen_params {
static constexpr float kMetersPerInch = 0.0254f;
#ifdef __ANDROID__
void initializeAndroid(JavaVM* vm, jobject context);
#endif
void getScreenSizeInMeters(int width_pixels, int height_pixels,
                           float* out_width_meters, float* out_height_meters);
}  // namespace cardboard::screen_params

#endif  // CARDBOARD_SDK_SCREEN_PARAMS_H_
