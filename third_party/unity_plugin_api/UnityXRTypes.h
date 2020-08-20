// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once

#if UNITY
#include "Modules/Subsystems/ProviderInterface/UnityTypes.h"
#else
#include "UnityTypes.h"
#endif

/// The maximum length of a string, used in some structs with the XR headers
enum { kUnityXRStringSize = kUnitySubsystemsStringSize };

/// Simple 2-Element Float Vector
typedef struct UnityXRVector2
{
    /// X component of the vector.
    float x;

    /// Y component of the vector.
    float y;
} UnityXRVector2;

/// Simple 3-Element float vector
typedef struct UnityXRVector3
{
    /// X component of the vector.
    float x;

    /// Y component of the vector.
    float y;

    /// Z component of the vector.
    float z;
} UnityXRVector3;

/// Simple 4 Element Quaternion with indices ordered x, y, z, and w in order
typedef struct UnityXRVector4
{
    /// X component of the vector.
    float x;

    /// Y component of the vector.
    float y;

    /// Z component of the vector.
    float z;

    /// W component of the vector.
    float w;
} UnityXRVector4;

/// A simple struct representing a point in space with position and orientation
typedef struct UnityXRPose
{
    /// Position of the pose.
    UnityXRVector3 position;

    /// Rotation of the pose, stored as a quaternion.
    UnityXRVector4 rotation;
} UnityXRPose;

/// A 4x4 column-major matrix
typedef struct UnityXRMatrix4x4
{
    ///. The columns of the matrix
    UnityXRVector4 columns[4];
} UnityXRMatrix4x4;

/// Multiple ways of representing a projection.
typedef enum UnityXRProjectionType
{
    /// Projection represented as tangents of half angles from center.
    kUnityXRProjectionTypeHalfAngles,

    /// Projection represented as a 4x4 matrix.
    kUnityXRProjectionTypeMatrix
} UnityXRProjectionType;

/// Projection as tangent of half angles on near plane.
typedef struct UnityXRProjectionHalfAngles
{
    /// Tangent of the half angle from center to left clipping plane. (should be negative)
    float left;
    /// Tangent of the half angle from center to right clipping plane. (should be positive)
    float right;
    /// Tangent of the half angle from center to top clipping plane. (should be positive)
    float top;
    /// Tangent of the half angle from center to bottom clipping plane. (should be negative)
    float bottom;
} UnityXRProjectionHalfAngles;

/// Container for the different methods of representing a projection matrix.
typedef struct UnityXRProjection
{
    /// Choose the way this projection is represented.
    UnityXRProjectionType type;

    union
    {
        /// Valid if type is kUnityXRProjectionTypeHalfAngles.
        UnityXRProjectionHalfAngles halfAngles;

        /// Valid if type is kUnityXRProjectionTypeMatrix.
        UnityXRMatrix4x4 matrix;
    } data; ///< Contains all supported ways of representing a projection matrix.
} UnityXRProjection;

/// A 2D rectangle defined by X and Y position, width and height.
typedef struct UnityXRRectf
{
    /// X position of the rectangle.
    float x;

    /// Y position of the rectangle.
    float y;

    /// Width of the rectangle.
    float width;

    /// Height of the rectangle.
    float height;
} UnityXRRectf;

/// An RGBA color
typedef struct UnityXRColorRGBA32
{
    /// The red channel
    uint8_t red;

    /// The green channel
    uint8_t green;

    /// The blue channel
    uint8_t blue;

    /// The alpha channel
    uint8_t alpha;
} UnityXRColorRGBA32;

/// Represents milliseconds of UTC time since Unix epoch 1970-01-01T00:00:00Z.
typedef int64_t UnityXRTimeStamp;
