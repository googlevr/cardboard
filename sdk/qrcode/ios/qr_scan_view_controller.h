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
#import <UIKit/UIKit.h>

/**
 * Block signature that is called when a QR code scan is finished.
 *
 * @param succeeded Indicates that whether the QR code scan is successful.
 */
typedef void (^QrScanCompletionCallback)(BOOL succeeded);

/**
 * ViewController that scans a QR code and returns the result in the callback.
 */
@interface CardboardQRScanViewController : UIViewController

/**
 * Inits with completion callback.
 *
 * @param completion Callback to be executed upon completion.
 */
- (nonnull instancetype)initWithCompletion:(nonnull QrScanCompletionCallback)completion;

@end
