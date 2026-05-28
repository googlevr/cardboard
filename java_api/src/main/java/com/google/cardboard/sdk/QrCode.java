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

import androidx.annotation.Nullable;

/** Cardboard SDK QR Code API. */
public class QrCode {

  /** Class only contains static methods. */
  private QrCode() {}

  /**
   * Gets currently saved device parameters.
   *
   * @return device parameters serialized using cardboard_device.proto. If there are not saved
   *     device parameters, this method returns null.
   */
  @Nullable
  public static byte[] getSavedDeviceParams() {
    return nativeQrCodeGetSavedDeviceParams();
  }

  /**
   * Gets Cardboard V1 device parameters.
   *
   * <p>Note that a call to {@code nativeQrCodeGetCardboardV1DeviceParams()} might lead to a
   * RuntimeException (which is intentionally uncaught). When this very unlikely event happens,
   * code is thought to be in a severe and faulty state and signalling it within Java provides more
   * tools to analyze the call graph.
   *
   * @return Cardboard V1 device parameters serialized using cardboard_device.proto.
   */
  public static byte[] getCardboardV1DeviceParams() {
    return nativeQrCodeGetCardboardV1DeviceParams();
  }

  /** Scans a QR code and saves the encoded device parameters. */
  public static void scanQrCodeAndSaveDeviceParams() {
    nativeQrCodeScanQrCodeAndSaveDeviceParams();
  }

  private static native byte[] nativeQrCodeGetSavedDeviceParams();

  private static native byte[] nativeQrCodeGetCardboardV1DeviceParams();

  private static native void nativeQrCodeScanQrCodeAndSaveDeviceParams();
}
