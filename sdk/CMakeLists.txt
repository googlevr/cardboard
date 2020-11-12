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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Standard Android dependencies
find_library(android-lib android)
find_library(GLESv2-lib GLESv2)
find_library(log-lib log)

# #gles3 - Library is only needed if OpenGL ES 3.0 support is desired. Remove
# the following line if OpenGL ES 3.0 support is not needed.
find_library(GLESv3-lib GLESv3)

include_directories(.)

# === Cardboard API ===
# Cardboard V1 sources
file(GLOB cardboard_v1_srcs "qrcode/cardboard_v1/*.cc")
# General Sources
file(GLOB general_srcs "*.cc")
# Sensors Sources
file(GLOB sensors_srcs "sensors/*.cc")
file(GLOB sensors_android_srcs "sensors/android/*.cc")
# JNI Util Sources
file(GLOB jni_util_srcs "jni_utils/android/*.cc")
# Util Sources
file(GLOB util_srcs "util/*.cc")
# QR Code Sources
file(GLOB qrcode_srcs "qrcode/android/*.cc")
# Screen Params Sources
file(GLOB screen_params_srcs "screen_params/android/*.cc")
# Device Params Sources
file(GLOB device_params_srcs "device_params/android/*.cc")
# Rendering Sources
file(GLOB rendering_srcs "rendering/*.cc")

# === Cardboard Unity JNI ===
file(GLOB cardboard_unity_jni_srcs "unity/android/*.cc")

# === Cardboard Unity Wrapper ===
file(GLOB cardboard_xr_unity_srcs "unity/xr_unity_plugin/*.cc")

# === Cardboard XR Provider for Unity ===
file(GLOB cardboard_xr_provider_srcs "unity/xr_provider/*.cc")

# Output binary
add_library(cardboard_api SHARED
    # Cardboard API sources.
    ${cardboard_v1_srcs}
    ${general_srcs}
    ${sensors_srcs}
    ${sensors_android_srcs}
    ${jni_util_srcs}
    ${util_srcs}
    ${qrcode_srcs}
    ${screen_params_srcs}
    ${device_params_srcs}
    ${rendering_srcs}
    # Cardboard Unity JNI sources
    ${cardboard_unity_jni_srcs}
    # Cardboard Unity Wrapper sources
    ${cardboard_xr_unity_srcs}
    # Cardboard XR Provider for Unity sources
    ${cardboard_xr_provider_srcs})

# Includes
target_include_directories(cardboard_api
    PRIVATE ../third_party/unity_plugin_api)

# Build
target_link_libraries(cardboard_api
    ${android-lib}
    ${GLESv2-lib}
    # #gles3 - Library is only needed if OpenGL ES 3.0 support is desired.
    # Remove the following line if OpenGL ES 3.0 support is not needed.
    ${GLESv3-lib}
    ${log-lib})
