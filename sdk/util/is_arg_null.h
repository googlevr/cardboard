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
#ifndef CARDBOARD_SDK_UTIL_IS_ARG_NULL_H_
#define CARDBOARD_SDK_UTIL_IS_ARG_NULL_H_

#include "util/logging.h"

// Validates function argument is not nullptr. It casts any pointer to void
// pointer.
#define CARDBOARD_IS_ARG_NULL(arg) \
  cardboard::IsArgNull(static_cast<const void*>(arg), #arg, __FILE__, __LINE__)

namespace cardboard {

/// Returns true if the argument is equal to nullptr. In that case it prints an
/// error log detailing the argument and function names.
///
/// @param[in]      arg_value               Argument value.
/// @param[in]      arg_name                Argument name.
/// @param[in]      file_name               File name.
/// @param[in]      line_number             Line number.
/// @return         true if the argument is equal to nullptr, false otherwise.
inline bool IsArgNull(const void* arg_value, const char* arg_name,
                      const char* file_name, int line_number) {
  if (arg_value == nullptr) {
    CARDBOARD_LOGE("[%s : %d] Argument %s was passed as a nullptr.", file_name,
                   line_number, arg_name);
    return true;
  }
  return false;
}

}  // namespace cardboard

#endif  // CARDBOARD_SDK_UTIL_IS_ARG_NULL_H_
