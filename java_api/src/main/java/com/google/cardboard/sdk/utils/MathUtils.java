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
package com.google.cardboard.sdk.utils;

import android.opengl.Matrix;

/** Math utility functions. */
public class MathUtils {

  private static final String TAG = MathUtils.class.getSimpleName();

  /** Class only contains static methods. */
  private MathUtils() {}

  /**
   * Gets isometry matrix from pose position and orientation.
   *
   * @param[in] posePosition Pose position.
   * @param[in] poseOrientation Pose orientation.
   * @return 4x4 isometry matrix as a float array.
   */
  public static float[] getMatrixFromPose(float[] posePosition, float[] poseOrientation) {
    float[] translationMatrix = new float[16];
    Matrix.setIdentityM(translationMatrix, 0);
    Matrix.translateM(translationMatrix, 0, posePosition[0], posePosition[1], posePosition[2]);
    float[] rotationMatrix = MathUtils.getRotationMatrixFromQuaternion(poseOrientation);
    float[] result = new float[16];
    Matrix.multiplyMM(result, 0, translationMatrix, 0, rotationMatrix, 0);
    return result;
  }

  /**
   * Gets rotation matrix from quaternion.
   *
   * @param[in] orientationQuaternion Orientation quaternion.
   * @return 4x4 rotation matrix as a float array.
   */
  private static float[] getRotationMatrixFromQuaternion(float[] orientationQuaternion) {
    float x = orientationQuaternion[0];
    float y = orientationQuaternion[1];
    float z = orientationQuaternion[2];
    float w = orientationQuaternion[3];

    float xx = 2 * x * x;
    float yy = 2 * y * y;
    float zz = 2 * z * z;

    float xy = 2 * x * y;
    float xz = 2 * x * z;
    float yz = 2 * y * z;

    float xw = 2 * x * w;
    float yw = 2 * y * w;
    float zw = 2 * z * w;

    float[] result = new float[16];
    result[0] = 1 - yy - zz;
    result[1] = xy + zw;
    result[2] = xz - yw;
    result[3] = 0;
    result[4] = xy - zw;
    result[5] = 1 - xx - zz;
    result[6] = yz + xw;
    result[7] = 0;
    result[8] = xz + yw;
    result[9] = yz - xw;
    result[10] = 1 - xx - yy;
    result[11] = 0;
    result[12] = 0;
    result[13] = 0;
    result[14] = 0;
    result[15] = 1;

    return result;
  }
}
