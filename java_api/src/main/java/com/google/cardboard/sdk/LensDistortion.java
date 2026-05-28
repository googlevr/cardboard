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
package com.google.cardboard.sdk;

import androidx.annotation.Nullable;
import com.google.cardboard.sdk.nativetypes.Mesh;
import com.google.cardboard.sdk.nativetypes.UvPoint;

/** Cardboard SDK Lens Distortion API. */
public class LensDistortion {

  // Opaque native pointer to the native LensDistortion instance.
  // This object is owned by this class instance and passed to the native methods.
  private long nativeLensDistortion;

  /**
   * Creates a new lens distortion object.
   *
   * <p>Preconditions: deviceParams Must not be null.
   *
   * @param[in] deviceParams The device parameters serialized using cardboard_device.proto.
   * @param[in] displayWidth Size in pixels of display width.
   * @param[in] displayHeight Size in pixels of display height.
   */
  public LensDistortion(byte[] deviceParams, int displayWidth, int displayHeight) {
    nativeLensDistortion = nativeLensDistortionCreate(deviceParams, displayWidth, displayHeight);
  }

  /**
   * Releases memory created at construct time by the native lens distortion object. This method
   * should be executed just once after construct time, then it will not have effect and a new
   * instance must be instantiated.
   */
  public void close() {
    nativeLensDistortionDestroy(nativeLensDistortion);
    nativeLensDistortion = 0;
  }

  /** Calls {@code close()} in case that method is mistakenly not called. */
  @Override
  protected void finalize() throws Throwable {
    // TODO(b/146156526): Remove finalize() usage from Java API.
    close();
    super.finalize();
  }

  /**
   * Gets the eyeFromHead matrix for a particular eye.
   *
   * <p>Preconditions: eyeFromHeadMatrix Must not be null.
   *
   * @param[in] eye Desired eye.
   * @param[out] eyeFromHeadMatrix 4x4 float eye from head matrix.
   */
  public void getEyeFromHeadMatrix(int eye, float[] eyeFromHeadMatrix) {
    nativeLensDistortionGetEyeFromHeadMatrix(nativeLensDistortion, eye, eyeFromHeadMatrix);
  }

  /**
   * Gets the ideal projection matrix for a particular eye.
   *
   * <p>Preconditions: z_near and z_far must be postive and z_near must be smaller than z_far.
   * Preconditions: projectionMatrix Must not be null.
   *
   * @param[in] eye Desired eye.
   * @param[in] zNear Near clip plane z-axis coordinate.
   * @param[in] zFar Far clip plane z-axis coordinate.
   * @param[out] projectionMatrix 4x4 float ideal projection matrix.
   */
  public void getEyeProjectionMatrix(int eye, float zNear, float zFar, float[] projectionMatrix) {
    nativeLensDistortionGetEyeProjectionMatrix(
        nativeLensDistortion, eye, zNear, zFar, projectionMatrix);
  }

  /**
   * Gets the distortion mesh for a particular eye.
   *
   * <p>Important: The distorsion {@code Mesh} that is returned by this function becomes invalid if
   * {@code LensDistortion} is destroyed.
   *
   * @param[in] eye Desired eye.
   * @return A distorsion mesh. When {@code destroy()} has been called, this method returns null.
   */
  @Nullable
  public Mesh getDistortionMesh(int eye) {
    return nativeLensDistortionGetDistortionMesh(nativeLensDistortion, eye);
  }

  /**
   * Applies lens inverse distortion function to a point normalized [0,1] in pre-distortion (eye
   * texture) space.
   *
   * @param[in] distortedUv Distorted UV point.
   * @param[in] eye Desired eye.
   * @return A point normalized [0,1] in the screen post distort space. When {@code destroy()} has
   *     been called, this method returns null.
   */
  @Nullable
  public UvPoint undistortedUvForDistortedUv(UvPoint distortedUv, int eye) {
    return nativeLensDistortionUndistortedUvForDistortedUv(nativeLensDistortion, distortedUv, eye);
  }

  /**
   * Applies lens distortion function to a point normalized [0,1] in the screen post-distortion
   * space.
   *
   * @param[in] undistortedUv Undistorted UV point.
   * @param[in] eye Desired eye.
   * @return A point normalized [0,1] in pre distort space (eye texture space). When {@code
   *     destroy()} has been called, this method returns null.
   */
  @Nullable
  public UvPoint distortedUvForUndistortedUv(UvPoint undistortedUv, int eye) {
    return nativeLensDistortionDistortedUvForUndistortedUv(
        nativeLensDistortion, undistortedUv, eye);
  }

  private native long nativeLensDistortionCreate(
      byte[] deviceParams, int displayWidth, int displayHeight);

  private native void nativeLensDistortionDestroy(long nativeLensDistortion);

  private native void nativeLensDistortionGetEyeFromHeadMatrix(
      long nativeLensDistortion, int eye, float[] eyeFromHeadMatrix);

  private native void nativeLensDistortionGetEyeProjectionMatrix(
      long nativeLensDistortion, int eye, float zNear, float zFar, float[] projectionMatrix);

  private native Mesh nativeLensDistortionGetDistortionMesh(long nativeLensDistortion, int eye);

  private native UvPoint nativeLensDistortionUndistortedUvForDistortedUv(
      long nativeLensDistortion, UvPoint distortedUv, int eye);

  private native UvPoint nativeLensDistortionDistortedUvForUndistortedUv(
      long nativeLensDistortion, UvPoint undistortedUv, int eye);
}
