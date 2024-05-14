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
#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support. Compile with -fobjc-arc"
#endif

#import "qrcode/ios/qr_scan_view_controller.h"

#import <AVFoundation/AVFoundation.h>

#import "qrcode/ios/device_params_helper.h"

// Other constants.
static NSString *const kQrSampleImageName = @"qrSample.png";
static NSString *const kTickmarksImageName = @"tickmarks.png";
static NSString *const kQrNoData = @"QR scan cancelled or other error";
static const CGFloat kGuidanceHeight = 116.0f;

@interface CardboardQRScanViewController () <AVCaptureMetadataOutputObjectsDelegate> {
  AVCaptureSession *_captureSession;
  AVCaptureVideoPreviewLayer *_videoPreviewLayer;
  QrScanCompletionCallback _completion;
  UIView *_preview;
  UIView *_tickmarkView;
  UIView *_guidanceView;
  UIView *_guidanceInsetView;
  BOOL _processing;
  BOOL _showingHUDMessage;
}
@end

@implementation CardboardQRScanViewController

+ (void)showConfirmationDialogWithCompletion:(QrScanCompletionCallback)completion {
  dispatch_async(dispatch_get_main_queue(), ^{
    completion(YES);
  });
}

+ (NSBundle *)resourceBundle {
  NSBundle *thisBundle = [NSBundle bundleForClass:[self class]];
  NSString *resourcePath = [thisBundle pathForResource:@"sdk" ofType:@"bundle"];
  if (!resourcePath) {
    // We must be in the sdk bundle as there is no resource bundle under us.
    return thisBundle;
  }
  return [NSBundle bundleWithPath:resourcePath];
}

+ (UIImage *)loadImageNamed:(NSString *)imageName {
  NSBundle *bundle = [self resourceBundle];
  UIImage *image = [UIImage imageNamed:imageName inBundle:bundle compatibleWithTraitCollection:nil];
  // If we cannot find the image in sdk bundle, try again in main bundle.
  if (image) {
    return image;
  } else {
    return [UIImage imageNamed:imageName inBundle:nil compatibleWithTraitCollection:nil];
  }
}

- (instancetype)initWithCompletion:(void (^)(BOOL succeed))completion {
  self = [super init];
  if (self) {
    _completion = completion;
  }

  return self;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didOutputMetadataObjects:(NSArray *)metadataObjects
              fromConnection:(AVCaptureConnection *)connection {
  if (_processing) {
    return;
  }

  if (metadataObjects != nil && [metadataObjects count] > 0) {
    AVMetadataMachineReadableCodeObject *metadataObj = [metadataObjects objectAtIndex:0];
    if ([[metadataObj type] isEqualToString:AVMetadataObjectTypeQRCode]) {
      _processing = YES;
      [self processQrCode:[metadataObj stringValue]];
    }
  }
}

- (void)viewDidLoad {
  [super viewDidLoad];
  // Initialize the back button.
  UIBarButtonItem *backBarButton = [[UIBarButtonItem alloc] initWithTitle:@"Cardboard QR Code"
                                                                    style:UIBarButtonItemStylePlain
                                                                   target:self
                                                                   action:@selector(skipAndClose)];
  self.navigationItem.title = @"Cardboard QR Code nav title";
  self.navigationItem.leftBarButtonItem = backBarButton;

  // Add preview screen.
  CGRect frame = [[UIScreen mainScreen] bounds];
  frame.size.height -= kGuidanceHeight;
  _preview = [[UIView alloc] initWithFrame:frame];
  _preview.backgroundColor = [UIColor blackColor];
  _preview.autoresizingMask = (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
  _preview.opaque = NO;
  [self.view addSubview:_preview];

  // Start capture session if possible.
  NSError *error;
  AVCaptureDevice *captureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
  AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice
                                                                      error:&error];
  if (!input) {
    NSLog(@"%@", [error localizedDescription]);
  } else {
    _captureSession = [[AVCaptureSession alloc] init];
    [_captureSession addInput:input];
    AVCaptureMetadataOutput *captureMetadataOutput = [[AVCaptureMetadataOutput alloc] init];
    [_captureSession addOutput:captureMetadataOutput];
    [captureMetadataOutput setMetadataObjectsDelegate:self queue:dispatch_get_main_queue()];
    [captureMetadataOutput
        setMetadataObjectTypes:[NSArray arrayWithObject:AVMetadataObjectTypeQRCode]];
    _videoPreviewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:_captureSession];
    [_videoPreviewLayer setVideoGravity:AVLayerVideoGravityResizeAspectFill];
    _videoPreviewLayer.frame = _preview.layer.bounds;
    [_preview.layer addSublayer:_videoPreviewLayer];
    [_captureSession startRunning];
  }

  // Add tick marks on preview screen.
  UIImage *tickmarkImage = [CardboardQRScanViewController loadImageNamed:kTickmarksImageName];
  _tickmarkView = [[UIImageView alloc] initWithImage:tickmarkImage];
  frame.size.width = tickmarkImage.size.width / 2;
  frame.size.height = tickmarkImage.size.height / 2;
  _tickmarkView.frame = frame;
  _tickmarkView.center = _preview.center;
  _tickmarkView.autoresizingMask =
      (UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin |
       UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin);
  [_preview addSubview:_tickmarkView];

  // Add guidance at the bottom with instructions on locating a QR code and a skip button.
  _guidanceView = [[UIView alloc] init];
  _guidanceView.backgroundColor = [UIColor whiteColor];
  frame = self.view.frame;
  frame.origin.y = frame.size.height;
  frame.size.height = kGuidanceHeight;
  frame.origin.y -= frame.size.height;
  _guidanceView.frame = frame;
  _guidanceView.autoresizingMask =
      (UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleWidth);
  [self.view addSubview:_guidanceView];

  _guidanceInsetView = [[UIView alloc] init];
  [self updateGuidanceInsetViewFrame];
  [_guidanceView addSubview:_guidanceInsetView];

  // Add sample qr image.
  UIImage *qrSampleImage = [CardboardQRScanViewController loadImageNamed:kQrSampleImageName];
  UIImageView *qrSampleView = [[UIImageView alloc] initWithImage:qrSampleImage];
  frame = qrSampleView.frame;
  frame.origin.x = 16;
  frame.origin.y = 16;
  frame.size.width = qrSampleImage.size.width / 2;
  frame.size.height = qrSampleImage.size.height / 2;
  qrSampleView.frame = frame;
  qrSampleView.autoresizingMask =
      (UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin);
  [_guidanceInsetView addSubview:qrSampleView];

  // Add find text.
  UILabel *findLabel = [[UILabel alloc] init];
  findLabel.text = @"Find this Cardboard symbol on your viewer";
  findLabel.font = [UIFont systemFontOfSize:16];
  [findLabel sizeToFit];
  frame = findLabel.frame;
  frame.origin.x = 64;
  frame.origin.y = 29 - frame.size.height / 2;
  findLabel.frame = frame;
  findLabel.autoresizingMask =
      (UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin);
  [_guidanceInsetView addSubview:findLabel];

  // Add a divider line.
  UIView *dividerView = [[UIView alloc] init];
  dividerView.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:.12f];
  frame = _guidanceInsetView.frame;
  frame.origin.x = 64;
  frame.size.width -= 64;
  frame.size.height = 1;
  frame.origin.y = (_guidanceInsetView.frame.size.height - frame.size.height) / 2;
  dividerView.frame = frame;
  dividerView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  [_guidanceInsetView addSubview:dividerView];

  // Add text about not finding symbol.
  UILabel *cantFindLabel = [[UILabel alloc] init];
  cantFindLabel.text = @"Can't find this symbol?";
  cantFindLabel.font = [UIFont systemFontOfSize:14];
  cantFindLabel.textColor = [UIColor grayColor];
  [cantFindLabel sizeToFit];
  frame = cantFindLabel.frame;
  frame.origin.x = 64;
  frame.origin.y = _guidanceInsetView.frame.size.height - 29 - frame.size.height / 2;
  cantFindLabel.frame = frame;
  cantFindLabel.autoresizingMask =
      (UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin);
  [_guidanceInsetView addSubview:cantFindLabel];

  // Add skip button.
  UIButton *skipButton = [UIButton buttonWithType:UIButtonTypeSystem];
  skipButton.accessibilityIdentifier = @"SkipButton";
  [skipButton setTitle:@"Skip" forState:UIControlStateNormal];
  [skipButton addTarget:nil
                 action:@selector(skipAndClose)
       forControlEvents:UIControlEventTouchUpInside];
  if ([[UIDevice currentDevice].systemVersion floatValue] >= 8.2) {
    skipButton.titleLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
  } else {
    skipButton.titleLabel.font = [UIFont systemFontOfSize:14];
  }
  [skipButton sizeToFit];
  frame = skipButton.frame;
  frame.origin.x = _guidanceInsetView.frame.size.width - frame.size.width - 22;
  frame.origin.y = _guidanceInsetView.frame.size.height - 29 - frame.size.height / 2;
  skipButton.frame = frame;
  skipButton.autoresizingMask =
      (UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleTopMargin);
  [_guidanceInsetView addSubview:skipButton];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(didChangeStatusBarOrientation:)
             name:UIApplicationDidChangeStatusBarOrientationNotification
           object:nil];
}

- (void)viewWillAppear:(BOOL)animated {
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  [super viewWillAppear:animated];

  [UIViewController attemptRotationToDeviceOrientation];
}

- (void)viewWillLayoutSubviews {
  [super viewWillLayoutSubviews];
  [self updateGuidanceInsetViewFrame];
}

- (void)viewDidLayoutSubviews {
  // Per http://stackoverflow.com/a/28226669 video preview layer does not work correctly with
  // auto constraints. It needs to be laid out explicitly.
  CGRect bounds = _preview.bounds;
  _videoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
  _videoPreviewLayer.frame = bounds;
  _videoPreviewLayer.connection.videoOrientation = [self videoOrientation];
}

- (BOOL)prefersStatusBarHidden {
  return YES;
}

- (void)didChangeStatusBarOrientation:(NSNotification *)notification {
  [self.view setNeedsLayout];
}

- (UIInterfaceOrientation)viewOrientation {
  return [UIApplication sharedApplication].statusBarOrientation;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
  return UIInterfaceOrientationMaskAll;
}

- (AVCaptureVideoOrientation)videoOrientation {
  UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
  switch (orientation) {
    case UIInterfaceOrientationPortrait:
      return AVCaptureVideoOrientationPortrait;
    case UIInterfaceOrientationPortraitUpsideDown:
      return AVCaptureVideoOrientationPortraitUpsideDown;
    case UIInterfaceOrientationLandscapeLeft:
      return AVCaptureVideoOrientationLandscapeLeft;
    case UIInterfaceOrientationLandscapeRight:
      return AVCaptureVideoOrientationLandscapeRight;
    default:
      return AVCaptureVideoOrientationPortrait;
  }
}

- (void)skipAndClose {
  [self finishCapture];
  NSData *deviceParams = [CardboardDeviceParamsHelper readSerializedDeviceParams];

  // If no parameter are saved yet, saves Cardboard V1 params by default.
  if (deviceParams.length == 0) {
    [CardboardDeviceParamsHelper saveCardboardV1Params];
  }

  _completion(YES);
}

- (void)finishCapture {
  [_captureSession stopRunning];
  _captureSession = nil;
  [_videoPreviewLayer removeFromSuperlayer];
}

- (void)freezePreview {
  [_captureSession stopRunning];
  _captureSession = nil;
}

- (void)processQrCode:(NSString *)data {
  // Ignore other coming frames;
  _processing = YES;

  // Check whether the QR code is valid.
  if (![data hasPrefix:@"http://"] && ![data hasPrefix:@"https://"]) {
    data = [NSString stringWithFormat:@"%@%@", @"http://", data];
  }
  NSURL *url = [NSURL URLWithString:data];
  if (!url) {
    // Invalid QR code.
    [self showShortMessage:@"Invalid QR Code"];
    _processing = NO;
    return;
  }

  // Resolve QR code via RPC.
  [CardboardDeviceParamsHelper
      resolveAndUpdateViewerProfileFromURL:url
                            withCompletion:^(BOOL success, NSError *error) {
                              if (success) {
                                [self finishCapture];
                                self->_completion(YES);
                              } else {
                                if (error) {
                                  [self showShortMessage:[error localizedDescription]];
                                  self->_processing = NO;
                                } else {
                                  [self showShortMessage:@"Invalid QR Code"];
                                  self->_processing = NO;
                                }
                              }
                            }];
}

- (void)showShortMessage:(NSString *)text {
  if (_showingHUDMessage) {
    return;
  }

  _showingHUDMessage = YES;
  UIAlertController *alert =
      [UIAlertController alertControllerWithTitle:nil
                                          message:text
                                   preferredStyle:UIAlertControllerStyleAlert];
  [self presentViewController:alert animated:YES completion:nil];
  int duration = 1;  // duration in seconds

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, duration * NSEC_PER_SEC),
                 dispatch_get_main_queue(), ^{
                   self->_showingHUDMessage = NO;
                   [alert dismissViewControllerAnimated:YES completion:nil];
                 });
}

- (void)updateGuidanceInsetViewFrame {
  UIEdgeInsets insets = UIEdgeInsetsZero;
  // The guidance view is at the bottom of the screen.
  insets.top = 0;
  _guidanceInsetView.frame = UIEdgeInsetsInsetRect(_guidanceView.bounds, insets);
}

@end
