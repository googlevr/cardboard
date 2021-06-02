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
#include "unity/xr_provider/math_tools.h"

#include <cmath>
#include <utility>

namespace cardboard::unity {
namespace {

// TODO(b/151817737): Compute pose position within SDK with custom rotation.
UnityXRVector4 QuatMul(const UnityXRVector4& q0, const UnityXRVector4& q1) {
  UnityXRVector4 result;
  result.x = q0.w * q1.x + q0.x * q1.w + q0.y * q1.z - q0.z * q1.y;
  result.y = q0.w * q1.y + q0.y * q1.w + q0.z * q1.x - q0.x * q1.z;
  result.z = q0.w * q1.z + q0.z * q1.w + q0.x * q1.y - q0.y * q1.x;
  result.w = q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z;
  return result;
}

// TODO(b/151817737): Compute pose position within SDK with custom rotation.
UnityXRVector3 QuatMulVec(const UnityXRVector4& q, const UnityXRVector3& v) {
  const UnityXRVector4 p = {v.x, v.y, v.z, 0.0f};
  const UnityXRVector4 pre_mul = QuatMul(q, p);
  const UnityXRVector4 q_conj = {-q.x, -q.y, -q.z, q.w};
  const UnityXRVector4 result = QuatMul(pre_mul, q_conj);
  return {result.x, result.y, result.z};
}

// TODO(b/151817737): Compute pose position within SDK with custom rotation.
UnityXRVector3 ApplyNeckModel(const UnityXRVector4& rotation) {
  // Position of the point between the eyes, relative to the neck pivot:
  const float kDefaultNeckHorizontalOffset = 0.080f;  // meters in Z
  const float kDefaultNeckVerticalOffset = 0.075f;    // meters in Y

  UnityXRVector3 eyes_center_offset = {0.0f, kDefaultNeckVerticalOffset,
                                       kDefaultNeckHorizontalOffset};
  // Rotate eyes around neck pivot point.
  UnityXRVector3 position = QuatMulVec(rotation, eyes_center_offset);
  // Measure new position relative to original center of head, because applying
  // a neck model should not elevate the camera.
  position.y -= kDefaultNeckVerticalOffset;

  return position;
}

// Adapted from: https://github.com/gingerBill/gb/blob/master/gb_math.h
static UnityXRVector4 CardboardTransformToUnityQuat(
    const std::array<float, 16>& transform_in) {
  // Cardboard matrices are row major, unity (and this algorithm) is column
  // major. So we transpose. This can be optimized.
  std::array<float, 16> transform = transform_in;
  std::swap(transform[1], transform[4]);
  std::swap(transform[2], transform[8]);
  std::swap(transform[6], transform[9]);

  UnityXRVector4 q;

  float trace = transform[0] + transform[5] + transform[10];
  float root;

  if (trace > 0.0f) {                // |w| > 1/2, may as well choose w > 1/2
    root = std::sqrt(trace + 1.0f);  // 2w
    q.w = 0.5f * root;
    root = 0.5f / root;  // 1/(4w)
    q.x = (transform[9] - transform[6]) * root;
    q.y = (transform[2] - transform[8]) * root;
    q.z = (transform[4] - transform[1]) * root;
  } else {  // |w| <= 1/2
    const std::array<int, 3> kNextIndex = {1, 2, 0};
    int i = (transform[5] > transform[0]) ? 1 : 0;
    i = (transform[10] > transform[i * 4 + i]) ? 2 : i;
    const int j = kNextIndex[i];
    const int k = kNextIndex[j];

    root = std::sqrt(transform[i * 4 + i] - transform[j * 4 + j] -
                     transform[k * 4 + k] + 1.0f);
    float* apk_quat[3] = {&q.x, &q.y, &q.z};
    *apk_quat[i] = 0.5f * root;
    root = 0.5f / root;
    q.w = (transform[k * 4 + j] - transform[j * 4 + k]) * root;
    *apk_quat[j] = (transform[j * 4 + i] + transform[i * 4 + j]) * root;
    *apk_quat[k] = (transform[k * 4 + i] + transform[i * 4 + k]) * root;
  }

  const float length = ((q.x * q.x) + (q.y * q.y) + (q.z * q.z) + (q.w * q.w));
  q.x = q.x / length;
  q.y = q.y / length;
  q.z = q.z / length;
  q.w = q.w / length;

  return q;
}

}  // namespace

UnityXRPose CardboardRotationToUnityPose(const std::array<float, 4>& rotation) {
  UnityXRPose result;

  // Sets Unity Pose's rotation. Unity expects forward as positive z axis,
  // whereas OpenGL expects forward as negative z.
  result.rotation.x = rotation.at(0);
  result.rotation.y = rotation.at(1);
  result.rotation.z = -rotation.at(2);
  result.rotation.w = rotation.at(3);

  // Computes Unity Pose's position.
  // Reasoning: It's easier to compute the position directly applying the neck
  // model to Unity's rotation instead of using the one provided by the SDK. To
  // use the provided position we should perform the following computation:
  //   1. Compute inverse rotation quaternion (OpenGL's coordinates frame).
  //   2. Apply the inverse rotation to the provided position.
  //   3. Modify the position vector to suit Unity's coordinates frame.
  //   4. Apply the new rotation (Unity's coordinates frame).
  // TODO(b/151817737): Compute pose position within SDK with custom rotation.
  result.position = ApplyNeckModel(result.rotation);

  return result;
}

// TODO(b/155113586): refactor this function to be part of the same
//                    transformation as the above.
UnityXRPose CardboardTransformToUnityPose(
    const std::array<float, 16>& transform) {
  UnityXRPose ret;
  ret.rotation = CardboardTransformToUnityQuat(transform);

  // Cardboard matrices are transforms into view space, unity wants transforms
  // into world space. (inverse)
  ret.rotation.x = -ret.rotation.x;
  ret.rotation.y = -ret.rotation.y;

  // Inverse + negate Z.
  ret.position.x = -transform[12];
  ret.position.y = -transform[13];
  ret.position.z = transform[14];

  // In order to find the inverse transform we need to apply the rotation.
  ret.position = QuatMulVec(ret.rotation, ret.position);

  return ret;
}

}  // namespace cardboard::unity
