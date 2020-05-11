cmake_minimum_required(VERSION 3.4.1)

# Standard Android dependencies
find_library(android-lib android)
find_library(GLESv2-lib GLESv2)
find_library(log-lib log)

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
# Output binary
add_library(cardboard_api SHARED
    ${cardboard_v1_srcs}
    ${general_srcs}
    ${sensors_srcs}
    ${sensors_android_srcs}
    ${jni_util_srcs}
    ${util_srcs}
    ${qrcode_srcs}
    ${screen_params_srcs}
    ${device_params_srcs})
# Includes
target_include_directories(cardboard_api
    PRIVATE ${PROTO_PATH} ${PROTO_PATH}/libprotobuf/android/include)
# Build
target_link_libraries(cardboard_api
    ${android-lib}
    ${GLESv2-lib}
    ${log-lib})

# === Cardboard Unity JNI ===
# Sources
file(GLOB cardboard_unity_jni_srcs "unity/android/*.cc")
# Output binary
add_library(cardboard_unity_jni SHARED ${cardboard_unity_jni_srcs})
# Build
target_link_libraries(cardboard_unity_jni ${android-lib} ${log-lib} cardboard_api)

# === Cardboard Unity Wrapper ===
# Sources
file(GLOB cardboard_xr_unity_srcs "unity/xr_unity_plugin/*.cc")
# Output binary
add_library(cardboard_xr_unity SHARED ${cardboard_xr_unity_srcs})
# Build
target_link_libraries(cardboard_xr_unity
  cardboard_api
  ${android-lib}
  ${GLESv2-lib}
  ${log-lib})
