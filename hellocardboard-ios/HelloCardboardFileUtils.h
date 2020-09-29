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
#ifndef HELLO_CARDBOARD_IOS_HELLO_CARDBOARD_FILE_UTILS_H_
#define HELLO_CARDBOARD_IOS_HELLO_CARDBOARD_FILE_UTILS_H_

#import <OpenGLES/ES2/gl.h>

#include <string>

namespace cardboard {
namespace hello_cardboard {

/**
 * Loads a png file.
 */
void LoadPngFile(const std::string& imageName, GLubyte** outImageData,
                 GLuint* outWidth, GLuint* outHeight);

/**
 * Loads a text file.
 */
std::string LoadTextFile(const std::string& fileName);

}  // namespace hello_cardboard
}  // namespace cardboard

#endif  // HELLO_CARDBOARD_IOS_HELLO_CARDBOARD_FILE_UTILS_H_
