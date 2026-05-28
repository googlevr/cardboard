# Proguard rules to preserve Cardboard OSS as a dependency.
#
# Because of native to and from Java calls, Java code is preserved.
# TODO(b/266823260): Remove forked ProGuard file.
-keep class com.google.cardboard.sdk.** { *; }
-keep class com.google.cardboard.proto.** { *; }