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
#ifndef CARDBOARD_SDK_DISTORTION_RENDERER_H_
#define CARDBOARD_SDK_DISTORTION_RENDERER_H_

#include <array>
#include <cstdio>

#include "include/cardboard.h"

namespace cardboard {

// @brief Interface to distort and render left and right eyes with different API
//        backends.
class DistortionRenderer {
 public:
  virtual ~DistortionRenderer() = default;
  virtual void SetMesh(const CardboardMesh* mesh, CardboardEye eye) = 0;
  virtual void RenderEyeToDisplay(
      uint64_t target, int x, int y, int width, int height,
      const CardboardEyeTextureDescription* left_eye,
      const CardboardEyeTextureDescription* right_eye) = 0;
};

}  // namespace cardboard

#endif  // CARDBOARD_SDK_DISTORTION_RENDERER_H_
