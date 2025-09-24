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
#import "screen_params/ios/sdk_bundle_finder.h"
#import "util/logging.h"

@implementation SDKBundleFinder

- (nullable NSBundle *)getSDKBundle {
  NSBundle *thisBundle = [NSBundle bundleForClass:[self class]];
  NSString *sdkBundlePath = [thisBundle pathForResource:@"sdk" ofType:@"bundle"];
  if (!sdkBundlePath) {
    // We must be in the sdk bundle as there is no resource bundle under us.
    return thisBundle;
  }

  return [NSBundle bundleWithPath:sdkBundlePath];
}

@end
