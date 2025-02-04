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

cmake_minimum_required(VERSION 3.22.1)
project(cardboard_sdk VERSION 1.0.0 LANGUAGES CXX)

# C++ flags.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra)

# If these are already defined, the existing value is used, rather than the default here.
option(CARDBOARDSDK_RENDERING_GLESv3 "Enable rendering with GLESv3" ON)
option(CARDBOARDSDK_RENDERING_VULKAN "Enable rendering with Vulkan" ON)
option(CARDBOARDSDK_UNITY_PLUGIN "Enable use as a Unity plugin" ON)

# Standard Android dependencies
find_library(ANDROID_LIB android)
find_library(GLESv2_LIB GLESv2)
find_library(LOG_LIB log)

# #gles3 - Library is only needed if OpenGL ES 3.0 support is desired. Remove
# the following line if OpenGL ES 3.0 support is not needed.
if(CARDBOARDSDK_RENDERING_GLESv3)
    find_library(GLESv3_LIB GLESv3)
endif()

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
file(GLOB rendering_opengl_srcs "rendering/opengl_*.cc")

if(CARDBOARDSDK_RENDERING_VULKAN)
    # #This is required for Vulkan rendering.
    file(GLOB rendering_vulkan_srcs "rendering/android/*.cc")
    file(GLOB rendering_vulkan_wrapper_srcs "rendering/android/vulkan/*.cc")
endif()

if(CARDBOARDSDK_UNITY_PLUGIN)
    # === Cardboard Unity JNI ===
    file(GLOB cardboard_unity_jni_srcs "unity/android/*.cc")

    # === Cardboard Unity Wrapper ===
    file(GLOB cardboard_xr_unity_srcs
    "unity/xr_unity_plugin/*.cc"
    "unity/xr_unity_plugin/vulkan/*.cc"
    )

    # === Cardboard XR Provider for Unity ===
    file(GLOB cardboard_xr_provider_srcs "unity/xr_provider/*.cc")
endif()

# Output binary
add_library(GfxPluginCardboard SHARED
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
    ${rendering_opengl_srcs}
    # #vulkan This is required for Vulkan rendering. Remove the following two
    # lines if Vulkan rendering is not needed.
    ${rendering_vulkan_srcs}
    ${rendering_vulkan_wrapper_srcs}
    # Cardboard Unity JNI sources
    ${cardboard_unity_jni_srcs}
    # Cardboard Unity Wrapper sources
    ${cardboard_xr_unity_srcs}
    # Cardboard XR Provider for Unity sources
    ${cardboard_xr_provider_srcs})

if(CARDBOARDSDK_UNITY_PLUGIN)
    # Includes
    target_include_directories(GfxPluginCardboard
        PRIVATE ../third_party/unity_plugin_api)
endif()

if(CARDBOARDSDK_RENDERING_VULKAN)
    # #This is required for Vulkan rendering.
    target_compile_definitions(GfxPluginCardboard PUBLIC VK_USE_PLATFORM_ANDROID_KHR=1)
endif()

# Build
target_link_libraries(GfxPluginCardboard
    PUBLIC
    ${ANDROID_LIB}
    ${GLESv2_LIB}
    ${LOG_LIB}
)

if(CARDBOARDSDK_RENDERING_VULKAN)
    target_link_libraries(GfxPluginCardboard
        PUBLIC
        # #This is required for Vulkan rendering (it is required to load
        # libvulkan.so at runtime).
        dl
    )
endif()

if(CARDBOARDSDK_RENDERING_GLESv3)
    target_link_libraries(GfxPluginCardboard
        PUBLIC
        # #Library is only needed if OpenGL ES 3.0 support is desired.
        ${GLESv3_LIB}
    )
endif()

set_property(
    TARGET GfxPluginCardboard
    APPEND_STRING
    PROPERTY
        LINK_FLAGS
        " -Wl,--version-script=\"${CMAKE_CURRENT_SOURCE_DIR}/cardboard_api.lds\""
)
set_property(
    TARGET GfxPluginCardboard
    APPEND
    PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cardboard_api.lds"
)
