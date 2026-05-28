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

import android.content.Context;

/**
 * Cardboard SDK Initialize API.
 *
 * <p>It is only needed to be called once per Activity. Code within sdk package is not guaranteed to
 * work from multiple Activities. Every time an Activity is destroyed where references to these
 * classes live, all destroy() methods should be called before de-referencing them. Every time an
 * Activity is created and the SDK JNI objects instantiated, a first call to initialize() is needed
 * and then other objects can be constructed.
 */
public class Initialize {
  static {
    System.loadLibrary("cardboard_sdk_jni");
  }

  /** Class only contains static methods. */
  private Initialize() {}

  /**
   * Initializes JavaVM and Android context.
   *
   * @param[in] context The current Context. It is generally an Activity instance or wraps one. The
   *     following is the list of methods that will be queried: getFilesDir(),
   *     getResources(), getSystemService(Context.WINDOW_SERVICE), startActivity(Intent),
   *     getDisplay().
   */
  public static void initialize(Context context) {
    nativeInitialize(context);
  }

  private static native void nativeInitialize(Context context);
}
