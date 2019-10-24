cmake_minimum_required(VERSION 3.4.1)

# Standard Android dependencies
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv2-lib GLESv2)
find_library(log-lib log)

include_directories(.)

# === Cardboard Device Proto Library  ===
# Sources
file(GLOB cardboard_device_proto_srcs "${PROTO_PATH}/cardboard_device.pb.cc")
# Output binary
add_library(cardboard_device_proto SHARED ${cardboard_device_proto_srcs})
# Includes
target_include_directories(cardboard_device_proto
    PRIVATE ${PROTO_PATH}/libprotobuf/android/include)
# Build
target_link_libraries(cardboard_device_proto
    ${log-lib}
    ${PROTO_PATH}/libprotobuf/android/lib/${ANDROID_ABI}/libprotobuf-lite.a)

# === Cardboard V1 JNI ===
# Sources
file(GLOB qrcode_jni_srcs "qrcode/android/jni/*.cc")
file(GLOB cardboard_v1_srcs "qrcode/cardboard_v1/*.cc")
# Output binary
add_library(cardboard_v1_jni
    SHARED ${qrcode_jni_srcs} ${cardboard_v1_srcs})
# Includes
target_include_directories(cardboard_v1_jni
    PRIVATE ${PROTO_PATH} ${PROTO_PATH}/libprotobuf/android/include)
# Build
target_link_libraries(cardboard_v1_jni cardboard_device_proto)

# === Cardboard API ===
# General Sources
file(GLOB general_srcs "*.cc")
# Sensors Sources
file(GLOB sensors_srcs "sensors/*.cc")
file(GLOB sensors_android_srcs "sensors/android/*.cc")
# Util Sources
file(GLOB util_srcs "util/*.cc")
# QR Code Sources
file(GLOB qrcode_srcs "qrcode/android/*.cc")
# Screen Params Sources
file(GLOB screen_params_srcs "screen_params/android/*.cc")
# Output binary
add_library(cardboard_api SHARED
    ${general_srcs}
    ${sensors_srcs}
    ${sensors_android_srcs}
    ${util_srcs}
    ${qrcode_srcs}
    ${screen_params_srcs})
# Includes
target_include_directories(cardboard_api
    PRIVATE ${PROTO_PATH} ${PROTO_PATH}/libprotobuf/android/include)
# Build
target_link_libraries(cardboard_api
    ${android-lib}
    ${EGL-lib}
    ${GLESv2-lib}
    ${log-lib}
    cardboard_device_proto)
