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
package com.google.cardboard.sdk.qrcode.camera;

import android.Manifest;
import android.content.Context;
import android.graphics.ImageFormat;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.WindowManager;
import androidx.annotation.RequiresPermission;
import com.google.android.gms.common.images.Size;
import com.google.android.gms.vision.Detector;
import com.google.android.gms.vision.Frame;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Manages the camera in conjunction with an underlying detector. This receives preview frames from
 * the camera at a specified rate, sending those frames to the detector as fast as it is able to
 * process those frames.
 *
 * <p>Deprecation warnings are suppressed on this class given that {@code android.hardware.Camera}
 * is currently marked as deprecated but intentionally used in order to ease the camera usage.
 */
@SuppressWarnings("deprecation")
public class CameraSource {
  private static final String TAG = CameraSource.class.getSimpleName();

  private static final float ASPECT_RATIO_TOLERANCE = 0.01f;

  // Preferred width in pixels.
  private static final int WIDTH = 1600;

  // Preferred height in pixels.
  private static final int HEIGHT = 1200;

  /**
   * These values may be requested by the caller. Due to hardware limitations, we may need to select
   * close, but not exactly the same values for these.
   */
  private static final float FPS = 15.0f;

  private final Context context;

  private final Object cameraLock = new Object();

  // Guarded by cameraLock
  private android.hardware.Camera camera;

  private int rotation;

  private Size previewSize;

  /**
   * Dedicated thread and associated runnable for calling into the detector with frames, as the
   * frames become available from the camera.
   */
  private Thread processingThread;

  private final FrameProcessingRunnable frameProcessor;

  /**
   * Map to convert between a byte array, received from the camera, and its associated byte buffer.
   */
  private final Map<byte[], ByteBuffer> bytesToByteBuffer = new HashMap<>();

  /**
   * Constructs a CameraSource.
   *
   * <p>Creates a camera source builder with the supplied context and detector. Camera preview
   * images will be streamed to the associated detector upon starting the camera source.
   *
   * @param context The Android's Application context.
   * @param detector Expects a QR code detector.
   * @throws IllegalArgumentException When any of the parameter preconditions is unmet.
   */
  public CameraSource(Context context, Detector<?> detector) {
    // Prerequisite evaluation.
    if (context == null) {
      Log.e(TAG, "context is null.");
      throw new IllegalArgumentException("No context supplied.");
    }
    if (detector == null) {
      Log.e(TAG, "detector is null.");
      throw new IllegalArgumentException("No detector supplied.");
    }

    this.context = context;
    frameProcessor = new FrameProcessingRunnable(detector);
    Log.i(TAG, "Successful CameraSource creation.");
  }

  /** Stops the camera and releases the resources of the camera and underlying detector. */
  public void release() {
    synchronized (cameraLock) {
      stop();
      frameProcessor.release();
    }
  }

  /**
   * Opens the camera and starts sending preview frames to the underlying detector. The supplied
   * surface holder is used for the preview so frames can be displayed to the user.
   *
   * @param surfaceHolder the surface holder to use for the preview frames
   * @throws IOException if the supplied surface holder could not be used as the preview display
   */
  @RequiresPermission(Manifest.permission.CAMERA)
  public CameraSource start(SurfaceHolder surfaceHolder) throws IOException {
    synchronized (cameraLock) {
      if (camera != null) {
        return this;
      }

      camera = createCamera();
      camera.setPreviewDisplay(surfaceHolder);
      camera.startPreview();

      processingThread = new Thread(frameProcessor);
      frameProcessor.setActive(true);
      processingThread.start();
    }
    return this;
  }

  /** Closes the camera and stops sending frames to the underlying frame detector. */
  public void stop() {
    synchronized (cameraLock) {
      frameProcessor.setActive(false);
      if (processingThread != null) {
        try {
          processingThread.join();
        } catch (InterruptedException e) {
          Log.d(TAG, "Frame processing thread interrupted on release.");
        }
        processingThread = null;
      }

      // Clear the buffer to prevent OOM exceptions
      bytesToByteBuffer.clear();

      if (camera != null) {
        camera.stopPreview();
        camera.setPreviewCallbackWithBuffer(null);
        try {
          camera.setPreviewTexture(null);
        } catch (Exception e) {
          Log.e(TAG, "Failed to clear camera preview: " + e);
        }
        camera.release();
        camera = null;
      }
    }
  }

  /** Returns the preview size that is currently in use by the underlying camera. */
  public Size getPreviewSize() {
    return previewSize;
  }

  /**
   * Opens the camera and applies the user settings.
   *
   * @throws RuntimeException if the method fails
   */
  private android.hardware.Camera createCamera() {
    int requestedCameraId =
        getIdForRequestedCamera(android.hardware.Camera.CameraInfo.CAMERA_FACING_BACK);
    if (requestedCameraId == -1) {
      Log.e(TAG, "Could not find requested camera.");
      throw new RuntimeException("Could not find requested camera.");
    }
    android.hardware.Camera camera = android.hardware.Camera.open(requestedCameraId);

    SizePair sizePair = selectSizePair(camera, WIDTH, HEIGHT);
    if (sizePair == null) {
      Log.e(TAG, "Could not find suitable preview size.");
      throw new RuntimeException("Could not find suitable preview size.");
    }
    Size pictureSize = sizePair.pictureSize();
    previewSize = sizePair.previewSize();

    int[] previewFpsRange = selectPreviewFpsRange(camera, FPS);
    if (previewFpsRange == null) {
      Log.e(TAG, "Could not find suitable preview frames per second range.");
      throw new RuntimeException("Could not find suitable preview frames per second range.");
    }

    android.hardware.Camera.Parameters parameters = camera.getParameters();

    if (pictureSize != null) {
      parameters.setPictureSize(pictureSize.getWidth(), pictureSize.getHeight());
    }

    parameters.setPreviewSize(previewSize.getWidth(), previewSize.getHeight());
    parameters.setPreviewFpsRange(
        previewFpsRange[android.hardware.Camera.Parameters.PREVIEW_FPS_MIN_INDEX],
        previewFpsRange[android.hardware.Camera.Parameters.PREVIEW_FPS_MAX_INDEX]);
    parameters.setPreviewFormat(ImageFormat.NV21);

    setRotation(camera, parameters, requestedCameraId);

    if (parameters
        .getSupportedFocusModes()
        .contains(android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
      parameters.setFocusMode(android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
    } else {
      Log.i(
          TAG, "Camera focus mode: FOCUS_MODE_CONTINUOUS_PICTURE is not supported on this device.");
    }

    camera.setParameters(parameters);

    // Four frame buffers are needed for working with the camera:
    //
    //   one for the frame that is currently being executed upon in doing detection
    //   one for the next pending frame to process immediately upon completing detection
    //   two for the frames that the camera uses to populate future preview images
    camera.setPreviewCallbackWithBuffer(new CameraPreviewCallback());
    camera.addCallbackBuffer(createPreviewBuffer(previewSize));
    camera.addCallbackBuffer(createPreviewBuffer(previewSize));
    camera.addCallbackBuffer(createPreviewBuffer(previewSize));
    camera.addCallbackBuffer(createPreviewBuffer(previewSize));

    Log.i(TAG, "Successfull camera creation.");
    return camera;
  }

  /**
   * Gets the id for the camera specified by the direction it is facing. Returns -1 if no such
   * camera was found.
   *
   * @param facing the desired camera (front-facing or rear-facing)
   */
  private static int getIdForRequestedCamera(int facing) {
    android.hardware.Camera.CameraInfo cameraInfo = new android.hardware.Camera.CameraInfo();
    for (int i = 0; i < android.hardware.Camera.getNumberOfCameras(); ++i) {
      android.hardware.Camera.getCameraInfo(i, cameraInfo);
      if (cameraInfo.facing == facing) {
        return i;
      }
    }
    return -1;
  }

  /**
   * Selects the most suitable preview and picture size, given the desired width and height.
   *
   * @param camera the camera to select a preview size from
   * @param desiredWidth the desired width of the camera preview frames
   * @param desiredHeight the desired height of the camera preview frames
   * @return the selected preview and picture size pair
   */
  private static SizePair selectSizePair(
      android.hardware.Camera camera, int desiredWidth, int desiredHeight) {
    List<SizePair> validPreviewSizes = generateValidPreviewSizeList(camera);

    // The method for selecting the best size is to minimize the sum of the differences between
    // the desired values and the actual values for width and height.
    SizePair selectedPair = null;
    int minDiff = Integer.MAX_VALUE;
    for (SizePair sizePair : validPreviewSizes) {
      Size size = sizePair.previewSize();
      int diff =
          Math.abs(size.getWidth() - desiredWidth) + Math.abs(size.getHeight() - desiredHeight);
      if (diff < minDiff) {
        selectedPair = sizePair;
        minDiff = diff;
      }
    }

    return selectedPair;
  }

  /**
   * Stores a preview size and a corresponding same-aspect-ratio picture size. To avoid distorted
   * preview images on some devices, the picture size must be set to a size that is the same aspect
   * ratio as the preview size or the preview may end up being distorted. If the picture size is
   * null, then there is no picture size with the same aspect ratio as the preview size.
   */
  private static class SizePair {
    private final Size preview;
    private Size picture;

    public SizePair(
        android.hardware.Camera.Size previewSize, android.hardware.Camera.Size pictureSize) {
      preview = new Size(previewSize.width, previewSize.height);
      if (pictureSize != null) {
        picture = new Size(pictureSize.width, pictureSize.height);
      }
    }

    public Size previewSize() {
      return preview;
    }

    public Size pictureSize() {
      return picture;
    }
  }

  /**
   * Generates a list of acceptable preview sizes. Preview sizes are not acceptable if there is not
   * a corresponding picture size of the same aspect ratio. If there is a corresponding picture size
   * of the same aspect ratio, the picture size is paired up with the preview size.
   */
  private static List<SizePair> generateValidPreviewSizeList(android.hardware.Camera camera) {
    android.hardware.Camera.Parameters parameters = camera.getParameters();
    List<android.hardware.Camera.Size> supportedPreviewSizes =
        parameters.getSupportedPreviewSizes();
    List<android.hardware.Camera.Size> supportedPictureSizes =
        parameters.getSupportedPictureSizes();
    List<SizePair> validPreviewSizes = new ArrayList<>();
    for (android.hardware.Camera.Size previewSize : supportedPreviewSizes) {
      float previewAspectRatio = (float) previewSize.width / (float) previewSize.height;

      // By looping through the picture sizes in order, we favor the higher resolutions.
      for (android.hardware.Camera.Size pictureSize : supportedPictureSizes) {
        float pictureAspectRatio = (float) pictureSize.width / (float) pictureSize.height;
        if (Math.abs(previewAspectRatio - pictureAspectRatio) < ASPECT_RATIO_TOLERANCE) {
          validPreviewSizes.add(new SizePair(previewSize, pictureSize));
          break;
        }
      }
    }

    // If there are no picture sizes with the same aspect ratio as any preview sizes, allow all
    // of the preview sizes and hope that the camera can handle it.
    if (validPreviewSizes.isEmpty()) {
      Log.w(TAG, "No preview sizes have a corresponding same-aspect-ratio picture size");
      for (android.hardware.Camera.Size previewSize : supportedPreviewSizes) {
        // The null picture size will let us know that we shouldn't set a picture size.
        validPreviewSizes.add(new SizePair(previewSize, null));
      }
    }

    return validPreviewSizes;
  }

  /**
   * Selects the most suitable preview frames per second range, given the desired frames per second.
   *
   * @param camera the camera to select a frames per second range from
   * @param desiredPreviewFps the desired frames per second for the camera preview frames
   * @return the selected preview frames per second range
   */
  private static int[] selectPreviewFpsRange(
      android.hardware.Camera camera, float desiredPreviewFps) {
    // The camera API uses integers scaled by a factor of 1000 frame rates.
    int desiredPreviewFpsScaled = (int) (desiredPreviewFps * 1000.0f);

    // The method for selecting the best range is to minimize the sum of the differences between
    // the desired value and the upper and lower bounds of the range.
    int[] selectedFpsRange = null;
    int minDiff = Integer.MAX_VALUE;
    List<int[]> previewFpsRangeList = camera.getParameters().getSupportedPreviewFpsRange();
    for (int[] range : previewFpsRangeList) {
      int deltaMin =
          desiredPreviewFpsScaled - range[android.hardware.Camera.Parameters.PREVIEW_FPS_MIN_INDEX];
      int deltaMax =
          desiredPreviewFpsScaled - range[android.hardware.Camera.Parameters.PREVIEW_FPS_MAX_INDEX];
      int diff = Math.abs(deltaMin) + Math.abs(deltaMax);
      if (diff < minDiff) {
        selectedFpsRange = range;
        minDiff = diff;
      }
    }
    return selectedFpsRange;
  }

  /**
   * Calculates the correct rotation for the given camera id and sets the rotation in the
   * parameters. It also sets the camera's display orientation and rotation.
   *
   * @param parameters the camera parameters for which to set the rotation
   * @param cameraId the camera id to set rotation based on
   */
  private void setRotation(
      android.hardware.Camera camera, android.hardware.Camera.Parameters parameters, int cameraId) {
    WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
    int degrees = 0;
    int rotation = windowManager.getDefaultDisplay().getRotation();
    switch (rotation) {
      case Surface.ROTATION_0:
        degrees = 0;
        break;
      case Surface.ROTATION_90:
        degrees = 90;
        break;
      case Surface.ROTATION_180:
        degrees = 180;
        break;
      case Surface.ROTATION_270:
        degrees = 270;
        break;
      default:
        Log.e(TAG, "Bad rotation value: " + rotation);
    }

    android.hardware.Camera.CameraInfo cameraInfo = new android.hardware.Camera.CameraInfo();
    android.hardware.Camera.getCameraInfo(cameraId, cameraInfo);

    int angle;
    int displayAngle;
    if (cameraInfo.facing == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT) {
      angle = (cameraInfo.orientation + degrees) % 360;
      displayAngle = (360 - angle) % 360; // compensate for it being mirrored
    } else { // back-facing
      angle = (cameraInfo.orientation - degrees + 360) % 360;
      displayAngle = angle;
    }

    // This corresponds to the rotation constants in frame.
    this.rotation = angle / 90;

    camera.setDisplayOrientation(displayAngle);
    parameters.setRotation(angle);
  }

  /**
   * Creates one buffer for the camera preview callback. The size of the buffer is based off of the
   * camera preview size and the format of the camera image.
   *
   * @return a new preview buffer of the appropriate size for the current camera settings
   */
  private byte[] createPreviewBuffer(Size previewSize) {
    int bitsPerPixel = ImageFormat.getBitsPerPixel(ImageFormat.NV21);
    long sizeInBits = (long) previewSize.getHeight() * previewSize.getWidth() * bitsPerPixel;
    int bufferSize = (int) Math.ceil(sizeInBits / 8.0d) + 1;

    // Creating the byte array this way and wrapping it, as opposed to using .allocate(),
    // should guarantee that there will be an array to work with.
    byte[] byteArray = new byte[bufferSize];
    ByteBuffer buffer = ByteBuffer.wrap(byteArray);
    bytesToByteBuffer.put(byteArray, buffer);
    return byteArray;
  }

  // ==============================================================================================
  // Frame processing
  // ==============================================================================================

  /** Called when the camera has a new preview frame. */
  private class CameraPreviewCallback implements android.hardware.Camera.PreviewCallback {
    @Override
    public void onPreviewFrame(byte[] data, android.hardware.Camera camera) {
      frameProcessor.setNextFrame(data, camera);
    }
  }

  /**
   * This runnable controls access to the underlying receiver, calling it to process frames when
   * available from the camera. This is designed to run detection on frames as fast as possible.
   */
  private class FrameProcessingRunnable implements Runnable {
    private Detector<?> detector;
    private final long startTimeMillis = SystemClock.elapsedRealtime();

    // This lock guards all of the member variables below.
    private final Object lock = new Object();
    private boolean active = true;

    // These pending variables hold the state associated with the new frame awaiting processing.
    private long pendingTimeMillis;
    private int pendingFrameId = 0;
    private ByteBuffer pendingFrameData;

    FrameProcessingRunnable(Detector<?> detector) {
      this.detector = detector;
    }

    /**
     * Releases the underlying receiver. This is only safe to do after the associated thread has
     * completed, which is managed in camera source's release method above.
     */
    void release() {
      detector.release();
      detector = null;
    }

    /** Marks the runnable as active/not active. Signals any blocked threads to continue. */
    void setActive(boolean active) {
      synchronized (lock) {
        this.active = active;
        lock.notifyAll();
      }
    }

    /**
     * Sets the frame data received from the camera. This adds the previous unused frame buffer (if
     * present) back to the camera, and keeps a pending reference to the frame data for future use.
     */
    void setNextFrame(byte[] data, android.hardware.Camera camera) {
      synchronized (lock) {
        if (pendingFrameData != null) {
          camera.addCallbackBuffer(pendingFrameData.array());
          pendingFrameData = null;
        }

        if (!bytesToByteBuffer.containsKey(data)) {
          Log.d(
              TAG,
              "Skipping frame.  Could not find ByteBuffer associated with the image"
                  + " "
                  + "data from the camera.");
          return;
        }

        // Timestamp and frame ID are saved to aware when frames are dropped along the way.
        pendingTimeMillis = SystemClock.elapsedRealtime() - startTimeMillis;
        pendingFrameId++;
        pendingFrameData = bytesToByteBuffer.get(data);

        // Notify the processor thread if it is waiting on the next frame.
        lock.notifyAll();
      }
    }

    /**
     * As long as the processing thread is active, this executes detection on frames continuously.
     * The next pending frame is either immediately available or hasn't been received yet. Once it
     * is available, we transfer the frame info to local variables and run detection on that frame.
     * It immediately loops back for the next frame without pausing.
     */
    @Override
    public void run() {
      Frame outputFrame;
      ByteBuffer data;

      while (true) {
        synchronized (lock) {
          while (active && (pendingFrameData == null)) {
            try {
              // Wait for the next frame to be received from the camera, since we
              // don't have it yet.
              lock.wait();
            } catch (InterruptedException e) {
              Log.d(TAG, "Frame processing loop terminated.", e);
              return;
            }
          }

          if (!active) {
            // Exit the loop once this camera source is stopped or released.
            return;
          }

          outputFrame =
              new Frame.Builder()
                  .setImageData(
                      pendingFrameData,
                      previewSize.getWidth(),
                      previewSize.getHeight(),
                      ImageFormat.NV21)
                  .setId(pendingFrameId)
                  .setTimestampMillis(pendingTimeMillis)
                  .setRotation(rotation)
                  .build();

          // Hold onto the frame data locally, so that we can use this for detection
          // below.  We need to clear pendingFrameData to ensure that this buffer isn't
          // recycled back to the camera before we are done using that data.
          data = pendingFrameData;
          pendingFrameData = null;
        }

        // The code below needs to run outside of synchronization, because this will allow
        // the camera to add pending frame(s) while we are running detection on the current
        // frame.
        try {
          detector.receiveFrame(outputFrame);
        } catch (Throwable t) {
          Log.e(TAG, "Exception thrown from receiver.", t);
        } finally {
          camera.addCallbackBuffer(data.array());
        }
      }
    }
  }
}
