/*
 * Copyright 2020 Google LLC
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
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;

import com.google.android.gms.common.GoogleSignatureVerifier;

/**
 * Helper class for querying Android package properties.
 */
public class PackageUtils {
  /** Class only contains static methods. */
  private PackageUtils() {}

  /**
   * Reports whether a given package is a "Google" package by checking if it is Google-signed.
   *
   * @param context The current Context. It is generally an Activity instance or wraps one.
   * @param packageName the name of the package in question
   * @return whether the specified package is a "Google" package
   */
  public static boolean isGooglePackage(Context context, String packageName) {
    if (context == null || packageName == null) {
      return false;
    }
    return GoogleSignatureVerifier.getInstance(context).isPackageGoogleSigned(packageName);
  }

  /**
   * Reports whether a given package is a "system" package.
   *
   * @param context The current Context. It is generally an Activity instance or wraps one.
   *     {@code Context.getPackageManager()} will be invoked.
   * @param packageName the name of the package in question
   * @return whether the specified package is a "system" package
   */
  public static boolean isSystemPackage(Context context, String packageName) {
    try {
      ApplicationInfo applicationInfo =
          context.getPackageManager().getApplicationInfo(packageName, 0);
      int applicationFlags = applicationInfo != null ? applicationInfo.flags : 0;
      if (((applicationFlags & ApplicationInfo.FLAG_SYSTEM) != 0)
          || ((applicationFlags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0)) {
        return true;
      }
    } catch (PackageManager.NameNotFoundException e) {
      return false;
    }
    return false;
  }
}
