/*
 * Copyright 2025 Google LLC
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
#ifndef PAPERSCOPE_LAUNCHER_SRC_SETTINGS_H_
#define PAPERSCOPE_LAUNCHER_SRC_SETTINGS_H_

#include <android/asset_manager.h>
#include <jni.h>

#include "cardboard.h"
#include "util.h"


namespace settings {

/**
 * Handles the native interface calls for the Settings Activity of the Cardboard
 * sample app.
 */
class Settings {
 public:
  /**
   * Creates the Native interface for the Settings Activity of the sample app.
   *
   * @param vm JavaVM pointer.
   * @param obj Android activity object.
   * @param asset_mgr_obj The asset manager object.
   */
  Settings(JavaVM* vm, jobject obj, jobject asset_mgr_obj);

  ~Settings();

  /**
   * Allows user to scan a Cardboard viewer.
   */
  void SwitchViewer();

 private:
  jobject java_asset_mgr_;
  AAssetManager* asset_mgr_;
};

}  // namespace settings

#endif  // PAPERSCOPE_LAUNCHER_SRC_SETTINGS_H_
