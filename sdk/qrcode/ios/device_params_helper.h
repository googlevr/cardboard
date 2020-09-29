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
 * Helper class to read and write cardboard device params.
 */
@interface CardboardDeviceParamsHelper : NSObject

/**
 * Reads currently saved device params. Returns nil if no params are saved yet.
 *
 * @return The currently saved encoded device params.
 */
+ (nullable NSData *)readSerializedDeviceParams;

/**
 * Loads and validates the device parameters from the given URL. Upon success it saves the viewer
 * profile data and calls the completion block with the result of validation and an optional error.
 *
 * @param url The URL to retrieve the device params from.
 * @param completion Callback to be executed upon completion.
 */
+ (void)resolveAndUpdateViewerProfileFromURL:(nullable NSURL *)url
                              withCompletion:
                                  (nonnull void (^)(BOOL success, NSError *_Nullable))completion;

/**
 * Writes Cardboard V1 device parameters in storage.
 */
+ (void)saveCardboardV1Params;

@end
