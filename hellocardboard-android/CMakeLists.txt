# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.4.1)

# C++ flags.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra)

# Standard Android dependencies
find_library(android-lib android)
find_library(GLESv2-lib GLESv2)
find_library(GLESv3-lib GLESv3)
find_library(log-lib log)

set(libs_dir ${CMAKE_CURRENT_SOURCE_DIR}/libraries)

# === Cardboard Sample ===
# Sources
file(GLOB native_srcs "src/main/jni/*.cc")
# Output binary
add_library(cardboard_jni SHARED ${native_srcs})
# Includes
target_include_directories(cardboard_jni PRIVATE ${libs_dir})
# Build
target_link_libraries(cardboard_jni
    ${android-lib}
    ${GLESv2-lib}
    ${GLESv3-lib}
    ${log-lib}
    ${libs_dir}/jni/${ANDROID_ABI}/libGfxPluginCardboard.so)
