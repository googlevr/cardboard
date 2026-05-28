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

/** Cardboard SDK Head Tracker API. */
public class HeadTracker {

  // Opaque native pointer to the native HeadTracker instance.
  // This object is owned by this class instance and passed to the native methods.
  private long nativeHeadTracker;

  /** Creates a new head tracker object. */
  public HeadTracker() {
    nativeHeadTracker = nativeHeadTrackerCreate();
  }

  /**
   * Releases memory created at construct time by the native head tracker object. This method should
   * be executed just once after construct time, then it will not have effect and a new instance
   * must be instantiated. If this function is not called, additional resources will be consumed.
   */
  public void close() {
    nativeHeadTrackerDestroy(nativeHeadTracker);
    nativeHeadTracker = 0;
  }

  /** Calls {@code close()} in case that method is mistakenly not called. */
  @Override
  protected void finalize() throws Throwable {
    // TODO(b/146156526): Remove finalize() usage from Java API.
    close();
    super.finalize();
  }

  /** Pauses head tracker and underlying device sensors. */
  public void pause() {
    nativeHeadTrackerPause(nativeHeadTracker);
  }

  /** Resumes head tracker and underlying device sensors. */
  public void resume() {
    nativeHeadTrackerResume(nativeHeadTracker);
  }

  /**
   * Gets the predicted head pose for a given timestamp.
   *
   * <p>Preconditions: position Must not be null. Preconditions: orientation Must not be null.
   *
   * @param[in] deltaTimeNs Delta time in nanoseconds from current system boot time to obtain the
   *     pose.
   * @param[in] deviceOrientation Orientation of the device.
   * @param[out] position 3 floats for (x, y, z).
   * @param[out] orientation 4 floats for quaternion.
   */
  public void getPose(
      long deltaTimeNs, int deviceOrientation, float[] position, float[] orientation) {
    nativeHeadTrackerGetPose(
        nativeHeadTracker, deltaTimeNs, deviceOrientation, position, orientation);
  }

  private native long nativeHeadTrackerCreate();

  private native void nativeHeadTrackerDestroy(long nativeHeadTracker);

  private native void nativeHeadTrackerPause(long nativeHeadTracker);

  private native void nativeHeadTrackerResume(long nativeHeadTracker);

  private native void nativeHeadTrackerGetPose(
      long nativeHeadTracker,
      long deltaTimeNs,
      int deviceOrientation,
      float[] position,
      float[] orientation);
}
