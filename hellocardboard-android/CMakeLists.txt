cmake_minimum_required(VERSION 3.4.1)

# Standard Android dependencies
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv2-lib GLESv2)
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
    ${EGL-lib}
    ${GLESv2-lib}
    ${log-lib}
    ${libs_dir}/jni/${ANDROID_ABI}/libcardboard_api.so)
