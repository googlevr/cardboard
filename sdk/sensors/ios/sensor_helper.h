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
#import <CoreMotion/CoreMotion.h>
#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, SensorHelperType) {
  SensorHelperTypeAccelerometer,
  SensorHelperTypeGyro,
};

// Helper class for running an arbitrary function on the CADisplayLink run loop.
@interface CardboardSensorHelper : NSObject

+ (CardboardSensorHelper*)sharedSensorHelper;

@property(readonly, nonatomic, getter=isAccelerometerAvailable) BOOL accelerometerAvailable;
@property(readonly, nonatomic, getter=isGyroAvailable) BOOL gyroAvailable;

@property(readonly, atomic) CMAccelerometerData* accelerometerData;
@property(readonly, atomic) CMDeviceMotion* deviceMotion;

// Starts the sensor callback.
- (void)start:(SensorHelperType)type callback:(void (^)(void))callback;

// Stops the sensor callback.
- (void)stop:(SensorHelperType)type callback:(void (^)(void))callback;

@end
