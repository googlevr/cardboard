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

#import <UIKit/UIKit.h>
#import <sys/utsname.h>

@interface ScreenOrientationHelper : NSObject {
    
}
@property CardboardScreenOrientation orientation;
-(void) orientationChanged:(NSNotification *)note;
@end

@implementation ScreenOrientationHelper {
    
}
@synthesize orientation;

-(id)init
{
    self = [super init];
    if (self) {
        UIDevice *device = [UIDevice currentDevice];
        [self setOrientationValue:device];
        [device beginGeneratingDeviceOrientationNotifications];
        
        [[NSNotificationCenter defaultCenter]
           addObserver:self selector:@selector(orientationChanged:)
           name:UIDeviceOrientationDidChangeNotification
           object:device];
    }
    return self;
}

-(void) orientationChanged:(NSNotification *)note {
    [self setOrientationValue:note.object];
}

-(void)setOrientationValue:(UIDevice *)device {
    UIDeviceOrientation orientation = [device orientation];
    
    if (!UIDeviceOrientationIsValidInterfaceOrientation(orientation)) {
        return;
    }
    UIInterfaceOrientationMask supportedInterfaces = -1;
    for (UIWindow *window in [[UIApplication sharedApplication] windows]) {
        if (!window.hidden && window.subviews.count > 0) {
            for (int i=0;i<window.subviews.count;i++) {
                UIView *view = window.subviews[window.subviews.count - 1 - i];
                if (!view.isHidden && window.rootViewController) {
                    if (window.rootViewController.shouldAutorotate) {
                        supportedInterfaces = window.rootViewController.supportedInterfaceOrientations;
                        break;
                    }
                }
            }
            break;
        }
    }
    
    if (orientation == UIDeviceOrientationLandscapeLeft && (supportedInterfaces & UIInterfaceOrientationMaskLandscapeRight) != 0) {
        self.orientation = kLandscapeLeft;
    } else if (orientation == UIDeviceOrientationLandscapeRight && (supportedInterfaces & UIInterfaceOrientationMaskLandscapeLeft) != 0) {
        self.orientation = kLandscapeRight;
    } else if (orientation == UIDeviceOrientationPortrait && (supportedInterfaces & UIInterfaceOrientationMaskPortrait) != 0) {
        self.orientation = kPortrait;
    } else if (orientation == UIDeviceOrientationPortraitUpsideDown && (supportedInterfaces & UIInterfaceOrientationMaskPortraitUpsideDown) != 0) {
        self.orientation = kPortraitUpsideDown;
    } else {
        self.orientation = kUnknown;
    }
}
@end

namespace cardboard {
namespace screen_params {

// iPhone Generations.
NSString *const kGIPDeviceGenerationiPhone2G = @"iPhone";
NSString *const kGIPDeviceGenerationiPhone3G = @"iPhone 3G";
NSString *const kGIPDeviceGenerationiPhone3GS = @"iPhone 3Gs";
NSString *const kGIPDeviceGenerationiPhone4 = @"iPhone 4";
NSString *const kGIPDeviceGenerationiPhone4S = @"iPhone 4s";
NSString *const kGIPDeviceGenerationiPhone5 = @"iPhone 5";
NSString *const kGIPDeviceGenerationiPhone5c = @"iPhone 5c";
NSString *const kGIPDeviceGenerationiPhone5s = @"iPhone 5s";
NSString *const kGIPDeviceGenerationiPhone6 = @"iPhone 6";
NSString *const kGIPDeviceGenerationiPhone6Plus = @"iPhone 6 Plus";
NSString *const kGIPDeviceGenerationiPhone6s = @"iPhone 6s";
NSString *const kGIPDeviceGenerationiPhone6sPlus = @"iPhone 6s Plus";
NSString *const kGIPDeviceGenerationiPhone7 = @"iPhone 7";
NSString *const kGIPDeviceGenerationiPhone7Plus = @"iPhone 7 Plus";
NSString *const kGIPDeviceGenerationiPhone8 = @"iPhone 8";
NSString *const kGIPDeviceGenerationiPhone8Plus = @"iPhone 8 Plus";
NSString *const kGIPDeviceGenerationiPhoneSE = @"iPhone SE";
NSString *const kGIPDeviceGenerationiPhoneX = @"iPhone X";
NSString *const kGIPDeviceGenerationiPhoneXr = @"iPhone Xr";
NSString *const kGIPDeviceGenerationiPhoneXs = @"iPhone Xs";
NSString *const kGIPDeviceGenerationiPhoneXsMax = @"iPhone Xs Max";
NSString *const kGIPDeviceGenerationiPhone11 = @"iPhone 11";
NSString *const kGIPDeviceGenerationiPhone11Pro = @"iPhone 11 Pro";
NSString *const kGIPDeviceGenerationiPhone11ProMax = @"iPhone 11 Pro Max";
NSString *const kGIPDeviceGenerationiPhoneSimulator = @"iPhone Simulator";
NSString *const kGIPDeviceGenerationiPhone12Mini = @"iPhone 12 Mini";
NSString *const kGIPDeviceGenerationiPhone12 = @"iPhone 12";
NSString *const kGIPDeviceGenerationiPhone12Pro = @"iPhone 12 Pro";
NSString *const kGIPDeviceGenerationiPhone12ProMax = @"iPhone 12 Pro Max";

// iPod touch Generations.
NSString *const kGIPDeviceGenerationiPodTouch7thGen = @"iPod touch (7th generation)";

// DPI for iPod touch, iPhone (default) and iPhone+: http://dpi.lv/
static CGFloat const kDefaultDpi = 326.0f;
static CGFloat const kIPhonePlusDpi = 401.0f;
static CGFloat const kIPhoneOledDpi = 458.0f;
static CGFloat const kIPhoneXrDpi = 324.0f;
static CGFloat const kIPhoneXsMaxDpi = 456.0f;
static CGFloat const kIPhone12MiniDpi = 476.0f;
static CGFloat const kIPhone12Dpi = 460.0f;

static ScreenOrientationHelper *helper = [[ScreenOrientationHelper alloc] init];

CardboardScreenOrientation getScreenOrientation() {
    return [helper orientation];
}

CGFloat getDpi() {
  // Gets model name.
  struct utsname systemInfo;
  uname(&systemInfo);

  NSString *modelName = [NSString stringWithCString:systemInfo.machine
                                           encoding:NSUTF8StringEncoding];

  // Narrows it down to a specific generation, if we recognize it.
  // Information taken from http://theiphonewiki.com/wiki/Models
  NSDictionary *models = @{
    @"iPhone1,1" : kGIPDeviceGenerationiPhone2G,
    @"iPhone1,2" : kGIPDeviceGenerationiPhone3G,
    @"iPhone2,1" : kGIPDeviceGenerationiPhone3GS,
    @"iPhone3,1" : kGIPDeviceGenerationiPhone4,
    @"iPhone3,2" : kGIPDeviceGenerationiPhone4,
    @"iPhone3,3" : kGIPDeviceGenerationiPhone4,
    @"iPhone4,1" : kGIPDeviceGenerationiPhone4S,
    @"iPhone4,2" : kGIPDeviceGenerationiPhone4S,
    @"iPhone5,1" : kGIPDeviceGenerationiPhone5,
    @"iPhone5,2" : kGIPDeviceGenerationiPhone5,
    @"iPhone5,3" : kGIPDeviceGenerationiPhone5c,
    @"iPhone5,4" : kGIPDeviceGenerationiPhone5c,
    @"iPhone6,1" : kGIPDeviceGenerationiPhone5s,
    @"iPhone6,2" : kGIPDeviceGenerationiPhone5s,
    @"iPhone7,1" : kGIPDeviceGenerationiPhone6Plus,  // 6+ is 7,1
    @"iPhone7,2" : kGIPDeviceGenerationiPhone6,      // 6  is 7,2
    @"iPhone8,1" : kGIPDeviceGenerationiPhone6s,
    @"iPhone8,2" : kGIPDeviceGenerationiPhone6sPlus,
    // There is no iPhone8,3.
    @"iPhone8,4" : kGIPDeviceGenerationiPhoneSE,
    @"iPhone9,1" : kGIPDeviceGenerationiPhone7,
    @"iPhone9,2" : kGIPDeviceGenerationiPhone7Plus,
    @"iPhone9,3" : kGIPDeviceGenerationiPhone7,
    @"iPhone9,4" : kGIPDeviceGenerationiPhone7Plus,
    @"iPhone10,1" : kGIPDeviceGenerationiPhone8,
    @"iPhone10,2" : kGIPDeviceGenerationiPhone8Plus,
    @"iPhone10,3" : kGIPDeviceGenerationiPhoneX,
    @"iPhone10,4" : kGIPDeviceGenerationiPhone8,
    @"iPhone10,5" : kGIPDeviceGenerationiPhone8Plus,
    @"iPhone10,6" : kGIPDeviceGenerationiPhoneX,
    @"iPhone11,2" : kGIPDeviceGenerationiPhoneXs,
    @"iPhone11,4" : kGIPDeviceGenerationiPhoneXsMax,
    @"iPhone11,6" : kGIPDeviceGenerationiPhoneXsMax,
    @"iPhone11,8" : kGIPDeviceGenerationiPhoneXr,
    @"iPhone12,1" : kGIPDeviceGenerationiPhone11,
    @"iPhone12,3" : kGIPDeviceGenerationiPhone11Pro,
    @"iPhone12,5" : kGIPDeviceGenerationiPhone11ProMax,
    @"iPhone13,1" : kGIPDeviceGenerationiPhone12Mini,
    @"iPhone13,2" : kGIPDeviceGenerationiPhone12,
    @"iPhone13,3" : kGIPDeviceGenerationiPhone12Pro,
    @"iPhone13,4" : kGIPDeviceGenerationiPhone12ProMax,
    @"iPod9,1" : kGIPDeviceGenerationiPodTouch7thGen,
  };
  NSString *model = models[modelName];
  if (!model) {
    return kDefaultDpi;
  }

  // Gets Dpi for a particular generation.
  NSDictionary *dpis = @{
    kGIPDeviceGenerationiPhone2G : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone3G : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone3GS : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone4 : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone4S : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone5 : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone5c : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone5s : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone6Plus : @(kIPhonePlusDpi),
    kGIPDeviceGenerationiPhone6 : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone6s : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone6sPlus : @(kIPhonePlusDpi),
    kGIPDeviceGenerationiPhoneSE : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone7 : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone7Plus : @(kIPhonePlusDpi),
    kGIPDeviceGenerationiPhone8 : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone8Plus : @(kIPhonePlusDpi),
    kGIPDeviceGenerationiPhoneX : @(kIPhoneOledDpi),
    kGIPDeviceGenerationiPhoneXs : @(kIPhoneOledDpi),
    kGIPDeviceGenerationiPhoneXsMax : @(kIPhoneXsMaxDpi),
    kGIPDeviceGenerationiPhoneXr : @(kIPhoneXrDpi),
    kGIPDeviceGenerationiPhone11 : @(kDefaultDpi),
    kGIPDeviceGenerationiPhone11Pro : @(kIPhoneOledDpi),
    kGIPDeviceGenerationiPhone11ProMax : @(kIPhoneOledDpi),
    kGIPDeviceGenerationiPhone12Mini : @(kIPhone12MiniDpi),
    kGIPDeviceGenerationiPhone12 : @(kIPhone12Dpi),
    kGIPDeviceGenerationiPhone12Pro : @(kIPhone12Dpi),
    kGIPDeviceGenerationiPhone12ProMax : @(kIPhoneOledDpi),
    kGIPDeviceGenerationiPodTouch7thGen : @(kDefaultDpi),
  };

  NSNumber *dpi = dpis[model];
  if (!dpi) {
    return kDefaultDpi;
  }

  return dpi.doubleValue;
}

void getScreenSizeInMeters(int width_pixels, int height_pixels, float* out_width_meters,
                           float* out_height_meters) {
  float dpi = getDpi();
  *out_width_meters = (width_pixels / dpi) * kMetersPerInch;
  *out_height_meters = (height_pixels / dpi) * kMetersPerInch;
}

}  // namespace screen_params
}  // namespace cardboard
