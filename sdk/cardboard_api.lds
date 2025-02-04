VERS_1.0 {
  global:
    # Export Cardboard Symbols.
    Cardboard*;
    *nativeIncrementDeviceParamsChangedCount;
    *JNIEnv*;
    *JavaVM*;
    JNI_OnLoad;
    *Unity*;
    RenderAPI_Vulkan_OnPluginLoad;

  # Hide everything else.
  local:
    *;
};
