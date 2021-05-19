
using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.XR;
using UnityEngine.XR.ARFoundation;

namespace Google.XR.Cardboard
{
    public class SixDoFPoseDriver : MonoBehaviour
    {
#if UNITY_ANDROID
        public const string CardboardApi = "cardboard_api";
#elif UNITY_IOS
        public const string CardboardApi = "__Internal";
#else
        public const string CardboardApi = "NOT_AVAILABLE";
#endif

        [DllImport(CardboardApi)]
        private static extern void CardboardUnity_AddSixDoFData(IntPtr ptr, Int64 timestamp, [In] float[] position, [In] float[] orientation);

        private ARCameraManager _arCameraManager;
        public ARCameraManager arCameraManager
        {
            get {
                if (!_arCameraManager)
                {
                    foreach (Camera camera in Camera.allCameras)
                    {
                        ARCameraManager _cameraManager = camera.gameObject.GetComponent<ARCameraManager>();
                        if (_cameraManager && _cameraManager.enabled)
                        {
                            _arCameraManager = _cameraManager;
                            break;
                        }
                    }
                }
                if (!_arCameraManager)
                {
                    Debug.LogError("[Aryzon] No ARCameraManager found, make sure there is one attached to an active camera.");
                }
                return _arCameraManager;
            }
            set { _arCameraManager = value; }
        }

        internal struct NullablePose
        {
            internal Vector3? position;
            internal Quaternion? rotation;
        }

        public void OnEnable()
        {
            Application.onBeforeRender += OnBeforeRender;
            arCameraManager.frameReceived += ArCameraManager_frameReceived;
#if UNITY_2020_1_OR_NEWER
            List<InputDevice> devices = new List<InputDevice>();
            InputDevices.GetDevicesWithCharacteristics(InputDeviceCharacteristics.TrackedDevice, devices);
            foreach (var device in devices)
            {
                if (device.characteristics.HasFlag(InputDeviceCharacteristics.TrackedDevice))
                {
                    CheckConnectedDevice(device, false);
                }
            }

            InputDevices.deviceConnected += OnInputDeviceConnected;
#endif // UNITY_UNITY_2020_1_OR_NEWER
        }

        public void OnDisable()
        {
            Application.onBeforeRender -= OnBeforeRender;
            arCameraManager.frameReceived -= ArCameraManager_frameReceived;

#if UNITY_2020_1_OR_NEWER
            InputDevices.deviceConnected -= OnInputDeviceConnected;
#endif // UNITY_UNITY_2020_1_OR_NEWER
        }

        void OnBeforeRender() => PerformUpdate();

        void PerformUpdate()
        {
#if !UNITY_EDITOR
            var updatedPose = GetPoseData();

            if (updatedPose.position.HasValue)
            {
                transform.localPosition = updatedPose.position.Value;
            }
            if (updatedPose.rotation.HasValue)
            {
                transform.localRotation = updatedPose.rotation.Value;
            }
#endif
        }

#if UNITY_2020_1_OR_NEWER
        static internal InputDevice? s_InputTrackingDevice = null;
        static internal InputDevice? s_CardboardHMDInputTrackingDevice = null;

        void OnInputDeviceConnected(InputDevice device) => CheckConnectedDevice(device);

        void CheckConnectedDevice(InputDevice device, bool displayWarning = true)
        {
            var positionSuccess = false;
            var rotationSuccess = false;
            if (!(positionSuccess = device.TryGetFeatureValue(CommonUsages.centerEyePosition, out Vector3 position)))
                positionSuccess = device.TryGetFeatureValue(CommonUsages.colorCameraPosition, out position);
            if (!(rotationSuccess = device.TryGetFeatureValue(CommonUsages.centerEyeRotation, out Quaternion rotation)))
                rotationSuccess = device.TryGetFeatureValue(CommonUsages.colorCameraRotation, out rotation);

            if (positionSuccess && rotationSuccess)
            {
                if (s_InputTrackingDevice == null)
                {
                    s_InputTrackingDevice = device;
                }
                else if (s_CardboardHMDInputTrackingDevice == null && device.name == "Cardboard HMD")
                {
                    s_CardboardHMDInputTrackingDevice = device;                     
                }
            }
        }

#else
        static internal List<UnityEngine.XR.XRNodeState> nodeStates = new List<UnityEngine.XR.XRNodeState>();
#endif // UNITY_2020_1_OR_NEWER

        public void AddSixDoFData(Vector3 position, Quaternion rotation, long timestampNs)
        {
            float[] positionArray = { position.x, position.y, position.z };
            float[] rotationArray = { rotation.x, rotation.y, rotation.z, rotation.w };
            CardboardUnity_AddSixDoFData(SixDoFCardboardStartup.inputPointer, timestampNs, positionArray, rotationArray);
        }

        private void ArCameraManager_frameReceived(ARCameraFrameEventArgs obj)
        {
            if (!SixDoFCardboardStartup.isStarted)
            {
            return;
            }
#if UNITY_2020_1_OR_NEWER

            if (s_InputTrackingDevice != null)
            {
                var pose = Pose.identity;
                var positionSuccess = false;
                var rotationSuccess = false;

                if (!(positionSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.centerEyePosition, out pose.position)))
                    positionSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.colorCameraPosition, out pose.position);
                if (!(rotationSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.centerEyeRotation, out pose.rotation)))
                    rotationSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.colorCameraRotation, out pose.rotation);

                if (positionSuccess && rotationSuccess)
                {
                    AddSixDoFData(pose.position, pose.rotation, (long)obj.timestampNs);
                }
            }
#else
            UnityEngine.XR.InputTracking.GetNodeStates(nodeStates);
            foreach (var nodeState in nodeStates)
            {
                if (nodeState.nodeType == UnityEngine.XR.XRNode.CenterEye)
                {
                    var pose = Pose.identity;
                    var positionSuccess = nodeState.TryGetPosition(out pose.position);
                    var rotationSuccess = nodeState.TryGetRotation(out pose.rotation);

                    if (positionSuccess && rotationSuccess)
                    {
                        AddSixDoFData(pose.position, pose.rotation, (long)obj.timestampNs);
                    }
                    break;
                }
            }
#endif
        }

        static internal NullablePose GetPoseData()
        {
            NullablePose resultPose = new NullablePose();

#if UNITY_2020_1_OR_NEWER
            if (!SixDoFCardboardStartup.isStarted && s_CardboardHMDInputTrackingDevice != null)
            {
                s_CardboardHMDInputTrackingDevice = null;
            }

            if (s_CardboardHMDInputTrackingDevice != null)
            {
                var pose = Pose.identity;
                var positionSuccess = false;
                var rotationSuccess = false;

                if (!(positionSuccess = s_CardboardHMDInputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.centerEyePosition, out pose.position)))
                    positionSuccess = s_CardboardHMDInputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.colorCameraPosition, out pose.position);
                if (!(rotationSuccess = s_CardboardHMDInputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.centerEyeRotation, out pose.rotation)))
                    rotationSuccess = s_CardboardHMDInputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.colorCameraRotation, out pose.rotation);

                if (positionSuccess)
                    resultPose.position = pose.position;
                if (rotationSuccess)
                    resultPose.rotation = pose.rotation;

                Debug.Log("x: " + pose.position.x + " y: " + pose.position.y + " z: " + pose.position.z);

                if (positionSuccess || rotationSuccess)
                    return resultPose;
            }
            else if (s_InputTrackingDevice != null)
            {
                var pose = Pose.identity;
                var positionSuccess = false;
                var rotationSuccess = false;

                if (!(positionSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.centerEyePosition, out pose.position)))
                    positionSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.colorCameraPosition, out pose.position);
                if (!(rotationSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.centerEyeRotation, out pose.rotation)))
                    rotationSuccess = s_InputTrackingDevice.Value.TryGetFeatureValue(CommonUsages.colorCameraRotation, out pose.rotation);

                if (positionSuccess)
                    resultPose.position = pose.position;
                if (rotationSuccess)
                    resultPose.rotation = pose.rotation;

                if (positionSuccess || rotationSuccess)
                    return resultPose;
            }
#else
            UnityEngine.XR.InputTracking.GetNodeStates(nodeStates);

            List<UnityEngine.XR.XRNodeState> states = new List<UnityEngine.XR.XRNodeState>();

            if (!SixDoFCardboardStartup.isStarted)
            {
                foreach (UnityEngine.XR.XRNodeState nodeState in nodeStates)
                {
                    if (nodeState.nodeType == UnityEngine.XR.XRNode.CenterEye)
                    {
                        var pose = Pose.identity;
                        var positionSuccess = nodeState.TryGetPosition(out pose.position);
                        var rotationSuccess = nodeState.TryGetRotation(out pose.rotation);

                        if (positionSuccess)
                            resultPose.position = pose.position;
                        if (rotationSuccess)
                            resultPose.rotation = pose.rotation;

                        break;
                    }
                }
            }
            else
            {
                foreach (UnityEngine.XR.XRNodeState nodeState in nodeStates)
                {
                    if (nodeState.nodeType == UnityEngine.XR.XRNode.CenterEye)
                    {
                        states.Add(nodeState);
                    }
                }

                if (nodeStates.Count > 0)
                {
                    UnityEngine.XR.XRNodeState nodeState = nodeStates[nodeStates.Count - 1];
                    var pose = Pose.identity;
                    var positionSuccess = nodeState.TryGetPosition(out pose.position);
                    var rotationSuccess = nodeState.TryGetRotation(out pose.rotation);

                    if (positionSuccess)
                        resultPose.position = pose.position;
                    if (rotationSuccess)
                        resultPose.rotation = pose.rotation;

                    return resultPose;
                }
            }
#endif // UNITY_2020_1_OR_NEWER
            return resultPose;
        }
    }
}
