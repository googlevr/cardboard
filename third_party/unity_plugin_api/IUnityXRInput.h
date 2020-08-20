// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#if UNITY
#   include "Modules/XR/ProviderInterface/UnityXRTypes.h"
#   include "Modules/XR/ProviderInterface/UnityXRSubsystemTypes.h"
#else
#   include "UnityXRTypes.h"
#   include "UnityXRSubsystemTypes.h"
#   include <stdint.h>
#   include <stdbool.h>
#endif

/// @file IUnityXRInput.h
/// @brief XR interface for input devices and associated structures
/// @see IUnityXRInputInterface

#if !defined(UINT32_MAX)
/// This value is the max value of an unsigned 32-bit integer if not supplied by existing includes.
#  define UINT32_MAX  4294967295UL
#endif

#if !defined(UINT64_MAX)
/// This value is the max value of an unsigned 64-bit integer if not supplied by existing includes.
#  define UINT64_MAX  18446744073709551615ULL
#endif

/// This structure describes what an Input Device is capable of. The provider fills in this structure when Unity requests it by calling UnityXRInputProvider::FillDeviceDefinition. Use functions prefixed with DeviceDefinition_ on IUnityXRInputInterface to modify.
typedef struct UnityXRInputDeviceDefinition UnityXRInputDeviceDefinition;

/// This structure stores the current state of all features on the input device. The provider fills in this structure when Unity requests it by calling UnityXRInputProvider::UpdateDeviceState. Use functions prefixed with DeviceState_ on IUnityXRInputInterface to modify.
typedef struct UnityXRInputDeviceState UnityXRInputDeviceState;

/// This identifier is a unique identifier for a connected device.  Unity and the Provider use these identifiers to identify which device. This identifier must be unique to each connected device reported by the provider.
typedef uint32_t UnityXRInternalInputDeviceId;

/// The index of a given input feature, returned when adding features to a definition. The provider uses these indices to with the device state to reference individual features.  These are sequential indices, incremented by one for each feature added.
typedef uint32_t UnityXRInputFeatureIndex;

/// Unity uses this constant when failing to add or reference a specific feature.
const UnityXRInputFeatureIndex kUnityInvalidXRInputFeatureIndex = UINT32_MAX;

/// An invalid device Id.  The provider uses this when wanting to request information that is provider wide in UnityXRInputProvider::HandleEvent, or Unity cannot find a device.
const UnityXRInternalInputDeviceId kUnityInvalidXRInternalInputDeviceId = UINT32_MAX;

/// The maximum size a feature of type kUnityXRInputFeatureTypeCustom can be, in bytes.
const int32_t kUnityMaxCustomInputFeatureSize = 128;

/// The maximum size an event of type kUnityXRInputEventTypeHapticSendBuffer can be.  The provider cannot declare UnityXRHapticCapabilities::bufferMaxSize greater than this, and the provider will not recieve a UnityXRHapticUpdate greater than this either.
const int32_t kUnityXRMaxHapticBufferSize = 4096;

/// The UnityXRHand::fingerBoneIndices first dimension accessors, representing individual fingers in a hand.
enum UnityXRHandFinger
{
    UnityXRFingerThumb,
    UnityXRFingerIndex,
    UnityXRFingerMiddle,
    UnityXRFingerRing,
    UnityXRFingerPinky,
    UnityXRFingerCount
};

/// The maximum number of joints for each finger. This is the second dimension of the UnityXRHand::fingerBonesIndices array.
enum { kUnityXRMaxFingerBoneCount = 5 };

/// Currently supported Input feature types.  Each type represents a unique type of data coming from a device
typedef enum UnityXRInputFeatureType
{
    /// Data in the form of a byte array. Set with DeviceState_SetCustomValue.
    kUnityXRInputFeatureTypeCustom = 0,
    /// Data in the form of a boolean. Set with DeviceState_SetBinaryValue.
    kUnityXRInputFeatureTypeBinary,
    /// Data in the form of an unsigned 32-bit integer. Set with DeviceState_SetDiscreteStateValue.
    kUnityXRInputFeatureTypeDiscreteStates,
    /// Data in the form of a 32-bit float. Set with DeviceState_SetAxis1DValue.
    kUnityXRInputFeatureTypeAxis1D,
    /// Data in the form a UnityXRVector2. Set with DeviceState_SetAxis1DValue.
    kUnityXRInputFeatureTypeAxis2D,
    /// Data in the form a UnityXRVector3. Set with DeviceState_SetAxis1DValue.
    kUnityXRInputFeatureTypeAxis3D,
    /// Data in the form a UnityXRVector4. Set with DeviceState_SetRotationValue.
    kUnityXRInputFeatureTypeRotation,
    /// Data in the form a UnityXRHand. Set with DeviceState_SetHandValue.
    kUnityXRInputFeatureTypeHand,
    /// Data in the form a UnityXRBone. Set with DeviceState_SetBoneValue.
    kUnityXRInputFeatureTypeBone,
    /// Data in the form a UnityXREyes. Set with DeviceState_SetEyesValue.
    kUnityXRInputFeatureTypeEyes,

    /// An Invalid feature type. Unity uses this when no data is available.
    kUnityXRInputFeatureTypeInvalid = UINT32_MAX,
} UnityXRInputFeatureType;

/// Combinative set of flags to help Unity identify the type of the device.
typedef enum UnityXRInputDeviceCharacteristics
{
    kUnityXRInputDeviceCharacteristicsNone = 0,
    /// This is attached to the head. e.g. goggles, glasses, or HMD
    kUnityXRInputDeviceCharacteristicsHeadMounted = 1 << 0,
    /// This has an available camera
    kUnityXRInputDeviceCharacteristicsCamera = 1 << 1,
    /// A device that is intended to be held in a hand
    kUnityXRInputDeviceCharacteristicsHeldInHand = 1 << 2,
    /// Contains detected hands data
    kUnityXRInputDeviceCharacteristicsHandTracking = 1 << 3,
    /// Contains eye tracking information
    kUnityXRInputDeviceCharacteristicsEyeTracking = 1 << 4,
    /// Able to be tracked in space, either with 3DOF or 6DOF tracking
    kUnityXRInputDeviceCharacteristicsTrackedDevice = 1 << 5,
    /// Has buttons and axes that let it act as a game controller
    kUnityXRInputDeviceCharacteristicsController = 1 << 6,
    /// Static sensor device to aid in tracking of other devices
    kUnityXRInputDeviceCharacteristicsTrackingReference = 1 << 7,
    /// Assigned to the left, such as a left hand sensor or left hand controller
    kUnityXRInputDeviceCharacteristicsLeft = 1 << 8,
    /// Assigned to the right, such as a right hand sensor or right hand controller
    kUnityXRInputDeviceCharacteristicsRight = 1 << 9,
    /// 3DOF sensors, but programmatically makes assumptions about the possible position of the device to simulate 3D positional tracking.
    kUnityXRInputDeviceCharacteristicsSimulated6DOF = 1 << 10
} UnityXRInputDeviceCharacteristics;

/// Contextual information about individual features.  These help Unity and developers understand how your features should be used.
typedef const char* UnityXRInputFeatureUsage;

// Pose & Tracking

/// A binary feature that represents if the device is currently tracking properly.  True means fully tracked, false means either partially or not tracked.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageIsTracked = "IsTracked";
/// A discrete state feature, backed by the UnityXRInputTrackingStateFlags enumeration identifying which actual tracking features are currently available and updating.  This value must never be above kUnityXRInputTrackingStateAll.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageTrackingState = "TrackingState";

/// A 3D Axis feature that represents the device's position in 3D space, in meters.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStatePosition flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageDevicePosition = "DevicePosition";
/// A Rotation feature that represents the device's rotation in 3D space, as a quaternion in radians.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateRotation flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageDeviceRotation = "DeviceRotation";
/// A 3D axis feature that represents a device's velocity in meters per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageDeviceVelocity = "DeviceVelocity";
/// A 3D axis feature that represents a device's angular velocity in radians per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageDeviceAngularVelocity = "DeviceAngularVelocity";
/// A 3D axis feature that represents a device's acceleration in meters per second per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageDeviceAcceleration = "DeviceAcceleration";
/// A 3D axis feature that represents a device's angular acceleration in radians per second per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageDeviceAngularAcceleration = "DeviceAngularAcceleration";

/// A 3D Axis feature that represents the left eye's or left eye display's position in 3D space, in meters.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStatePosition flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLeftEyePosition = "LeftEyePosition";
/// A Rotation feature that represents the left eye's or left eye display's rotation in 3D space, as a quaternion in radians.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateRotation flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLeftEyeRotation = "LeftEyeRotation";
/// A 3D axis feature that represents a left eye's or left eye display's velocity in meters per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLeftEyeVelocity = "LeftEyeVelocity";
/// A 3D axis feature that represents a left eye's or left eye display's angular velocity in radians per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLeftEyeAngularVelocity = "LeftEyeAngularVelocity";
/// A 3D axis feature that represents a left eye's or left eye display's acceleration in meters per second per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLeftEyeAcceleration = "LeftEyeAcceleration";
/// A 3D axis feature that represents a left eye's or left eye display's angular acceleration in radians per second per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLeftEyeAngularAcceleration = "LeftEyeAngularAcceleration";

/// A 3D Axis feature that represents the right eye's or right eye display's position in 3D space, in meters.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStatePosition flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageRightEyePosition = "RightEyePosition";
/// A Rotation feature that represents the right eye's or right eye display's rotation in 3D space, as a quaternion in radians.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateRotation flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageRightEyeRotation = "RightEyeRotation";
/// A 3D axis feature that represents a right eye's or right eye display's velocity in meters per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageRightEyeVelocity = "RightEyeVelocity";
/// A 3D axis feature that represents a right eye's or right eye display's angular velocity in radians per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageRightEyeAngularVelocity = "RightEyeAngularVelocity";
/// A 3D axis feature that represents a right eye's or right eye display's acceleration in meters per second per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageRightEyeAcceleration = "RightEyeAcceleration";
/// A 3D axis feature that represents a right eye's or right eye display's acceleration in radians per second per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageRightEyeAngularAcceleration = "RightEyeAngularAcceleration";

/// A 3D Axis feature that represents the center eye's or 'between the 2 eyes' position in 3D space, in meters.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStatePosition flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageCenterEyePosition = "CenterEyePosition";
/// A Rotation feature that represents the center eye's or 'between the 2 eyes' rotation in 3D space, as a quaternion in radians.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateRotation flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageCenterEyeRotation = "CenterEyeRotation";
/// A 3D axis feature that represents a center eye's or 'between the 2 eyes' velocity in meters per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageCenterEyeVelocity = "CenterEyeVelocity";
/// A 3D axis feature that represents a center eye's or 'between the 2 eyes' angular velocity in radians per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageCenterEyeAngularVelocity = "CenterEyeAngularVelocity";
/// A 3D axis feature that represents a center eye's or 'between the 2 eyes' acceleration in meters per second per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageCenterEyeAcceleration = "CenterEyeAcceleration";
/// A 3D axis feature that represents a center eye's or 'between the 2 eyes' acceleration in radians per second per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageCenterEyeAngularAcceleration = "CenterEyeAngularAcceleration";

/// A 3D Axis feature that represents the camera's position in 3D space, in meters.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStatePosition flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageColorCameraPosition = "CameraPosition";
/// A Rotation feature that represents the camera's rotation in 3D space, as a quaternion in radians.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateRotation flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageColorCameraRotation = "CameraRotation";
/// A 3D axis feature that represents a camera's velocity in meters per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageColorCameraVelocity = "CameraVelocity";
/// A 3D axis feature that represents a camera's angular velocity in radians per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularVelocity flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageColorCameraAngularVelocity = "CameraAngularVelocity";
/// A 3D axis feature that represents a camera's acceleration in meters per second per second.  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageColorCameraAcceleration = "CameraAcceleration";
/// A 3D axis feature that represents a camera's acceleration in radians per second per second, as euler angles (yaw, pitch, roll).  Should be updated if the TrackingState usage contains the kUnityXRInputTrackingStateAngularAcceleration flag.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageColorCameraAngularAcceleration = "CameraAngularAcceleration";

// Device Information

/// A 1D axis feature that represents the current battery level of the device, where 0 is no battery, and 1 is fully charged.  It must always be within the [0-1] range.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageBatteryLevel = "BatteryLevel";
/// A boolean that returns true when a user is currently wearing the headset
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageUserPresence = "UserPresence";

// Controller Axes

/// A 2D axis feature that represents a touchpad or joystick.  This must always be within the range [-1,1],[-1,1]. where 0,0 is the idle position, and Y-positive is away from the controller user.  This maps to input axes (1, 2) if the device's role is kUnityXRInputDeviceRoleLeftHanded, and (4, 5) if the role is kUnityXRInputDeviceRoleRightHanded, for the (X, Y) elements of the 2D axis.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsagePrimary2DAxis = "Primary2DAxis";
/// A 1D axis that maps to an index-actuated trigger.  This must always be within the range of [0,1] where 0 is open and 1 is fully squeezed.  This maps to input axis 8 if the device's role is kUnityXRInputDeviceRoleLeftHanded, and axis 9 if the role is kUnityXRInputDeviceRoleRightHanded.  If this is implemented, the device must also implement kUnityXRInputFeatureUsageTriggerButton.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageTrigger = "Trigger";
/// A 1D axis that maps to a hand squeeze activated grip.  This must always be within the range of [0,1] where 0 is open and 1 is fully squeezed.  This maps to input axis 10 if the device's role is kUnityXRInputDeviceRoleLeftHanded, and axis 11 if the role is kUnityXRInputDeviceRoleRightHanded. If this is implemented, the device must also implement kUnityXRInputFeatureUsageGripButton.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageGrip = "Grip";
/// A 2D axis representing a second joystick or touchpad, used in addition to kUnityXRInputFeatureUsagePrimary2DAxis.  This must always be within the range [-1,1],[-1,1]. where 0,0 is the idle position, a Y-positive is away from the controller user.  This maps to input axes (16, 17) if the device's role is kUnityXRInputDeviceRoleLeftHanded, and (18, 19) if the role is kUnityXRInputDeviceRoleRightHanded, for the (X, Y) elements of the 2D axis.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageSecondary2DAxis = "Secondary2DAxis";

// Controller Buttons

/// A binary feature representing the primary button on a controller.  This would commonly be used as an accept or advance button in menus.  If this is actuated, then kUnityXRInputFeatureUsagePrimaryTouch must be actuated as well, if it exists.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsagePrimaryButton = "PrimaryButton";
/// A binary feature representing the touch state of a primary button on the controller.  If this is implemented, the device must implement kUnityXRInputFeatureUsagePrimaryButton.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsagePrimaryTouch = "PrimaryTouch";
/// A binary feature representing the secondary button on a controller.  This would commonly be used as a back or alternate button.  If this is actuated, then kUnityXRInputFeatureUsageSecondaryTouch must be actuated as well if it exists.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageSecondaryButton = "SecondaryButton";
/// A binary feature representing the touch state of a secondary button on the controller.  If this is implemented, the device must implement kUnityXRInputFeatureUsageSecondaryButton.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageSecondaryTouch = "SecondaryTouch";
/// A binary feature representing whether a hand-actuated squeeze is triggered.  This maps to input manager button 4 if the device's role is kUnityXRInputDeviceRoleLeftHanded, and 5 if the role is kUnityXRInputDeviceRoleRightHanded.  If this is implemented, the device must also implement kUnityXRInputFeatureUsageGrip.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageGripButton = "GripButton";
/// A binary feature representing whether a hand-actuated squeeze is triggered.  This maps to input manager button 14 if the device's role is kUnityXRInputDeviceRoleLeftHanded, and 15 if the role is kUnityXRInputDeviceRoleRightHanded.  If this is implemented, the device must also implement kUnityXRInputFeatureUsageTrigger.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageTriggerButton = "TriggerButton";
/// A binary feature representing a non-gameplay pause or menu button.  This is normally not in easy, common reach.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageMenuButton = "MenuButton";
/// A binary feature representing a depression or click of the kUnityXRInputFeatureUsagePrimary2DAxis 2D axis.  If this is implemented, the device must implement kUnityXRInputFeatureUsagePrimary2DAxis.  If this is actuated, then kUnityXRInputFeatureUsagePrimary2DAxisTouch must be actuated as well, if it exists.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsagePrimary2DAxisClick = "Primary2DAxisClick";
/// A binary feature representing a light touch of the kUnityXRInputFeatureUsagePrimary2DAxis 2D axis.  If this is implemented, the device must implement kUnityXRInputFeatureUsagePrimary2DAxis.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsagePrimary2DAxisTouch = "Primary2DAxisTouch";
/// A binary feature representing a depression or click of the kUnityXRInputFeatureUsageSecondary2DAxis 2D axis.  If this is implemented, the device must implement kUnityXRInputFeatureUsageSecondary2DAxis.  If this is actuated, then kUnityXRInputFeatureUsageSecondary2DAxisTouch must be actuated as well, if it exists.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageSecondary2DAxisClick = "Secondary2DAxisClick";
/// A binary feature representing a light touch of the kUnityXRInputFeatureUsageSecondary2DAxis 2D axis.  If this is implemented, the device must implement kUnityXRInputFeatureUsageSecondary2DAxis.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageSecondary2DAxisTouch = "Secondary2DAxisTouch";

// Direct Legacy Buttons

/// A binary button mapped directly to button index 0 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton0 = "ButtonId0";
/// A binary button mapped directly to button index 1 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton1 = "ButtonId1";
/// A binary button mapped directly to button index 2 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton2 = "ButtonId2";
/// A binary button mapped directly to button index 3 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton3 = "ButtonId3";
/// A binary button mapped directly to button index 4 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton4 = "ButtonId4";
/// A binary button mapped directly to button index 5 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton5 = "ButtonId5";
/// A binary button mapped directly to button index 6 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton6 = "ButtonId6";
/// A binary button mapped directly to button index 7 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton7 = "ButtonId7";
/// A binary button mapped directly to button index 8 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton8 = "ButtonId8";
/// A binary button mapped directly to button index 9 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton9 = "ButtonId9";
/// A binary button mapped directly to button index 10 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton10 = "ButtonId10";
/// A binary button mapped directly to button index 11 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton11 = "ButtonId11";
/// A binary button mapped directly to button index 12 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton12 = "ButtonId12";
/// A binary button mapped directly to button index 13 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton13 = "ButtonId13";
/// A binary button mapped directly to button index 14 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton14 = "ButtonId14";
/// A binary button mapped directly to button index 15 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton15 = "ButtonId15";
/// A binary button mapped directly to button index 16 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton16 = "ButtonId16";
/// A binary button mapped directly to button index 17 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton17 = "ButtonId17";
/// A binary button mapped directly to button index 18 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton18 = "ButtonId18";
/// A binary button mapped directly to button index 19 in UnityEngine.Input.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyButton19 = "ButtonId19";

// Direct legacy axes

/// A 1D axis, that maps directly to axis index 1 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis1 = "AxisId1";
/// A 1D axis, that maps directly to axis index 2 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis2 = "AxisId2";
/// A 1D axis, that maps directly to axis index 3 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis3 = "AxisId3";
/// A 1D axis, that maps directly to axis index 4 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis4 = "AxisId4";
/// A 1D axis, that maps directly to axis index 5 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis5 = "AxisId5";
/// A 1D axis, that maps directly to axis index 6 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis6 = "AxisId6";
/// A 1D axis, that maps directly to axis index 7 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis7 = "AxisId7";
/// A 1D axis, that maps directly to axis index 8 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis8 = "AxisId8";
/// A 1D axis, that maps directly to axis index 9 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis9 = "AxisId9";
/// A 1D axis, that maps directly to axis index 10 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis10 = "AxisId10";
/// A 1D axis, that maps directly to axis index 11 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis11 = "AxisId11";
/// A 1D axis, that maps directly to axis index 12 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis12 = "AxisId12";
/// A 1D axis, that maps directly to axis index 13 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis13 = "AxisId13";
/// A 1D axis, that maps directly to axis index 14 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis14 = "AxisId14";
/// A 1D axis, that maps directly to axis index 15 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis15 = "AxisId15";
/// A 1D axis, that maps directly to axis index 16 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis16 = "AxisId16";
/// A 1D axis, that maps directly to axis index 17 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis17 = "AxisId17";
/// A 1D axis, that maps directly to axis index 18 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis18 = "AxisId18";
/// A 1D axis, that maps directly to axis index 19 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis19 = "AxisId19";
/// A 1D axis, that maps directly to axis index 20 in UnityEngine.Input.  This must be in the range [-1,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis20 = "AxisId20";
/// A 1D axis, that maps directly to axis index 21 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis21 = "AxisId21";
/// A 1D axis, that maps directly to axis index 22 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis22 = "AxisId22";
/// A 1D axis, that maps directly to axis index 23 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis23 = "AxisId23";
/// A 1D axis, that maps directly to axis index 24 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis24 = "AxisId24";
/// A 1D axis, that maps directly to axis index 25 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis25 = "AxisId25";
/// A 1D axis, that maps directly to axis index 26 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis26 = "AxisId26";
/// A 1D axis, that maps directly to axis index 27 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis27 = "AxisId27";
/// A 1D axis, that maps directly to axis index 28 in UnityEngine.Input.  This must be in the range [0,1].
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageLegacyAxis28 = "AxisId28";

/// A UnityXRHands feature, representing hand tracking data.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageHandData = "HandData";
/// A UnityXREyes feature, representing eye tracking data.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageEyesData = "EyesData";

// Usages being phased out
// These map to legacy axes, but are features that exist on only 0 or 1 devices, and are not common.

/// A 2D axis that maps to a non-handed touchpad or joystick.  This must always be within the range [-1,1],[-1,1]. where 0,0 is the idle position, and Y-positive is away from the controller user.  This maps to input axes (6, 7) for the (X, Y) elements of the 2D axis.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageDPad = "DPad";
/// A 1D Axis resting position for the thumb that contains capacitive touch.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageThumbrest = "Thumbrest";
/// An Index finger actuated capacitive touch.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageIndexTouch = "IndexTouch";
/// A Thumb actuated capacitive touch.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageThumbTouch = "ThumbTouch";
/// A 1D axis representing the bend amount of an index finger, where 0 is fully extended, and 1 is fully curled or gripped.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageIndexFinger = "IndexFinger";
/// A 1D axis representing the bend amount of an middle finger, where 0 is fully extended, and 1 is fully curled or gripped.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageMiddleFinger = "MiddleFinger";
/// A 1D axis representing the bend amount of an ring finger, where 0 is fully extended, and 1 is fully curled or gripped.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsageRingFinger = "RingFinger";
/// A 1D axis representing the bend amount of an pinky finger, where 0 is fully extended, and 1 is fully curled or gripped.
UnityXRInputFeatureUsage const kUnityXRInputFeatureUsagePinkyFinger = "PinkyFinger";

/// The kUnityXRInputFeatureUsageTrackingState usage uses these flags to inform Unity and developers what kind of tracking data is currently available. If an individual flag is raised, then all reported features of that type must be currently updating.
typedef enum UnityXRInputTrackingStateFlags
{
    /// No tracking data is available.
    kUnityXRInputTrackingStateNone = 0,
    /// Positional tracking data is available and being updated. This includes: kUnityXRInputFeatureUsageDevicePosition, kUnityXRInputFeatureUsageLeftEyePosition, kUnityXRInputFeatureUsageRightEyePosition, kUnityXRInputFeatureUsageCenterEyePosition, and kUnityXRInputFeatureUsageColorCameraPosition.
    kUnityXRInputTrackingStatePosition = 1 << 0,
    /// Rotational tracking data is available and being updated. This includes: kUnityXRInputFeatureUsageDeviceRotation, kUnityXRInputFeatureUsageLeftEyeRotation, kUnityXRInputFeatureUsageRightEyeRotation, kUnityXRInputFeatureUsageCenterEyeRotation, and kUnityXRInputFeatureUsageColorCameraRotation.
    kUnityXRInputTrackingStateRotation = 1 << 1,
    /// Velocity tracking data is available and being updated. This includes: kUnityXRInputFeatureUsageDeviceVelocity, kUnityXRInputFeatureUsageLeftEyeVelocity, kUnityXRInputFeatureUsageRightEyeVelocity, kUnityXRInputFeatureUsageCenterEyeVelocity, and kUnityXRInputFeatureUsageColorCameraVelocity.
    kUnityXRInputTrackingStateVelocity = 1 << 2,
    /// Angular velocity tracking data is available and being updated. This includes: kUnityXRInputFeatureUsageDeviceAngularVelocity, kUnityXRInputFeatureUsageLeftEyeAngularVelocity, kUnityXRInputFeatureUsageRightEyeAngularVelocity, kUnityXRInputFeatureUsageCenterEyeAngularVelocity, and kUnityXRInputFeatureUsageColorCameraAngularVelocity.
    kUnityXRInputTrackingStateAngularVelocity = 1 << 3,
    /// Acceleration tracking data is available and being updated. This includes: kUnityXRInputFeatureUsageDeviceAcceleration, kUnityXRInputFeatureUsageLeftEyeAcceleration, kUnityXRInputFeatureUsageRightEyeAcceleration, kUnityXRInputFeatureUsageCenterEyeAcceleration, and kUnityXRInputFeatureUsageColorCameraAcceleration.
    kUnityXRInputTrackingStateAcceleration = 1 << 4,
    /// Angular acceleration tracking data is available and being updated. This includes: kUnityXRInputFeatureUsageDeviceAngularAcceleration, kUnityXRInputFeatureUsageLeftEyeAngularAcceleration, kUnityXRInputFeatureUsageRightEyeAngularAcceleration, kUnityXRInputFeatureUsageCenterEyeAcceleration, and kUnityXRInputFeatureUsageColorCameraAngularAcceleration.
    kUnityXRInputTrackingStateAngularAcceleration = 1 << 5,

    /// All types of reported tracking data are currently updating.
    kUnityXRInputTrackingStateAll = (1 << 6) - 1
} UnityXRInputTrackingStateFlags;

/// UnityXRInputProvider::UpdateDeviceState uses this to inform the provider which kind of update is being requested.
typedef enum UnityXRInputUpdateType
{
    /// Unity calls the provider with this type of update before it iterates over MonoBehaviour::Update calls and coroutine continuations; these should represent where the device current is.
    kUnityXRInputUpdateTypeDynamic = 0,
    /// Unity calls the provider with this type of update right before we prepare to render to the headset, and just before Application.OnBeforeRender is invoked.  These can often use a forward predicted tracking position, and should represent where you'd like to render the scene given the time it will take before that gets displayed to the headset.
    kUnityXRInputUpdateTypeBeforeRender,
} UnityXRInputUpdateType;

/// This provides context for where the provider has placed the origin point (the point with coordinates (0,0,0)) in the real world. Determines where reported tracking values are relative to.
typedef enum UnityXRInputTrackingOriginModeFlags
{
    /// An unknown tracking origin.  Either not yet set, or the result of an error.
    kUnityXRInputTrackingOriginModeUnknown = 0,
    /// Represents the tracking origin as determined by the first location tracking was reported for the primary device.
    kUnityXRInputTrackingOriginModeDevice = 1,
    /// Represents a tracking origin at the place where the provider has determined where the floor is. Tracked devices are automatically offset by the distance from the origin to the primary device.
    kUnityXRInputTrackingOriginModeFloor = 2,
    /// Represents a tracking origin relative to a reported input device with the UnityXRInputDeviceCapabilies flag of kUnityXRInputDeviceCharacteristicsTrackingReference.
    kUnityXRInputTrackingOriginModeTrackingReference = 4
} UnityXRInputTrackingOriginModeFlags;

/// This represents what you can change on the device definition without disconnecting the device.
/// If the provider calls UnityXRInputInterface::InputSubsystem_DeviceConfigChanged, Unity will call back with IUnityXRInputProvider::UpdateDeviceConfig and supply a config for you to modify.  Developers will be notified through the InputDevices.DeviceConfigChanged event.
typedef struct UnityXRInputDeviceConfig
{
    /// The characteristics of the device, identifying how the device is used and what it can report.
    UnityXRInputDeviceCharacteristics characteristics;
} UnityXRInputDeviceConfig;

/// Unity sends this to the provider when it needs to know the haptic capabilities of a specific device.
/// UnityXRInputProvider::HandleEvent's buffer parameter points to an object of this type when the event type is kUnityXRInputEventTypeHapticGetCapabilities.
/// The provider fills in this structure for Unity to process.
typedef struct UnityXRHapticCapabilities
{
    /// The number of haptic channels available on this device.
    unsigned int numChannels;
    /// Tells Unity that this device supports the kUnityXRInputEventTypeHapticSendImpulse event.
    bool supportsImpulse;
    /// Tells Unity that this device supports the kUnityXRInputEventTypeHapticSendBuffer event.
    bool supportsBuffer;
    /// The buffer frequency the device operates at in Hertz.  This value must be greater than 0 if supportsBuffer is true, and 0 otherwise.  This tells Unity how fast the device consumes buffered haptic data.
    unsigned int bufferFrequencyHz;
    /// The max amount of buffer data that can be stored provider-side.
    unsigned int bufferMaxSize;
    /// The optimal size of a device's buffer, taking into account frequency and latency.
    unsigned int bufferOptimalSize;
} UnityXRHapticCapabilities;

/// A simple struct representing a bone with possible connections to other bones (with feature index)
typedef struct UnityXRBone
{
    /// Feature index of parent bone (kUnityInvalidXRInputFeatureIndex indicates bone is root and has no parent)
    UnityXRInputFeatureIndex parentBoneIndex;

    /// Positions for the bone's joint
    UnityXRVector3 position;

    /// Rotation of the Bone joint
    UnityXRVector4 rotation;
} UnityXRBone;

/// A simple struct representing a hand with possible connections to other bones (with feature index)
typedef struct UnityXRHand
{
	/// Feature index of root bone of hand (kUnityInvalidXRInputFeatureIndex indicates no root)
    UnityXRInputFeatureIndex rootBoneIndex;

    /// Feature indices of finger bones (kUnityInvalidXRInputFeatureIndex indicates that bone does not exist)
    UnityXRInputFeatureIndex fingerBonesIndices[UnityXRFingerCount][kUnityXRMaxFingerBoneCount];
} UnityXRHand;

/// A simple struct representing a set of eyes with possible connections to other bones (with feature index)
typedef struct UnityXREyes
{
    /// Pose of left Eye
    UnityXRPose leftEyePose;

    /// Pose of right Eye
    UnityXRPose rightEyePose;

    /// Fixation point that both eyes are fixated on
    UnityXRVector3 fixationPoint;

    /// Float value indicating how much the left eye is open
    float leftOpenAmount;

    /// Float value indicating how much the right eye is open
    float rightOpenAmount;
} UnityXREyes;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The Plugin Interface to be implemented by provider
typedef struct SUBSYSTEM_PROVIDER UnityXRInputProvider
{
    /// A provider-specified pointer which will be passed to every callback as the userData parameter.
    void* userData;

    /// A call from Unity at the beginning of each device update pass.  Device's update twice a frame: once before the Monobehaviour::UpdateLoop and once right before rendering.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] updateType The type of tick this is, to help providers get reference for when in a frame this is being called.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * Tick)(UnitySubsystemHandle handle, void* userData, UnityXRInputUpdateType updateType);

    /// Unity requesting the provider to fill in connected device information on a device that has been reported as connected.  Used to create customized device states.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] deviceId The Id of the device that needs a definition filled
    /// @param[in] definition The definition to be filled by the plugin
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * FillDeviceDefinition)(UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceDefinition* definition);

    /// Unity responding to a request to update a device's configuration.  This is triggered by calling IUnityXRInputInterface::InputSubsystem_DeviceConfigChanged.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] deviceId The Id of the device that needs a it's configuration changed.
    /// @param[in] config The configuration to be filled by the plugin.
    /// @return kUnitySubsystemErrorCodeSuccess The config was successfully updated.
    /// @return kUnitySubsystemErrorCodeFailure The config could not be updated.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * UpdateDeviceConfig)(UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceConfig* config);

    /// A call from Unity when it needs a snapshot of a specific device's state.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] deviceId The Id of the device whose state is requested.
    /// @param[in] updateType The type of update being polled for.   For BeforeRender we are predominantly interested in pose data, in the time expected for rendering
    /// @param[in] state The customized DeviceState to fill in.  The indices within this state match the order those input features were added from FillDeviceDefinition
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * UpdateDeviceState)(UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, UnityXRInputUpdateType updateType, UnityXRInputDeviceState* state);

    /// Simple, generic method callback to inform the plugin or individual devices of events occurring within unity
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] eventType The type of event being sent
    /// @param[in] deviceId The Id of the device that should handle the event.  Will be kUnityInvalidXRInputFeatureIndex if event is for all devices.
    /// @param[in] buffer An in/out payload of data.  The contents are dictated by the eventType.  See UnityXRInputEventType for details of each payload.
    /// @param[in] size The size of the buffer payload.  This can be used to verify that the type is actually what was expected.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * HandleEvent)(UnitySubsystemHandle handle, void* userData, unsigned int eventType, UnityXRInternalInputDeviceId deviceId, void* buffer, unsigned int size);

    /// A call from Unity when the developer requests a recenter.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @return kUnitySubsystemErrorCodeSuccess The provider was successfully recentered.
    /// @return kUnitySubsystemErrorCodeFailure The provider was unable to recenter.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * HandleRecenter)(UnitySubsystemHandle handle, void* userData);

    /// A call from Unity when the developer requests a haptic impulse or rumble.  This should only be processed if the device reports that it supports haptic impulses via QueryHapticCapabilities.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] deviceId The input device that the message is being directed to.
    /// @param[in] channel The haptic channel that the impulse is requested for.
    /// @param[in] amplitude The requested amplitude in a [0,1] range.
    /// @param[in] duration The >0 duration that the impulse is requested for.
    /// @return kUnitySubsystemErrorCodeSuccess The provider will trigger the haptic impulse on the specified device.
    /// @return kUnitySubsystemErrorCodeFailure The provider will not be triggering the haptic impulse.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * HandleHapticImpulse)(UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, int channel, float amplitude, float duration);

    /// A call from Unity when the developer is requesting a buffered haptic effect on an input device.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] deviceId The input device that the message is being directed to.
    /// @param[in] channel The haptic channel that the impulse is requested for.
    /// @param[in] buffer The haptic buffer to process with samples in a [0,255] range.
    /// @param[in] bufferSize The number of samples being sent in the buffer variable.
    /// @return kUnitySubsystemErrorCodeSuccess The provider will successfully process the haptic buffer.
    /// @return kUnitySubsystemErrorCodeFailure The provider will not process the haptic buffer.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * HandleHapticBuffer)(UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, int channel, unsigned int bufferSize, const unsigned char* const buffer);

    /// A call from Unity to request the haptic capabilities of a device.  The provider is expected to fill in the UnityXRHapticCapabilities struct that is passed in.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] deviceId The input device that the message is being directed to.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * QueryHapticCapabilities)(UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, UnityXRHapticCapabilities* capabilities);

    /// A call from Unity when the developer requests a stop of all haptic effects on a specific device.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] deviceId The input device that the message is being directed to.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * HandleHapticStop)(UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId);

    /// A call from Unity requesting the current tracking origin mode.  The provider is expected to set the UnityXRInputTrackingOrigin variable with the current tracking mode.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] trackingOriginMode The tracking origin mode that the provider is currently in.
    /// @return kUnitySubsystemErrorCodeSuccess The trackingOrigin was successfully retrieved.
    /// @return kUnitySubsystemErrorCodeFailure The trackingOrigin could not be retrieved.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * QueryTrackingOriginMode)(UnitySubsystemHandle handle, void* userData, UnityXRInputTrackingOriginModeFlags* trackingOriginMode);

    /// A call from Unity requesting the supported tracking origin modes.  The provider is expected to set the UnityXRInputTrackingOrigin variable with all supported modes.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] supportedTrackingOriginModes Bitwise or of all supported tracking origin modes.
    /// @return kUnitySubsystemErrorCodeSuccess All supported tracking origins were successfully retrieved.
    /// @return kUnitySubsystemErrorCodeFailure The supported tracking origins could not be retrieved.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * QuerySupportedTrackingOriginModes)(UnitySubsystemHandle handle, void* userData, UnityXRInputTrackingOriginModeFlags* supportedTrackingOriginModes);

    /// A call from Unity when the developer wants to set a new tracking origin mode.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData User specified data.
    /// @param[in] trackingOriginMode The tracking origin mode that the developer wants to set the provider to.
    /// @return kUnitySubsystemErrorCodeSuccess The tracking origin was successfully set.
    /// @return kUnitySubsystemErrorCodeInvalidArguments The tracking origin is not of a type supported by the provider via QuerySupportedTrackingOriginModes.
    /// @return kUnitySubsystemErrorCodeFailure Any other situation where the tracking origin was not able to be set.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * HandleSetTrackingOriginMode)(UnitySubsystemHandle handle, void* userData, UnityXRInputTrackingOriginModeFlags trackingOriginMode);

    /// Unity calls this when requesting a state at a specific time in the past
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] userData Custom data set when registering the provider
    /// @param[in] time The historical time Unity requests the state from
    /// @param[in] deviceId The Id of the device whose state is requested.
    /// @param[in] state The Device State that the provider must fill in.  The indices within this state match the order those input features were added from FillDeviceDefinition
    /// @return kUnitySubsystemErrorCodeSuccess State block successfully generated
    /// @return kUnitySubsystemErrorCodeFailure Holographic space is not available
    /// @return kUnitySubsystemErrorCodeFailure Unable to build historic state data
    /// @return kUnitySubsystemErrorCodeInvalidArguments the state provided is null
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * TryGetDeviceStateAtTime)(UnitySubsystemHandle handle, void* userData, UnityXRTimeStamp time, UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceState* state);
} UnityXRInputProvider;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The functions available to the provider to call back into Unity.
/// These are prefixed with InputSubsystem_ if it's for global subsystem calls, DeviceDefinition_ for updating elements of specific device definitions, and DeviceState_ for updating specific device states.
///
/// IUnityXRInputInterface's exist for the lifetime of the application, and are safe to cache locally.
UNITY_DECLARE_INTERFACE(SUBSYSTEM_INTERFACE IUnityXRInputInterface)
{
    /// Entry-point for getting callbacks when the input subsystem is initialized / started / stopped / shutdown.
    ///
    /// Example usage:
    /// @code
    /// #include "IUnityXRInput"
    ///
    /// static IUnityXRInputInterface* s_XrInput = NULL;
    ///
    /// extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    /// UnityPluginLoad(IUnityInterfaces* unityInterfaces)
    /// {
    ///     s_XrInput = unityInterfaces->Get<IUnityXRInputInterface>();
    ///     UnityLifecycleProvider inputLifecycleHandler =
    ///     {
    ///         NULL,  // This can be any object you want to be passed as userData to the following functions
    ///         &Lifecycle_Initialize,
    ///         &Lifecycle_Start,
    ///         &Lifecycle_Stop,
    ///         &Lifecycle_Shutdown
    ///     };
    ///     s_XrInput->RegisterLifecycleProvider("pluginName", "input plugin", &inputLifecycleHandler);
    /// }
    /// @endcode
    ///
    /// @param[in] pluginName Name of the plugin which was registered in your xr.json
    /// @param[in] id Id of the subsystem that was registered in your xr.json
    /// @param[in] provider Callbacks to register.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * RegisterLifecycleProvider)(const char* pluginName, const char* id, const UnityLifecycleProvider * provider);

    /// Used to register input provider specific callbacks.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] provider Provider callbacks you want called.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SUBSYSTEM_REGISTER_PROVIDER(UnityXRInputProvider) RegisterInputProvider)(UnitySubsystemHandle handle, const UnityXRInputProvider * provider);

    /// Used to report a newly connected device.  The only information required is a unique id from the provider.  Afterwards, Unity will call your registered UnityXRInputProvider for any future information it needs about that device.
    /// Can be called from any thread.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] deviceId A unique id the provider will use to identify the newly connected device in future calls.
    /// @return kUnitySubsystemErrorCodeSuccess Device connection was successfully reported
    /// @return kUnitySubsystemErrorCodeInvalidArguments handle was not a valid pointer to an input subsystem.
    /// @return kUnitySubsystemErrorCodeInvalidArguments deviceId was kUnityInvalidXRInternalInputId.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * InputSubsystem_DeviceConnected)(UnitySubsystemHandle handle, UnityXRInternalInputDeviceId deviceId);

    /// Used to report a device disconnecting.  This will stop all requests for device state updates, and report all features as being no longer available.
    /// Can be called from any thread.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] deviceId The unique id used to identify the device in all InputProvider calls.
    /// @return kUnitySubsystemErrorCodeSuccess Device disconnection was successfully reported
    /// @return kUnitySubsystemErrorCodeInvalidArguments handle was not a valid pointer to an input subsystem.
    /// @return kUnitySubsystemErrorCodeInvalidArguments deviceId was kUnityInvalidXRInternalInputId.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * InputSubsystem_DeviceDisconnected)(UnitySubsystemHandle handle, UnityXRInternalInputDeviceId deviceId);

    /// Reports a device changing configuration.  This allows you to supply a new device definition for the same device Id and update state.  The new device definition is totally blank and you need to fill in all settings when UnityXRInputProvider::FillDeviceDefinition is called.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] deviceId The unique id of the device whose configuration changed.
    /// @return kUnitySubsystemErrorCodeSuccess A device configuration change was successfully reported.
    /// @return kUnitySubsystemErrorCodeInvalidArguments handle was not a valid pointer to an input subsystem.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * InputSubsystem_DeviceConfigChanged)(UnitySubsystemHandle handle, UnityXRInternalInputDeviceId deviceId);

    /// Reports upwards to the developer that the tracking origin has changed.  This will be reported to the developer through the C# API event XRInputSubsystem.trackingOriginUpdated.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @return kUnitySubsystemErrorCodeSuccess A tracking origin update was successfully reported.
    /// @return kUnitySubsystemErrorCodeInvalidArguments handle was not a valid pointer to an input subsystem.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * InputSubsystem_TrackingOriginUpdated)(UnitySubsystemHandle handle);

    /// A call by Unity to set a tracking boundary.  The Tracking boundary represents a boundary region that maps to a physical space in the real world.
    ///
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param[in] boundaryPoints A pointer to the list of boundary in clockwise, sequential order.
    /// @param[in] boundaryPointCount The number of boundary points being pointed to by boundaryPoints.
    /// @return kUnitySubsystemErrorCodeSuccess Boundary points were successfully set.
    /// @return kUnitySubsystemErrorCodeInvalidArguments handle was not a valid pointer to an input subsystem.
    /// @return kUnitySubsystemErrorCodeInvalidArguments boundaryPoints is a null pointer.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * InputSubsystem_SetTrackingBoundary)(UnitySubsystemHandle handle, const UnityXRVector3* boundaryPoints, uint32_t boundaryPointCount);


    /// TEMPORARY: Returns native pointer to some sdk specific resources of the active legacy VR SDK.  The format of the data changes depending on the active legacy SDK.
    /// @param[in] handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @return kUnitySubsystemErrorCodeSuccess Platform data successfully returned
    /// @return kUnitySubsystemErrorCodeInvalidArguments handle was not a valid pointer to an input subsystem.
    /// @return kUnitySubsystemErrorCodeInvalidArguments platformData parameter was NULL.
    /// @return kUnitySubsystemErrorCodeFailure platformData was unavailable.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * InputSubsystem_GetPlatformData)(UnitySubsystemHandle handle, void** platformData);

    /////////////////////////////////////////////////////
    // Device Definition APIs
    // A Definition of what your device is capable of doing.  Immutable information are provided here when Unity called IUnityXRInputProvider::FillDeviceDefinition with a specific Device Id.
    // It will only be called once for each device connection.  For APIs that use and update the device definition see IUnityXRInputSubsystem functions prefixed with DeviceDefinition

    /// Sets the name of the device.  Used to inform users of what's connected.
    ///
    /// @param[in] definition A reference to the device definition you'd like to change the feature value on.
    /// @param[in] deviceName The name of the new device.
    /// @return kUnitySubsystemErrorCodeSuccess Device name was successfully set
    /// @return kUnitySubsystemErrorCodeInvalidArguments definition was not a valid pointer to a device definition.
    /// @return kUnitySubsystemErrorCodeInvalidArguments deviceName was either null, sized 0, or larger than kUnityXRStringSize.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceDefinition_SetName)(UnityXRInputDeviceDefinition* definition, const char* deviceName);

    /// Sets the capabilities of the device.  These are combinative flags used to describe what the device is capable of.
    ///
    /// @param[in] definition A reference to the device definition you'd like to change the feature value on.
    /// @param[in] deviceFlags The contextual flags that you'd like to set for this device.
    /// @return kUnitySubsystemErrorCodeSuccess If successfully set.
    /// @return kUnitySubsystemErrorCodeFailure If the characteristics could not be set.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceDefinition_SetCharacteristics)(UnityXRInputDeviceDefinition* definition, UnityXRInputDeviceCharacteristics deviceCapabilities);

    /// Identifies the Manufacturer of this device.  Reported upwards to the Developer to make it easier to identify device families.
    ///
    /// @param[in] definition A reference to the device definition you'd like to change the feature value on.
    /// @param[in] manufacturer The manufacturer of the device.
    /// @return kUnitySubsystemErrorCodeSuccess Device manufacturer was successfully set
    /// @return kUnitySubsystemErrorCodeInvalidArguments definition was not a valid pointer to a device definition.
    /// @return kUnitySubsystemErrorCodeInvalidArguments manufacturer was null, or larger than kUnityXRStringSize.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceDefinition_SetManufacturer)(UnityXRInputDeviceDefinition* definition, const char* manufacturer);

    /// Identifies the Serial Number of this device.  Reported upwards to the Developer to make it easier to identify very specific devices.
    ///
    /// @param[in] definition A reference to the device definition you'd like to change the feature value on.
    /// @param[in] serialNumber The serial number of the device
    /// @return kUnitySubsystemErrorCodeSuccess Device serial number was successfully set
    /// @return kUnitySubsystemErrorCodeInvalidArguments definition was not a valid pointer to a device definition.
    /// @return kUnitySubsystemErrorCodeInvalidArguments serialNumber was null or larger than kUnityXRStringSize.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceDefinition_SetSerialNumber)(UnityXRInputDeviceDefinition* definition, const char* serialNumber);

    /// Sets whether or not the XRSDK Input System can call into this device to query for historical input when requested.
    /// @param[in] definition A reference to the device state you'd like to change the feature value on.
    /// @param[in] whether the input system can call into this device for historical input.
    /// @return kUnitySubsystemErrorCodeSuccess If successfully set
    /// @return kUnitySubsystemErrorCodeInvalidArguments If the provided definition is null
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceDefinition_SetCanQueryForDeviceStateAtTime)(UnityXRInputDeviceDefinition* definition, bool enabled);

    /// Reports an input feature of the device, supplying the name specific to this device and a type.  Use the returned index when setting that feature's value within the associated UnityXRInputDeviceState.
    ///
    /// @param[in] definition A reference to the device definition you'd like to change the feature value on.
    /// @param[in] name The name of this input feature
    /// @param[in] type The type of feature this is, which dictates the space and properties of the input feature
    /// @return The assigned feature index, to be used with UnityXRInputDeviceState when setting the value of the added input feature.
    /// @return kUnityInvalidXRInputFeatureIndex definition was not a valid pointer to a device definition.
    /// @return kUnityInvalidXRInputFeatureIndex name was null or larger than kUnityXRStringSize.
    /// @return kUnityInvalidXRInputFeatureIndex type was outside of the valid UnityXRInputFeatureType range.
    UnityXRInputFeatureIndex(UNITY_INTERFACE_API * DeviceDefinition_AddFeature)(UnityXRInputDeviceDefinition* definition, const char* name, UnityXRInputFeatureType type);

    /// Reports an input feature of the device, supplying a name specific to this device, and a size.  This is used for custom data being passed to a specific template.
    ///
    /// @param[in] definition A reference to the device definition you'd like to change the feature value on.
    /// @param[in] name The name of this input feature
    /// @param[in] featureSizeInBytes The size of this feature in bytes.
    /// @return The assigned index, to be used with UnityXRInputDeviceState when setting the value of the added input feature.
    /// @return kUnityInvalidXRInputFeatureIndex definition was not a valid pointer to a device definition.
    /// @return kUnityInvalidXRInputFeatureIndex name was null or larger than kUnityXRStringSize.
    /// @return kUnityInvalidXRInputFeatureIndex size was not between 1 and kUnityMaxCustomInputFeatureSize.
    UnityXRInputFeatureIndex(UNITY_INTERFACE_API * DeviceDefinition_AddCustomFeature)(UnityXRInputDeviceDefinition* definition, const char* name, unsigned int featureSizeInBytes);

    /// Reports an input feature of the device, supplying the name specific to this device, a suggested usage, and a type.  Use the returned index when setting that feature's value within the associated UnityXRInputDeviceState. Name must not be null and must be shorter than kUnityXRStringSize.
    ///
    /// @param[in] definition A reference to the device definition you'd like to change the feature value on.
    /// @param[in] featureType The type of feature this is, which dictates the space and properties of the input feature
    /// @param[in] usageHint These are used by our various Input Systems to provide context for your feature.  It let's us know and understand how it should be used.
    /// @return The assigned index, to be used with UnityXRInputDeviceState when setting the value of the added input feature.
    /// @return kUnityInvalidXRInputFeatureIndex definition was not a valid pointer to a device definition.
    /// @return kUnityInvalidXRInputFeatureIndex name was null or larger than kUnityXRStringSize.
    /// @return kUnityInvalidXRInputFeatureIndex type was outside of the valid UnityXRInputFeatureType range.
    UnityXRInputFeatureIndex(UNITY_INTERFACE_API * DeviceDefinition_AddFeatureWithUsage)(UnityXRInputDeviceDefinition* definition, const char* name, UnityXRInputFeatureType type, UnityXRInputFeatureUsage usageHint);

    /// Adds a Usage hint to a specific feature index on an InputDeviceDefinition.  Usage hints are used to provide context for our InputSystems in order to understand how this feature should be used.
    ///
    /// @param[in] definition A reference to the device definition you'd like change the usage on.
    /// @param[in] featureIndex The Index of the feature you want to add the UsageHint to.  This is the value returned when you call DeviceDefinition_AddFeature initially
    /// @param[in] usageHint These are used by our various Input Systems to provide context for your feature.  It let's us know and understand how it should be used.
    /// @return kXRErrorCodeSuccess If the usage was successfully added.
    /// @return kUnitySubsystemErrorCodeInvalidArguments definition was not a valid pointer to a device definition.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex was less than 0 or greater than the total number of current features.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceDefinition_AddUsageAtIndex)(UnityXRInputDeviceDefinition* definition, UnityXRInputFeatureIndex featureIndex, UnityXRInputFeatureUsage usageHint);

    //////////////////////////////////////////////////////////
    // Device State APIs
    // A Structure setup to hold your individual device's state.  This is customized based on what was filled in for that devices' definition.
    // The Feature Indices are in the same order that AddFeature was called when filling in your device's definition.

    /// Sets the custom-sized buffer at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeCustom
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureBuffer A pointer to the buffer containing the data you want to set
    /// @param[in] featureSizeInBytes The size of the feature being set.
    /// @return kXRErrorCodeSuccess If the custom feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid custom feature.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureBuffer was null.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureSize did not match the feature size declared in the device definition.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetCustomValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, const void* featureBuffer, unsigned int featureSizeInBytes);

    /// Sets the binary value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeBinary
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The binary value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the binary feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid binary feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetBinaryValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, bool featureValue);

    /// Sets the discrete state value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeDiscreteStates
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The discrete state value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the discrete state feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid discrete state feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetDiscreteStateValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, unsigned int featureValue);

    /// Sets the axis value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeAxis1D
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The axis value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the 1D axis feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid  1D axis feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetAxis1DValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, float featureValue);

    /// Sets the Axis 2D value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeAxis2D
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The 2D axis value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the 2D axis feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid 2D axis feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetAxis2DValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, UnityXRVector2 featureValue);

    /// Sets a 3D axis value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeAxis3D
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The 3D axis value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the 3D axis feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid 3D axis feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetAxis3DValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, UnityXRVector3 featureValue);

    /// Sets a rotation value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeRotation
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The quaternion value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the rotation feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid rotation feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetRotationValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, UnityXRVector4 featureValue);

    /// Sets a bone value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeBone
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The bone value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the bone feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid bone feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetBoneValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, UnityXRBone featureValue);

    /// Sets a hand value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeHand
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the feature is located.
    /// @param[in] featureValue The bone value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the hand feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid hand feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetHandValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, UnityXRHand featureValue);

    /// Sets a eyes value at a specific feature index.  This is used for features of type UnityXRInputFeatureTypeEyes
    ///
    /// @param[in] state A reference to the device state you'd like to change the feature value on.
    /// @param[in] featureIndex The index in device state that the UnityXRInputFeatureTypeEyes feature is located.
    /// @param[in] featureValue The eyes value you want to set the feature to.
    /// @return kXRErrorCodeSuccess If the eyes feature was successfully modified.
    /// @return kUnitySubsystemErrorCodeInvalidArguments state was not a valid pointer to a device state.
    /// @return kUnitySubsystemErrorCodeInvalidArguments featureIndex did not point to a valid eyes feature.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetEyesValue)(UnityXRInputDeviceState* state, UnityXRInputFeatureIndex featureIndex, UnityXREyes featureValue);

    /// Sets the time that this device state was taken.
    ///
    /// @param[in] state The device state you'd like to set the time of.
    /// @param[in] deviceTime The time in milliseconds since unix epoch that this snapshot was taken.
    /// @return kUnitySubsystemErrorCodeSuccess If successfully set.
    /// @return kUnitySubsystemErrorCodeInvalidArguments If the provided state is null.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * DeviceState_SetDeviceTime)(UnityXRInputDeviceState* state, UnityXRTimeStamp deviceTime);
};

UNITY_REGISTER_INTERFACE_GUID(0x2B53FA871CDA6802ULL, 0x942BCA0C8EF13193ULL, IUnityXRInputInterface)
