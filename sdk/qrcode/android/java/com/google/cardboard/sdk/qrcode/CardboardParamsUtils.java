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
package com.google.cardboard.sdk.qrcode;

import android.net.Uri;
import android.os.Environment;
import android.util.Base64;
import android.util.Log;
import com.google.cardboard.sdk.deviceparams.CardboardV1DeviceParams;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
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

  /**
   * Obtains the physical parameters of a Cardboard headset from a Uri (as bytes).
   *
   * @param uri Uri to read the parameters from.
   * @return A bytes buffer with the Cardboard headset parameters or null in case of error.
   */
  public static byte[] createFromUri(Uri uri) {
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
  public static boolean isCardboardUri(Uri uri) {
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
   * Returns a file in the Cardboard configuration folder of the device.
   *
   * <p>This method creates a folder named {@link #CARDBOARD_CONFIG_FOLDER} in the SD card if not
   * already present.
   *
   * @return The file object of the desired file. Note that the file might not exist.
   * @throws IllegalStateException If the configuration folder path exists but it's not a folder.
   */
  private static File getDeviceParamsFile() {
    File configFolder =
        new File(Environment.getExternalStorageDirectory(), CARDBOARD_CONFIG_FOLDER);

    if (!configFolder.exists()) {
      configFolder.mkdirs();
    } else if (!configFolder.isDirectory()) {
      throw new IllegalStateException(
          configFolder + " already exists as a file, but is expected to be a directory.");
    }

    return new File(configFolder, CARDBOARD_DEVICE_PARAMS_FILE);
  }

  /**
   * Reads the device parameters from a predefined external storage location.
   *
   * @return The stored params. Null if the params do not exist or the read fails.
   */
  public static byte[] readDeviceParamsFromExternalStorage() {
    byte[] paramBytes = null;

    try {
      InputStream stream = null;
      try {
        stream = InputStreamProvider.get(getDeviceParamsFile());
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
   * @return whether the parameters were successfully written.
   */
  public static boolean writeDeviceParamsToExternalStorage(byte[] paramBytes) {
    boolean success = false;
    OutputStream stream = null;
    try {
      stream = OutputStreamProvider.get(getDeviceParamsFile());
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

  /** Writes Cardboard V1 device parameters in external storage. */
  public static void saveCardboardV1DeviceParams() {
    byte[] deviceParams = CardboardV1DeviceParams.build().toByteArray();
    boolean status = writeDeviceParamsToExternalStorage(deviceParams);
    if (!status) {
      Log.e(TAG, "Could not write Cardboard parameters to external storage.");
    }
  }
}
