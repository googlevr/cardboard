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

import android.content.Context;
import android.util.Log;
import android.widget.Toast;
import zxingcpp.BarcodeReader;
import com.google.cardboard.sdk.R;

/**
 * Class for processing QR code data. The QR code content should be a URI which has a parameter
 * named 'p' in the query string. This parameter contains the Cardboard Viewer Parameters encoded in
 * Base64. If needed, the URI can be redirected up to MAX_REDIRECTS times.
 */
public class QrCodeContentProcessor {
  private static final String TAG = QrCodeContentProcessor.class.getSimpleName();

  private final Listener listener;

  public QrCodeContentProcessor(Listener listener) {
    this.listener = listener;
  }

  /**
   * Consume the item instance detected from an Activity or Fragment level by implementing the
   * QrCodeProcessListener interface method onQrCodeProcessed.
   */
  public interface Listener {
    void onQrCodeSaved(boolean status);
  }

  /**
   * Processes detected QR code and save obtained device parameters.
   *
   * @param context The current Context. It is generally an Activity instance or wraps one, or an
   *     Application. It is used to write device params to scoped storage via {@code
   *     Context.getFilesDir()}.
   */
  public void processAndSaveQrCode(BarcodeReader.Result qrCode, Context context) {
    new ProcessAndSaveQrCodeTask(context).execute(qrCode);
  }

  /**
   * Asynchronous Task to process QR code. Once it is processed, obtained parameters are saved in
   * external storage.
   */
  public class ProcessAndSaveQrCodeTask
      extends AsyncTask<BarcodeReader.Result, CardboardParamsUtils.UriToParamsStatus> {
    private final Context context;

    /**
     * Contructs a ProcessAndSaveQrCodeTask.
     *
     * @param context The current Context. It is generally an Activity instance or wraps one, or an
     *     Application. It is used to write device params to scoped storage via {@code
     *     Context.getFilesDir()}.
     */
    public ProcessAndSaveQrCodeTask(Context context) {
      this.context = context;
    }

    @Override
    protected CardboardParamsUtils.UriToParamsStatus doInBackground(BarcodeReader.Result qrCode) {
      UrlFactory urlFactory = new UrlFactory();
      return getParamsFromQrCode(qrCode, urlFactory);
    }

    @Override
    protected void onPostExecute(CardboardParamsUtils.UriToParamsStatus result) {
      boolean status = false;
      if (result.statusCode == CardboardParamsUtils.UriToParamsStatus.STATUS_UNEXPECTED_FORMAT) {
        Log.d(TAG, String.valueOf(R.string.invalid_qr_code));
        Toast.makeText(context, R.string.invalid_qr_code, Toast.LENGTH_LONG).show();
      } else if (result.statusCode
          == CardboardParamsUtils.UriToParamsStatus.STATUS_CONNECTION_ERROR) {
        Log.d(TAG, String.valueOf(R.string.connection_error));
        Toast.makeText(context, R.string.connection_error, Toast.LENGTH_LONG).show();
      } else if (result.params != null) {
        status = CardboardParamsUtils.writeDeviceParams(result.params, context);
        Log.d(TAG, "Could " + (!status ? "not " : "") + "write Cardboard parameters to storage.");
      }

      listener.onQrCodeSaved(status);
    }
  }

  /**
   * Attempts to convert a QR code detection result into device parameters (as a protobuf).
   *
   * <p>This function analyses the obtained string from a QR code in order to get the device
   * parameters by calling {@code CardboardParamsUtils.getParamsFromUriString}.
   *
   * @param barcode The detected QR code.
   * @param urlFactory Factory for creating URL instance for HTTPS connection.
   * @return Cardboard device parameters, or null if there is an error.
   */
  private static CardboardParamsUtils.UriToParamsStatus getParamsFromQrCode(
      BarcodeReader.Result barcode, UrlFactory urlFactory) {
    if (barcode.getText() == null) {
      Log.e(TAG, "Invalid QR code format: text is null");
      return CardboardParamsUtils.UriToParamsStatus.error(
          CardboardParamsUtils.UriToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }

    return CardboardParamsUtils.getParamsFromUriString(barcode.getText(), urlFactory);
  }
}
