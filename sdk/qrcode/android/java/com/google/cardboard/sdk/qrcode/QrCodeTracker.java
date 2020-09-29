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

import com.google.android.gms.vision.Tracker;
import com.google.android.gms.vision.barcode.Barcode;

/**
 * QrCodeTracker is used for tracking or reading a QR code. This is used to receive newly detected
 * items, add a graphical representation to an overlay, update the graphics as the item changes, and
 * remove the graphics when the item goes away.
 */
public class QrCodeTracker extends Tracker<Barcode> {
  private final Listener listener;

  /**
   * Consume the item instance detected from an Activity or Fragment level by implementing the
   * Listener interface method onQrCodeDetected.
   */
  public interface Listener {
    void onQrCodeDetected(Barcode qrCode);
  }

  QrCodeTracker(Listener listener) {
    this.listener = listener;
  }

  /** Start tracking the detected item instance. */
  @Override
  public void onNewItem(int id, Barcode item) {
    if (item.displayValue != null) {
      listener.onQrCodeDetected(item);
    }
  }
}
