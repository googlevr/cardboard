# Proguard rules to preserve Cardboard OSS as a dependency.
#
# Because of native to and from Java calls, Java code is preserved.
-keep class com.google.cardboard.sdk.** { *; }
-keep class com.google.cardboard.proto.** { *; }
