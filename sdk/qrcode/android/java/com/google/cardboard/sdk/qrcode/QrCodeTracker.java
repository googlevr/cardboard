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
package com.google.cardboard.sdk.qrcode;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

import zxingcpp.BarcodeReader;

/**
 * QrCodeTracker is used for tracking or reading a QR code. This is used to receive newly detected
 * items, add a graphical representation to an overlay, update the graphics as the item changes, and
 * remove the graphics when the item goes away.
 */
public class QrCodeTracker {
  private final Listener listener;
  private final HashSet<BarcodeReader.Result> lastData = new HashSet<>();

  /**
   * Consume the item instance detected from an Activity or Fragment level by implementing the
   * Listener interface method onQrCodeDetected.
   */
  public interface Listener {
    void onQrCodeDetected(BarcodeReader.Result qrCode);
  }

  public QrCodeTracker(Listener listener) {
    this.listener = listener;
  }

  public void onItemsDetected(List<BarcodeReader.Result> data) {
    for (BarcodeReader.Result result : data) {
      if (lastData.stream().anyMatch(otherResult -> Arrays.equals(result.getBytes(), otherResult.getBytes()))) {
        // This QR code already was detected in last frame, it's not new.
        continue;
      }
      listener.onQrCodeDetected(result);
    }
    lastData.clear();
    lastData.addAll(data);
  }
}
