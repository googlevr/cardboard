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

#import "HelloCardboardViewController.h"

#import "HelloCardboardOverlayView.h"
#import "HelloCardboardRenderer.h"
#import "cardboard.h"

@interface HelloCardboardViewController () <GLKViewControllerDelegate, HelloCardboardOverlayViewDelegate> {
  CardboardLensDistortion *_cardboardLensDistortion;
  CardboardHeadTracker *_cardboardHeadTracker;
  std::unique_ptr<cardboard::hello_cardboard::HelloCardboardRenderer> _renderer;

  // This counter keeps track of the successful device parameters save operations count.
  int _deviceParamsChangedCount;
}
@end

@implementation HelloCardboardViewController

- (void)dealloc {
  CardboardLensDistortion_destroy(_cardboardLensDistortion);
  CardboardHeadTracker_destroy(_cardboardHeadTracker);
}

- (void)viewDidLoad {
  [super viewDidLoad];

  self.delegate = self;

  // Create an overlay view on top of the GLKView.
  HelloCardboardOverlayView *overlayView = [[HelloCardboardOverlayView alloc] initWithFrame:self.view.bounds];
  overlayView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  overlayView.delegate = self;
  [self.view addSubview:overlayView];

  // Add a tap gesture to handle viewer trigger action.
  UITapGestureRecognizer *tapGesture =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(didTapGLView:)];
  [self.view addGestureRecognizer:tapGesture];

  // Prevents screen to turn off.
  UIApplication.sharedApplication.idleTimerDisabled = YES;

  // Create an OpenGL ES context and assign it to the view loaded from storyboard
  GLKView *glkView = (GLKView *)self.view;
  glkView.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

  // Set animation frame rate.
  self.preferredFramesPerSecond = 60;

  // Set the GL context.
  [EAGLContext setCurrentContext:glkView.context];

  // Make sure the glkView has bound its offscreen buffers before calling into cardboard.
  [glkView bindDrawable];

  // Create cardboard head tracker.
  _cardboardHeadTracker = CardboardHeadTracker_create();
  _cardboardLensDistortion = nil;

  // Set the counter to -1 to force a device params update.
  _deviceParamsChangedCount = -1;
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];
  if (@available(iOS 11.0, *)) {
    [self setNeedsUpdateOfHomeIndicatorAutoHidden];
  }
  [self resumeCardboard];
}

- (BOOL)prefersHomeIndicatorAutoHidden {
  return true;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
  // Cardboard only supports landscape right orientation for inserting the phone in the viewer.
  return UIInterfaceOrientationMaskLandscapeRight;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect {
  if (![self updateDeviceParams]) {
    return;
  }

  _renderer->DrawFrame();
}

- (void)glkViewControllerUpdate:(GLKViewController *)controller {
  // Perform GL state update before drawing.
}

- (BOOL)deviceParamsChanged {
  return _deviceParamsChangedCount != CardboardQrCode_getDeviceParamsChangedCount();
}

- (BOOL)updateDeviceParams {
  // Check if device parameters have changed.
  if (![self deviceParamsChanged]) {
    return YES;
  }

  uint8_t *encodedDeviceParams;
  int size;
  CardboardQrCode_getSavedDeviceParams(&encodedDeviceParams, &size);

  if (size == 0) {
    return NO;
  }

  // Using native scale as we are rendering directly to the screen.
  CGRect screenRect = self.view.bounds;
  CGFloat screenScale = UIScreen.mainScreen.nativeScale;
  int height = screenRect.size.height * screenScale;
  int width = screenRect.size.width * screenScale;

  // Rendering coordinates asumes landscape orientation.
  if (height > width) {
    int temp = height;
    height = width;
    width = temp;
  }

  // Create CardboardLensDistortion.
  CardboardLensDistortion_destroy(_cardboardLensDistortion);
  _cardboardLensDistortion =
      CardboardLensDistortion_create(encodedDeviceParams, size, width, height);

  // Initialize HelloCardboardRenderer.
  _renderer.reset(new cardboard::hello_cardboard::HelloCardboardRenderer(
      _cardboardLensDistortion, _cardboardHeadTracker, width, height));
  _renderer->InitializeGl();

  CardboardQrCode_destroy(encodedDeviceParams);

  _deviceParamsChangedCount = CardboardQrCode_getDeviceParamsChangedCount();

  return YES;
}

- (void)pauseCardboard {
  self.paused = true;
  CardboardHeadTracker_pause(_cardboardHeadTracker);
}

- (void)resumeCardboard {
  // Check for device parameters existence in app storage. If they're missing,
  // we must scan a Cardboard QR code and save the obtained parameters.
  uint8_t *buffer;
  int size;
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);
  if (size == 0) {
    [self switchViewer];
  }
  CardboardQrCode_destroy(buffer);

  CardboardHeadTracker_resume(_cardboardHeadTracker);
  self.paused = false;
}

- (void)switchViewer {
  CardboardQrCode_scanQrCodeAndSaveDeviceParams();
}

- (void)didTapGLView:(id)sender {
  if (_renderer != NULL) {
    _renderer->OnTriggerEvent();
  }
}

- (void)didTapBackButton {
  // User pressed the back button. Pop this view controller.
  NSLog(@"User pressed back button");
}

- (void)didPresentSettingsDialog:(BOOL)presented {
  // The overlay view is presenting the settings dialog. Pause our rendering while presented.
  self.paused = presented;
}

- (void)didChangeViewerProfile {
  [self pauseCardboard];
  [self switchViewer];
  [self resumeCardboard];
}

@end
