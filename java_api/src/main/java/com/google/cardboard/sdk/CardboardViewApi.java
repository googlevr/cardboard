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

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.hardware.display.DisplayManager;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;
import androidx.core.app.ActivityCompat;
import com.google.cardboard.proto.CardboardDevice;
import com.google.cardboard.sdk.deviceparams.CardboardV1DeviceParams;
import com.google.cardboard.sdk.deviceparams.DeviceParamsUtils;
import com.google.cardboard.sdk.nativetypes.EyeTextureDescription;
import com.google.cardboard.sdk.nativetypes.EyeType;
import com.google.cardboard.sdk.nativetypes.Mesh;
import com.google.cardboard.sdk.nativetypes.UvPoint;
import java.util.ArrayList;
import java.util.List;

/**
 * Wrapper around {@code DistortionRenderer}, {@code HeadTracker}, {@code LensDistortion}, {@code
 * QrCode} and {@code Initialize}.
 */
public class CardboardViewApi {
  /**
   * Enum that corresponds to the values of CardboardViewportOrientation in
   * google3/third_party/cardboard_oss/sdk/include/cardboard.h
   */
  private enum CardboardViewportOrientation {
    // Corresponds to CardboardViewportOrientation::kLandscapeLeft
    LANDSCAPE_LEFT(0),
    // Corresponds to CardboardViewportOrientation::kLandscapeRight
    LANDSCAPE_RIGHT(1),
    // Corresponds to CardboardViewportOrientation::kPortrait
    PORTRAIT(2),
    // Corresponds to CardboardViewportOrientation::kPortraitUpsideDown
    PORTRAIT_REVERSED(3);

    private final int value;

    private CardboardViewportOrientation(int value) {
      this.value = value;
    }

    public int getValue() {
      return value;
    }
  }

  /** Holds the screen size information in pixels. */
  public static class ScreenSize {
    final int width;
    final int height;

    /**
     * Constructs a ScreenSize.
     *
     * @param[in] width The screen width in pixels. It must be positive.
     * @param[in] height The screen height in pixels. It must be positive.
     */
    ScreenSize(int width, int height) {
      this.width = width;
      this.height = height;
    }
  }

  /** Holds device's field of view. */
  public static class FieldOfView {
    final float left;
    final float top;
    final float right;
    final float bottom;

    /**
     * Constructs a FieldOfView.
     *
     * @param[in] left The left angle in degrees of the field of view. It must be positive.
     * @param[in] right The right angle in degrees of the field of view. It must be positive.
     * @param[in] bottom The bottom angle in degrees of the field of view. It must be positive.
     * @param[in] top The top angle in degrees of the field of view. It must be positive.
     */
    public FieldOfView(float left, float right, float bottom, float top) {
      this.left = left;
      this.top = top;
      this.right = right;
      this.bottom = bottom;
    }
  }

  /**
   * Default delta time in nanoseconds to get the future pose estimate from the {@code HeadTracker}.
   */
  private static final long PREDICTION_TIME_WITHOUT_VSYNC_NANOS = 50000000;

  /** Default display value to forward to {@code DistortionRenderer}. */
  private static final long NATIVE_TARGET_DISPLAY = 0;

  /** Holds the screen size information. */
  private ScreenSize screenSize;

  /** Holds the {@code DistortionRenderer} object. */
  private DistortionRenderer distortionRenderer;

  /** Holds the {@code LensDistortion} object. */
  private LensDistortion lensDistortion;

  /** Holds the {@code HeadTracker} object. */
  private HeadTracker headTracker;

  /** Holds device's field of view. */
  private FieldOfView fieldOfView;

  /** Delta time in nanoseconds to obtain the pose from {@code HeadTracker}. */
  private long deltaTimeInNs;

  /** The current Context. It is generally an Activity instance or wraps one. */
  private final Context context;

  /** The current display. Used to get the display rotation. */
  private final Display display;

  /** The current display rotation. */
  private int displayRotation;

  /** The cached display rotation from the previous call to {@code getPose()}. */
  private int cachedDisplayRotation;

  /** The current device orientation. */
  private CardboardViewportOrientation deviceOrientation;

  private static final String CARDBOARD_CONFIGURE_ACTION =
      "com.google.vrtoolkit.cardboard.CONFIGURE";

  /** Tag for logging purposes. */
  private static final String TAG = CardboardViewApi.class.getSimpleName();

  /**
   * Constructs a CardboardViewApi.
   *
   * <p>It initializes the JNI modules and loads the {@code HeadTracker}. For the pose estimation, a
   * default positive delta time is assigned which leads to obtaining a future pose every time it is
   * queried.
   *
   * <p>Consumers of this object should refer to {@code loadDeviceParams()} and {@code
   * initializeRenderThread()} to fully initialize this object with the device (viewer) information.
   *
   * @param[in] context The current Context. It is generally an Activity instance or wraps one. The
   *     following is the list of methods that will be queried: getFilesDir(), getPackageManager(),
   *     getResources(), getSystemService(Context.WINDOW_SERVICE), startActivity(Intent),
   *     getContentResolver(), getDisplay().
   */
  @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
  public CardboardViewApi(Context context) {
    // Guarantees proper initialization of the JavaVM and Android context.
    Initialize.initialize(context);

    this.context = context;
    distortionRenderer = null;
    lensDistortion = null;
    headTracker = new HeadTracker();

    screenSize = new ScreenSize(0, 0);
    fieldOfView = new FieldOfView(0f, 0f, 0f, 0f);

    deltaTimeInNs = PREDICTION_TIME_WITHOUT_VSYNC_NANOS;
    if (VERSION.SDK_INT >= VERSION_CODES.JELLY_BEAN_MR1) {
      DisplayManager displayManager =
          (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
      display = displayManager.getDisplay(Display.DEFAULT_DISPLAY);
    } else {
      // This is necessary because our minimum API level is 16
      display =
          ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
    }
    displayRotation = 0;
    cachedDisplayRotation = -1;
    deviceOrientation = CardboardViewportOrientation.LANDSCAPE_LEFT;
  }

  /**
   * Releases resources taken at construct time and after initializing device parameters.
   *
   * <p>Preconditions: it must be called from the rendering thread.
   */
  public void close() {
    if (distortionRenderer != null) {
      distortionRenderer.close();
      distortionRenderer = null;
    }
    if (lensDistortion != null) {
      lensDistortion.close();
      lensDistortion = null;
    }
    if (headTracker != null) {
      headTracker.close();
      headTracker = null;
    }
  }

  /**
   * Forwards a call to {@code close()} and is provided as a safe-guard in case it is not called.
   */
  @Override
  protected void finalize() throws Throwable {
    // TODO(b/146156526): Remove finalize() usage from Java API.
    close();
    super.finalize();
  }

  /**
   * Sets current screen parameters.
   *
   * @param[in] width The screen width in pixels.
   * @param[in] height The screen height in pixels.
   */
  public void setScreenParams(int width, int height) {
    screenSize = new ScreenSize(width, height);
  }

  /**
   * Getter of the configured width property of the screen parameters.
   *
   * @return The width in pixels of the screen.
   */
  public int getScreenParamsWidth() {
    return screenSize.width;
  }

  /**
   * Getter of the configured height property of the screen parameters.
   *
   * @return The height in pixels of the screen.
   */
  public int getScreenParamsHeight() {
    return screenSize.height;
  }

  /**
   * Getter of the device's field of view top angle.
   *
   * @return The device's field of view top angle
   */
  public float getFieldOfViewParamsTop() {
    return fieldOfView.top;
  }

  /**
   * Getter of the device's field of view left angle.
   *
   * @return The device's field of view left angle.
   */
  public float getFieldOfViewParamsLeft() {
    return fieldOfView.left;
  }

  /**
   * Getter of the device's field of view right angle.
   *
   * @return The device's field of view right angle.
   */
  public float getFieldOfViewParamsRight() {
    return fieldOfView.right;
  }

  /**
   * Getter of the device's field of view bottom angle.
   *
   * @return The device's field of view bottom angle.
   */
  public float getFieldOfViewParamsBottom() {
    return fieldOfView.bottom;
  }

  /**
   * Computes the eye from head matrix.
   *
   * <p>Preconditions: {@code LensDistortion} must be previously and successfully initialized. When
   * it is unmet, a call to this function results in a no-op.
   *
   * @param[in] eye Desired eye.
   * @param[out] eyeFromHeadMatrix 4x4 float eye from head matrix.
   * @throws IllegalArgumentException When @p eye is not a valid eye type.
   */
  public void getEyeFromHeadMatrix(int eye, float[] eyeFromHeadMatrix) {
    if (!isValidEyeType(eye)) {
      throw new IllegalArgumentException("eye is invalid.");
    }
    if (lensDistortion == null) {
      Log.w(TAG, "Tried to getEyeFromHeadMatrix but LensDistortion is not initialized.");
      return;
    }
    lensDistortion.getEyeFromHeadMatrix(eye, eyeFromHeadMatrix);
  }

  /**
   * Computes the projection matrix for an eye.
   *
   * <p>Preconditions: {@code LensDistortion} must be previously and successfully initialized. When
   * it is unmet, a call to this function results in a no-op.
   *
   * @param[in] eye Desired eye.
   * @param[in] zNear Near clip plane z-axis coordinate.
   * @param[in] zFar Far clip plane z-axis coordinate.
   * @param[out] projectionMatrix 4x4 float ideal projection matrix.
   * @throws IllegalArgumentException When @p eye is not a valid eye type.
   */
  public void getEyeProjectionMatrix(int eye, float zNear, float zFar, float[] projectionMatrix) {
    if (!isValidEyeType(eye)) {
      throw new IllegalArgumentException("eye is invalid.");
    }
    if (lensDistortion == null) {
      Log.w(TAG, "Tried to getEyeProjectionMatrix but LensDistortion is not initialized.");
      return;
    }
    lensDistortion.getEyeProjectionMatrix(eye, zNear, zFar, projectionMatrix);
  }

  /**
   * Computes the distortion {@code Mesh} for an eye.
   *
   * <p>Preconditions: {@code LensDistortion} must be previously and successfully initialized.
   *
   * <p>Important: The distorsion {@code Mesh} that is returned by this function becomes invalid if
   * {@code LensDistortion} is destroyed.
   *
   * @param[in] eye Desired eye.
   * @return The distortion {@code Mesh}.
   * @throws IllegalStateException When this method is called before a successful initialization of
   *     {@code LensDistortion}.
   */
  public Mesh getDistortionMesh(int eye) {
    if (lensDistortion == null) {
      throw new IllegalStateException("LensDistortion has not been initialized yet.");
    }
    if (!isValidEyeType(eye)) {
      throw new IllegalArgumentException("eye is invalid.");
    }
    return lensDistortion.getDistortionMesh(eye);
  }

  /**
   * Undistorts a {@code UvPoint} for an eye.
   *
   * <p>Preconditions: {@code LensDistortion} must be previously and successfully initialized.
   *
   * @param[in] distortedUv A distorted {@code UvPoint} point to undistort.
   * @param[in] eye Desired eye.
   * @return An undistorted {@code UvPoint}.
   * @throws IllegalStateException When this method is called before a successful initialization of
   *     {@code LensDistortion}.
   */
  UvPoint undistortedUvForDistortedUv(UvPoint distortedUv, int eye) {
    if (lensDistortion == null) {
      throw new IllegalStateException("LensDistortion has not been initialized yet.");
    }
    if (!isValidEyeType(eye)) {
      throw new IllegalArgumentException("eye is invalid.");
    }
    return lensDistortion.undistortedUvForDistortedUv(distortedUv, eye);
  }

  /**
   * Distorts a {@code UvPoint} for an eye.
   *
   * <p>Preconditions: {@code LensDistortion} must be previously and successfully initialized.
   *
   * @param[in] undistortedUv An undistorted {@code UvPoint} point to distort.
   * @param[in] eye Desired eye.
   * @return A distorted {@code UvPoint}.
   * @throws IllegalStateException When this method is called before a successful initialization of
   *     {@code LensDistortion}.
   */
  UvPoint distortedUvForUndistortedUv(UvPoint undistortedUv, int eye) {
    if (lensDistortion == null) {
      throw new IllegalStateException("LensDistortion has not been initialized yet.");
    }
    if (!isValidEyeType(eye)) {
      throw new IllegalArgumentException("eye is invalid.");
    }
    return lensDistortion.distortedUvForUndistortedUv(undistortedUv, eye);
  }

  /**
   * Initializes all GL-related modules.
   *
   * <p>Preconditions: must be called from the rendering thread.
   */
  public void initializeRenderThread() {
    if (distortionRenderer != null) {
      distortionRenderer.close();
    }
    distortionRenderer = new DistortionRenderer();
  }

  /**
   * Sets a {@code Mesh} for an eye.
   *
   * <p>Preconditions: {@code DistortionRenderer} must be previously and successfully initialized.
   * When it is unment, a call to this function results in a no-op.
   *
   * <p>Preconditions: must be called from the rendering thread.
   *
   * @param[in] mesh The {@code Mesh} to set.
   * @param[in] eye Desired eye.
   */
  public void setMesh(Mesh mesh, int eye) {
    if (distortionRenderer == null) {
      Log.w(TAG, "Tried to setMesh but DistortionRenderer is not initialized.");
      return;
    }
    if (!isValidEyeType(eye)) {
      throw new IllegalArgumentException("eye is invalid.");
    }
    distortionRenderer.setMesh(mesh, eye);
  }

  /**
   * Renders left and right eye textures to display.
   *
   * <p>Preconditions: {@code DistortionRenderer} must be previously and successfully initialized.
   * When it is unment, a call to this function results in a no-op.
   *
   * <p>Preconditions: must be called from the rendering thread.
   *
   * @param[in] leftEyeTextureDescription Left eye texture description.
   * @param[in] rightEyeTextureDescription Right eye texture description.
   */
  public void renderEyeToDisplay(
      EyeTextureDescription leftEyeTextureDescription,
      EyeTextureDescription rightEyeTextureDescription) {
    if (distortionRenderer == null) {
      Log.w(TAG, "Tried to renderEyeToDisplay but DistortionRenderer is not initialized.");
      return;
    }
    // TODO(b/156227563): Unhardcode rect information.
    distortionRenderer.renderEyeToDisplay(
        NATIVE_TARGET_DISPLAY,
        0,
        0,
        screenSize.width,
        screenSize.height,
        leftEyeTextureDescription,
        rightEyeTextureDescription);
  }

  /**
   * Pauses the {@code HeadTracker} pose tracking.
   *
   * <p>Precondition: the {@code HeadTracker} is initialized. When it is unmet, a call to this
   * function results in a no-op.
   */
  public void pauseHeadTracker() {
    if (headTracker != null) {
      headTracker.pause();
    } else {
      Log.w(TAG, "Tried to pause the HeadTracker but it is not initialized.");
    }
  }

  /**
   * Resumes the {@code HeadTracker} pose tracking.
   *
   * <p>Precondition: the {@code HeadTracker} is initialized. When it is unmet, a call to this
   * function results in a no-op.
   */
  public void resumeHeadTracker() {
    if (headTracker != null) {
      headTracker.resume();
    } else {
      Log.w(TAG, "Tried to resume the HeadTracker but it is not initialized.");
    }
  }

  /**
   * Sets the delta time in nanoseconds used to compute the pose at some point in time with respect
   * to the call time of {@code getPose()}.
   */
  public void setDeltaTimeInNs(long deltaTimeInNs) {
    this.deltaTimeInNs = deltaTimeInNs;
  }

  /**
   * Gets the value of {@code deltaTimeInNs}.
   *
   * @return The time in nanoseconds used to compute the pose at some point in time with respect to
   *     the call time of {@code getPose()}.
   */
  public long getDeltaTimeInNs() {
    return deltaTimeInNs;
  }

  /**
   * Gets the pose of the {@code HeadTracker}.
   *
   * <p>Precondition: the {@code HeadTracker} is initialized. When it is unmet, a call to this
   * function results in a no-op.
   *
   * @param[out] position 3 floats for (x, y, z).
   * @param[out] orientation 4 floats for quaternion.
   */
  public void getPose(float[] position, float[] orientation) {
    displayRotation = display.getRotation();

    if (cachedDisplayRotation != displayRotation) {
      setDeviceOrientation(displayRotation);
    }

    if (headTracker != null) {
      headTracker.getPose(deltaTimeInNs, deviceOrientation.getValue(), position, orientation);
    } else {
      Log.w(TAG, "Tried to get the pose from HeadTracker but it is not initialized.");
    }
  }

  /**
   * Checks for device parameters existence and initializes {@code LensDistortion} module if they
   * exist.
   *
   * <p>Loading flow: - If the application is run in Android P or below, the READ_EXTERNAL_STORAGE
   * permission is checked. If granted, parameters are read from external storage. - If the
   * application is run in Android Q or above, device parameters are read from scoped storage. - If
   * the previous step failed, it uses Cardboard V1 device parameters.
   */
  public void loadDeviceParams() {
    byte[] deviceParams = null;

    if ((VERSION.SDK_INT <= VERSION_CODES.P && isReadExternalStoragePermissionEnabled())
        || VERSION.SDK_INT > VERSION_CODES.P) {
      deviceParams = QrCode.getSavedDeviceParams();
    }

    if (deviceParams == null) {
      deviceParams = QrCode.getCardboardV1DeviceParams();
    }

    initializeFieldOfView(deviceParams);
    if (lensDistortion != null) {
      lensDistortion.close();
    }
    lensDistortion = new LensDistortion(deviceParams, screenSize.width, screenSize.height);
  }

  /**
   * Checks if a Cardboard viewer has previously been scanned and its parameters saved.
   *
   * @return true if a Cardboard viewer has previously been scanned and its parameters saved.
   */
  public boolean hasSavedDeviceParams() {
    if (VERSION.SDK_INT <= VERSION_CODES.P && !isReadExternalStoragePermissionEnabled()) {
      return false;
    }
    return QrCode.getSavedDeviceParams() != null;
  }

  /**
   * Returns the device parameters of the Cardboard viewer.
   *
   * @return the device parameters of the Cardboard viewer.
   */
  public CardboardDevice.DeviceParams getSavedDeviceParams() {
    return DeviceParamsUtils.parseCardboardDeviceParams(QrCode.getSavedDeviceParams());
  }

  /** Scans a Cardboard QR code from a viewer and saves the obtained parameters. */
  public void scanViewerQrCode() {
    QrCode.scanQrCodeAndSaveDeviceParams();
  }

  /** Launches VRCore Settings Activity. */
  public void launchVrCoreSettingsActivity() {
    PackageManager pm = context.getPackageManager();
    Intent settingsIntent = new Intent();
    settingsIntent.setAction(CARDBOARD_CONFIGURE_ACTION).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

    // Grab the settings intent handlers whose package begins with 'com.google.'
    // Note that the result should be ordered by *descending* priority, but we make
    // no such assumption.
    // TODO: b/441496226 - Add <queries> declaration to manifest.
    List<ResolveInfo> resolveInfos = pm.queryIntentActivities(settingsIntent, 0 /* flags */);
    List<Intent> googleIntentsWithHighestPriority = new ArrayList<>();
    Integer highestGoogleIntentPriority = null;
    for (ResolveInfo info : resolveInfos) {
      String pkgName = info.activityInfo.packageName;
      if (!PackageUtils.isGooglePackage(pkgName)) {
        continue;
      }

      // With N+, apps are no longer allowed to give >0 priority to intent-filters.
      // Prioritizing system apps avoids the disambiguation dialog when
      // legacy Cardboard apps (with a priority of 0) are installed alongside a
      // system VrCore installation.
      int priority = info.priority;
      if (PackageUtils.isSystemPackage(context, pkgName)) {
        ++priority;
      }

      if (highestGoogleIntentPriority == null) {
        highestGoogleIntentPriority = priority;
      } else if (priority > highestGoogleIntentPriority.intValue()) {
        // If the intent has a higher priority, accept it and ignore all lower priority matches.
        highestGoogleIntentPriority = priority;
        googleIntentsWithHighestPriority.clear();
      } else if (priority < highestGoogleIntentPriority.intValue()) {
        // If the intent has a lesser priority, ignore it.
        continue;
      }

      Intent intent = new Intent(settingsIntent);
      intent.setClassName(pkgName, info.activityInfo.name);
      googleIntentsWithHighestPriority.add(intent);
    }

    if (!googleIntentsWithHighestPriority.isEmpty()) {
      // If there is only one matching Google-owned activity with the highest priority, have the
      // dialog directly launch it and avoid offering a chooser for any third-party apps that
      // intercept the intent. Otherwise, go through intent resolution as usual. Note this may show
      // third-party apps that intercept the intent, which is fine since the list is guaranteed to
      // also contain at least 2 Google-owned Cardboard configuration Activities.
      Intent intent =
          googleIntentsWithHighestPriority.size() == 1
              ? googleIntentsWithHighestPriority.get(0)
              : settingsIntent;
      // Go straight to the activity.
      context.startActivity(intent);
    }
  }

  /**
   * Checks if reading external storage permission is granted.
   *
   * @return whether the permission is already granted.
   */
  private boolean isReadExternalStoragePermissionEnabled() {
    return ActivityCompat.checkSelfPermission(context, Manifest.permission.READ_EXTERNAL_STORAGE)
        == PackageManager.PERMISSION_GRANTED;
  }

  /**
   * Determines whether {@code eye} is equal to either {@code EyeType.LEFT} or {@code
   * EyeType.RIGHT}.
   *
   * @param eye An integer pointing to either the left or right eye.
   * @return true When {@code eye} is equal to either {@code EyeType.LEFT} or {@code EyeType.RIGHT}.
   */
  private static boolean isValidEyeType(int eye) {
    return eye == EyeType.LEFT || eye == EyeType.RIGHT;
  }

  /**
   * Initializes the field of view form device parameters.
   *
   * <p>Left eye field of view is an optional paramater. When it is provided, we set the custom
   * device field of view. Otherwise, Cardboard V1 device field of view is set.
   *
   * @param[in] deviceParamsBuffer A byte buffer with the serialized device parameters.
   */
  private void initializeFieldOfView(byte[] deviceParamsBuffer) {
    final CardboardDevice.DeviceParams deviceParams =
        DeviceParamsUtils.parseCardboardDeviceParams(deviceParamsBuffer);
    if (deviceParams.getLeftEyeFieldOfViewAnglesCount() == 4) {
      fieldOfView =
          new FieldOfView(
              deviceParams.getLeftEyeFieldOfViewAngles(0),
              deviceParams.getLeftEyeFieldOfViewAngles(1),
              deviceParams.getLeftEyeFieldOfViewAngles(2),
              deviceParams.getLeftEyeFieldOfViewAngles(3));
    } else {
      fieldOfView =
          new FieldOfView(
              CardboardV1DeviceParams.CARDBOARD_V1_FOV_ANGLES[0],
              CardboardV1DeviceParams.CARDBOARD_V1_FOV_ANGLES[1],
              CardboardV1DeviceParams.CARDBOARD_V1_FOV_ANGLES[2],
              CardboardV1DeviceParams.CARDBOARD_V1_FOV_ANGLES[3]);
    }
  }

  private void setDeviceOrientation(int displayRotation) {
    if (displayRotation == Surface.ROTATION_0) {
      // This corresponds to CardboardViewportOrientation::kPortrait
      deviceOrientation = CardboardViewportOrientation.PORTRAIT;
    } else if (displayRotation == Surface.ROTATION_180) {
      // This corresponds to CardboardViewportOrientation::kPortrait
      deviceOrientation = CardboardViewportOrientation.PORTRAIT_REVERSED;
    } else if (displayRotation == Surface.ROTATION_90) {
      // This corresponds to CardboardViewportOrientation::kLandscapeLeft
      deviceOrientation = CardboardViewportOrientation.LANDSCAPE_LEFT;
    } else if (displayRotation == Surface.ROTATION_270) {
      // This corresponds to CardboardViewportOrientation::kLandscapeRight
      deviceOrientation = CardboardViewportOrientation.LANDSCAPE_RIGHT;
    }
  }
}
