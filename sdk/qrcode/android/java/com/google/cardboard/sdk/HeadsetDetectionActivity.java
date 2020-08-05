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

import android.content.Intent;
import android.net.Uri;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.widget.Toast;
import com.google.cardboard.sdk.qrcode.CardboardParamsUtils;

/**
 * A very light-weight activity with no layout, whose sole purpose is to react to external intents
 * containing cardboard V1 NFC tag.
 */
public class HeadsetDetectionActivity extends AppCompatActivity {

  /** Legacy URI scheme used in original Cardboard NFC tag. */
  private static final String URI_SCHEME_LEGACY_CARDBOARD = "cardboard";

  /** Legacy URI host used in original Cardboard NFC tag. */
  private static final String URI_HOST_LEGACY_CARDBOARD = "v1.0.0";

  /** URI of original cardboard NFC. */
  private static final Uri URI_ORIGINAL_CARDBOARD_NFC =
      new Uri.Builder()
          .scheme(URI_SCHEME_LEGACY_CARDBOARD)
          .authority(URI_HOST_LEGACY_CARDBOARD)
          .build();

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    if (getIntent() != null) {
      processStartupIntent(getIntent());
    }
    finish();
  }

  // Checks whether the startup intent contains cardboard v1 NFC tag.
  // If that's the case, updates the parameters.
  private void processStartupIntent(Intent startupIntent) {
    if (NfcAdapter.ACTION_NDEF_DISCOVERED.equals(startupIntent.getAction())
        && startupIntent.getData() != null) {
      // Saves V1 Cardboard params.
      Uri uri = startupIntent.getData();
      if (URI_ORIGINAL_CARDBOARD_NFC.equals(uri)) {
        CardboardParamsUtils.saveCardboardV1DeviceParams(getApplicationContext());
      }

      Toast.makeText(this, R.string.viewer_detected, Toast.LENGTH_SHORT).show();
    }
  }
}
