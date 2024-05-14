/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support. Compile with -fobjc-arc"
#endif

#import "HelloCardboardAppDelegate.h"
#import "HelloCardboardViewController.h"

@interface HelloCardboardAppDelegate () <UINavigationControllerDelegate>

@end

@implementation HelloCardboardAppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  UINavigationController *navigationController = [[UINavigationController alloc]
      initWithRootViewController:[[HelloCardboardViewController alloc] init]];
  navigationController.delegate = self;
  navigationController.navigationBarHidden = YES;

  self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
  self.window.rootViewController = navigationController;
  [self.window makeKeyAndVisible];

  // Adjust main window frame to not display on top of notch if exists.
  UIEdgeInsets safeInsets = UIApplication.sharedApplication.keyWindow.safeAreaInsets;
  CGRect bounds = UIScreen.mainScreen.bounds;
  bounds.origin.x += safeInsets.left;
  bounds.size.width -= safeInsets.left;
  self.window.frame = bounds;

  return YES;
}

// Make the navigation controller defer the check of supported orientation to its visible view
// controller. This allows |CardboardViewController| to lock the orientation in VR mode and allow
// all orientations for QRScanViewController.
- (UIInterfaceOrientationMask)navigationControllerSupportedInterfaceOrientations:
    (UINavigationController *)navigationController {
  return [navigationController.visibleViewController supportedInterfaceOrientations];
}

@end
