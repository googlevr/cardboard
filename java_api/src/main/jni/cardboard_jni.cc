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

#include <android/log.h>
#include <jni.h>
#include <stdint.h>

#include <memory>

#include <GLES2/gl2.h>
#include "include/cardboard.h"
#include "jni_utils/android/jni_utils.h"

#define JNI_METHOD(return_type, clazz, method_name) \
  JNIEXPORT return_type JNICALL                     \
      Java_com_google_cardboard_sdk_##clazz##_##method_name

namespace {

JavaVM* java_vm_;
jclass mesh_class_;
jclass uv_point_class_;
jclass eye_texture_description_class_;

constexpr const char* kConstructorMethod = "<init>";
constexpr const char* kJniIntSignature = "I";
constexpr const char* kJniFloatSignature = "F";
constexpr const char* kJniLongSignature = "J";

constexpr const char* kEyeTextureDescriptionTexture = "texture";
constexpr const char* kEyeTextureDescriptionLeftU = "leftU";
constexpr const char* kEyeTextureDescriptionRightU = "rightU";
constexpr const char* kEyeTextureDescriptionTopV = "topV";
constexpr const char* kEyeTextureDescriptionBottomV = "bottomV";

constexpr const char* kUvPointU = "u";
constexpr const char* kUvPointV = "v";

constexpr const char* kMeshIndices = "indices";
constexpr const char* kMeshNIndices = "nIndices";
constexpr const char* kMeshVertices = "vertices";
constexpr const char* kMeshUvs = "uvs";
constexpr const char* kMeshNVertices = "nVertices";

constexpr uint64_t kNanosInSeconds = 1000000000;

// Type conversion between native code pointers to intptr_t which are implicitly
// converted into jlongs afterwards if required.
// @param ptr A pointer.
// @tparam T Any native type.
// @return The address that @p ptr holds converted to an intptr_t.
template <typename T>
constexpr intptr_t ToJniPointer(const T* ptr) {
  return reinterpret_cast<intptr_t>(ptr);
}

// Finds and creates global references to java classes used within this module.
// TODO(b/180938531): Release these global references.
void LoadJNIResources(JNIEnv* env) {
  mesh_class_ =
      reinterpret_cast<jclass>(env->NewGlobalRef(cardboard::jni::LoadJClass(
          env, "com/google/cardboard/sdk/nativetypes/Mesh")));
  uv_point_class_ =
      reinterpret_cast<jclass>(env->NewGlobalRef(cardboard::jni::LoadJClass(
          env, "com/google/cardboard/sdk/nativetypes/UvPoint")));
  eye_texture_description_class_ =
      reinterpret_cast<jclass>(env->NewGlobalRef(cardboard::jni::LoadJClass(
          env, "com/google/cardboard/sdk/nativetypes/EyeTextureDescription")));
}

}  // anonymous namespace

extern "C" {

// Creates a CardboardEyeTextureDescription out of @p eye.
//
// @param env The JNI environment.
// @param eye A Java EyeTextureDescription reference.
// @return A CardboardEyeTextureDescription instance with the same contents than
// @p eye.
CardboardEyeTextureDescription CardboardEyeTextureDescriptionFromJobject(
    JNIEnv* env, jobject eye) {
  const jlong java_texture = env->GetLongField(
      eye, env->GetFieldID(eye_texture_description_class_,
                           kEyeTextureDescriptionTexture, kJniLongSignature));
  const jfloat java_left_u = env->GetFloatField(
      eye, env->GetFieldID(eye_texture_description_class_,
                           kEyeTextureDescriptionLeftU, kJniFloatSignature));
  const jfloat java_right_u = env->GetFloatField(
      eye, env->GetFieldID(eye_texture_description_class_,
                           kEyeTextureDescriptionRightU, kJniFloatSignature));
  const jfloat java_top_v = env->GetFloatField(
      eye, env->GetFieldID(eye_texture_description_class_,
                           kEyeTextureDescriptionTopV, kJniFloatSignature));
  const jfloat java_bottom_v = env->GetFloatField(
      eye, env->GetFieldID(eye_texture_description_class_,
                           kEyeTextureDescriptionBottomV, kJniFloatSignature));
  return CardboardEyeTextureDescription{
      static_cast<uint64_t>(java_texture), static_cast<float>(java_left_u),
      static_cast<float>(java_right_u), static_cast<float>(java_top_v),
      static_cast<float>(java_bottom_v)};
}

// Converts @p eye into a CardboardEye enumeration.
// @param eye It could be 0 or 1 which represents CardboardEye::kLeft and
// CardboardEye::kRight respectively.
// @return CardboardEye::kLeft when @p eye is 0. Otherwise CardboardEye::kRight.
constexpr CardboardEye CardboardEyeFromJint(jint eye) {
  return eye == 0 ? kLeft : kRight;
}

// @return The time since the boot in nano seconds.
int64_t GetBootTimeNano() {
  struct timespec res;
  clock_gettime(CLOCK_BOOTTIME, &res);
  return (res.tv_sec * kNanosInSeconds) + res.tv_nsec;
}

// Library entry point.
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
  java_vm_ = vm;
  return JNI_VERSION_1_6;
}

// Initialization API

JNI_METHOD(void, Initialize, nativeInitialize)
(JNIEnv* env, jobject, jobject context) {
  Cardboard_initializeAndroid(java_vm_, context);
  LoadJNIResources(env);
}

// Lens Distortion API

JNI_METHOD(jlong, LensDistortion, nativeLensDistortionCreate)
(JNIEnv* env, jobject, jbyteArray device_params, jint display_width,
 jint display_height) {
  jbyte* device_params_array = env->GetByteArrayElements(device_params, 0);
  const jsize size = env->GetArrayLength(device_params);
  const jlong lens_distortion = ToJniPointer(CardboardLensDistortion_create(
      reinterpret_cast<const uint8_t*>(device_params_array), size,
      display_width, display_height));
  env->ReleaseByteArrayElements(device_params, device_params_array, 0);
  return lens_distortion;
}

JNI_METHOD(void, LensDistortion, nativeLensDistortionDestroy)
(JNIEnv*, jobject, jlong lens_distortion) {
  if (lens_distortion == 0) {
    return;
  }
  CardboardLensDistortion_destroy(
      reinterpret_cast<CardboardLensDistortion*>(lens_distortion));
}

JNI_METHOD(void, LensDistortion, nativeLensDistortionGetEyeFromHeadMatrix)
(JNIEnv* env, jobject, jlong lens_distortion, jint eye,
 jfloatArray eye_from_head_matrix) {
  if (lens_distortion == 0) {
    return;
  }
  jfloat* eye_from_head_matrix_array =
      env->GetFloatArrayElements(eye_from_head_matrix, 0);
  CardboardLensDistortion_getEyeFromHeadMatrix(
      reinterpret_cast<CardboardLensDistortion*>(lens_distortion),
      CardboardEyeFromJint(eye), eye_from_head_matrix_array);
  env->ReleaseFloatArrayElements(eye_from_head_matrix,
                                 eye_from_head_matrix_array, 0);
}

JNI_METHOD(void, LensDistortion, nativeLensDistortionGetEyeProjectionMatrix)
(JNIEnv* env, jobject, jlong lens_distortion, jint eye, jfloat z_near,
 jfloat z_far, jfloatArray projection_matrix) {
  if (lens_distortion == 0) {
    return;
  }
  jfloat* projection_matrix_array =
      env->GetFloatArrayElements(projection_matrix, 0);
  CardboardLensDistortion_getProjectionMatrix(
      reinterpret_cast<CardboardLensDistortion*>(lens_distortion),
      CardboardEyeFromJint(eye), z_near, z_far, projection_matrix_array);
  env->ReleaseFloatArrayElements(projection_matrix, projection_matrix_array, 0);
}

JNI_METHOD(jobject, LensDistortion, nativeLensDistortionGetDistortionMesh)
(JNIEnv* env, jobject, jlong lens_distortion, jint eye) {
  if (lens_distortion == 0) {
    return NULL;
  }
  CardboardMesh mesh;
  CardboardLensDistortion_getDistortionMesh(
      reinterpret_cast<CardboardLensDistortion*>(lens_distortion),
      CardboardEyeFromJint(eye), &mesh);
  // Get constructor method
  constexpr const char* kConstructorSignature = "(JIJJI)V";
  const jmethodID meshConstructor =
      env->GetMethodID(mesh_class_, kConstructorMethod, kConstructorSignature);
  // Return built object
  const jlong java_indices = ToJniPointer(mesh.indices);
  const jint java_n_indices = static_cast<const jint>(mesh.n_indices);
  const jlong java_vertices = ToJniPointer(mesh.vertices);
  const jlong java_uvs = ToJniPointer(mesh.uvs);
  const jint java_n_vertices = static_cast<const jint>(mesh.n_vertices);
  return env->NewObject(mesh_class_, meshConstructor, java_indices,
                        java_n_indices, java_vertices, java_uvs,
                        java_n_vertices);
}

JNI_METHOD(jobject, LensDistortion,
           nativeLensDistortionUndistortedUvForDistortedUv)
(JNIEnv* env, jobject, jlong lens_distortion, jobject distorted_uv,
 jint eye) {
  if (lens_distortion == 0) {
    return NULL;
  }
  // Load native structure
  const CardboardUv native_distorted_uv{
      env->GetFloatField(
          distorted_uv,
          env->GetFieldID(uv_point_class_, kUvPointU, kJniFloatSignature)),
      env->GetFloatField(
          distorted_uv,
          env->GetFieldID(uv_point_class_, kUvPointV, kJniFloatSignature))};
  const CardboardUv native_undistorted_uv =
      CardboardLensDistortion_undistortedUvForDistortedUv(
          reinterpret_cast<CardboardLensDistortion*>(lens_distortion),
          &native_distorted_uv, CardboardEyeFromJint(eye));
  // Get constructor method
  constexpr const char* kConstructorSignature = "(FF)V";
  const jmethodID uvPointConstructor = env->GetMethodID(
      uv_point_class_, kConstructorMethod, kConstructorSignature);
  // Return built object
  return env->NewObject(uv_point_class_, uvPointConstructor,
                        static_cast<jfloat>(native_undistorted_uv.u),
                        static_cast<jfloat>(native_undistorted_uv.v));
}

JNI_METHOD(jobject, LensDistortion,
           nativeLensDistortionDistortedUvForUndistortedUv)
(JNIEnv* env, jobject, jlong lens_distortion, jobject undistorted_uv,
 jint eye) {
  if (lens_distortion == 0) {
    return NULL;
  }
  // Load native structure
  const CardboardUv native_undistorted_uv{
      env->GetFloatField(
          undistorted_uv,
          env->GetFieldID(uv_point_class_, kUvPointU, kJniFloatSignature)),
      env->GetFloatField(
          undistorted_uv,
          env->GetFieldID(uv_point_class_, kUvPointV, kJniFloatSignature))};
  const CardboardUv native_distorted_uv =
      CardboardLensDistortion_distortedUvForUndistortedUv(
          reinterpret_cast<CardboardLensDistortion*>(lens_distortion),
          &native_undistorted_uv, CardboardEyeFromJint(eye));
  // Get constructor method
  constexpr const char* kConstructorSignature = "(FF)V";
  const jmethodID uvPointConstructor = env->GetMethodID(
      uv_point_class_, kConstructorMethod, kConstructorSignature);
  // Return built object
  return env->NewObject(uv_point_class_, uvPointConstructor,
                        static_cast<jfloat>(native_distorted_uv.u),
                        static_cast<jfloat>(native_distorted_uv.v));
}

// Distortion Renderer API

JNI_METHOD(jlong, DistortionRenderer, nativeDistortionRendererCreate)
(JNIEnv*, jobject ) {
  CardboardOpenGlEsDistortionRendererConfig config{kGlTexture2D};
  return ToJniPointer(CardboardOpenGlEs2DistortionRenderer_create(&config));
}

JNI_METHOD(void, DistortionRenderer, nativeDistortionRendererDestroy)
(JNIEnv*, jobject, jlong distortion_renderer) {
  if (distortion_renderer == 0) {
    return;
  }
  CardboardDistortionRenderer_destroy(
      reinterpret_cast<CardboardDistortionRenderer*>(distortion_renderer));
}

JNI_METHOD(void, DistortionRenderer, nativeDistortionRendererSetMesh)
(JNIEnv* env, jobject, jlong distortion_renderer, jobject mesh, jint eye) {
  if (distortion_renderer == 0) {
    return;
  }
  // Load native structure
  const CardboardMesh native_mesh{
      reinterpret_cast<int*>(env->GetLongField(
          mesh, env->GetFieldID(mesh_class_, kMeshIndices, kJniLongSignature))),
      static_cast<int>(env->GetIntField(
          mesh, env->GetFieldID(mesh_class_, kMeshNIndices, kJniIntSignature))),
      reinterpret_cast<float*>(env->GetLongField(
          mesh,
          env->GetFieldID(mesh_class_, kMeshVertices, kJniLongSignature))),
      reinterpret_cast<float*>(env->GetLongField(
          mesh, env->GetFieldID(mesh_class_, kMeshUvs, kJniLongSignature))),
      static_cast<int>(env->GetIntField(
          mesh,
          env->GetFieldID(mesh_class_, kMeshNVertices, kJniIntSignature)))};
  CardboardDistortionRenderer_setMesh(
      reinterpret_cast<CardboardDistortionRenderer*>(distortion_renderer),
      &native_mesh, CardboardEyeFromJint(eye));
}

JNI_METHOD(void, DistortionRenderer, nativeDistortionRendererRenderEyeToDisplay)
(JNIEnv* env, jobject, jlong distortion_renderer, jlong target_display,
 jint x, jint y, jint width, jint height, jobject left_eye, jobject right_eye) {
  if (distortion_renderer == 0) {
    return;
  }
  // Load native structures
  CardboardEyeTextureDescription native_left_eye =
      CardboardEyeTextureDescriptionFromJobject(env, left_eye);
  CardboardEyeTextureDescription native_right_eye =
      CardboardEyeTextureDescriptionFromJobject(env, right_eye);

  CardboardDistortionRenderer_renderEyeToDisplay(
      reinterpret_cast<CardboardDistortionRenderer*>(distortion_renderer),
      static_cast<uint64_t>(target_display), x, y, width, height,
      &native_left_eye, &native_right_eye);
}

// Head Tracker API

JNI_METHOD(jlong, HeadTracker, nativeHeadTrackerCreate)
(JNIEnv*, jobject) {
  return ToJniPointer(CardboardHeadTracker_create());
}

JNI_METHOD(void, HeadTracker, nativeHeadTrackerDestroy)
(JNIEnv* , jobject , jlong head_tracker) {
  if (head_tracker == 0) {
    return;
  }
  CardboardHeadTracker_destroy(
      reinterpret_cast<CardboardHeadTracker*>(head_tracker));
}

JNI_METHOD(void, HeadTracker, nativeHeadTrackerPause)
(JNIEnv*, jobject, jlong head_tracker) {
  if (head_tracker == 0) {
    return;
  }
  CardboardHeadTracker_pause(
      reinterpret_cast<CardboardHeadTracker*>(head_tracker));
}

JNI_METHOD(void, HeadTracker, nativeHeadTrackerResume)
(JNIEnv*, jobject , jlong head_tracker) {
  if (head_tracker == 0) {
    return;
  }
  CardboardHeadTracker_resume(
      reinterpret_cast<CardboardHeadTracker*>(head_tracker));
}

JNI_METHOD(void, HeadTracker, nativeHeadTrackerGetPose)
(JNIEnv* env, jobject , jlong head_tracker, jlong delta_time_ns,
 jint deviceOrientation, jfloatArray position, jfloatArray orientation) {
  if (head_tracker == 0) {
    return;
  }
  jfloat* position_array = env->GetFloatArrayElements(position, 0);
  jfloat* orientation_array = env->GetFloatArrayElements(orientation, 0);
  const int64_t timestamp_ns = GetBootTimeNano() + delta_time_ns;
  CardboardHeadTracker_getPose(
      reinterpret_cast<CardboardHeadTracker*>(head_tracker), timestamp_ns,
      static_cast<CardboardViewportOrientation>(deviceOrientation),
      position_array, orientation_array);
  env->ReleaseFloatArrayElements(position, position_array, 0);
  env->ReleaseFloatArrayElements(orientation, orientation_array, 0);
}

// QR Code Scanner API

JNI_METHOD(jbyteArray, QrCode, nativeQrCodeGetSavedDeviceParams)
(JNIEnv* env, jobject) {
  uint8_t* buffer{};
  int size{};
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);
  if (size == 0) {
    return NULL;
  }
  jbyteArray device_params = env->NewByteArray(size);
  env->SetByteArrayRegion(device_params, 0, size, (jbyte*)buffer);
  CardboardQrCode_destroy(buffer);
  return device_params;
}

JNI_METHOD(jbyteArray, QrCode, nativeQrCodeGetCardboardV1DeviceParams)
(JNIEnv* env, jobject) {
  uint8_t* buffer{};
  int size{};
  CardboardQrCode_getCardboardV1DeviceParams(&buffer, &size);
  if (size == 0) {
    cardboard::jni::ThrowJavaRuntimeException(
        env,
        "An error occured while retrieving Cardboard V1 device parameters.");
    return NULL;
  }
  jbyteArray device_params = env->NewByteArray(size);
  env->SetByteArrayRegion(device_params, 0, size, (jbyte*)buffer);
  return device_params;
}

JNI_METHOD(void, QrCode, nativeQrCodeScanQrCodeAndSaveDeviceParams)
(JNIEnv*, jobject ) { CardboardQrCode_scanQrCodeAndSaveDeviceParams(); }

}  // extern "C"
