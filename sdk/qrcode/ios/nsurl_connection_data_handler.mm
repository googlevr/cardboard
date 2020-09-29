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
#import "qrcode/ios/nsurl_connection_data_handler.h"

static NSString *const kCardboardDeviceParamsUrlPrefix = @"http://google.com/cardboard/cfg";
static NSString *const kOriginalCardboardDeviceParamsUrl = @"http://g.co/cardboard";

@implementation CardboardNSURLConnectionDataHandler {
  OnUrlCompletion _completion;
  BOOL _success;
}

// Returns true if given URL identifies a Cardboard device using current scheme.
+ (BOOL)isCardboardDeviceUrl:(NSURL *)url {
  return [[url absoluteString] hasPrefix:kCardboardDeviceParamsUrlPrefix];
}

- (instancetype)initWithOnUrlCompletion:(OnUrlCompletion)completion {
  self = [super init];
  if (self) {
    _completion = completion;
    _success = NO;
  }
  return self;
}

- (NSURLRequest *)connection:(NSURLConnection *)connection
             willSendRequest:(NSURLRequest *)request
            redirectResponse:(NSURLResponse *)response {
  if (request.URL) {
    if ([CardboardNSURLConnectionDataHandler isCardboardDeviceUrl:request.URL]) {
      _success = YES;
      _completion(request.URL, nil);
      return nil;
    }

    if ([kOriginalCardboardDeviceParamsUrl isEqualToString:request.URL.absoluteString]) {
      _success = YES;
      _completion(request.URL, nil);
      return nil;
    }
  }
  return request;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
  if (!_success) {
    _completion(nil, nil);
  }
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
  _success = NO;
  _completion(nil, error);
}

@end
