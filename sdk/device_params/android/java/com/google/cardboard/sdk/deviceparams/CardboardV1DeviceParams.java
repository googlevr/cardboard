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

import com.google.cardboard.proto.CardboardDevice;

/** Holds Cardboard V1 default device parameters. The device was released at Google I/O in 2014. */
public final class CardboardV1DeviceParams {
  public static final String CARDBOARD_V1_VENDOR = "Google, Inc.";
  public static final String CARDBOARD_V1_MODEL = "Cardboard v1";
  public static final CardboardDevice.DeviceParams.ButtonType CARDBOARD_V1_PRIMARY_BUTTON_TYPE =
      CardboardDevice.DeviceParams.ButtonType.MAGNET;
  public static final float CARDBOARD_V1_SCREEN_TO_LENS_DISTANCE = 0.042f;
  public static final float CARDBOARD_V1_INTER_LENS_DISTANCE = 0.06f;
  public static final CardboardDevice.DeviceParams.VerticalAlignmentType
      CARDBOARD_V1_VERTICAL_ALIGNMENT_TYPE =
          CardboardDevice.DeviceParams.VerticalAlignmentType.BOTTOM;
  public static final float CARDBOARD_V1_TRAY_TO_LENS_CENTER_DISTANCE = 0.035f;
  public static final float[] CARDBOARD_V1_DISTORTION_COEFFS = {0.441f, 0.156f};
  public static final float[] CARDBOARD_V1_FOV_ANGLES = {40.0f, 40.0f, 40.0f, 40.0f};

  private CardboardV1DeviceParams() {
  }

  /**
   * Builds a default initialized {@code CardboardDevice.DeviceParams} with V1 device parameters.
   */
  public static CardboardDevice.DeviceParams build() {
    CardboardDevice.DeviceParams.Builder deviceParamsBuilder =
        CardboardDevice.DeviceParams.newBuilder();
    deviceParamsBuilder
        .setVendor(CARDBOARD_V1_VENDOR)
        .setModel(CARDBOARD_V1_MODEL)
        .setPrimaryButton(CARDBOARD_V1_PRIMARY_BUTTON_TYPE)
        .setScreenToLensDistance(CARDBOARD_V1_SCREEN_TO_LENS_DISTANCE)
        .setInterLensDistance(CARDBOARD_V1_INTER_LENS_DISTANCE)
        .setVerticalAlignment(CARDBOARD_V1_VERTICAL_ALIGNMENT_TYPE)
        .setTrayToLensDistance(CARDBOARD_V1_TRAY_TO_LENS_CENTER_DISTANCE);

    for (float coefficient : CARDBOARD_V1_DISTORTION_COEFFS) {
      deviceParamsBuilder.addDistortionCoefficients(coefficient);
    }

    for (float angle : CARDBOARD_V1_FOV_ANGLES) {
      deviceParamsBuilder.addLeftEyeFieldOfViewAngles(angle);
    }

    return deviceParamsBuilder.build();
  }
}
