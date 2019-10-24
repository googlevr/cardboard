/*
 * Copyright 2019 Google Inc. All Rights Reserved.
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
package com.google.cardboard.qrcode;

import android.net.Uri;
import android.support.annotation.Nullable;
import android.util.Log;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

/** UrlFactory for producing a url connection. */
public class UrlFactory {
  public static final String TAG = UrlFactory.class.getSimpleName();

  // Return connection object, or null on error.
  @Nullable
  public HttpURLConnection openHttpConnection(@Nullable Uri uri) throws IOException {
    URL url;
    try {
      url = new URL(uri.toString());
    } catch (MalformedURLException e) {
      Log.w(TAG, e.toString());
      return null;
    }
    URLConnection urlConnection = url.openConnection();
    if (!(urlConnection instanceof HttpURLConnection)) {
      Log.w(TAG, "Expected HttpURLConnection");
      throw new IllegalArgumentException("Expected HttpURLConnection");
    }
    return (HttpURLConnection) urlConnection;
  }
}
