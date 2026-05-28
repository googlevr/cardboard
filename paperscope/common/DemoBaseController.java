package com.google.vr.cardboard.paperscope.demo.common;

import static java.lang.Math.min;

import android.view.OrientationEventListener;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.CardboardView.Renderer;

/** Controller for the demo activity. */
public class DemoBaseController<T extends Renderer> {
  private static final String TAG = DemoBaseController.class.getSimpleName();
  protected final CardboardView cardboardView;
  protected T renderer;
  private static final int TILT_DETECTION_TOLERANCE_IN_DEGREES = 30;
  private static final int LANDSCAPE_LEFT_DEGREES = 270;
  private static final int LANDSCAPE_RIGHT_DEGREES = 90;

  public DemoBaseController(CardboardView cardboardView) {
    this.cardboardView = cardboardView;
    CardboardView.setUseCardboardGlSurfaceView(true);
    this.cardboardView.setOnViewDetachedRunnable(this::shutdown);
    this.cardboardView.setStereoRenderMode(true);
  }

  public void setOnCloseButtonListener(Runnable onCloseButtonListener) {
    cardboardView.setOnBackButtonClick(onCloseButtonListener);
  }

  public void setOnSettingsButtonListener(Runnable onSettingsButtonListener) {
    cardboardView.setOnSettingsButtonClick(onSettingsButtonListener);
  }

  public void enableOnTriggerEventListener() {}

  public void onResume() {
    cardboardView.onResume();
  }

  public void onPause() {
    cardboardView.onPause();
  }

  public void shutdown() {
    if (renderer != null) {
      renderer.onRendererShutdown();
    }
    cardboardView.onDestroy();
  }

  /**
   * Determines whether the activity should be finished based on the device's orientation.
   *
   * @param orientation The current orientation of the device in degrees.
   * @return True if the activity should be finished, false otherwise.
   */
  public boolean shouldFinishActivity(int orientation) {
    if (orientation == OrientationEventListener.ORIENTATION_UNKNOWN) {
      return false;
    }

    return !isLandscape(orientation);
  }

  /**
   * Checks if the device is in a landscape orientation (left or right).
   *
   * @param orientationDegrees The current orientation of the device in degrees.
   * @return True if the device is in a landscape orientation, false otherwise.
   */
  private boolean isLandscape(int orientationDegrees) {
    return isLandscapeLeft(orientationDegrees) || isLandscapeRight(orientationDegrees);
  }

  /**
   * Checks if the device is in a left landscape orientation.
   *
   * @param orientationDegrees The current orientation of the device in degrees.
   * @return True if the device is in a left landscape orientation, false otherwise.
   */
  private boolean isLandscapeLeft(int orientationDegrees) {
    return getCircularDistance(orientationDegrees, LANDSCAPE_LEFT_DEGREES)
        < TILT_DETECTION_TOLERANCE_IN_DEGREES;
  }

  /**
   * Checks if the device is in a right landscape orientation.
   *
   * @param orientationDegrees The current orientation of the device in degrees.
   * @return True if the device is in a right landscape orientation, false otherwise.
   */
  private boolean isLandscapeRight(int orientationDegrees) {
    return getCircularDistance(orientationDegrees, LANDSCAPE_RIGHT_DEGREES)
        < TILT_DETECTION_TOLERANCE_IN_DEGREES;
  }

  /**
   * Returns the shortest distance between two angles on a circle.
   *
   * @param angle1 The first angle, in degrees.
   * @param angle2 The second angle, in degrees.
   * @return The shortest distance, in degrees.
   */
  private int getCircularDistance(int angle1, int angle2) {
    int diff = Math.abs(angle1 - angle2);
    return min(diff, 360 - diff);
  }
}
