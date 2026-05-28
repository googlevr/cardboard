/*
 * Copyright 2024 Google LLC
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
package com.google.cardboard.sdk;

import android.opengl.Matrix;

/** Describes the head transform independently of any eye parameters. */
public class HeadTransform {

  /** Epsilon value used to detect Gimbal lock situations when computing Euler angles. */
  private static final float GIMBAL_LOCK_EPSILON = 1e-2f;

  private final float[] headView;

  public HeadTransform() {
    headView = new float[16];
    Matrix.setIdentityM(headView, 0);
  }

  /**
   * @hide Returns the array where the camera to head matrix transform is stored. For internal use
   *     only.
   * @return The array where the headView matrix is stored.
   */
  public float[] getHeadView() {
    return headView;
  }

  /**
   * A matrix representing the transform from the camera to the head.
   *
   * <p>Head origin is defined as the center point between the two eyes.
   *
   * @param headView Array where the 4x4 column-major transformation matrix will be written to.
   * @param offset Offset in the array where data should be written.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getHeadView(float[] headView, int offset) {
    // Ensure the result fits.
    if (offset + 16 > headView.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    System.arraycopy(this.headView, 0, headView, offset, 16);
  }

  /**
   * Provides the direction the head is looking towards as a 3x1 unit vector.
   *
   * <p>Note that in OpenGL the forward vector points into the -Z direction. Make sure to invert it
   * if ever used to compute the basis of a right-handed system.
   *
   * @param forward Array where the forward vector will be written to.
   * @param offset Offset in the array where data should be written.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getForwardVector(float[] forward, int offset) {
    // Ensure the result fits.
    if (offset + 3 > forward.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    // Same as multiplying the inverse of the rotation component of the matrix by (0, 0, -1, 0).
    for (int i = 0; i < 3; ++i) {
      forward[i + offset] = -headView[2 + i * 4];
    }
  }

  /**
   * Provides the upwards direction of the head as a 3x1 unit vector.
   *
   * @param up Array where the up vector will be written to.
   * @param offset Offset in the array where data should be written.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getUpVector(float[] up, int offset) {
    // Ensure the result fits.
    if (offset + 3 > up.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    // Same as multiplying the inverse of the rotation component of the matrix by (0, 1, 0, 0).
    for (int i = 0; i < 3; ++i) {
      up[i + offset] = headView[1 + i * 4];
    }
  }

  /**
   * Provides the rightwards direction of the head as a 3x1 unit vector.
   *
   * @param right Array where the right vector will be written to.
   * @param offset Offset in the array where data should be written.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getRightVector(float[] right, int offset) {
    // Ensure the result fits.
    if (offset + 3 > right.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    // Same as multiplying the inverse of the rotation component of the matrix by (1, 0, 0, 0).
    for (int i = 0; i < 3; ++i) {
      right[i + offset] = headView[i * 4];
    }
  }

  /**
   * Provides the quaternion representing the head rotation.
   *
   * @param quaternion Array where the quaternion (x, y, z, w) will be written to.
   * @param offset Offset in the array where data should be written.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getQuaternion(float[] quaternion, int offset) {
    // Ensure the result fits.
    if (offset + 4 > quaternion.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    // Based on the code from http://www.cs.ucr.edu/~vbz/resources/quatut.pdf.
    float[] m = headView;
    float t = m[0] + m[5] + m[10];
    float x;
    float y;
    float z;
    float w;

    if (t >= 0) {
      float s = (float) Math.sqrt(t + 1);
      w = 0.5f * s;
      s = 0.5f / s;
      x = (m[9] - m[6]) * s;
      y = (m[2] - m[8]) * s;
      z = (m[4] - m[1]) * s;

    } else if ((m[0] > m[5]) && (m[0] > m[10])) {
      float s = (float) Math.sqrt(1.0f + m[0] - m[5] - m[10]);
      x = s * 0.5f;
      s = 0.5f / s;
      y = (m[4] + m[1]) * s;
      z = (m[2] + m[8]) * s;
      w = (m[9] - m[6]) * s;

    } else if (m[5] > m[10]) {
      float s = (float) Math.sqrt(1.0f + m[5] - m[0] - m[10]);
      y = s * 0.5f;
      s = 0.5f / s;
      x = (m[4] + m[1]) * s;
      z = (m[9] + m[6]) * s;
      w = (m[2] - m[8]) * s;

    } else {
      float s = (float) Math.sqrt(1.0f + m[10] - m[0] - m[5]);
      z = s * 0.5f;
      s = 0.5f / s;
      x = (m[2] + m[8]) * s;
      y = (m[9] + m[6]) * s;
      w = (m[4] - m[1]) * s;
    }

    quaternion[offset + 0] = x;
    quaternion[offset + 1] = y;
    quaternion[offset + 2] = z;
    quaternion[offset + 3] = w;
  }

  /**
   * Provides the Euler angles representation of the head rotation.
   *
   * <p>Use of Euler angles is discouraged as they might be subject to Gimbal lock situations. Use
   * quaternions or rotation matrices instead whenever possible.
   *
   * <p>The provided values represent the viewport rotation as pitch, yaw and roll angles where the
   * matrix R = Rz(roll) * Rx(pitch) * Ry(yaw) represents the full rotation. This rotation matrix
   * order ensures both yaw and roll are in the full [-pi, pi] interval.
   *
   * <p>The angles are provided in radians, in this order and within the following intervals:
   *
   * <ul>
   *   <li>Pitch (X axis): [-pi/2, pi/2]
   *   <li>Yaw (Y axis): [-pi, pi]
   *   <li>Roll (Z axis): [-pi, pi]
   * </ul>
   *
   * <p>The X-Y-Z axes are the basis of a right-handed OpenGL-style coordinate system. During Gimbal
   * lock this method enforces yaw to 0 and provides a valid roll angle.
   *
   * @param eulerAngles Array where the 3 angles will be written to.
   * @param offset Offset in the array where data should be written.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getEulerAngles(float[] eulerAngles, int offset) {
    // Ensure the result fits.
    if (offset + 3 > eulerAngles.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    // Compute pitch.
    float pitch = (float) Math.asin(headView[6]);
    float yaw;
    float roll;

    // Check the absolute value of the pitch cosine to detect Gimbal lock situations.
    if (Math.sqrt(1.0f - headView[6] * headView[6]) >= GIMBAL_LOCK_EPSILON) {

      // No Gimbal lock.
      yaw = (float) Math.atan2(-headView[2], headView[10]);
      roll = (float) Math.atan2(-headView[4], headView[5]);

    } else {
      // Gimbal lock detected.
      yaw = 0.0f;
      roll = (float) Math.atan2(headView[1], headView[0]);
    }

    eulerAngles[offset + 0] = -pitch;
    eulerAngles[offset + 1] = -yaw;
    eulerAngles[offset + 2] = -roll;
  }

  /**
   * Provides the relative translation of the head as a 3x1 vector.
   *
   * @param translation Array where the translation vector will be written to.
   * @param offset Offset in the array where data should be written.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getTranslation(float[] translation, int offset) {
    // Ensure the result fits.
    if (offset + 3 > translation.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    // Same as multiplying the matrix by (0, 0, 0, 1).
    for (int i = 0; i < 3; ++i) {
      translation[i + offset] = headView[12 + i];
    }
  }
}
