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
#include "include/cardboard.h"

#include "distortion_renderer.h"
#include "head_tracker.h"
#include "lens_distortion.h"
#include "qr_code.h"
#include "screen_params.h"

// TODO(b/134142617): Revisit struct/class hierarchy.
struct CardboardLensDistortion : cardboard::LensDistortion {};
struct CardboardDistortionRenderer : cardboard::DistortionRenderer {};
struct CardboardHeadTracker : cardboard::HeadTracker {};

extern "C" {

#ifdef __ANDROID__
void Cardboard_initializeAndroid(JavaVM* vm, jobject context) {
  JNIEnv* env;
  vm->GetEnv((void**)&env, JNI_VERSION_1_6);
  jobject global_context = env->NewGlobalRef(context);

  cardboard::qrcode::initializeAndroid(vm, global_context);
  cardboard::screen_params::initializeAndroid(vm, global_context);
}
#endif

CardboardLensDistortion* CardboardLensDistortion_create(
    const uint8_t* encoded_device_params, int size, int display_width,
    int display_height) {
  return reinterpret_cast<CardboardLensDistortion*>(
      new cardboard::LensDistortion(encoded_device_params, size, display_width,
                                    display_height));
}

void CardboardLensDistortion_destroy(CardboardLensDistortion* lens_distortion) {
  delete lens_distortion;
}

void CardboardLensDistortion_getEyeMatrices(
    CardboardLensDistortion* lens_distortion, float* projection_matrix,
    float* eye_from_head_matrix, CardboardEye eye) {
  static_cast<cardboard::LensDistortion*>(lens_distortion)
      ->GetEyeMatrices(projection_matrix, eye_from_head_matrix, eye);
}

void CardboardLensDistortion_getDistortionMesh(
    CardboardLensDistortion* lens_distortion, CardboardEye eye,
    CardboardMesh* mesh) {
  *mesh = static_cast<cardboard::LensDistortion*>(lens_distortion)
              ->GetDistortionMesh(eye);
}

CardboardUv CardboardLensDistortion_undistortedUvForDistortedUv(
    CardboardLensDistortion* lens_distortion, const CardboardUv* distorted_uv,
    CardboardEye eye) {
  std::array<float, 2> in = {distorted_uv->u, distorted_uv->v};
  std::array<float, 2> out =
      static_cast<cardboard::LensDistortion*>(lens_distortion)
          ->UndistortedUvForDistortedUv(in, eye);

  CardboardUv ret;
  ret.u = out[0];
  ret.v = out[1];
  return ret;
}

CardboardUv CardboardLensDistortion_distortedUvForUndistortedUv(
    CardboardLensDistortion* lens_distortion, const CardboardUv* undistorted_uv,
    CardboardEye eye) {
  std::array<float, 2> in = {undistorted_uv->u, undistorted_uv->v};
  std::array<float, 2> out =
      static_cast<cardboard::LensDistortion*>(lens_distortion)
          ->DistortedUvForUndistortedUv(in, eye);

  CardboardUv ret;
  ret.u = out[0];
  ret.v = out[1];
  return ret;
}

CardboardDistortionRenderer* CardboardDistortionRenderer_create() {
  return reinterpret_cast<CardboardDistortionRenderer*>(
      new cardboard::DistortionRenderer());
}

void CardboardDistortionRenderer_destroy(
    CardboardDistortionRenderer* renderer) {
  delete renderer;
}

void CardboardDistortionRenderer_setMesh(CardboardDistortionRenderer* renderer,
                                         const CardboardMesh* mesh,
                                         CardboardEye eye) {
  static_cast<cardboard::DistortionRenderer*>(renderer)->SetMesh(mesh, eye);
}

void CardboardDestortionRenderer_renderEyeToDisplay(
    CardboardDistortionRenderer* renderer, int target_display,
    int display_width, int display_height,
    const CardboardEyeTextureDescription* left_eye,
    const CardboardEyeTextureDescription* right_eye) {
  static_cast<cardboard::DistortionRenderer*>(renderer)->RenderEyeToDisplay(
      target_display, display_width, display_height, left_eye, right_eye);
}

CardboardHeadTracker* CardboardHeadTracker_create() {
  return reinterpret_cast<CardboardHeadTracker*>(new cardboard::HeadTracker());
}

void CardboardHeadTracker_destroy(CardboardHeadTracker* head_tracker) {
  delete head_tracker;
}

void CardboardHeadTracker_pause(CardboardHeadTracker* head_tracker) {
  static_cast<cardboard::HeadTracker*>(head_tracker)->Pause();
}

void CardboardHeadTracker_resume(CardboardHeadTracker* head_tracker) {
  static_cast<cardboard::HeadTracker*>(head_tracker)->Resume();
}

void CardboardHeadTracker_getPose(CardboardHeadTracker* head_tracker,
                                  int64_t timestamp_ns, float* position,
                                  float* orientation) {
  std::array<float, 3> out_position;
  std::array<float, 4> out_orientation;
  static_cast<cardboard::HeadTracker*>(head_tracker)
      ->GetPose(timestamp_ns, out_position, out_orientation);
  std::memcpy(position, &out_position[0], 3 * sizeof(float));
  std::memcpy(orientation, &out_orientation[0], 4 * sizeof(float));
}


void CardboardQrCode_getSavedDeviceParams(uint8_t** encoded_device_params,
                                          int* size) {
  std::vector<uint8_t> device_params =
      cardboard::qrcode::getCurrentSavedDeviceParams();
  *size = device_params.size();
  *encoded_device_params = new uint8_t[*size];
  memcpy(*encoded_device_params, &device_params[0], *size);
}

void CardboardQrCode_destroy(const uint8_t* encoded_device_params) {
  delete[] encoded_device_params;
}

void CardboardQrCode_scanQrCodeAndSaveDeviceParams() {
  cardboard::qrcode::scanQrCodeAndSaveDeviceParams();
}

}  // extern "C"
