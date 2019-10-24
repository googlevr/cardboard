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
#include <android/log.h>
#include <jni.h>

#include <vector>

#include "qrcode/cardboard_v1/cardboard_v1.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_google_cardboard_qrcode_CardboardParamsUtils_##method_name

extern "C" {

JNI_METHOD(jbyteArray, nativeGetCardboardV1DeviceParams)
(JNIEnv* env, jobject obj) {
  std::vector<uint8_t> cardboard_v1_params =
      cardboard::qrcode::getCardboardV1DeviceParams();
  jbyteArray result = env->NewByteArray(cardboard_v1_params.size());
  env->SetByteArrayRegion(result, 0, cardboard_v1_params.size(),
                          (jbyte*)cardboard_v1_params.data());
  return result;
}

}  // extern "C"
