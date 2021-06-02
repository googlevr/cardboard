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
#ifndef CARDBOARD_SDK_UTIL_IS_INITIALIZED_H_
#define CARDBOARD_SDK_UTIL_IS_INITIALIZED_H_

/// @def CARDBOARD_IS_NOT_INITIALIZED()
/// Validates that the Cardboard SDK is initialized. Returns true if the
/// Cardboard SDK is not initialized, false otherwise.
#define CARDBOARD_IS_NOT_INITIALIZED() \
  !cardboard::util::IsInitialized(__FILE__, __LINE__)

namespace cardboard::util {

/// Sets the Cardboard SDK as initialized.
void SetIsInitialized();

/// Returns true if the SDK has been initialized. If not, it prints an error log
/// detailing the file name and line number.
///
/// @param[in]      file_name               File name.
/// @param[in]      line_number             Line number.
/// @return         true if the SDK has been initialized, false otherwise.
bool IsInitialized(const char* file_name, int line_number);

}  // namespace cardboard::util

#endif  // CARDBOARD_SDK_UTIL_IS_INITIALIZED_H_
