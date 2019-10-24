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
#include "polynomial_radial_distortion.h"

#include <cmath>

#include "testing/base/public/gunit.h"

namespace cardboard {

TEST(PolynomialRadialDistortionTest, TestCardboardDevicePolynomials) {
  const float kDefaultFloatTolerance = 1.0e-3f;
  std::vector<std::pair<float, std::vector<float>>> device_range_and_params = {
      // Cardboard v1:
      {1.57f, {0.441f, 0.156f}},
      // Cardboard v2:
      {1.7f, {0.34f, 0.55f}}};

  for (const auto& device : device_range_and_params) {
    PolynomialRadialDistortion distortion(device.second);
    for (float radius = 0.0f; radius < device.first; radius += 0.01f) {
      // Choose a point whose distance from zero is |radius|.  Rotate by the
      // radius so that we're testing a range of points that aren't on a line.
      std::array<float, 2> point = {std::cosf(radius) * radius,
                                    std::sinf(radius) * radius};
      std::array<float, 2> inverse_point = distortion.DistortInverse(point);
      std::array<float, 2> check = distortion.Distort(inverse_point);

      EXPECT_NEAR(point[0], check[0], kDefaultFloatTolerance);
      EXPECT_NEAR(point[1], check[1], kDefaultFloatTolerance);
    }
  }
}

}  // namespace cardboard
