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
#include "util/is_initialized.h"

#include "util/logging.h"

namespace {

#ifdef __ANDROID__

bool is_initialized = false;

#endif

}  // namespace

namespace cardboard::util {

#ifdef __ANDROID__

void SetIsInitialized() { is_initialized = true; }

bool IsInitialized(const char* file_name, int line_number) {
  if (!is_initialized) {
    CARDBOARD_LOGE(
        "[%s : %d] Cardboard SDK is not initialized yet. Please call "
        "Cardboard_initializeAndroid().",
        file_name, line_number);
    return false;
  }
  return true;
}

#else

void SetIsInitialized() {}

bool IsInitialized(const char* /*file_name*/, int /*line_number*/) {
  return true;
}

#endif

}  // namespace cardboard::util
