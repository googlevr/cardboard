// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle;

/**
 * Commonly used constants for LightCycle.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class Constants {
  /** The width of the generated thumbnails in pixels. */
  public static final int THUMBNAIL_WIDTH = 1000;

  /** The aspect ratio of the generated thumbnails. */
  public static final float THUMBNAIL_ASPECT_RATIO = 4.0f;

  /** The desired preview frame width in pixels. */
  public static final int DESIRED_PREVIEW_IMAGE_WIDTH = 320;

  /**
   * The maximum number of pixels allowed in a preview image. Size is limited to
   * make optical flow in gyro bias calibration feasible.
   */
  public static final int MAX_PREVIEW_IMAGE_PIXELS = 500000;

  /** The desired width in pixels for the captured images. **/
  public static final int DESIRED_PICTURE_IMAGE_WIDTH = 3000;

  /**
   * The desired aspect ratio (width / height) for both the preview and
   * full-sized photos.
   **/
  public static final double DESIRED_WIDTH_TO_HEIGHT_RATIO = 4.0 / 3.0;

  /** Rendering color constants */
  public static final float[] GRAY = {0.4196f, 0.4196f, 0.4196f, 1.0f};
  public static final float[] TRANSPARENT_GRAY =
      {0.4196f, 0.4196f, 0.4196f, 0.5f};
  public static final float[] GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
  public static final float[] ANDROID_BLUE = {0.2f, 0.71f, 0.898f, 1.0f};
  public static final float[] ANDROID_GRAY = {0.8f, 0.8f, 0.8f, 1.0f};
  public static final float[] BACKGROUND_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
  public static final float[] WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
  public static final float[] TRANSPARENT_WHITE = {1.0f, 1.0f, 1.0f, 0.6f};
  public static final float[] GROUND_PLANE_COLOR = {1.0f, 1.0f, 1.0f, 0.1f};

}
