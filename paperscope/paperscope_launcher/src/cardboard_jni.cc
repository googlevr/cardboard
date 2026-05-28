/*
 * Copyright 2024 Google LLC
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
#include <string>

#include "cardboard_launcher.h"
#include "magic_window.h"
#include "main.h"
#include "settings.h"

#define JNI_LAUNCHER_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL                       \
      Java_com_google_vr_cardboard_paperscope_demo_CardboardLauncherActivity_##method_name  // NOLINT

#define JNI_MAGIC_WINDOW_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL                           \
      Java_com_google_vr_cardboard_paperscope_demo_magicwindow_MagicWindowActivity_##method_name  // NOLINT

#define JNI_SETTINGS_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL                           \
      Java_com_google_vr_cardboard_paperscope_demo_settings_SettingsActivity_##method_name  // NOLINT

#define JNI_MAIN_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL                       \
      Java_com_google_vr_cardboard_paperscope_demo_MainActivity_##method_name  // NOLINT

namespace {

inline jlong launcher_jptr(cardboard_launcher::CardboardLauncher* native_app) {
  return reinterpret_cast<intptr_t>(native_app);
}

inline cardboard_launcher::CardboardLauncher* launcher_native(jlong ptr) {
  return reinterpret_cast<cardboard_launcher::CardboardLauncher*>(ptr);
}

inline jlong magic_window_jptr(magic_window::MagicWindow* native_app) {
  return reinterpret_cast<intptr_t>(native_app);
}

inline magic_window::MagicWindow* magic_window_native(jlong ptr) {
  return reinterpret_cast<magic_window::MagicWindow*>(ptr);
}

inline jlong settings_jptr(settings::Settings* native_app) {
  return reinterpret_cast<intptr_t>(native_app);
}

inline settings::Settings* settings_native(jlong ptr) {
  return reinterpret_cast<settings::Settings*>(ptr);
}

inline jlong main_jptr(main::Main* native_app) {
  return reinterpret_cast<intptr_t>(native_app);
}

inline main::Main* main_native(jlong ptr) {
  return reinterpret_cast<main::Main*>(ptr);
}

JavaVM* javaVm;

}  // anonymous namespace

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
  javaVm = vm;
  return JNI_VERSION_1_6;
}

// Cardboard Launcher Functions
JNI_LAUNCHER_METHOD(jlong, nativeOnCreate)
(JNIEnv* /*env*/, jobject obj, jobject asset_mgr) {
  return launcher_jptr(
      new cardboard_launcher::CardboardLauncher(javaVm, obj, asset_mgr));
}

JNI_LAUNCHER_METHOD(void, nativeOnDestroy)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  delete launcher_native(native_app);
}

JNI_LAUNCHER_METHOD(void, nativeOnSurfaceCreated)
(JNIEnv* env, jobject /*obj*/, jlong native_app) {
  launcher_native(native_app)->OnSurfaceCreated(env);
}

JNI_LAUNCHER_METHOD(void, nativeOnDrawFrame)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  launcher_native(native_app)->OnDrawFrame();
}

JNI_LAUNCHER_METHOD(void, nativeOnTriggerEvent)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  launcher_native(native_app)->OnTriggerEvent();
}

JNI_LAUNCHER_METHOD(void, nativeOnPause)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  launcher_native(native_app)->OnPause();
}

JNI_LAUNCHER_METHOD(void, nativeOnResume)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  launcher_native(native_app)->OnResume();
}

JNI_LAUNCHER_METHOD(void, nativeSetScreenParams)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app, jint width, jint height) {
  launcher_native(native_app)->SetScreenParams(width, height);
}

JNI_LAUNCHER_METHOD(void, nativeSwitchViewer)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  launcher_native(native_app)->SwitchViewer();
}

JNI_LAUNCHER_METHOD(bool, nativeShouldLaunchDemo)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  return launcher_native(native_app)->ShouldLaunchDemo();
}

JNI_LAUNCHER_METHOD(jstring, nativeDemoToLaunch)
(JNIEnv* env, jobject /*obj*/, jlong native_app) {
  return env->NewStringUTF(
      (launcher_native(native_app)->DemoToLaunch()).c_str());
}

JNI_LAUNCHER_METHOD(void, nativeDemoLaunched)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  launcher_native(native_app)->DemoLaunched();
}

// Cardboard Magic Window Functions
JNI_MAGIC_WINDOW_METHOD(jlong, nativeOnCreate)
(JNIEnv* /*env*/, jobject obj, jobject asset_mgr) {
  return magic_window_jptr(
      new magic_window::MagicWindow(javaVm, obj, asset_mgr));
}

JNI_MAGIC_WINDOW_METHOD(void, nativeOnDestroy)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  delete magic_window_native(native_app);
}

JNI_MAGIC_WINDOW_METHOD(void, nativeOnSurfaceCreated)
(JNIEnv* env, jobject /*obj*/, jlong native_app) {
  magic_window_native(native_app)->OnSurfaceCreated(env);
}

JNI_MAGIC_WINDOW_METHOD(void, nativeOnDrawFrame)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  magic_window_native(native_app)->OnDrawFrame();
}

JNI_MAGIC_WINDOW_METHOD(void, nativeOnResume)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  magic_window_native(native_app)->OnResume();
}

JNI_MAGIC_WINDOW_METHOD(void, nativeOnPause)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  magic_window_native(native_app)->OnPause();
}

JNI_MAGIC_WINDOW_METHOD(void, nativeSetScreenParams)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app, jint width, jint height) {
  magic_window_native(native_app)->SetScreenParams(width, height);
}

JNI_MAGIC_WINDOW_METHOD(void, nativeScanViewer)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  magic_window_native(native_app)->ScanViewer();
}

JNI_MAGIC_WINDOW_METHOD(bool, nativeHasSavedDeviceParams)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  return magic_window_native(native_app)->HasSavedDeviceParams();
}

// Cardboard Settings Functions
JNI_SETTINGS_METHOD(jlong, nativeOnCreate)
(JNIEnv* /*env*/, jobject obj, jobject asset_mgr) {
  return settings_jptr(
      new settings::Settings(javaVm, obj, asset_mgr));
}

JNI_SETTINGS_METHOD(void, nativeSwitchViewer)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  settings_native(native_app)->SwitchViewer();
}

// Cardboard Settings Functions
JNI_MAIN_METHOD(jlong, nativeOnCreate)
(JNIEnv* /*env*/, jobject obj, jobject asset_mgr) {
  return main_jptr(
      new main::Main(javaVm, obj, asset_mgr));
}

JNI_MAIN_METHOD(void, nativeSwitchViewer)
(JNIEnv* /*env*/, jobject /*obj*/, jlong native_app) {
  main_native(native_app)->SwitchViewer();
}

}  // extern "C"
