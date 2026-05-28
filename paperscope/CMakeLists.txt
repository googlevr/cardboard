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
project(paperscope VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

# C++ flags.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra)

# dependencies
find_library(ANDROID_LIB android)
find_library(GLESv2_LIB GLESv2)
find_library(GLESv3_LIB GLESv3)
find_library(LOG_LIB log)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
    GIT_TAG        VER-2-13-2
)

FetchContent_MakeAvailable(freetype)

set(CARDBOARD_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sdk)
set(CARDBOARD_JAVA_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../java_api)
set(PAPERSCOPE_LAUNCHER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/paperscope_launcher)

# Path to the prebuilt Cardboard SDK library extracted by Gradle
set(CARDBOARD_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libraries/jni/${ANDROID_ABI}/libGfxPluginCardboard.so)

# Generate localization strings
set(GENSTRINGS_SCRIPT "${PAPERSCOPE_LAUNCHER_DIR}/common/tools/genstrings.py")
set(RESOURCE_FILE "${PAPERSCOPE_LAUNCHER_DIR}/resources/strings/en.lproj/cardboard.strings")
set(NAMESPACE "cardboard")
set(OUT_DIR "${PAPERSCOPE_LAUNCHER_DIR}/generated_strings")

file(MAKE_DIRECTORY ${OUT_DIR})

add_custom_command(
    OUTPUT
        ${OUT_DIR}/${NAMESPACE}_strings.h
        ${OUT_DIR}/${NAMESPACE}_strings.cc
    COMMAND python3 ${GENSTRINGS_SCRIPT}
            --resource_file=${RESOURCE_FILE}
            --namespace=${NAMESPACE}
            --out_dir=${OUT_DIR}
            --language=cc
    DEPENDS
        ${GENSTRINGS_SCRIPT}
        ${RESOURCE_FILE}
    COMMENT "Generating ${NAMESPACE}_strings C++ files..."
)

# Explicitly set this to STATIC so CMake knows how to build it
add_library(cardboard_strings STATIC
    ${OUT_DIR}/${NAMESPACE}_strings.cc
    ${OUT_DIR}/${NAMESPACE}_strings.h
)
target_include_directories(cardboard_strings PUBLIC ${OUT_DIR})

# Sources
file(GLOB PAPERSCOPE_SOURCES "${PAPERSCOPE_LAUNCHER_DIR}/src/*.cc")

add_library(cardboard_jni SHARED
    ${PAPERSCOPE_SOURCES}
    ${PAPERSCOPE_LAUNCHER_DIR}/common/strings/localized_strings.cc
    ${CARDBOARD_SDK_DIR}/util/matrix_4x4.cc
)

# Includes
target_include_directories(cardboard_jni PRIVATE
    ${PAPERSCOPE_LAUNCHER_DIR}/src
    ${PAPERSCOPE_LAUNCHER_DIR}/common/strings
    ${CARDBOARD_SDK_DIR}
    ${CARDBOARD_SDK_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glm
)

add_library(cardboard_sdk_jni SHARED
    ${CARDBOARD_JAVA_API_DIR}/src/main/jni/cardboard_jni.cc
    ${CARDBOARD_SDK_DIR}/jni_utils/android/jni_utils.cc
)

target_include_directories(cardboard_sdk_jni PRIVATE
    ${CARDBOARD_SDK_DIR}
)

# Link the actual Cardboard library and Android system libraries
target_link_libraries(cardboard_sdk_jni PRIVATE
    ${CARDBOARD_LIB}
    ${ANDROID_LIB}
    ${LOG_LIB}
    ${GLESv2_LIB}
)

# Build
target_link_libraries(cardboard_jni PRIVATE
    ${ANDROID_LIB}
    ${GLESv2_LIB}
    ${GLESv3_LIB}
    ${LOG_LIB}
    ${CARDBOARD_LIB}
    cardboard_sdk_jni
    cardboard_strings
    freetype
)

target_link_options(cardboard_sdk_jni PRIVATE "-Wl,-z,max-page-size=16384")
target_link_options(cardboard_jni PRIVATE "-Wl,-z,max-page-size=16384")
