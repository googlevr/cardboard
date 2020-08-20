// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
/// @file UnityXRDisplayStats.h
/// @brief Use these constants with the XRStats interface to expose common Display subsystem stats to the Unity C# API
/// @see IUnityXRStats.h

/// Report the time spent by the GPU for the application last frame, in seconds
const char* const kUnityStatsGPUTimeApp = "GPUAppLastFrameTime";

/// Report the time spent by the GPU for the compositor last frame, in seconds
const char* const kUnityStatsGPUTimeCompositor = "GPUCompositorLastFrameTime";

/// Report the number of dropped frames
const char* const kUnityStatsDroppedFrameCount = "droppedFrameCount";

/// Report the number of times the current frame has been drawn to the device.
const char* const kUnityStatsFramePresentCount = "framePresentCount";

/// Report the HMD’s Refresh rate in frames per second
const char* const kUnityStatsDisplayRefreshRate = "displayRefreshRate";

/// Report the latency from when the last predicted tracking information was queried by the application to
/// when the middle scanline of the target frame is illuminated on the HMD display
const char* const kUnityStatsMotionToPhoton = "motionToPhoton";
