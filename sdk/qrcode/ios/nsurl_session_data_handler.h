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

/** The completion will be called once the target url is fetched. */
typedef void (^OnUrlCompletion)(NSURL* _Nullable targetUrl, NSError* _Nullable error);

/**
 * Helper class that handles NSURL session data.
 */
@interface NSURLSessionDataHandler : NSObject <NSURLSessionDataDelegate>

/**
 * Analyzes if the given URL identifies an original Cardboard viewer (or equivalent).
 *
 * @param url URL to analyze.
 * @return true if the given URL identifies an original Cardboard viewer (or equivalent).
 */
+ (BOOL)isOriginalCardboardDeviceUrl:(nonnull NSURL*)url;

/**
 * Analyzes if the given URL identifies a Cardboard device using current scheme.
 *
 * @param url URL to analyze.
 * @return true if the given URL identifies a Cardboard device using current scheme.
 */
+ (BOOL)isCardboardDeviceUrl:(nonnull NSURL*)url;

/**
 * Inits handler with OnUrlCompletion callback.
 *
 * @param completion Callback to be executed upon completion.
 */
- (nonnull instancetype)initWithOnUrlCompletion:(nonnull OnUrlCompletion)completion;

@end
