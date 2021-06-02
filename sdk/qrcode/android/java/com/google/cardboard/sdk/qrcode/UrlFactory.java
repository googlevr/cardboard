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

import android.net.Uri;
import android.util.Log;
import androidx.annotation.Nullable;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

/** UrlFactory for producing a HttpURLConnection connection. */
public class UrlFactory {
  public static final String TAG = UrlFactory.class.getSimpleName();
  private static final String HTTPS_SCHEME = "https";

  // Return connection object, or null on error.
  @Nullable
  public HttpURLConnection openHttpsConnection(@Nullable Uri uri) throws IOException {
    URL url;
    try {
      // Always opens an HTTPS connection.
      url = new URL(uri.buildUpon().scheme(HTTPS_SCHEME).build().toString());
    } catch (MalformedURLException e) {
      Log.w(TAG, e.toString());
      return null;
    }
    URLConnection urlConnection = url.openConnection();
    // Return type is HttpURLConnection. When using Cronet as the app's URLStreamHandlerFactory, we
    // always get back an HttpURLConnection (see
    // https://developer.android.com/guide/topics/connectivity/cronet/reference/org/chromium/net/CronetEngine.html#public-abstract-urlstreamhandlerfactory-createurlstreamhandlerfactory).
    // Because the URL scheme is guaranteed to be https, we can safely return the type
    // HttpURLConnection.
    if (!(urlConnection instanceof HttpURLConnection)) {
      Log.w(TAG, "Expected HttpURLConnection");
      throw new IllegalArgumentException("Expected HttpURLConnection");
    }
    return (HttpURLConnection) urlConnection;
  }
}
