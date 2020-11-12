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
import android.net.Uri;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;
import android.support.annotation.Nullable;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.cardboard.sdk.R;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.ProtocolException;

/**
 * Class for processing QR code data. The QR code content should be a URI which has a parameter
 * named 'p' in the query string. This parameter contains the Cardboard Viewer Parameters encoded in
 * Base64. If needed, the URI can be redirected up to MAX_REDIRECTS times.
 */
public class QrCodeContentProcessor {
  private static final String TAG = QrCodeContentProcessor.class.getSimpleName();

  private final Listener listener;

  private static final int MAX_REDIRECTS = 5;
  private static final String HTTP_SCHEME_PREFIX = "http://";
  private static final String HTTPS_SCHEME_PREFIX = "https://";
  private static final int HTTPS_TIMEOUT_MS = 5 * 1000;

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

  /** Holds status for conversion from raw QR code to Cardboard device params. */
  public static class QrCodeToParamsStatus {
    public static final int STATUS_OK = 0;
    public static final int STATUS_UNEXPECTED_FORMAT = 1;
    public static final int STATUS_CONNECTION_ERROR = 2;

    public final int statusCode;
    /** Only not null when statusCode is STATUS_OK. */
    @Nullable public final byte[] params;

    public static QrCodeToParamsStatus success(byte[] params) {
      return new QrCodeToParamsStatus(STATUS_OK, params);
    }

    public static QrCodeToParamsStatus error(int statusCode) {
      return new QrCodeToParamsStatus(statusCode, null);
    }

    private QrCodeToParamsStatus(int statusCode, @Nullable byte[] params) {
      this.statusCode = statusCode;
      this.params = params;
    }
  }

  /**
   * Processes detected QR code and save obtained device parameters.
   *
   * @param context The current Context. It is generally an Activity instance or wraps one, or an
   *     Application. It is used to write device params to scoped storage via
   *     {@code Context.getFilesDir()}.
   */
  public void processAndSaveQrCode(Barcode qrCode, Context context) {
    new ProcessAndSaveQrCodeTask(context).execute(qrCode);
  }

  /**
   * Asynchronous Task to process QR code. Once it is processed, obtained parameters are saved in
   * external storage.
   */
  public class ProcessAndSaveQrCodeTask extends AsyncTask<Barcode, Void, QrCodeToParamsStatus> {
    private final Context context;

    /**
     * Contructs a ProcessAndSaveQrCodeTask.
     *
     * @param context The current Context. It is generally an Activity instance or wraps one, or an
     *     Application. It is used to write device params to scoped storage via
     *     {@code Context.getFilesDir()}.
     */
    public ProcessAndSaveQrCodeTask(Context context) {
      this.context = context;
    }

    @Override
    @Nullable
    protected QrCodeToParamsStatus doInBackground(Barcode... qrCode) {
      UrlFactory urlFactory = new UrlFactory();
      return getParamsFromQrCode(qrCode[0], urlFactory);
    }

    @Override
    protected void onPostExecute(QrCodeToParamsStatus result) {
      super.onPostExecute(result);
      boolean status = false;
      if (result.statusCode == QrCodeToParamsStatus.STATUS_UNEXPECTED_FORMAT) {
        Log.d(TAG, String.valueOf(R.string.invalid_qr_code));
        Toast.makeText(context, R.string.invalid_qr_code, Toast.LENGTH_LONG).show();
      } else if (result.statusCode == QrCodeToParamsStatus.STATUS_CONNECTION_ERROR) {
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
   * parameters. If the obtained string matches a Cardboard V1 string format, the parameters are
   * taken directly from the code. If the obtained string matches a Cardboard V2 string format, the
   * parameters are taken from the URI query string (up to 5 redirections supported). This function
   * only supports HTTPS connections, so if an HTTP scheme is found, it is replaced by an HTTPS one.
   *
   * @param barcode The detected QR code.
   * @param urlFactory Factory for creating URL instance for HTTPS connection.
   * @return Cardboard device parameters, or null if there is an error.
   */
  private static QrCodeToParamsStatus getParamsFromQrCode(Barcode barcode, UrlFactory urlFactory) {
    if (barcode.valueFormat != Barcode.TEXT && barcode.valueFormat != Barcode.URL) {
      Log.e(TAG, "Invalid QR code format: " + barcode.valueFormat);
      return QrCodeToParamsStatus.error(QrCodeToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }

    Uri uri = Uri.parse(barcode.rawValue);
    if (uri == null) {
      Log.e(TAG, "Error when parsing scanned URI: " + barcode.rawValue);
      return QrCodeToParamsStatus.error(QrCodeToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }

    // If needed, prefix free text results with a https prefix.
    if (uri.getScheme() == null) {
      uri = Uri.parse(HTTPS_SCHEME_PREFIX + uri);
    }

    // Follow redirects to support URL shortening.
    try {
      Log.d(TAG, "Following redirects for original URI: " + uri);
      uri = followCardboardParamRedirect(uri, MAX_REDIRECTS, urlFactory);
    } catch (IOException e) {
      Log.w(TAG, "Error while following URL redirect " + e);
      return QrCodeToParamsStatus.error(QrCodeToParamsStatus.STATUS_CONNECTION_ERROR);
    }

    if (uri == null) {
      Log.e(TAG, "Error when following URI redirects");
      return QrCodeToParamsStatus.error(QrCodeToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }

    byte[] params = CardboardParamsUtils.createFromUri(uri);
    if (params == null) {
      Log.e(TAG, "Error when parsing device parameters from URI query string: " + uri);
      return QrCodeToParamsStatus.error(QrCodeToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }
    return QrCodeToParamsStatus.success(params);
  }

  /**
   * Follow HTTPS redirect until we reach a valid Cardboard device URI.
   *
   * <p>Network access is only used if the given URI is not already a cardboard device. Only HTTPS
   * headers are transmitted, and the final URI is not accessed.
   *
   * @param uri The initial URI.
   * @param maxRedirects Maximum number of redirects to follow.
   * @param urlFactory Factory for creating URL instance for HTTPS connection.
   * @return Cardboard device URI, or null if there is an error.
   */
  @Nullable
  @SuppressWarnings("nullness:assignment.type.incompatible")
  private static Uri followCardboardParamRedirect(
      Uri uri, int maxRedirects, final UrlFactory urlFactory) throws IOException {
    int numRedirects = 0;
    while (uri != null && !CardboardParamsUtils.isCardboardUri(uri)) {
      if (numRedirects >= maxRedirects) {
        Log.d(TAG, "Exceeding the number of maximum redirects: " + maxRedirects);
        return null;
      }
      uri = resolveHttpsRedirect(uri, urlFactory);
      numRedirects++;
    }
    return uri;
  }

  /**
   * Dereference an HTTPS redirect without reading resource body.
   *
   * @param uri The initial URI.
   * @param urlFactory Factory for creating URL instance for HTTPS connection.
   * @return Redirected URI, or null if there is no redirect or an error.
   */
  @Nullable
  private static Uri resolveHttpsRedirect(Uri uri, UrlFactory urlFactory) throws IOException {
    HttpURLConnection connection = urlFactory.openHttpsConnection(uri);
    if (connection == null) {
      return null;
    }
    // Rather than follow redirects internally, we follow one hop at the time.
    // We don't want to issue even a HEAD request to the Cardboard URI.
    connection.setInstanceFollowRedirects(false);
    connection.setDoInput(false);
    connection.setConnectTimeout(HTTPS_TIMEOUT_MS);
    connection.setReadTimeout(HTTPS_TIMEOUT_MS);
    // Workaround for Android bug with HEAD requests on KitKat devices.
    // See: https://code.google.com/p/android/issues/detail?id=24672.
    connection.setRequestProperty("Accept-Encoding", "");
    try {
      connection.setRequestMethod("HEAD");
    } catch (ProtocolException e) {
      Log.w(TAG, e.toString());
      return null;
    }
    try {
      connection.connect();
      int responseCode = connection.getResponseCode();
      Log.i(TAG, "Response code: " + responseCode);
      if (responseCode != HttpURLConnection.HTTP_MOVED_PERM
          && responseCode != HttpURLConnection.HTTP_MOVED_TEMP) {
        return null;
      }
      String location = connection.getHeaderField("Location");
      if (location == null) {
        Log.d(TAG, "Returning null because of null location.");
        return null;
      }
      Log.i(TAG, "Location: " + location);

      Uri redirectUri = Uri.parse(location.replaceFirst(HTTP_SCHEME_PREFIX, HTTPS_SCHEME_PREFIX));
      if (redirectUri == null || redirectUri.compareTo(uri) == 0) {
        Log.d(TAG, "Returning null because of wrong redirect URI.");
        return null;
      }
      Log.i(TAG, "Param URI redirect to " + redirectUri);
      uri = redirectUri;
    } finally {
      connection.disconnect();
    }
    return uri;
  }
}
