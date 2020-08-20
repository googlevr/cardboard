/*
 * Copyright 2019 Google LLC. All Rights Reserved.
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
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Toast;
import android.support.v4.app.ActivityCompat;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.vision.MultiProcessor;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;
import com.google.cardboard.sdk.qrcode.CardboardParamsUtils;
import com.google.cardboard.sdk.qrcode.QrCodeContentProcessor;
import com.google.cardboard.sdk.qrcode.QrCodeTracker;
import com.google.cardboard.sdk.qrcode.QrCodeTrackerFactory;
import com.google.cardboard.sdk.qrcode.camera.CameraSource;
import com.google.cardboard.sdk.qrcode.camera.CameraSourcePreview;
import java.io.IOException;

/**
 * Manages the QR code capture activity. It scans permanently with the camera until it finds a valid
 * QR code.
 */
public class QrCodeCaptureActivity extends AppCompatActivity
    implements QrCodeTracker.Listener, QrCodeContentProcessor.Listener {
  private static final String TAG = QrCodeCaptureActivity.class.getSimpleName();

  // Intent request code to handle updating play services if needed.
  private static final int RC_HANDLE_GMS = 9001;

  // Permission request codes
  private static final int PERMISSIONS_REQUEST_CODE = 2;

  // Min sdk version required for google play services.
  private static final int MIN_SDK_VERSION = 23;

  private CameraSource cameraSource;
  private CameraSourcePreview cameraSourcePreview;

  // Flag used to avoid saving the device parameters more than once.
  private static boolean qrCodeSaved = false;

  /** Initializes the UI and creates the detector pipeline. */
  @Override
  public void onCreate(Bundle icicle) {
    super.onCreate(icicle);
    setContentView(R.layout.qr_code_capture);

    cameraSourcePreview = findViewById(R.id.preview);
  }

  /**
   * Checks for activity permissions.
   *
   * @return whether the permissions are already granted.
   */
  private boolean arePermissionsEnabled() {
    boolean cameraPermission =
        ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
            == PackageManager.PERMISSION_GRANTED;
    boolean writePermission =
        ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
            == PackageManager.PERMISSION_GRANTED;
    return (cameraPermission && writePermission);
  }

  /** Handles the requests for activity permissions. */
  private void requestPermissions() {
    final String[] permissions =
        new String[] {Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE};
    ActivityCompat.requestPermissions(this, permissions, PERMISSIONS_REQUEST_CODE);
  }

  /** Callback for the result from requesting permissions. */
  @Override
  public void onRequestPermissionsResult(
      int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (!arePermissionsEnabled()) {
      Toast.makeText(this, R.string.no_permissions, Toast.LENGTH_LONG).show();
      if (!ActivityCompat.shouldShowRequestPermissionRationale(
              this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
          || !ActivityCompat.shouldShowRequestPermissionRationale(
              this, Manifest.permission.CAMERA)) {
        // Permission denied with checking "Do not ask again".
        launchPermissionsSettings();
      }
      finish();
    }
  }

  private void launchPermissionsSettings() {
    Intent intent = new Intent();
    intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
    intent.setData(Uri.fromParts("package", getPackageName(), null));
    startActivity(intent);
  }

  /** Creates and starts the camera. */
  private void createCameraSource() {
    Context context = getApplicationContext();

    BarcodeDetector qrCodeDetector =
        new BarcodeDetector.Builder(context).setBarcodeFormats(Barcode.QR_CODE).build();

    QrCodeTrackerFactory qrCodeFactory = new QrCodeTrackerFactory(this);

    qrCodeDetector.setProcessor(new MultiProcessor.Builder<>(qrCodeFactory).build());

    // Check that native dependencies are downloaded.
    if (!qrCodeDetector.isOperational()) {
      Log.w(TAG, "Detector dependencies are not yet available.");

      // Check for low storage.
      IntentFilter lowStorageFilter = new IntentFilter(Intent.ACTION_DEVICE_STORAGE_LOW);
      boolean hasLowStorage = registerReceiver(null, lowStorageFilter) != null;

      if (hasLowStorage) {
        Toast.makeText(this, R.string.low_storage_error, Toast.LENGTH_LONG).show();
        Log.w(TAG, getString(R.string.low_storage_error));
      }
    }

    // Creates and starts the camera.
    cameraSource =
        new CameraSource.Builder(getApplicationContext(), qrCodeDetector)
            .setFacing(CameraSource.CAMERA_FACING_BACK)
            .setRequestedPreviewSize(1600, 1024)
            .setRequestedFps(15.0f)
            .setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)
            .setFlashMode(null)
            .build();
  }

  /** Restarts the camera. */
  @Override
  protected void onResume() {
    super.onResume();
    // Checks for activity permissions, if not granted, requests them.
    if (!arePermissionsEnabled()) {
      requestPermissions();
      return;
    }

    createCameraSource();
    qrCodeSaved = false;
    startCameraSource();
  }

  /** Stops the camera. */
  @Override
  protected void onPause() {
    super.onPause();
    if (cameraSourcePreview != null) {
      cameraSourcePreview.stop();
      cameraSourcePreview.release();
    }
  }

  /** Starts or restarts the camera source, if it exists. */
  private void startCameraSource() {
    // Check that the device has play services available.
    int code =
        GoogleApiAvailability.getInstance()
            .isGooglePlayServicesAvailable(getApplicationContext(), MIN_SDK_VERSION);
    if (code != ConnectionResult.SUCCESS) {
      Dialog dlg = GoogleApiAvailability.getInstance().getErrorDialog(this, code, RC_HANDLE_GMS);
      dlg.show();
    }

    if (cameraSource != null) {
      try {
        cameraSourcePreview.start(cameraSource);
      } catch (IOException e) {
        Log.e(TAG, "Unable to start camera source.", e);
        cameraSource.release();
        cameraSource = null;
      } catch (SecurityException e) {
        Log.e(TAG, "Security exception: ", e);
      }
    }
  }

  /** Callback for when "SKIP" is touched */
  public void skipQrCodeCapture(View view) {
    Log.d(TAG, "QR code capture skipped");

    // Check if there are already saved parameters, if not save Cardboard V1 ones.
    byte[] deviceParams = CardboardParamsUtils.readDeviceParamsFromExternalStorage();
    if (deviceParams == null) {
      CardboardParamsUtils.saveCardboardV1DeviceParams();
    }
    finish();
  }

  /**
   * Callback for when a QR code is detected.
   *
   * @param qrCode Detected QR code.
   */
  @Override
  public void onQrCodeDetected(Barcode qrCode) {
    if (qrCode != null && !qrCodeSaved) {
      qrCodeSaved = true;
      QrCodeContentProcessor qrCodeContentProcessor = new QrCodeContentProcessor(this);
      qrCodeContentProcessor.processAndSaveQrCode(qrCode, this);
    }
  }

  /**
   * Callback for when a QR code is processed and the parameters are saved in external storage.
   *
   * @param status Whether the parameters were successfully processed and saved.
   */
  @Override
  public void onQrCodeSaved(boolean status) {
    if (status) {
      Log.d(TAG, "Device parameters saved in external storage.");
      cameraSourcePreview.stop();
      nativeIncrementQrCodeScanCount();
      finish();
    } else {
      Log.e(TAG, "Device parameters not saved in external storage.");
    }
    qrCodeSaved = false;
  }

  private native void nativeIncrementQrCodeScanCount();
}
