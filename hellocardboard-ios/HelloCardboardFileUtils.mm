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
#import "HelloCardboardFileUtils.h"

#import <UIKit/UIKit.h>

#include <string>

namespace cardboard {
namespace hello_cardboard {

void LoadPngFile(const std::string& imageName, GLubyte** outImageData, GLuint* outWidth,
                 GLuint* outHeight) {
  NSString* imagePath = [NSString stringWithUTF8String:imageName.c_str()];
  UIImage* image = [UIImage imageNamed:imagePath];
  GLuint width = static_cast<GLuint>(CGImageGetWidth(image.CGImage));
  GLuint height = static_cast<GLuint>(CGImageGetHeight(image.CGImage));
  GLubyte* imageData = new GLubyte[width * height * 4];
  CGContextRef imageContext =
      CGBitmapContextCreate(imageData, image.size.width, image.size.height, 8, image.size.width * 4,
                            CGColorSpaceCreateDeviceRGB(), kCGImageAlphaPremultipliedLast);
  CGContextDrawImage(imageContext, CGRectMake(0.0, 0.0, image.size.width, image.size.height),
                     image.CGImage);
  CGContextRelease(imageContext);
  *outImageData = imageData;
  *outWidth = image.size.width;
  *outHeight = image.size.height;
}

std::string LoadTextFile(const std::string& fileName) {
  NSString* file = [NSString stringWithUTF8String:fileName.c_str()];
  NSString* path = [NSBundle.mainBundle pathForResource:file ofType:@"obj"];
  NSString* content = [NSString stringWithContentsOfFile:path
                                                encoding:NSUTF8StringEncoding
                                                   error:nil];
  std::string fileBuffer = std::string(content.UTF8String);
  return fileBuffer;
}

}  // namespace hello_cardboard
}  // namespace cardboard
