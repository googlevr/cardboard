using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

using UnityEngine;
using UnityEngine.XR;

namespace Google.XR.Cardboard
{
    /// <summary>
    /// Initializes Cardboard XR Plugin for 6DoF use.
    /// </summary>
    public class SixDoFCardboardStartup : MonoBehaviour
    {
        private static IntPtr _inputPointer;
        public static IntPtr inputPointer
        {
            get { if (isStarted) { return _inputPointer; } else { return IntPtr.Zero; } }
            set { _inputPointer = value; }
        }

        private static IntPtr _displayPointer;
        public static IntPtr displayPointer
        {
            get { if (isStarted) { return _displayPointer; } else { return IntPtr.Zero; } }
            set { _displayPointer = value; }
        }

        private XRLoader loader;

        public static bool isInitialized = false;
        public static bool isStarted = false;

        private string inputMatch = "Input";

        private void Start()
        {
            StartCardboard();
        }

        public void StartCardboard()
        {
            // Configures the app to not shut down the screen and sets the brightness to maximum.
            // Brightness control is expected to work only in iOS, see:
            // https://docs.unity3d.com/ScriptReference/Screen-brightness.html.
            Screen.sleepTimeout = SleepTimeout.NeverSleep;
            Screen.brightness = 1.0f;
            
            if (!loader)
            {
                loader = new XRLoader();
            }
#if !UNITY_EDITOR
            loader.Initialize();
#endif
            loader.Start();
            ConnectCardboardInputSystem();

            isStarted = true;

            ReloadDeviceParams();

            if (!Api.HasDeviceParams())
            {
                Api.ScanDeviceParams();
            }
        }

        public void StopCardboard()
        {
            if (loader)
            {
                loader.Stop();
                loader.Deinitialize();
            }
            isStarted = false;
        }

        public void ReloadDeviceParams()
        {
            if (!isStarted)
            {
                return;
            }
            Api.ReloadDeviceParams();
        }

        public void Update()
        {
            if (!isStarted)
            {
                return;
            }

            if (Api.IsGearButtonPressed)
            {
                Api.ScanDeviceParams();
            }

            if (Api.IsCloseButtonPressed)
            {
                Application.Quit();
            }

            if (Api.HasNewDeviceParams())
            {
                Api.ReloadDeviceParams();
            }

            Api.UpdateScreenParams();
        }

        private void ConnectCardboardInputSystem()
        {
            List<XRInputSubsystemDescriptor> inputs = new List<XRInputSubsystemDescriptor>();
            SubsystemManager.GetSubsystemDescriptors(inputs);

            foreach (var d in inputs)
            {
                if (d.id.Equals(inputMatch))
                {
                    XRInputSubsystem inputInst = d.Create();

                    if (inputInst != null)
                    {
                        GCHandle handle = GCHandle.Alloc(inputInst);
                        inputPointer = GCHandle.ToIntPtr(handle);
                    }
                }
            }
        }
    }
}
