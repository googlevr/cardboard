/*
 * Copyright 2025 Google LLC
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
#ifndef CARDBOARD_SDK_SCREEN_PARAMS_IOS_SDK_BUNDLE_FINDER_H_
#define CARDBOARD_SDK_SCREEN_PARAMS_IOS_SDK_BUNDLE_FINDER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * A helper class to get the SDK bundle path.
 */
@interface SDKBundleFinder : NSObject

/**
 * Gets the current SDK bundle.
 * @return The SDK bundle.
 */
- (nullable NSBundle *)getSDKBundle;

@end

NS_ASSUME_NONNULL_END

#endif  // CARDBOARD_SDK_SCREEN_PARAMS_IOS_SDK_BUNDLE_FINDER_H_
