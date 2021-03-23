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

#include <memory>

#include "hello_cardboard_app.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_google_cardboard_VrActivity_##method_name

namespace {

inline jlong jptr(ndk_hello_cardboard::HelloCardboardApp* native_app) {
  return reinterpret_cast<intptr_t>(native_app);
}

inline ndk_hello_cardboard::HelloCardboardApp* native(jlong ptr) {
  return reinterpret_cast<ndk_hello_cardboard::HelloCardboardApp*>(ptr);
}

JavaVM* javaVm;

}  // anonymous namespace

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  javaVm = vm;
  return JNI_VERSION_1_6;
}

JNI_METHOD(jlong, nativeOnCreate)
(JNIEnv* env, jobject obj, jobject asset_mgr) {
  return jptr(new ndk_hello_cardboard::HelloCardboardApp(javaVm, obj, asset_mgr));
}

JNI_METHOD(void, nativeOnDestroy)
(JNIEnv* env, jobject obj, jlong native_app) { delete native(native_app); }

JNI_METHOD(void, nativeOnSurfaceCreated)
(JNIEnv* env, jobject obj, jlong native_app) {
  native(native_app)->OnSurfaceCreated(env);
}

JNI_METHOD(void, nativeStartFrame)
(JNIEnv* env, jobject obj, jlong native_app) {
    native(native_app)->StartFrame();
}

// REF https://stackoverflow.com/a/1086633
JNI_METHOD(void, nativeGetParams)
(JNIEnv* env, jobject obj,  jlong native_app, jobject return_obj)
{
    //Get the Params struct from the C++ app
    ndk_hello_cardboard::AppParams_t app_params = native(native_app)->GetAppParams();

    // Get the class of the input object
    jclass clazz = env->GetObjectClass(return_obj);

    // Set the simple fields for object
    env->SetIntField(return_obj, env->GetFieldID(clazz, "screenWidth", "I"),  app_params.screen_width);
    env->SetIntField(return_obj, env->GetFieldID(clazz, "screenHeight", "I"), app_params.screen_height);

    // Set the array fields

    // Transfer head_view array Matrix4x4 -> jfloatArray[16]
    float* c_arr      = app_params.headView.ToGlArray().data();  //data coming in from C
    jfloatArray j_arr = env->NewFloatArray(16);            //data going out to Java
    // Get the object field, returns JObject (because Array is instance of Object)
    // move from the temp structure to the java structure
    env->SetFloatArrayRegion(j_arr, 0, 16, reinterpret_cast<const jfloat *>(c_arr));
    // Cast it to a jdoublearray
    env->SetObjectField(return_obj, env->GetFieldID(clazz, "headView", "[F"), j_arr);

}

JNI_METHOD(void, nativeFinishFrame)
(JNIEnv* env, jobject obj, jlong native_app) {
  native(native_app)->FinishFrame();
}

JNI_METHOD(void, nativeOnTriggerEvent)
(JNIEnv* env, jobject obj, jlong native_app) {
  native(native_app)->OnTriggerEvent();
}

JNI_METHOD(void, nativeOnPause)
(JNIEnv* env, jobject obj, jlong native_app) { native(native_app)->OnPause(); }

JNI_METHOD(void, nativeOnResume)
(JNIEnv* env, jobject obj, jlong native_app) { native(native_app)->OnResume(); }

JNI_METHOD(void, nativeSetScreenParams)
(JNIEnv* env, jobject obj, jlong native_app, jint width, jint height) {
  native(native_app)->SetScreenParams(width, height);
}

JNI_METHOD(void, nativeSwitchViewer)
(JNIEnv* env, jobject obj, jlong native_app) {
  native(native_app)->SwitchViewer();
}

JNI_METHOD(void, nativeSetVsyncEnabled)
(JNIEnv * env, jobject obj, jlong native_app, jboolean patch_enabled) {
  native(native_app)->SetVsyncEnabled(patch_enabled);
}

JNI_METHOD(bool, nativeGetVsyncEnabled)
(JNIEnv * env, jobject obj, jlong native_app) {
  return native(native_app)->GetVsyncEnabled();
}

}  // extern "C"
