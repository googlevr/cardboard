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
#import <UIKit/UIKit.h>

/**
 * Defines a delegate protocol for |HelloCardboardOverlayView| class. It's used to respond to the user
 * tapping various controls in the overlay view.
 */
@protocol HelloCardboardOverlayViewDelegate <NSObject>

@optional

/** Called when the user tapped on the GL rendering view. */
- (void)didTapTriggerButton;

/** Called when the user tapped the Back arrow button. */
- (void)didTapBackButton;

/**
 * Called to get a presenting view controller to present the settings dialog. If this is not
 * implemented, the presenting view controller is derived from the applications rootViewController.
 */
- (UIViewController *)presentingViewControllerForSettingsDialog;

/**
 * Called when the setting dialog is presented. Delegates can use this to pause/resume GL rendering.
 */
- (void)didPresentSettingsDialog:(BOOL)presented;

/** Called when the VR viewer is paired by the user. Delegates should refresh the GL rendering. */
- (void)didChangeViewerProfile;

@end

/**
 * Defines a view to display various overlay controls.
 */
@interface HelloCardboardOverlayView : UIView

@property(nonatomic, weak) id<HelloCardboardOverlayViewDelegate> delegate;

@property(nonatomic) BOOL hidesAlignmentMarker;
@property(nonatomic) BOOL hidesBackButton;
@property(nonatomic) BOOL hidesSettingsButton;
@property(nonatomic) BOOL hidesTransitionView;

@end
