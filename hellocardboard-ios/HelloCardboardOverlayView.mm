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
#import "HelloCardboardOverlayView.h"

// Content padding for menu-style buttons.
static const UIEdgeInsets kMenuButtonContentPadding = {
    // top, left, bottom, right
    8, 8, 8, 8};

// Padding for the image when it exists in the menu-style button.
static const UIEdgeInsets kMenuButtonImagePadding = {
    // top, left, bottom, right
    0, 0, 0, 8};

// Padding for the title when it exists in the menu-style button.
static const UIEdgeInsets kMenuButtonTitlePadding = {
    // top, left, bottom, right
    0, 8, 0, 16};

// Padding at the bottom of the options menu sheet.
static const CGFloat kMenuSheetBottomPadding = 8;

// DPI for iPhone (default) and iPhone+: http://dpi.lv/
static CGFloat const kDefaultDpi = 326.0f;
static CGFloat const kIPhone6PlusDpi = 401.0f;
static CGFloat const kMetersPerInch = 0.0254f;

const CGFloat kAlignmentMarkerHeight6Plus = (28.0f / (kMetersPerInch * 1000)) * kIPhone6PlusDpi / 3;
const CGFloat kAlignmentMarkerHeight = (28.0f / (kMetersPerInch * 1000)) * kDefaultDpi / 2;

@interface HelloCardboardOverlayView ()

@property(nonatomic) UIView *alignmentMarker;
@property(nonatomic) UIView *overlayInsetView;
@property(nonatomic) UIButton *backButton;
@property(nonatomic) UIView *menuButtonsBackgroundView;
@property(nonatomic) UIView *menuButtonsInsetView;
@property(nonatomic) UIButton *settingsButton;
@property(nonatomic) UIButton *switchButton;
@property(nonatomic) UIView *settingsBackgroundView;

/**
 * Redefine the Designated Initializer to avoid compiler errors.
 **/
- (instancetype)initWithCoder:(NSCoder *)aDecoder;
- (instancetype)initWithFrame:(CGRect)frame;
- (instancetype)initWithFrame:(CGRect)frame
         createTransitionView:(BOOL)createTransitionView NS_DESIGNATED_INITIALIZER;
@end

@implementation HelloCardboardOverlayView

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
  return [self initWithFrame:CGRectZero];
}

- (instancetype)initWithFrame:(CGRect)frame {
  return [self initWithFrame:frame createTransitionView:YES];
}

- (instancetype)initWithFrame:(CGRect)frame createTransitionView:(BOOL)createTransitionView {
  if (self = [super initWithFrame:frame]) {
    CGRect viewBounds = self.bounds;
    UIEdgeInsets insets = [HelloCardboardOverlayView landscapeModeSafeAreaInsets];

    // Alignment marker.
    _alignmentMarker = [[UIView alloc] init];
    _alignmentMarker.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.7];
    [self addSubview:_alignmentMarker];

    _overlayInsetView = [[UIView alloc] init];
    _overlayInsetView.frame = UIEdgeInsetsInsetRect(viewBounds, insets);
    [self addSubview:_overlayInsetView];

    CGRect overlayInsetBounds = _overlayInsetView.bounds;

    // We should always be fullscreen.
    self.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;

    // Settings button.
    UIImage *settingsImage = [UIImage imageNamed:@"HelloCardboard.bundle/ic_settings_white"
                                        inBundle:nil
                   compatibleWithTraitCollection:nil];
    _settingsButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    _settingsButton.tintColor = [UIColor whiteColor];
    [_settingsButton setImage:settingsImage forState:UIControlStateNormal];
    [_settingsButton addTarget:self
                        action:@selector(didTapSettingsButton:)
              forControlEvents:UIControlEventTouchUpInside];
    [_settingsButton sizeToFit];
    CGRect bounds = _settingsButton.bounds;
    _settingsButton.frame = CGRectMake(CGRectGetWidth(overlayInsetBounds) - CGRectGetWidth(bounds),
                                       0, CGRectGetWidth(bounds), CGRectGetHeight(bounds));
    _settingsButton.autoresizingMask =
        UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    [_overlayInsetView addSubview:_settingsButton];

    // Back button.
    _backButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    _backButton.tintColor = [UIColor whiteColor];
    _backButton.autoresizingMask =
        UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    [_backButton setImage:[UIImage imageNamed:@"HelloCardboard.bundle/ic_arrow_back_ios_white"
                                                   inBundle:nil
                              compatibleWithTraitCollection:nil]
                 forState:UIControlStateNormal];
    [_backButton addTarget:self
                    action:@selector(didTapBackButton:)
          forControlEvents:UIControlEventTouchUpInside];
    [_backButton sizeToFit];
    [_overlayInsetView addSubview:_backButton];

    // Settings background view.
    _settingsBackgroundView = [[UIView alloc] initWithFrame:frame];
    _settingsBackgroundView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.65];
    _settingsBackgroundView.autoresizingMask =
        UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    _settingsBackgroundView.hidden = YES;
    [self addSubview:_settingsBackgroundView];

    // Add tap gesture to dismiss the settings view.
    UITapGestureRecognizer *tapGesture =
        [[UITapGestureRecognizer alloc] initWithTarget:self
                                                action:@selector(didTapSettingsBackgroundView:)];
    [_settingsBackgroundView addGestureRecognizer:tapGesture];

    // Switch menu button.
    UIImage *cardboardImage = [UIImage imageNamed:@"HelloCardboard.bundle/ic_cardboard"
                                         inBundle:nil
                    compatibleWithTraitCollection:nil];
    NSString *title = @"Switch Cardboard viewer";
    _switchButton = [self menuButtonWithTitle:title image:cardboardImage placedBelowView:nil];
    [_switchButton addTarget:self
                      action:@selector(didTapSwitchButton:)
            forControlEvents:UIControlEventTouchUpInside];

    // Button background view.
    _menuButtonsBackgroundView = [[UIView alloc] init];
    _menuButtonsBackgroundView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.65];
    CGFloat backgroundHeight = CGRectGetHeight(_switchButton.frame) + kMenuSheetBottomPadding;
    _menuButtonsBackgroundView.frame = CGRectMake(0, CGRectGetHeight(viewBounds) - backgroundHeight,
                                                  CGRectGetWidth(viewBounds), backgroundHeight);
    _menuButtonsBackgroundView.autoresizingMask =
        UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleWidth;
    [_settingsBackgroundView addSubview:_menuButtonsBackgroundView];

    _menuButtonsInsetView = [[UIView alloc] init];
    [self updateMenuButtonsInsetViewFrame];
    [_menuButtonsBackgroundView addSubview:_menuButtonsInsetView];

    [_menuButtonsInsetView addSubview:_switchButton];
  }
  self.accessibilityIdentifier = @"overlay_view";
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];

  CGRect viewBounds = self.bounds;
  UIEdgeInsets insets = [HelloCardboardOverlayView landscapeModeSafeAreaInsets];
  _overlayInsetView.frame = UIEdgeInsetsInsetRect(viewBounds, insets);

  [self updateMenuButtonsInsetViewFrame];

  // Position the alignment marker at the bottom of the view, but centered horizontally.
  CGFloat alignmentMarkerHeight = [HelloCardboardOverlayView alignmentMarkerHeight];

  _alignmentMarker.frame =
      CGRectMake(CGRectGetMidX(viewBounds) - 1,
                 CGRectGetHeight(viewBounds) - alignmentMarkerHeight - insets.bottom, 2,
                 alignmentMarkerHeight);
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  UIView *view = [super hitTest:point withEvent:event];
  if (view == self || view == _overlayInsetView) {
    return nil;
  }
  return view;
}

- (void)showSettingsDialog {
  [self didTapSettingsButton:nil];
}

- (void)didTapBackButton:(id)sender {
  if ([self.delegate respondsToSelector:@selector(didTapBackButton)]) {
    [self.delegate didTapBackButton];
  }
}

- (void)didTapSettingsButton:(id)sender {
  self.settingsBackgroundView.hidden = NO;
}

- (void)didTapSettingsBackgroundView:(id)sender {
  self.settingsBackgroundView.hidden = YES;
}

- (void)didTapSwitchButton:(id)sender {
  if ([self.delegate respondsToSelector:@selector(didTapBackButton)]) {
    [self.delegate didChangeViewerProfile];
  }
  self.settingsBackgroundView.hidden = YES;
}

- (void)didTapBackButton {
  [self didTapBackButton:nil];
}

- (UIButton *)menuButtonWithTitle:(NSString *)title
                            image:(UIImage *)image
                  placedBelowView:(UIView *)aboveView {
  UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
  button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
  button.imageEdgeInsets = kMenuButtonImagePadding;
  button.titleEdgeInsets = kMenuButtonTitlePadding;
  button.contentEdgeInsets = kMenuButtonContentPadding;
  [button setTitle:title forState:UIControlStateNormal];
  [button setImage:image forState:UIControlStateNormal];
  [button sizeToFit];
  button.frame = CGRectMake(0, CGRectGetMaxY(aboveView.frame), CGRectGetWidth(self.bounds),
                            CGRectGetHeight(button.bounds));
  button.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleWidth;
  return button;
}

- (void)updateMenuButtonsInsetViewFrame {
  UIEdgeInsets menuButtonsInsets = [HelloCardboardOverlayView landscapeModeSafeAreaInsets];
  // The menu buttons are a menu at the bottom of the screen.
  menuButtonsInsets.top = 0;
  _menuButtonsInsetView.frame =
      UIEdgeInsetsInsetRect(_menuButtonsBackgroundView.bounds, menuButtonsInsets);
}

+ (NSString *)appName {
  // This set of fallbacks are specified in Apple Technical Q&A QA1544, "Obtaining the localized
  // application name in Cocoa".
  NSDictionary *localizedInfoDictionary = NSBundle.mainBundle.localizedInfoDictionary;
  NSString *bundleDisplayName = localizedInfoDictionary[@"CFBundleDisplayName"];
  if (bundleDisplayName) {
    return bundleDisplayName;
  }
  NSString *bundleName = localizedInfoDictionary[(NSString *)kCFBundleNameKey];
  if (bundleName) {
    return bundleName;
  }
  return NSProcessInfo.processInfo.processName;
}

+ (UIEdgeInsets)landscapeModeSafeAreaInsets {
  return UIApplication.sharedApplication.keyWindow.safeAreaInsets;
}

+ (CGFloat)alignmentMarkerHeight {
  bool iphone6plus = ([[UIScreen mainScreen] scale] > 2);
  CGFloat height = (iphone6plus ? kAlignmentMarkerHeight6Plus : kAlignmentMarkerHeight);
  return height;
}

@end
