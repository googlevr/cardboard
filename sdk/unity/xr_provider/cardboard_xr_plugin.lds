VERS_1.0 {
  global:
    # Exports from cardboard_xr_plugin
    UnityPluginLoad*;
    *LoadDisplay*;
    *LoadInput*;
    *CardboardRotationToUnityPose*;
    *CardboardTransformToUnityPose*;

    # Exports from unity_jni_android
    JNI_OnLoad;
    getJavaVm;
    CardboardUnity_initializeAndroid;

    # Exports from cardboard_xr_unity
    CardboardUnity_setScreenParams;
    CardboardUnity_setWidgetCount;
    CardboardUnity_setWidgetParams;
    CardboardUnity_setDeviceParametersChanged;

    # Exports from cardboard api
    Cardboard*;

  local:
   *;
};