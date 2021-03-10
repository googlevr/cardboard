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
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.WindowManager;
import android.content.res.Configuration;
import android.hardware.SensorManager;

/** Utility methods to manage the screen parameters. */
public abstract class ScreenParamsUtils {
  /** Holds the screen orientation. */
  private static ScreenOrientation screenOrientation;
    
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
    
  private static class ScreenOrientation {
        
    public static final int LANDSCAPE_LEFT = 0;
    public static final int PORTRAIT = 1;
    public static final int LANDSCAPE_RIGHT = 2;
    public static final int PORTRAIT_UPSIDE_DOWN = 3;
    public static final int UNKNOWN = -1;
        
    private static Context context;
    private static int orientation;
    private static int previousOrientation;
    private static OrientationEventListener orientationEventListener;

    public ScreenOrientation(final Context context) {
      this.context = context;
      this.orientation = getCurrentOrientation(context);

      this.orientationEventListener = new OrientationEventListener(context, SensorManager.SENSOR_DELAY_UI) {
        public void onOrientationChanged(int orientationIn) {

          // A call to getCurrentOrientation() can take a long time so we do not want to call it every frame.
          // context.getResources().getConfiguration().orientation detects landscape to portrait changes, this
          // is used to detect a change between any landscape (left or right) and any portrait (up or down).
          // It does not tell us the difference between landscape left to landscape right. We therefore do
          // another check near the range where a rotation change may occur from landscape left to right and vice
          // versa. This is between 60 and 120 for landscape right and between 240 and 300 for landscape left.

          int newOrientation = context.getResources().getConfiguration().orientation;
          if (orientationIn != ORIENTATION_UNKNOWN) {
            if (previousOrientation != newOrientation) {
              // Device rotate to/from landscape to/from portrait
              orientation = getCurrentOrientation(ScreenOrientation.context);
            } else if (orientationIn >= 60 && orientationIn <= 120 && orientation != LANDSCAPE_RIGHT) {
              // Device possibly rotated from landscape left to landscape right rotation
              orientation = getCurrentOrientation(ScreenOrientation.context);
            } else if (orientationIn <= 300 && orientationIn >= 240 && orientation != LANDSCAPE_LEFT) {
              // Device possibly rotated from landscape right to landscape left rotation
              orientation = getCurrentOrientation(ScreenOrientation.context);
            }
            previousOrientation = newOrientation;
          }
        }
      };
      if (this.orientationEventListener.canDetectOrientation()) {
        this.orientationEventListener.enable();
      }
    }
  }
    
  public static int getScreenOrientation(Context context) {
    if (screenOrientation == null) {
      screenOrientation = new ScreenOrientation(context);
    }
    return screenOrientation.orientation;
  }

  // This call takes a long time so we don't call this every frame, only when rotation changes
  private static int getCurrentOrientation (Context context) {
    Display defaultDisplay;

    if (VERSION.SDK_INT <= VERSION_CODES.Q) {
      defaultDisplay = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE))
                  .getDefaultDisplay();
    } else {
      defaultDisplay = context.getDisplay();
    }

    int orientation = context.getResources().getConfiguration().orientation;
    int rotation = defaultDisplay.getRotation();

    if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
      if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_90) {
        return ScreenOrientation.LANDSCAPE_LEFT;
      }
      return ScreenOrientation.LANDSCAPE_RIGHT;
    } else if (orientation == Configuration.ORIENTATION_PORTRAIT) {
      if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_90) {
        return ScreenOrientation.PORTRAIT;
      }
      return ScreenOrientation.PORTRAIT_UPSIDE_DOWN;
    }
    // Unexpected orientation value.
    return ScreenOrientation.UNKNOWN;
    }
}
