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
import android.os.Build;
import android.os.Environment;
import android.util.Base64;
import android.util.Log;
import androidx.annotation.ChecksSdkIntAtLeast;
import androidx.annotation.Nullable;
import com.google.cardboard.sdk.UsedByNative;
import com.google.cardboard.sdk.deviceparams.CardboardV1DeviceParams;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.ProtocolException;
import java.nio.ByteBuffer;

/** Utility methods for managing configuration parameters. */
public abstract class CardboardParamsUtils {
  private static final String TAG = CardboardParamsUtils.class.getSimpleName();

  /** URL key used to encode Cardboard device parameters. */
  private static final String URI_KEY_PARAMS = "p";

  /** Name of the folder where Cardboard configuration files are stored. */
  private static final String CARDBOARD_CONFIG_FOLDER = "Cardboard";

  /** Name of the file containing device parameters of the currently paired Cardboard device. */
  private static final String CARDBOARD_DEVICE_PARAMS_FILE = "current_device_params";

  /** Sentinel value for including device params in a stream. */
  private static final int CARDBOARD_DEVICE_PARAMS_STREAM_SENTINEL = 0x35587a2b;

  private static final String HTTPS_SCHEME = "https";
  private static final String HTTP_SCHEME = "http";

  /** URI short host of Google. */
  private static final String URI_HOST_GOOGLE_SHORT = "g.co";

  /** URI host of Google. */
  private static final String URI_HOST_GOOGLE = "google.com";

  /** URI path of Cardboard home page. */
  private static final String URI_PATH_CARDBOARD_HOME = "cardboard";

  /** URI path used in viewer param NFC and QR codes. */
  private static final String URI_PATH_CARDBOARD_CONFIG = "cardboard/cfg";

  /** Flags to encode and decode in Base64 device parameters in the Uri. */
  private static final int URI_CODING_PARAMS = Base64.URL_SAFE | Base64.NO_WRAP | Base64.NO_PADDING;

  /** URI of original cardboard QR code. */
  private static final Uri URI_ORIGINAL_CARDBOARD_QR_CODE =
      new Uri.Builder()
          .scheme(HTTPS_SCHEME)
          .authority(URI_HOST_GOOGLE_SHORT)
          .appendEncodedPath(URI_PATH_CARDBOARD_HOME)
          .build();

  private static final int MAX_REDIRECTS = 5;
  private static final String HTTP_SCHEME_PREFIX = "http://";
  private static final String HTTPS_SCHEME_PREFIX = "https://";
  private static final int HTTPS_TIMEOUT_MS = 5 * 1000;

  /** Enum to determine which storage source to use. */
  private enum StorageSource {
    SCOPED_STORAGE,
    EXTERNAL_STORAGE
  };

  /** Holds status for conversion from a URI to Cardboard device params. */
  public static class UriToParamsStatus {
    public static final int STATUS_OK = 0;
    public static final int STATUS_UNEXPECTED_FORMAT = 1;
    public static final int STATUS_CONNECTION_ERROR = 2;

    public final int statusCode;
    /** Only not null when statusCode is STATUS_OK. */
    @Nullable public final byte[] params;

    public static UriToParamsStatus success(byte[] params) {
      return new UriToParamsStatus(STATUS_OK, params);
    }

    public static UriToParamsStatus error(int statusCode) {
      return new UriToParamsStatus(statusCode, null);
    }

    private UriToParamsStatus(int statusCode, @Nullable byte[] params) {
      this.statusCode = statusCode;
      this.params = params;
    }
  }

  /**
   * Obtains the Cardboard device parameters from a Uri string and saves them.
   *
   * <p>Obtains the Cardboard device parameters from a Uri string (passed as a bytes array) and
   * saves them into a predefined storage location.
   *
   * @param uriAsBytes URI string (as a bytes array) used to get the device parameters.
   * @param context The current Context. It is or wraps an Activity or an Application instance.
   */
  @UsedByNative
  public static void saveParamsFromUri(byte[] uriAsBytes, Context context) {
    String uriAsString = new String(uriAsBytes);
    UriToParamsStatus uriToParamsStatus = getParamsFromUriString(uriAsString, new UrlFactory());
    if (uriToParamsStatus.statusCode != UriToParamsStatus.STATUS_OK) {
      Log.e(TAG, "Error when trying to get the Cardboard device params from URI: " + uriAsString);
      return;
    }

    boolean status = writeDeviceParams(uriToParamsStatus.params, context);
    Log.d(TAG, "Could " + (!status ? "not " : "") + "save Cardboard device parameters.");
  }

  /**
   * Saves the Cardboard V1 device parameters into a predefined storage location.
   *
   * @param context The current Context. It is or wraps an Activity or an Application instance.
   */
  public static void saveCardboardV1DeviceParams(Context context) {
    byte[] deviceParams = CardboardV1DeviceParams.build().toByteArray();
    boolean status = writeDeviceParams(deviceParams, context);
    Log.d(TAG, "Could " + (!status ? "not " : "") + "save Cardboard V1 device parameters.");
  }

  /**
   * Obtains the Cardboard device parameters from a URI string.
   *
   * <p>Analyses the URI obtained from a string in order to get the device parameters. If the
   * obtained string matches a Cardboard V1 string format, the parameters are taken directly from
   * the code. If the obtained string matches a Cardboard V2 string format, the parameters are taken
   * from the URI query string (up to 5 redirections supported). This function only supports HTTPS
   * connections. In case a URI containing an HTTP scheme is provided, it will be replaced by an
   * HTTPS one.
   *
   * @param uriAsString URI string used to get the device parameters.
   * @param urlFactory Factory for creating URL instance for HTTPS connection.
   * @return A UriToParamsStatus instance containing the obtained result.
   */
  public static UriToParamsStatus getParamsFromUriString(
      String uriAsString, UrlFactory urlFactory) {
    Uri uri = Uri.parse(uriAsString);
    if (uri == null) {
      Log.e(TAG, "Error when parsing URI: " + uri);
      return UriToParamsStatus.error(UriToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }

    // If needed, prefix free text results with a https prefix.
    if (uri.getScheme() == null) {
      uri = Uri.parse(HTTPS_SCHEME_PREFIX + uri);
    } else if ((uri.getScheme()).equals(HTTP_SCHEME)) {
      // If the prefix is http, replace it with https.
      uri = Uri.parse(uri.toString().replaceFirst(HTTP_SCHEME_PREFIX, HTTPS_SCHEME_PREFIX));
    }

    // Follow redirects to support URL shortening.
    try {
      Log.d(TAG, "Following redirects for original URI: " + uri);
      uri = followCardboardParamRedirect(uri, MAX_REDIRECTS, urlFactory);
    } catch (IOException e) {
      Log.w(TAG, "Error while following URL redirect " + e);
      return UriToParamsStatus.error(UriToParamsStatus.STATUS_CONNECTION_ERROR);
    }

    if (uri == null) {
      Log.e(TAG, "Error when following URI redirects");
      return UriToParamsStatus.error(UriToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }

    byte[] params = CardboardParamsUtils.createFromUri(uri);
    if (params == null) {
      Log.e(TAG, "Error when parsing device parameters from URI query string: " + uri);
      return UriToParamsStatus.error(UriToParamsStatus.STATUS_UNEXPECTED_FORMAT);
    }
    return UriToParamsStatus.success(params);
  }

  /**
   * Reads the device parameters from a predefined storage location by forwarding a call to {@code
   * readDeviceParamsFromStorage()}.
   *
   * <p>Based on the API level, different behaviours are expected. When the API level is below
   * Android Q´s API level external storage is used. When the API level is exactly the same as
   * Android Q's API level, a migration from external storage to scoped storage is performed. When
   * there are device parameters in both in external and scoped storage, scoped storage is prefered.
   * When the API level is greater than Android Q's API level scoped storage is used.
   *
   * @param context The current Context. It is or wraps an Activity or an Application instance.
   * @return A byte array with proto encoded device parameters.
   */
  @UsedByNative
  public static byte[] readDeviceParams(Context context) {
    if (!isAtLeastQ()) {
      Log.d(TAG, "Reading device parameters from external storage.");
      return readDeviceParamsFromStorage(StorageSource.EXTERNAL_STORAGE, context);
    }

    Log.d(TAG, "Reading device parameters from both scoped and external storage.");
    byte[] externalDeviceParams =
        readDeviceParamsFromStorage(StorageSource.EXTERNAL_STORAGE, context);
    byte[] internalDeviceParams =
        readDeviceParamsFromStorage(StorageSource.SCOPED_STORAGE, context);

    // There are device parameters only in external storage --> a copy to internal storage is done.
    if (externalDeviceParams != null && internalDeviceParams == null) {
      Log.d(TAG, "About to copy external device parameters to scoped storage.");
      if (!writeDeviceParamsToStorage(
          externalDeviceParams, StorageSource.SCOPED_STORAGE, context)) {
        Log.e(TAG, "Error writing device parameters to scoped storage.");
      }
      return externalDeviceParams;
    }
    return internalDeviceParams;
  }

  /**
   * Writes the device parameters to a predefined storage location by forwarding a call to {@code
   * writeDeviceParamsToStorage()}.
   *
   * <p>Based on the API level, different behaviours are expected. When the API level is below
   * Android Q´s API level external storage is used. Otherwise, scoped storage is used.
   *
   * @param context The current Context. It is or wraps an Activity or an Application instance.
   * @return true when the write operation is successful.
   */
  public static boolean writeDeviceParams(byte[] deviceParams, Context context) {
    StorageSource storageSource;
    if (isAtLeastQ()) {
      storageSource = StorageSource.SCOPED_STORAGE;
      Log.d(TAG, "Writing device parameters to scoped storage.");
    } else {
      storageSource = StorageSource.EXTERNAL_STORAGE;
      Log.d(TAG, "Writing device parameters to external storage.");
    }
    return writeDeviceParamsToStorage(deviceParams, storageSource, context);
  }

  /**
   * Obtains the physical parameters of a Cardboard headset from a Uri (as bytes).
   *
   * @param uri Uri to read the parameters from.
   * @return A bytes buffer with the Cardboard headset parameters or null in case of error.
   */
  private static byte[] createFromUri(Uri uri) {
    if (uri == null) {
      return null;
    }

    byte[] deviceParams;
    if (isOriginalCardboardDeviceUri(uri)) {
      deviceParams = CardboardV1DeviceParams.build().toByteArray();
    } else if (isCardboardDeviceUri(uri)) {
      deviceParams = readDeviceParamsFromUri(uri);
    } else {
      Log.w(TAG, String.format("URI \"%s\" not recognized as Cardboard viewer.", uri));
      deviceParams = null;
    }

    return deviceParams;
  }

  /**
   * Analyzes if the given URI identifies a Cardboard viewer.
   *
   * @param uri Uri to analyze.
   * @return true if the given URI identifies a Cardboard viewer.
   */
  private static boolean isCardboardUri(Uri uri) {
    return isOriginalCardboardDeviceUri(uri) || isCardboardDeviceUri(uri);
  }

  /**
   * Analyzes if the given URI identifies an original Cardboard viewer (or equivalent).
   *
   * @param uri Uri to analyze.
   * @return true if the given URI identifies an original Cardboard viewer (or equivalent).
   */
  private static boolean isOriginalCardboardDeviceUri(Uri uri) {
    // Note for "cardboard:" scheme case we're lax about path, parameters, etc. since
    // some viewers compatible with original Cardboard are known to take liberties.
    return URI_ORIGINAL_CARDBOARD_QR_CODE.equals(uri);
  }

  /**
   * Analyzes if the given URI identifies a Cardboard device using current scheme.
   *
   * @param uri Uri to analyze.
   * @return true if the given URI identifies a Cardboard device using current scheme.
   */
  private static boolean isCardboardDeviceUri(Uri uri) {
    return HTTPS_SCHEME.equals(uri.getScheme())
        && URI_HOST_GOOGLE.equals(uri.getAuthority())
        && ("/" + URI_PATH_CARDBOARD_CONFIG).equals(uri.getPath());
  }

  /**
   * Decodes device parameters in URI from Base64 to bytes.
   *
   * @param uri Uri to get the parameters from.
   * @return device parameters in bytes, or null in case of error.
   */
  private static byte[] readDeviceParamsFromUri(Uri uri) {
    String paramsEncoded = uri.getQueryParameter(URI_KEY_PARAMS);
    if (paramsEncoded == null) {
      Log.w(TAG, "No Cardboard parameters in URI.");
      return null;
    }

    try {
      return Base64.decode(paramsEncoded, URI_CODING_PARAMS);
    } catch (Exception e) {
      Log.w(TAG, "Parsing Cardboard parameters from URI failed: " + e);
      return null;
    }
  }

  /**
   * Reads the device parameters from a predefined storage location.
   *
   * @param storageSource When {@code StorageSource.SCOPED_STORAGE}, the path is in the scoped
   *     storage. Otherwise, the SD card is used.
   * @param context The current Context. It is generally an Activity instance or wraps one, or an
   *     Application. It is used to read from scoped storage when @p storageSource is {@code
   *     StorageSource.SCOPED_STORAGE} via {@code Context.getFilesDir()}.
   * @return The stored params. Null if the params do not exist or the read fails.
   */
  private static byte[] readDeviceParamsFromStorage(StorageSource storageSource, Context context) {
    byte[] paramBytes = null;

    try {
      InputStream stream = null;
      try {
        stream = InputStreamProvider.get(getDeviceParamsFile(storageSource, context));
        paramBytes = readDeviceParamsFromInputStream(stream);
      } finally {
        if (stream != null) {
          try {
            stream.close();
          } catch (IOException e) {
            // Pass
          }
        }
      }
    } catch (FileNotFoundException e) {
      Log.d(TAG, "Parameters file not found for reading: " + e);
    } catch (IllegalStateException e) {
      Log.w(TAG, "Error reading parameters: " + e);
    }
    return paramBytes;
  }

  /**
   * Reads the parameters from a given input stream.
   *
   * @param inputStream Input stream containing device params.
   * @return A bytes buffer or null in case of error.
   */
  private static byte[] readDeviceParamsFromInputStream(InputStream inputStream) {
    if (inputStream == null) {
      return null;
    }

    try {
      // Stream format is sentinel (4 byte int) + size (4 byte int) + proto.
      // Values are big endian.
      ByteBuffer header = ByteBuffer.allocate(2 * Integer.SIZE / Byte.SIZE);
      if (inputStream.read(header.array(), 0, header.array().length) == -1) {
        Log.e(TAG, "Error parsing param record: end of stream.");
        return null;
      }
      int sentinel = header.getInt();
      int length = header.getInt();
      if (sentinel != CARDBOARD_DEVICE_PARAMS_STREAM_SENTINEL) {
        Log.e(TAG, "Error parsing param record: incorrect sentinel.");
        return null;
      }
      byte[] paramBytes = new byte[length];
      if (inputStream.read(paramBytes, 0, paramBytes.length) == -1) {
        Log.e(TAG, "Error parsing param record: end of stream.");
        return null;
      }
      return paramBytes;
    } catch (IOException e) {
      Log.w(TAG, "Error reading parameters: " + e);
    }
    return null;
  }

  /**
   * Writes device parameters to external storage.
   *
   * @param paramBytes The parameters to be written.
   * @param storageSource When {@code StorageSource.SCOPED_STORAGE}, the path is in the scoped
   *     storage. Otherwise, the SD card is used.
   * @param context The current Context. It is generally an Activity instance or wraps one, or an
   *     Application. It is used to write to scoped storage when {@code VERSION.SDK_INT >=
   *     VERSION_CODES.Q} via {@code Context.getFilesDir()}.
   * @return whether the parameters were successfully written.
   */
  private static boolean writeDeviceParamsToStorage(
      byte[] paramBytes, StorageSource storageSource, Context context) {
    boolean success = false;
    OutputStream stream = null;
    try {
      stream = OutputStreamProvider.get(getDeviceParamsFile(storageSource, context));
      success = writeDeviceParamsToOutputStream(paramBytes, stream);
    } catch (FileNotFoundException e) {
      Log.e(TAG, "Parameters file not found for writing: " + e);
    } catch (IllegalStateException e) {
      Log.w(TAG, "Error writing parameters: " + e);
    } finally {
      if (stream != null) {
        try {
          stream.close();
        } catch (IOException e) {
          // Pass
        }
      }
    }
    return success;
  }

  /**
   * Attempts to write the parameters into the given output stream.
   *
   * @param paramBytes The parameters to be written.
   * @param outputStream OutputStream in which the parameters are stored.
   * @return whether the parameters were successfully written.
   */
  private static boolean writeDeviceParamsToOutputStream(
      byte[] paramBytes, OutputStream outputStream) {
    try {
      // Stream format is sentinel (4 byte int) + size (4 byte int) + proto.
      // Values are big endian.
      ByteBuffer header = ByteBuffer.allocate(2 * Integer.SIZE / Byte.SIZE);
      header.putInt(CARDBOARD_DEVICE_PARAMS_STREAM_SENTINEL);
      header.putInt(paramBytes.length);
      outputStream.write(header.array());
      outputStream.write(paramBytes);
      return true;
    } catch (IOException e) {
      Log.w(TAG, "Error writing parameters: " + e);
      return false;
    }
  }

  /**
   * Returns a file in the Cardboard configuration folder of the device.
   *
   * <p>This method creates a folder named {@link #CARDBOARD_CONFIG_FOLDER} in either the scoped
   * storage of the application or the SD card if not already present depending on the value of @p
   * useScopedStorage.
   *
   * <p>Deprecation warnings are suppressed on this method given that {@code
   * Environment.getExternalStorageDirectory()} is currently marked as deprecated but intentionally
   * used in order to ease the storage migration process.
   *
   * @param storageSource When {@code StorageSource.SCOPED_STORAGE}, the path is in the scoped
   *     storage. Otherwise, the SD card is used.
   * @param context The current Context. It is generally an Activity instance or wraps one, or an
   *     Application. It is used to write to scoped storage when @p storageSource is {@code
   *     StorageSource.SCOPED_STORAGE} via {@code Context.getFilesDir()}.
   * @return The file object of the desired file. Note that the file might not exist.
   * @throws IllegalStateException If the configuration folder path exists but it's not a folder.
   */
  @SuppressWarnings("deprecation")
  private static File getDeviceParamsFile(StorageSource storageSource, Context context) {
    File configFolder =
        new File(
            storageSource == StorageSource.SCOPED_STORAGE
                ? context.getFilesDir()
                : Environment.getExternalStorageDirectory(),
            CARDBOARD_CONFIG_FOLDER);

    if (!configFolder.exists()) {
      configFolder.mkdirs();
    } else if (!configFolder.isDirectory()) {
      throw new IllegalStateException(
          configFolder + " already exists as a file, but is expected to be a directory.");
    }

    return new File(configFolder, CARDBOARD_DEVICE_PARAMS_FILE);
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
  private static Uri followCardboardParamRedirect(
      Uri uri, int maxRedirects, final UrlFactory urlFactory) throws IOException {
    int numRedirects = 0;
    while (uri != null && !isCardboardUri(uri)) {
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

  /**
   * Checks whether the current Android version is Q or greater.
   *
   * @return true if the current Android version is Q or greater, false otherwise.
   */
  @ChecksSdkIntAtLeast(api = Build.VERSION_CODES.Q)
  private static boolean isAtLeastQ() {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q;
  }
}
