# Update Unity Plugin API

Follow these steps to update the Unity Plugin API in a gmac:

1. Download last Unity version.

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
