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
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageProxy;
import androidx.camera.core.Preview;
import androidx.camera.core.resolutionselector.ResolutionSelector;
import androidx.camera.core.resolutionselector.ResolutionStrategy;
import androidx.camera.lifecycle.ProcessCameraProvider;
import androidx.camera.view.PreviewView;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.LifecycleOwner;
import zxingcpp.BarcodeReader;

import com.google.cardboard.sdk.qrcode.QrCodeTracker;
import com.google.common.util.concurrent.ListenableFuture;

import java.io.IOException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Manages the camera in conjunction with an underlying detector. This receives preview frames from
 * the camera at a specified rate, sending those frames to the detector as fast as it is able to
 * process those frames.
 */
public class CameraSource implements Runnable {
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

  private final LifecycleOwner lifecycleOwner;

  private final BarcodeReader detector;

  private final QrCodeTracker listener;

  private final ExecutorService cameraExecutor;

  private ListenableFuture<ProcessCameraProvider> cameraProviderFuture;

  private Preview preview;

  private ImageAnalysis imageAnalysis;

  private PreviewView previewView;

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
  public CameraSource(Context context, LifecycleOwner lifecycleOwner, BarcodeReader detector, QrCodeTracker listener) {
    // Prerequisite evaluation.
    if (context == null) {
      Log.e(TAG, "context is null.");
      throw new IllegalArgumentException("No context supplied.");
    }
    if (lifecycleOwner == null) {
      Log.e(TAG, "lifecycleOwner is null.");
      throw new IllegalArgumentException("No lifecycleOwner supplied.");
    }
    if (detector == null) {
      Log.e(TAG, "detector is null.");
      throw new IllegalArgumentException("No detector supplied.");
    }
    if (listener == null) {
      Log.e(TAG, "listener is null.");
      throw new IllegalArgumentException("No listener supplied.");
    }

    this.context = context;
    this.lifecycleOwner = lifecycleOwner;
    this.detector = detector;
    this.listener = listener;
    cameraExecutor = Executors.newSingleThreadExecutor();
    Log.i(TAG, "Successful CameraSource creation.");
  }

  /** Stops the camera and releases the resources of the camera and underlying detector. */
  public void release() {
    stop();
    cameraExecutor.shutdown();
  }

  /**
   * Opens the camera and starts sending preview frames to the underlying detector. The supplied
   * surface holder is used for the preview so frames can be displayed to the user.
   *
   * @param preview the Previewview to use for the preview frames
   * @throws IOException if the supplied surface holder could not be used as the preview display
   */
  @RequiresPermission(Manifest.permission.CAMERA)
  public void start(PreviewView preview) throws IOException {
    cameraProviderFuture = ProcessCameraProvider.getInstance(context);
    cameraProviderFuture.addListener(this, ContextCompat.getMainExecutor(context));
    previewView = preview;
  }

  /**
   * Called by cameraProviderFuture when the camera provider becomes available.
   */
  @Override
  public void run() {
    ProcessCameraProvider cameraProvider;
    try {
      cameraProvider = cameraProviderFuture.get();
    } catch (ExecutionException | InterruptedException e) {
      // No errors need to be handled for this Future.
      // This should never be reached.
      throw new RuntimeException(e);
    }
    preview = new Preview.Builder()
            .build();
    CameraSelector cameraSelector = new CameraSelector.Builder()
            .requireLensFacing(CameraSelector.LENS_FACING_BACK)
            .build();
    preview.setSurfaceProvider(previewView.getSurfaceProvider());

    imageAnalysis =
            new ImageAnalysis.Builder()
                    .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_YUV_420_888)
                    .setResolutionSelector(new ResolutionSelector.Builder()
                            .setAllowedResolutionMode(ResolutionSelector.PREFER_CAPTURE_RATE_OVER_HIGHER_RESOLUTION)
                            .setResolutionStrategy(new ResolutionStrategy(new Size(WIDTH, HEIGHT),
                                    ResolutionStrategy.FALLBACK_RULE_CLOSEST_HIGHER_THEN_LOWER))
                            .build())
                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                    .build();

    imageAnalysis.setAnalyzer(cameraExecutor, new ImageAnalysis.Analyzer() {
      @Override
      public void analyze(@NonNull ImageProxy imageProxy) {
        listener.onItemsDetected(detector.read(imageProxy));
        imageProxy.close();
      }
    });

    cameraProvider.bindToLifecycle(lifecycleOwner, cameraSelector, imageAnalysis, preview);
  }

  /** Closes the camera and stops sending frames to the underlying frame detector. */
  public void stop() {
    if (!cameraProviderFuture.isDone()) {
      cameraProviderFuture.cancel(false);
    } else if (cameraProviderFuture.isDone() && !cameraProviderFuture.isCancelled()) {
      ProcessCameraProvider cameraProvider;
      try {
        cameraProvider = cameraProviderFuture.get();
      } catch (ExecutionException | InterruptedException e) {
        // No errors need to be handled for this Future.
        // This should never be reached.
        throw new RuntimeException(e);
      }
      cameraProvider.unbind(imageAnalysis, preview);
    }
  }
}
