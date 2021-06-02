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
import android.content.res.Configuration;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import androidx.annotation.RequiresPermission;
import com.google.android.gms.common.images.Size;
import java.io.IOException;

/**
 * Manages the surface that shows the camera preview, adapting it to the size and orientation of the
 * phone.
 */
public class CameraSourcePreview extends ViewGroup {
  private static final String TAG = CameraSourcePreview.class.getSimpleName();

  private final Context context;
  private final SurfaceView surfaceView;
  private boolean startRequested;
  private boolean surfaceAvailable;
  private CameraSource cameraSource;

  public CameraSourcePreview(Context context, AttributeSet attrs) {
    super(context, attrs);
    this.context = context;
    startRequested = false;
    surfaceAvailable = false;

    surfaceView = new SurfaceView(context);
    surfaceView.getHolder().addCallback(new SurfaceCallback());
    addView(surfaceView);
  }

  @RequiresPermission(Manifest.permission.CAMERA)
  public void start(CameraSource cameraSource) throws IOException {
    if (cameraSource == null) {
      stop();
    }

    this.cameraSource = cameraSource;

    if (this.cameraSource != null) {
      startRequested = true;
      startIfReady();
    }
  }

  public void stop() {
    if (cameraSource != null) {
      cameraSource.stop();
    }
  }

  public void release() {
    if (cameraSource != null) {
      cameraSource.release();
      cameraSource = null;
    }
  }

  @RequiresPermission(Manifest.permission.CAMERA)
  private void startIfReady() throws IOException {
    if (startRequested && surfaceAvailable) {
      cameraSource.start(surfaceView.getHolder());
      startRequested = false;
    }
  }

  private class SurfaceCallback implements SurfaceHolder.Callback {
    @Override
    public void surfaceCreated(SurfaceHolder surface) {
      surfaceAvailable = true;
      try {
        startIfReady();
      } catch (SecurityException se) {
        Log.e(TAG, "Do not have permission to start the camera", se);
      } catch (IOException e) {
        Log.e(TAG, "Could not start camera source.", e);
      }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surface) {
      surfaceAvailable = false;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    int width = 320;
    int height = 240;
    if (cameraSource != null) {
      Size size = cameraSource.getPreviewSize();
      if (size != null) {
        width = size.getWidth();
        height = size.getHeight();
      }
    }

    // Swap width and height sizes when in portrait, since it will be rotated 90 degrees
    if (isPortraitMode()) {
      int tmp = width;
      width = height;
      height = tmp;
    }

    final int layoutWidth = right - left;
    final int layoutHeight = bottom - top;

    int childHeight;
    int childWidth;

    // Fits height when in portrait mode and with when in landscape mode.
    if (isPortraitMode()) {
      childHeight = layoutHeight;
      childWidth = (int) (((float) layoutHeight / (float) height) * width);
    } else {
      childWidth = layoutWidth;
      childHeight = (int) (((float) layoutWidth / (float) width) * height);
    }

    for (int i = 0; i < getChildCount(); ++i) {
      getChildAt(i).layout(0, 0, childWidth, childHeight);
    }

    try {
      startIfReady();
    } catch (SecurityException se) {
      Log.e(TAG, "Do not have permission to start the camera", se);
    } catch (IOException e) {
      Log.e(TAG, "Could not start camera source.", e);
    }
  }

  private boolean isPortraitMode() {
    int orientation = context.getResources().getConfiguration().orientation;
    if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
      return false;
    }
    if (orientation == Configuration.ORIENTATION_PORTRAIT) {
      return true;
    }

    Log.d(TAG, "isPortraitMode returning false by default");
    return false;
  }
}
