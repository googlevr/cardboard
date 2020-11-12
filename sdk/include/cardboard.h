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
#ifndef CARDBOARD_SDK_INCLUDE_CARDBOARD_H_
#define CARDBOARD_SDK_INCLUDE_CARDBOARD_H_

#ifdef __ANDROID__
#include <jni.h>
#endif

#include <stdint.h>

/// @defgroup types Cardboard SDK types
/// @brief Various types used in the Cardboard SDK.
/// @{

/// Struct to hold UV coordinates.
typedef struct CardboardUv {
  /// u coordinate.
  float u;
  /// v coordinate.
  float v;
} CardboardUv;

/// Enum to distinguish left and right eyes.
typedef enum CardboardEye {
  /// Left eye.
  kLeft = 0,
  /// Right eye.
  kRight = 1,
} CardboardEye;

/// Struct representing a 3D mesh with 3D vertices and corresponding UV
/// coordinates.
typedef struct CardboardMesh {
  /// Indices buffer.
  int* indices;
  /// Number of indices.
  int n_indices;
  /// Vertices buffer. 2 floats per vertex: x, y.
  float* vertices;
  /// UV coordinates buffer. 2 floats per uv: u, v.
  float* uvs;
  /// Number of vertices.
  int n_vertices;
} CardboardMesh;

/// Struct to hold information about an eye texture.
typedef struct CardboardEyeTextureDescription {
  /// The texture with eye pixels.
  uint32_t texture;
  /// u coordinate of the left side of the eye.
  float left_u;
  /// u coordinate of the right side of the eye.
  float right_u;
  /// v coordinate of the top side of the eye.
  float top_v;
  /// v coordinate of the bottom side of the eye.
  float bottom_v;
} CardboardEyeTextureDescription;

/// An opaque Lens Distortion object.
typedef struct CardboardLensDistortion CardboardLensDistortion;

/// An opaque Distortion Renderer object.
typedef struct CardboardDistortionRenderer CardboardDistortionRenderer;

/// An opaque Head Tracker object.
typedef struct CardboardHeadTracker CardboardHeadTracker;

/// @}

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Initialization (Android only)
/////////////////////////////////////////////////////////////////////////////
/// @defgroup initialization Initialization (Android only)
/// @brief This module initializes the JavaVM and Android context.
///
/// Important: This function is only used by Android and it's mandatory to call
///     this function before using any other Cardboard APIs.
/// @{

#ifdef __ANDROID__
/// Initializes the JavaVM and Android context.
///
/// @details The following methods are required to work for the parameter
///   @p context:
///
/// -
/// [Context.getFilesDir()](https://developer.android.com/reference/android/content/Context#getFilesDir())
/// -
/// [Context.getResources()](https://developer.android.com/reference/android/content/Context#getResources())
/// -
/// [Context.getSystemService(Context.WINDOW_SERVICE)](https://developer.android.com/reference/android/content/Context#getSystemService(java.lang.String))
/// -
/// [Context.startActivity(Intent)](https://developer.android.com/reference/android/content/Context#startActivity(android.content.Intent))
/// -
/// [Context.getDisplay()](https://developer.android.com/reference/android/content/Context#getDisplay())
///
/// @pre @p vm Must not be null.
/// @pre @p context Must not be null.
/// When it is unmet a call to this function results in a no-op.
///
/// @param[in]      vm                      JavaVM pointer
/// @param[in]      context                 The current Android Context. It is
///                                         generally an Activity instance or
///                                         wraps one.
void Cardboard_initializeAndroid(JavaVM* vm, jobject context);
#endif

/// @}

/////////////////////////////////////////////////////////////////////////////
// Lens Distortion
/////////////////////////////////////////////////////////////////////////////
/// @defgroup lens-distortion Lens Distortion
/// @brief This module calculates the projection and eyes distortion matrices,
///     based on the device (Cardboard viewer) and screen parameters. It also
///     includes functions to calculate the distortion for a single point.
/// @{

/// Creates a new lens distortion object and initializes it with the values from
/// @c encoded_device_params.
///
/// @pre @p encoded_device_params Must not be null.
/// When it is unmet, a call to this function results in a no-op and returns a
/// nullptr.
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
/// @pre @p lens_distortion Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
void CardboardLensDistortion_destroy(CardboardLensDistortion* lens_distortion);

/// Gets the eye_from_head matrix for a particular eye.
///
/// @pre @p lens_distortion Must not be null.
/// @pre @p eye_from_head_matrix Must not be null.
/// When it is unmet, a call to this function results in a no-op and a default
/// value is returned (identity matrix).
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      eye                     Desired eye.
/// @param[out]     eye_from_head_matrix    4x4 float eye from head matrix.
void CardboardLensDistortion_getEyeFromHeadMatrix(
    CardboardLensDistortion* lens_distortion, CardboardEye eye,
    float* eye_from_head_matrix);

/// Gets the ideal projection matrix for a particular eye.
///
/// @pre @p lens_distortion Must not be null.
/// @pre @p projection_matrix Must not be null.
/// When it is unmet, a call to this function results in a no-op and a default
/// value is returned (identity matrix).
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      eye                     Desired eye.
/// @param[in]      z_near                  Near clip plane z-axis coordinate.
/// @param[in]      z_far                   Far clip plane z-axis coordinate.
/// @param[out]     projection_matrix       4x4 float ideal projection matrix.
void CardboardLensDistortion_getProjectionMatrix(
    CardboardLensDistortion* lens_distortion, CardboardEye eye, float z_near,
    float z_far, float* projection_matrix);

/// Gets the field of view half angles for a particular eye.
///
/// @pre @p lens_distortion Must not be null.
/// @pre @p field_of_view Must not be null.
/// When it is unmet, a call to this function results in a no-op and a default
/// value is returned (all angles equal to 45 degrees).
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      eye                     Desired eye.
/// @param[out]     field_of_view           4x1 float half angles in radians,
///                                         angles are disposed [left, right,
///                                         bottom, top].
void CardboardLensDistortion_getFieldOfView(
    CardboardLensDistortion* lens_distortion, CardboardEye eye,
    float* field_of_view);

/// Gets the distortion mesh for a particular eye.
///
/// @pre @p lens_distortion Must not be null.
/// @pre @p mesh Must not be null.
/// When it is unmet, a call to this function results in a no-op and a default
/// value is returned (empty values).
///
/// Important: The distorsion mesh that is returned by this function becomes
/// invalid if CardboardLensDistortion is destroyed.
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      eye                     Desired eye.
/// @param[out]     mesh                    Distortion mesh.
void CardboardLensDistortion_getDistortionMesh(
    CardboardLensDistortion* lens_distortion, CardboardEye eye,
    CardboardMesh* mesh);

/// Applies lens inverse distortion function to a point normalized [0,1] in
/// pre-distortion (eye texture) space.
///
/// @pre @p lens_distortion Must not be null.
/// @pre @p distorted_uv Must not be null.
/// When it is unmet, a call to this function results in a no-op and returns an
/// invalid struct (in other words, both UV coordinates are equal to -1).
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      distorted_uv            Distorted UV point.
/// @param[in]      eye                     Desired eye.
/// @return         Point normalized [0,1] in the screen post distort space.
CardboardUv CardboardLensDistortion_undistortedUvForDistortedUv(
    CardboardLensDistortion* lens_distortion, const CardboardUv* distorted_uv,
    CardboardEye eye);

/// Applies lens distortion function to a point normalized [0,1] in the screen
/// post-distortion space.
///
/// @pre @p lens_distortion Must not be null.
/// @pre @p undistorted_uv Must not be null.
/// When it is unmet, a call to this function results in a no-op and returns an
/// invalid struct (in other words, both UV coordinates are equal to -1).
///
/// @param[in]      lens_distortion         Lens distortion object pointer.
/// @param[in]      undistorted_uv          Undistorted UV point.
/// @param[in]      eye                     Desired eye.
/// @return         Point normalized [0,1] in pre distort space (eye texture
///     space).
CardboardUv CardboardLensDistortion_distortedUvForUndistortedUv(
    CardboardLensDistortion* lens_distortion, const CardboardUv* undistorted_uv,
    CardboardEye eye);
/// @}

/////////////////////////////////////////////////////////////////////////////
// Distortion Renderer
/////////////////////////////////////////////////////////////////////////////
/// @defgroup distortion-renderer Distortion Renderer
/// @brief This module renders the eyes textures into the display.
///
/// Important: This module functions must be called from the render thread.
/// @{

/// Creates a new distortion renderer object. It uses OpenGL ES 2.0 as the
/// rendering API. Must be called from the render thread.
///
/// @return         Distortion renderer object pointer
CardboardDistortionRenderer* CardboardOpenGlEs2DistortionRenderer_create();

/// Creates a new distortion renderer object. It uses OpenGL ES 3.0 as the
/// rendering API. Must be called from the render thread.
///
/// @return         Distortion renderer object pointer
CardboardDistortionRenderer* CardboardOpenGlEs3DistortionRenderer_create();

/// Creates a new distortion renderer object. It uses Metal as the rendering
/// API. Must be called from the render thread.
///
/// @return         Distortion renderer object pointer
CardboardDistortionRenderer* CardboardMetalDistortionRenderer_create();

/// Creates a new distortion renderer object. It uses Vulkan as the rendering
/// API. Must be called from the render thread.
///
/// @return         Distortion renderer object pointer
CardboardDistortionRenderer* CardboardVulkanDistortionRenderer_create();

/// Destroys and releases memory used by the provided distortion renderer
/// object. Must be called from render thread.
///
/// @pre @p renderer Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      renderer                Distortion renderer object pointer.
void CardboardDistortionRenderer_destroy(CardboardDistortionRenderer* renderer);

/// Sets the distortion Mesh for a particular eye. Must be called from render
/// thread.
///
/// @pre @p renderer Must not be null.
/// @pre @p mesh Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      renderer                Distortion renderer object pointer.
/// @param[in]      mesh                    Distortion mesh.
/// @param[in]      eye                     Desired eye.
void CardboardDistortionRenderer_setMesh(CardboardDistortionRenderer* renderer,
                                         const CardboardMesh* mesh,
                                         CardboardEye eye);

/// Renders eye textures to a rectangle in the display. Must be called from
/// render thread.
///
/// @pre @p renderer Must not be null.
/// @pre @p left_eye Must not be null.
/// @pre @p right_eye Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      renderer                Distortion renderer object pointer.
/// @param[in]      target_display          Target display.
/// @param[in]      x                       x coordinate of the rectangle's
///                                         lower left corner in pixels.
/// @param[in]      y                       y coordinate of the rectangle's
///                                         lower left corner in pixels.
/// @param[in]      width                   Size in pixels of the rectangle's
///                                         width.
/// @param[in]      height                  Size in pixels of the rectangle's
///                                         height.
/// @param[in]      left_eye                Left eye texture description.
/// @param[in]      right_eye               Right eye texture description.
void CardboardDistortionRenderer_renderEyeToDisplay(
    CardboardDistortionRenderer* renderer, int target_display, int x, int y,
    int width, int height, const CardboardEyeTextureDescription* left_eye,
    const CardboardEyeTextureDescription* right_eye);

/// @}

/////////////////////////////////////////////////////////////////////////////
// Head Tracker
/////////////////////////////////////////////////////////////////////////////
/// @defgroup head-tracker Head Tracker
/// @brief This module calculates the predicted head's pose for a given
///     timestamp. It takes data from accelerometer and gyroscope sensors and
///     uses a Kalman filter to generate the output value. The head's pose is
///     returned as a quaternion. To have control of the usage of the sensors,
///     this module also includes pause and resume functions.
/// @{

/// Creates a new head tracker object.
///
/// @return         head tracker object pointer
CardboardHeadTracker* CardboardHeadTracker_create();

/// Destroys and releases memory used by the provided head tracker object.
///
/// @pre @p head_tracker Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      head_tracker            Head tracker object pointer.
void CardboardHeadTracker_destroy(CardboardHeadTracker* head_tracker);

/// Pauses head tracker and underlying device sensors.
///
/// @pre @p head_tracker Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      head_tracker            Head tracker object pointer.
void CardboardHeadTracker_pause(CardboardHeadTracker* head_tracker);

/// Resumes head tracker and underlying device sensors.
///
/// @pre @p head_tracker Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      head_tracker            Head tracker object pointer.
void CardboardHeadTracker_resume(CardboardHeadTracker* head_tracker);

/// Gets the predicted head pose for a given timestamp.
///
/// @pre @p head_tracker Must not be null.
/// @pre @p position Must not be null.
/// @pre @p orientation Must not be null.
/// When it is unmet, a call to this function results in a no-op and default
/// values are returned (zero values and identity quaternion, respectively).
///
/// @param[in]      head_tracker            Head tracker object pointer.
/// @param[in]      timestamp_ns            The timestamp for the pose in
///     nanoseconds in system monotonic clock.
/// @param[out]     position                3 floats for (x, y, z).
/// @param[out]     orientation             4 floats for quaternion
void CardboardHeadTracker_getPose(CardboardHeadTracker* head_tracker,
                                  int64_t timestamp_ns, float* position,
                                  float* orientation);

/// @}

/////////////////////////////////////////////////////////////////////////////
// QR Code Scanner
/////////////////////////////////////////////////////////////////////////////
/// @defgroup qrcode-scanner QR Code Scanner
/// @brief This module manages the entire process of capturing, decoding and
///     getting the device parameters from a QR code. It also saves and loads
///     the device parameters to and from the external storage.
/// @{

/// Gets currently saved devices parameters. This function allocates memory for
/// the parameters, so it must be released using CardboardQrCode_destroy.
///
/// @pre @p encoded_device_params Must not be null.
/// @pre @p size Must not be null.
/// When it is unmet, a call to this function results in a no-op and default
/// values are returned (empty values).
///
/// @param[out]     encoded_device_params   Reference to the device parameters
///     serialized using cardboard_device.proto.
/// @param[out]     size                    Size in bytes of
///     encoded_device_params.
void CardboardQrCode_getSavedDeviceParams(uint8_t** encoded_device_params,
                                          int* size);

/// Releases memory used by the provided encoded_device_params array.
///
/// @pre @p encoded_device_params Must not be null.
/// When it is unmet, a call to this function results in a no-op.
///
/// @param[in]      encoded_device_params   The device parameters serialized
///     using cardboard_device.proto.
void CardboardQrCode_destroy(const uint8_t* encoded_device_params);

/// Scans a QR code and saves the encoded device parameters.
///
/// @details Upon termination, it will increment a counter that can be queried
///          via @see CardboardQrCode_getQrCodeScanCount() when new device
///          parameters where succesfully saved.
void CardboardQrCode_scanQrCodeAndSaveDeviceParams();

/// Gets the count of successful device parameters read and save operations.
///
/// @return The count of successful device parameters read and save operations.
int CardboardQrCode_getQrCodeScanCount();

/// Gets Cardboard V1 device parameters.
///
/// @details This function does not use external storage, and stores into @p
///          encoded_device_params the value of a pointer storing proto buffer.
///          Users of this API should not free memory.
///
/// @pre @p encoded_device_params Must not be null.
/// @pre @p size Must not be null.
/// When it is unmet, a call to this function results in a no-op and default
/// values are returned (empty values).
///
/// @param[out]     encoded_device_params   Reference to the device parameters.
/// @param[out]     size                    Size in bytes of
///                                         @p encoded_device_params.
void CardboardQrCode_getCardboardV1DeviceParams(uint8_t** encoded_device_params,
                                                int* size);

/// @}

#ifdef __cplusplus
}
#endif

#endif  // CARDBOARD_SDK_INCLUDE_CARDBOARD_H_
