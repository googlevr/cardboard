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
package com.google.cardboard.sdk.deviceparams;

import android.util.Log;
import androidx.annotation.Nullable;
import com.google.cardboard.proto.CardboardDevice;
import com.google.cardboard.sdk.UsedByNative;
import com.google.protobuf.ExtensionRegistryLite;
import com.google.protobuf.InvalidProtocolBufferException;

/** Utility class to parse device parameters. */
public class DeviceParamsUtils {

  private static final String TAG = DeviceParamsUtils.class.getSimpleName();

  /** Class only contains static methods. */
  private DeviceParamsUtils() {}

  /**
   * Parses device parameters from serialized buffer.
   *
   * @param[in] serializedDeviceParams Device parameters byte buffer.
   * @return The embedded params. Null if the embedded params do not exist or parsing fails.
   */
  @Nullable
  @UsedByNative
  public static CardboardDevice.DeviceParams parseCardboardDeviceParams(
      byte[] serializedDeviceParams) {
    try {
      return CardboardDevice.DeviceParams.parseFrom(
          serializedDeviceParams, ExtensionRegistryLite.getEmptyRegistry());
    } catch (InvalidProtocolBufferException e) {
      Log.w(TAG, "Parsing cardboard parameters from buffer failed: " + e);
      return null;
    }
  }
}
