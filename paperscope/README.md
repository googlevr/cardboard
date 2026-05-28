# Cardboard Demo App (Paperscope)

This app is the Android demo application for Google Cardboard. It is based on the public [Cardboard Demo App](https://play.google.com/store/apps/details?id=com.google.samples.apps.cardboarddemo) available on the Google Play Store, but without the Exhibit demo.

## Features

The Demo App showcases various capabilities of the Cardboard SDK, such as:

*   **Photosphere**: Immersive 360-degree photo viewer.
*   **Magic Window**: A window into a virtual world using the device's sensors.
*   **My Videos**: Media viewer for immersive/stereoscopic video content.

## Requirements

*   **OpenJDK 17** (`openjdk@17`)
*   **Android Studio** or Gradle
*   **ADB** (Android Debug Bridge) available in your `PATH`
*   A physical Android device (recommended for optimal head tracking and VR rendering)

## Building and Running

### Using Android Studio
1.  Open Android Studio.
2.  Select **Open an existing project** and choose the directory containing the cloned Cardboard repository.
3.  Wait for the project to sync with the Gradle files.
4.  From the run configurations dropdown, select the **paperscope** configuration.
5.  Connect your Android device via USB (with USB Debugging enabled).
6.  Click **Run** or press `Shift+F10` to install and launch the app on your device.

### Using Gradle (Command Line)
To build and install the app using the Gradle wrapper from the command line,
run:

```shell
./gradlew :sdk:assembleDebug :paperscope:assembleDebug
adb install -r paperscope/build/outputs/apk/debug/paperscope-debug.apk
```

Make sure `adb` is connected and authorized on your device.
