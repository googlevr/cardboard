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
#import "screen_params.h"
#import "screen_params/ios/sdk_bundle_finder.h"
#import "util/logging.h"
#import <UIKit/UIKit.h>
#import <sys/utsname.h>
#import <fstream>
#import <string>
#import <sstream>
#import <string_view>
#import <unordered_map>


namespace cardboard {
namespace screen_params {

constexpr CGFloat kDefaultDpi = 326.0f;
using IosScreenParamsMap = std::unordered_map<std::string, std::pair<std::string, CGFloat>>;

std::array<std::string, 3> stringSplit(const std::string& s, const char delimiter) {
    std::array<std::string, 3> tokens;
    std::string token;
    std::istringstream tokenStream(s);
  
    for (uint32_t i = 0; i < 3 && std::getline(tokenStream, token, delimiter); i++) {
      tokens[i] = token;
    }
  
    return tokens;
}

IosScreenParamsMap loadIosScreenParamsFromFile(std::string_view filePath) {
  IosScreenParamsMap screenParams;

  std::ifstream screenParamsFile{filePath.data()};
  if (!screenParamsFile.is_open()) {
    CARDBOARD_LOGE("Couldn't open file: %s", filePath.data());
    return screenParams;
  }

  std::string line;
  std::getline(screenParamsFile, line);  // Skip the header
  while (std::getline(screenParamsFile, line)) {
    const std::array<std::string, 3> tokens {stringSplit(line, ',')};
    screenParams[tokens.at(0)] = {tokens.at(1), strtod(tokens.at(2).c_str(), nullptr)};
  }

  return screenParams;
}

CGFloat getDpi() {
  // Gets model name.
  struct utsname systemInfo;
  uname(&systemInfo);

  NSString *machineName = [NSString stringWithCString:systemInfo.machine
                                             encoding:NSUTF8StringEncoding];
  machineName = [machineName stringByReplacingOccurrencesOfString:@"," withString:@"."];
  const std::string modelName{[machineName UTF8String]};

  CARDBOARD_LOGI("Model name: %s", modelName.c_str());

  const SDKBundleFinder *bundleFinder = [[SDKBundleFinder alloc] init];
  const NSBundle *sdkBundle = [bundleFinder getSDKBundle];
  const NSString *screenParamsFilePath = [sdkBundle pathForResource:@"resolutions" ofType:@"csv"];
  const IosScreenParamsMap screenParams{loadIosScreenParamsFromFile([screenParamsFilePath UTF8String])};

  if (screenParams.find(modelName) == screenParams.end()) {
    CARDBOARD_LOGE("Couldn't find screen params for model: %s", modelName.c_str());
    return kDefaultDpi;
  }

  return screenParams.at(modelName).second;
}

void getScreenSizeInMeters(int width_pixels, int height_pixels, float *out_width_meters,
                           float *out_height_meters) {
  const float dpi = getDpi();
  *out_width_meters = (width_pixels / dpi) * kMetersPerInch;
  *out_height_meters = (height_pixels / dpi) * kMetersPerInch;
}

}  // namespace screen_params
}  // namespace cardboard
