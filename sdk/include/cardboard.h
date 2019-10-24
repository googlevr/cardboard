/*
 * Copyright 2019 Google Inc. All Rights Reserved.
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
#ifndef CARDBOARD_SDK_INCLUDE_CARDBOARD_H_
#define CARDBOARD_SDK_INCLUDE_CARDBOARD_H_

#ifdef __ANDROID__
#include <jni.h>
#endif

#include <stdint.h>

#ifdef __ANDROID__
#include <GLES2/gl2.h>
#endif
#ifdef __APPLE__
#include <OpenGLES/ES2/gl.h>
#endif

/// UV coordinates helper struct.
typedef struct CardboardUv {
  float u, v;
} CardboardUv;

/// Enum to distinguish left and right eyes.
typedef enum CardboardEye {
  kLeft = 0,
  kRight = 1,
} CardboardEye;

/// Struct representing a 3D mesh with 3D vertices and corresponding UV
/// coordinates.
typedef struct CardboardMesh {
  int* indices;
  int n_indices;
  float* vertices;  // 2 floats per vertex: x, y.
  float* uvs;       // 2 floats per uv: u, v.
  int n_vertices;
} CardboardMesh;

/// Struct to hold information about an eye texture.
typedef struct CardboardEyeTextureDescription {
  GLuint texture;           // The texture with eye pixels.
  int layer;                // The layer index inside the texture.
  float left_u;             // u coordinate of the left side of the eye.
  float right_u;            // u coordinate of the right side of the eye.
  float top_v;              // v coordinate of the top side of the eye.
  float bottom_v;           // v coordinate of the bottom side.
  float eye_from_head[16];  // The translation from head coordinate frame  to
                            // the eye frame.
} CardboardEyeTextureDescription;

/// An opaque Lens Distortion object.
typedef struct CardboardLensDistortion CardboardLensDistortion;

/// An opaque Distortion Renderer object.
typedef struct CardboardDistortionRenderer CardboardDistortionRenderer;

/// An opaque Head Tracker object.
typedef struct CardboardHeadTracker CardboardHeadTracker;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ANDROID__
/// Initializes JavaVM and Android activity context.
///
/// @param[in]      vm                      JavaVM pointer
/// @param[in]      context                 Android activity context
void Cardboard_initializeAndroid(JavaVM* vm, jobject context);
#endif

/// Lens Distortion functions.

/// Creates a new lens distortion object and initializes it with the values from
/// @c encoded_device_params.
///
/// @param[in]      encoded_device_params   The device parameters serialized
///     using cardboard_device.proto.
/// @param[in]      size                    Size in bytes of
///     encoded_device_params.
/// @param[in]      display_width           Size in pixels of display width.
/// @param[in]      display_height          Size in pixels of display height.
/// @return         Lens distortion object pointer.
CardboardLensDistortion* CardboardLensDistortion_create(
    const uint8_t* encoded_device_params, int size, int display_width,
    int display_height);

/// Destroys and releases memory used by the provided lens distortion object.
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
void CardboardLensDistortion_destroy(CardboardLensDistortion* lens_distortion);

/// Gets the ideal projection and eye_from_head matrices for a particular eye.
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[out]     projection_matrix       4x4 float ideal projection matrix.
/// @param[out]     eye_from_head_matrix    4x4 float eye from head matrix.
/// @param[in]      eye                     Desired eye.
void CardboardLensDistortion_getEyeMatrices(
    CardboardLensDistortion* lens_distortion, float* projection_matrix,
    float* eye_from_head_matrix, CardboardEye eye);

/// Gets the distortion mesh for a particular eye.
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      eye                     Desired eye.
/// @param[out]     mesh                    Distortion mesh.
void CardboardLensDistortion_getDistortionMesh(
    CardboardLensDistortion* lens_distortion, CardboardEye eye,
    CardboardMesh* mesh);

/// Applies lens inverse distortion function to a point normalized [0,1] in pre
/// distort space (eye texture space).
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      distorted_uv            Distorted UV point.
/// @param[in]      eye                     Desired eye.
/// @return         Point normalized [0,1] in the screen post distort space.
CardboardUv CardboardLensDistortion_undistortedUvForDistortedUv(
    CardboardLensDistortion* lens_distortion, const CardboardUv* distorted_uv,
    CardboardEye eye);

/// Applies lens distortion function to a point normalized [0,1] in the screen
/// post distort space.
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      undistorted_uv          Undistorted UV point.
/// @param[in]      eye                     Desired eye.
/// @return         Point normalized [0,1] in pre distort space (eye texture
///     space).
CardboardUv CardboardLensDistortion_distortedUvForUndistortedUv(
    CardboardLensDistortion* lens_distortion, const CardboardUv* undistorted_uv,
    CardboardEye eye);

/// Distortion Renderer functions.

/// Creates a new distortion renderer object. Must be called from rendering
/// thread.
///
/// @return         Distortion renderer object pointer
CardboardDistortionRenderer* CardboardDistortionRenderer_create();

/// Destroys and releases memory used by the provided distortion renderer
/// object. Must be called from rendering thread.
///
/// @param[in]      renderer                Distortion renderer object pointer.
void CardboardDistortionRenderer_destroy(CardboardDistortionRenderer* renderer);

/// Sets Distortion Mesh for a particular eye. Must be called from rendering
/// thread.
///
/// @param[in]      renderer                Distortion renderer object pointer.
/// @param[in]      mesh                    Distortion mesh.
/// @param[in]      eye                     Desired eye.
void CardboardDistortionRenderer_setMesh(CardboardDistortionRenderer* renderer,
                                         const CardboardMesh* mesh,
                                         CardboardEye eye);

/// Renders eye textures to display. Must be called from rendering thread.
///
/// @param[in]      renderer                Distortion renderer object pointer.
/// @param[in]      target_display          Target display.
/// @param[in]      display_width           Size in pixels of display width.
/// @param[in]      display_height          Size in pixels of display height.
/// @param[in]      left_eye                Left eye texture description.
/// @param[in]      right_eye               Right eye texture description.
void CardboardDestortionRenderer_renderEyeToDisplay(
    CardboardDistortionRenderer* renderer, int target_display,
    int display_width, int display_height,
    const CardboardEyeTextureDescription* left_eye,
    const CardboardEyeTextureDescription* right_eye);

/// Head Tracker functions.

/// Creates a new head tracker object.
///
/// @return         head tracker object pointer
CardboardHeadTracker* CardboardHeadTracker_create();

/// Destroys and releases memory used by the provided head tracker object.
///
/// @param[in]      head_tracker            Head tracker object pointer.
void CardboardHeadTracker_destroy(CardboardHeadTracker* head_tracker);

/// Pauses head tracker and underlying device sensors.
///
/// @param[in]      head_tracker            Head tracker object pointer.
void CardboardHeadTracker_pause(CardboardHeadTracker* head_tracker);

/// Resumes head tracker and underlying device sensors.
///
/// @param[in]      head_tracker            Head tracker object pointer.
void CardboardHeadTracker_resume(CardboardHeadTracker* head_tracker);

/// Gets the predicted head pose for a given timestamp.
///
/// @param[in]      head_tracker            Head tracker object pointer.
/// @param[in]      timestamp_ns            The timestamp for the pose in
///     nanoseconds in system monotonic clock.
/// @param[out]     position                3 floats for (x, y, z).
/// @param[out]     orientation             4 floats for quaternion
void CardboardHeadTracker_getPose(CardboardHeadTracker* head_tracker,
                                  int64_t timestamp_ns, float* position,
                                  float* orientation);

/// QR Code scan functions.

/// Gets currently saved devices parameters. This function allocates memory for
/// the parameters, so it must be released using CardboardQrCode_destroy.
///
/// @param[out]     encoded_device_params   Reference to the device parameters
///     serialized using cardboard_device.proto.
/// @param[out]     size                    Size in bytes of
///     encoded_device_params.
void CardboardQrCode_getSavedDeviceParams(uint8_t** encoded_device_params,
                                          int* size);

/// Releases memory used by the provided encoded_device_params array.
///
/// @param[in]      encoded_device_params   The device parameters serialized
///     using cardboard_device.proto.
void CardboardQrCode_destroy(const uint8_t* encoded_device_params);

/// Scans a QR code and saves the encoded device parameters.
void CardboardQrCode_scanQrCodeAndSaveDeviceParams();

#ifdef __cplusplus
}
#endif

#endif  // CARDBOARD_SDK_INCLUDE_CARDBOARD_H_
