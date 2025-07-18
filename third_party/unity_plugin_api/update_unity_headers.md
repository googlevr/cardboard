# Update Unity Plugin API

TODO: b/349102234 - Update all headers to have the same version.

The plugin API folder has a mix of headers from different Unity versions. Each interface is standalone and identified to the Unity engine with a GUID. The Unity engine supports all legacy interfaces. Unity has to support older interfaces working alongside newer ones because there may be a mix of old and new packages in a project. See each header to figure out which version it is. The version can be found at the end of the file e.g. in `IUnityXRMeshing.h`

```UNITY_REGISTER_INTERFACE_GUID(0xede929dfd83a492aULL, 0xb70be1f8ba304c69ULL, IUnityXRMeshInterface); // 2021.2.0a14 - 4/9/2021```

If there is not a version present then the version is `2020.3.25f1`.

## Update Unity XR headers

Coordinate with the [XR APIs team](go/xrapis#contact).

## Update non-XR headers

Follow these steps to update the Unity Plugin API in a gmac:

1. Download the latest Unity version.

1. Open a terminal and run:
```sh
gcert
```

1. Copy the headers directory to your workspace. Using your cloudtop instance, it would be:
```sh
scp -r $UNITY_PATH/Unity/Hub/Editor/$UNITY_VERSION/Unity.app/Contents/PluginAPI/* $YOUR_USERNAME.c.googlers.com:/google/src/cloud/$YOUR_USERNAME/$NAME_OF_YOUR_WORKSPACE/google3/third_party/unity/PluginApi
```
Where:
- `$UNITY_PATH` is the path where Unity is installed.
- `$UNITY_VERSION` is the version of the last LTS version (eg. 2020.3.25f1).
- `$YOUR_USERNAME` is your google username.
- `$NAME_OF_YOUR_WORKSPACE` is the name of your fig client.

1. In the [BUILD file](https://source.corp.google.com/piper///depot/google3/third_party/unity/PluginApi/BUILD), update the variable `PLUGIN_API_UNITY_VERSION`.
