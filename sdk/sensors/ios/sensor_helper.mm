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
#import "sensors/ios/sensor_helper.h"

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

// iOS CMMotionManager updates actually happen at one of a set of intervals:
// 10ms, 20ms, 40ms, 80ms, 100ms, and so on, so best to use exactly one of the
// supported update intervals.
// Sample accelerometer and gyro every 10ms.
static const NSTimeInterval kAccelerometerUpdateInterval = 0.01;
static const NSTimeInterval kGyroUpdateInterval = 0.01;

@interface CardboardSensorHelper ()
@property(atomic) CMAccelerometerData *accelerometerData;
@property(atomic) CMDeviceMotion *deviceMotion;
@end

@implementation CardboardSensorHelper {
  CMMotionManager *_motionManager;
  NSOperationQueue *_queue;
  NSMutableSet *_accelerometerCallbacks;
  NSMutableSet *_deviceMotionCallbacks;
  NSLock *_accelerometerLock;
  NSLock *_deviceMotionLock;
}

+ (CardboardSensorHelper *)sharedSensorHelper {
  static CardboardSensorHelper *singleton;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    singleton = [[CardboardSensorHelper alloc] init];
  });
  return singleton;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _motionManager = [[CMMotionManager alloc] init];

    _queue = [[NSOperationQueue alloc] init];
    // Tune these to your performance prefs.
    // maxConcurrent set to anything >1 crashes Cardboard SDK.
    _queue.maxConcurrentOperationCount = 1;
    if ([_queue respondsToSelector:@selector(setQualityOfService:)]) {
      // Use highest quality of service.
      _queue.qualityOfService = NSQualityOfServiceUserInteractive;
    }
    _accelerometerCallbacks = [[NSMutableSet alloc] init];
    _deviceMotionCallbacks = [[NSMutableSet alloc] init];

    _accelerometerLock = [[NSLock alloc] init];
    _deviceMotionLock = [[NSLock alloc] init];
  }
  return self;
}

- (void)start:(SensorHelperType)type callback:(void (^)(void))callback {
  switch (type) {
    case SensorHelperTypeAccelerometer: {
      [self
          invokeBlock:^{
            [self->_accelerometerCallbacks addObject:callback];
          }
             withLock:_accelerometerLock];

      if (_motionManager.isAccelerometerActive) break;

      _motionManager.accelerometerUpdateInterval = kAccelerometerUpdateInterval;
      [_motionManager startAccelerometerUpdatesToQueue:_queue
                                           withHandler:^(CMAccelerometerData *accelerometerData,
                                                         NSError * /*error*/) {
                                             if (self.accelerometerData.timestamp !=
                                                 accelerometerData.timestamp) {
                                               self.accelerometerData = accelerometerData;
                                               [self
                                                   invokeBlock:^{
                                                     for (void (^callback)(void)
                                                              in self->_accelerometerCallbacks) {
                                                       callback();
                                                     }
                                                   }
                                                      withLock:self->_accelerometerLock];
                                             }
                                           }];
    } break;

    case SensorHelperTypeGyro: {
      [self
          invokeBlock:^{
            [self->_deviceMotionCallbacks addObject:callback];
          }
             withLock:_deviceMotionLock];

      if (_motionManager.isDeviceMotionActive) break;

      _motionManager.deviceMotionUpdateInterval = kGyroUpdateInterval;
      [_motionManager
          startDeviceMotionUpdatesToQueue:_queue
                              withHandler:^(CMDeviceMotion *motionData, NSError * /*error*/) {
                                if (self.deviceMotion.timestamp != motionData.timestamp) {
                                  self.deviceMotion = motionData;
                                  [self
                                      invokeBlock:^{
                                        for (void (^callback)(void)
                                                 in self->_deviceMotionCallbacks) {
                                          callback();
                                        }
                                      }
                                         withLock:self->_deviceMotionLock];
                                }
                              }];
    } break;
  }
}

- (void)stop:(SensorHelperType)type callback:(void (^)(void))callback {
  switch (type) {
    case SensorHelperTypeAccelerometer: {
      [self
          invokeBlock:^{
            [self->_accelerometerCallbacks removeObject:callback];
          }
             withLock:_accelerometerLock];
      if (_accelerometerCallbacks.count == 0) {
        [_motionManager stopAccelerometerUpdates];
      }
    } break;

    case SensorHelperTypeGyro: {
      [self
          invokeBlock:^{
            [self->_deviceMotionCallbacks removeObject:callback];
          }
             withLock:_deviceMotionLock];
      if (_deviceMotionCallbacks.count == 0) {
        [_motionManager stopDeviceMotionUpdates];
      }
    } break;
  }
}

- (BOOL)isAccelerometerAvailable {
  return [_motionManager isAccelerometerAvailable];
}

- (BOOL)isGyroAvailable {
  return [_motionManager isDeviceMotionAvailable];
}

- (void)invokeBlock:(void (^)(void))block withLock:(NSLock *)lock {
  [lock lock];
  block();
  [lock unlock];
}

@end
