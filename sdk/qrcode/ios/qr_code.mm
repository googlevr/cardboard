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
#import "qr_code.h"

#import <AVFoundation/AVFoundation.h>
#import <libkern/OSAtomic.h>
#import <UIKit/UIKit.h>

#import "qrcode/ios/device_params_helper.h"
#import "qrcode/ios/qr_scan_view_controller.h"

namespace cardboard {
namespace qrcode {
namespace {

volatile int32_t qrCodeScanCount = 0;

void incrementQrCodeScanCount() { OSAtomicIncrement32Barrier(&qrCodeScanCount); }

void showQRScanViewController() {
  UIViewController *presentingViewController = nil;
  presentingViewController = [UIApplication sharedApplication].keyWindow.rootViewController;
  while (presentingViewController.presentedViewController) {
    presentingViewController = presentingViewController.presentedViewController;
  }
  if (presentingViewController.isBeingDismissed) {
    presentingViewController = presentingViewController.presentingViewController;
  }

  __block CardboardQRScanViewController *qrViewController =
      [[CardboardQRScanViewController alloc] initWithCompletion:^(BOOL succeeded) {
        incrementQrCodeScanCount();
        [qrViewController dismissViewControllerAnimated:YES completion:nil];
      }];

  qrViewController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
  [presentingViewController presentViewController:qrViewController animated:YES completion:nil];
}

void requestPermissionInSettings() {
  NSURL *settingURL = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
  [UIApplication.sharedApplication openURL:settingURL];
}

void prerequestCameraPermissionForQRScan() {
  [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                           completionHandler:^(BOOL granted) {
                             dispatch_async(dispatch_get_main_queue(), ^{
                               if (granted) {
                                 showQRScanViewController();
                               } else {
                                 requestPermissionInSettings();
                               }
                             });
                           }];
}

}  // anonymous namespace

std::vector<uint8_t> getCurrentSavedDeviceParams() {
  NSData *deviceParams = [CardboardDeviceParamsHelper readSerializedDeviceParams];
  std::vector<uint8_t> result(
      static_cast<const uint8_t *>(deviceParams.bytes),
      static_cast<const uint8_t *>(deviceParams.bytes) + deviceParams.length);
  return result;
}

void scanQrCodeAndSaveDeviceParams() {
  switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]) {
    case AVAuthorizationStatusAuthorized:
      // We already have camera permissions - proceed to QR scan controller.
      showQRScanViewController();
      return;

    case AVAuthorizationStatusNotDetermined:
      // We have not yet shown the camera request prompt.
      prerequestCameraPermissionForQRScan();
      return;

    default:
      // We've had permission explicitly rejected before.
      requestPermissionInSettings();
      return;
  }
}

int getQrCodeScanCount() { return qrCodeScanCount; }

}  // namespace qrcode
}  // namespace cardboard
