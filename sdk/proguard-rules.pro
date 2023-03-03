# Proguard rules to preserve Cardboard OSS as a dependency.

# Keep classes, methods, and fields that are accessed with JNI.
-keep class com.google.cardboard.sdk.UsedByNative
-keepclasseswithmembers,includedescriptorclasses class ** {
  @com.google.cardboard.sdk.UsedByNative *;
}

# According to the ProGuard version being used, `-shrinkunusedprotofields`
# flag can be added to enable protobuf-related optimizations.
-keep class com.google.cardboard.proto.** { *; }
