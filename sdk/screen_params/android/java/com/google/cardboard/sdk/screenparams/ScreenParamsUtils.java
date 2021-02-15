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
package com.google.cardboard.sdk.screenparams;

import android.content.Context;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.DisplayMetrics;
import android.view.WindowManager;

// Aryzon multiple orientations
import android.content.res.Configuration;
import android.hardware.SensorManager;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.Surface;

/** Utility methods to manage the screen parameters. */
public abstract class ScreenParamsUtils {
  /** Holds the screen pixel density. */
  public static class ScreenPixelDensity {
    /** The exact number of pixels per inch in the x direction. */
    public final float xdpi;
    /** The exact number of pixels per inch in the y direction. */
    public final float ydpi;

    /**
     * Constructor.
     *
     * @param[in] xdpi The exact number of pixels per inch in the x direction.
     * @param[in] ydpi The exact number of pixels per inch in the y direction.
     */
    public ScreenPixelDensity(float xdpi, float ydpi) {
      this.xdpi = xdpi;
      this.ydpi = ydpi;
    }
  }

  /**
   * Constructor.
   *
   * <p>This class only contains static methods.
   */
  private ScreenParamsUtils() {}

  /**
   * Gets the screen pixel density.
   *
   * @param context The application context. When the {@code VERSION.SDK_INT} is less or equal
   *     to {@code VERSION_CODES.Q}, @p context will be used to retrieve a {@code WindowManager}.
   *     Otherwise, {@code Context} interface will be used.
   * @return A ScreenPixelDensity.
   */
  public static ScreenPixelDensity getScreenPixelDensity(Context context) {
    DisplayMetrics displayMetrics = new DisplayMetrics();
    if (VERSION.SDK_INT <= VERSION_CODES.Q) {
      ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE))
          .getDefaultDisplay()
          .getMetrics(displayMetrics);
    } else {
      context.getDisplay().getMetrics(displayMetrics);
    }
    return new ScreenPixelDensity(displayMetrics.xdpi, displayMetrics.ydpi);
  }
    
    // Aryzon multiple orientations
    public static class ScreenOrientation {
      /**
       * The screen orientation
       * 0 = lanscape left
       * 1 = portrait
       * 2 = landscape right
       * 3 = portrait upside-down
       */
      public static Context context;
      public static int orientation;
      public static int previousOrientation;
      public static OrientationEventListener orientationEventListener;

      public ScreenOrientation(final Context context) {
        this.context = context;
        this.orientation = CurrentOrientation(context);

        this.orientationEventListener = new OrientationEventListener(context, SensorManager.SENSOR_DELAY_UI) {
          public void onOrientationChanged(int orientation_) {

            int newOrientation = context.getResources().getConfiguration().orientation;

            if (orientation_ != ORIENTATION_UNKNOWN && previousOrientation != newOrientation) {
              orientation = CurrentOrientation(ScreenOrientation.context);
              previousOrientation = newOrientation;
            }
          }
        };
        if (this.orientationEventListener.canDetectOrientation()) {
          this.orientationEventListener.enable();
        }
      }
    }

    private static ScreenOrientation screenOrientation;
    private static Display defaultDisplay;
    
    public static int getScreenOrientation(Context context) {
      if (screenOrientation == null) {

        screenOrientation = new ScreenOrientation(context);
      }
      return screenOrientation.orientation;

    }

    private static int CurrentOrientation (Context context) {
      if (defaultDisplay == null) {
        WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        defaultDisplay = windowManager.getDefaultDisplay();
      }

      int orientation = context.getResources().getConfiguration().orientation;
      // This call takes a long time so we don't call this every frame, only when rotation changes
      int rotation = defaultDisplay.getRotation();

      if (orientation == Configuration.ORIENTATION_LANDSCAPE
              &&  (rotation == Surface.ROTATION_0
              ||  rotation == Surface.ROTATION_90)) {
        return 0;
      } else if (orientation == Configuration.ORIENTATION_PORTRAIT
              &&  (rotation == Surface.ROTATION_0
              ||  rotation == Surface.ROTATION_90)) {
        return 1;
      } else if (orientation == Configuration.ORIENTATION_LANDSCAPE
              &&  (rotation == Surface.ROTATION_180
              ||  rotation == Surface.ROTATION_270)) {
        return 2;
      } else if (orientation == Configuration.ORIENTATION_PORTRAIT
              &&  (rotation == Surface.ROTATION_180
              ||  rotation == Surface.ROTATION_270)) {
        return 3;
      }
      return 3;
    }
}
