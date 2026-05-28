/*
 * Copyright 2016 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.cardboard.sdk;

import android.opengl.EGL14;
import android.opengl.GLSurfaceView.EGLContextFactory;
import android.opengl.GLSurfaceView.EGLWindowSurfaceFactory;
import android.os.Build;
import android.util.Log;

import java.nio.IntBuffer;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

/**
 * @hide
 * Factory class for creating contexts and surfaces.
 */
public class EglFactory implements EGLContextFactory, EGLWindowSurfaceFactory {
  private static final String TAG = "GvrEglFactory";
  public static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
  private static final int EGL_CONTEXT_PRIORITY_LEVEL = 0x3100;
  private static final int EGL_CONTEXT_PRIORITY_HIGH = 0x3101;
  private static final int EGL_PROTECTED_CONTENT_EXT = 0x32C0;
  private static final int EGL_CONTEXT_OPENGL_NO_ERROR_KHR = 0x31B3;
  private static final int EGL_CONTEXT_FLAGS_KHR = 0x30FC;
  private static final int EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR = 0x00000001;
  private static final int MIN_REQUIRED_CONTEXT_CLIENT_VERSION = 2;

  private boolean usePriority = false;
  private boolean useProtected = false;
  private boolean useDebug = false;
  private boolean errorReportingEnabled = true;
  private int eglContextClientVersion = 2;
  private EGLContext sharedContext = EGL10.EGL_NO_CONTEXT;

  public EglFactory() {
  }

  /**
   * Sets whether priority contexts should be created.
   *
   * <p> Disabled by default. This must be called before calling setRenderer.
   *
   * @param enabled Whether to create priority contexts.
   */
  public void setUsePriorityContext(boolean enabled) {
    usePriority = enabled;
  }

  /**
   * Sets whether protected buffers should be used.
   *
   * <p> Disabled by default. This must be called before calling setRenderer.
   *
   * @param enabled Whether to use protected buffers.
   * @throws RuntimeException if EGL 1.4 is unavailable.
   */
  public void setUseProtectedBuffers(boolean enabled) {
    if (enabled && (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1)) {
      throw new RuntimeException(
          "Protected buffer support requires EGL 1.4, available only on Jelly Bean MR1 and later.");
    }
    useProtected = enabled;
  }

  /**
   * Sets whether KHR_debug logging should be enabled during context creation.
   *
   * <p> Disabled by default. This must be called before calling setRenderer.
   *
   * @param enabled Whether to use KHR_debug.
   */
  public void setUseDebug(boolean enabled) {
    useDebug = enabled;
  }

  /**
   * Sets up OpenGL context sharing.
   *
   * <p> EGL_NO_CONTEXT by default.  This must be called before createContext to enable sharing.
   *
   * @param context The existing EGL context with which to share OpenGL resources.
   */
  public void setSharedContext(EGLContext context) {
    sharedContext = context;
  }

  /**
   * Enables or disables error reporting for the created context.
   *
   * <p> When set to false, the context will be created with EGL_CONTEXT_OPENGL_NO_ERROR_KHR
   * attribute set to true, which will cause all glGetError calls to return GL_NO_ERROR.
   * The default is to report errors normally.
   *
   * @param enabled Whether to enable error reporting.
   */
  public void setErrorReportingEnabled(boolean enabled) {
    errorReportingEnabled = enabled;
  }

  @Override
  public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
    IntBuffer attribs = IntBuffer.allocate(10);

    attribs.put(EGL_CONTEXT_CLIENT_VERSION);
    attribs.put(eglContextClientVersion);

    // Note that Adreno drivers do not require this flag because glEnable(GL_DEBUG_OUTPUT) is
    // sufficient, however Mali drivers require the context flag *and* the OpenGL state.
    if (useDebug) {
      attribs.put(EGL_CONTEXT_FLAGS_KHR);
      attribs.put(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
    }

    if (usePriority) {
      if (Build.FINGERPRINT.contains("generic_x86")) {
        Log.w(TAG, "EGL_CONTEXT_PRIORITY_LEVEL not supported on emulators.");
      } else {
        attribs.put(EGL_CONTEXT_PRIORITY_LEVEL);
        attribs.put(EGL_CONTEXT_PRIORITY_HIGH);
      }
    }
    if (useProtected && supportsProtectedContent(egl, display)) {
      attribs.put(EGL_PROTECTED_CONTENT_EXT);
      attribs.put(EGL14.EGL_TRUE);
    }
    if (!errorReportingEnabled) {
      final String eglExtensionString = egl.eglQueryString(display, EGL10.EGL_EXTENSIONS);
      if (eglExtensionString.contains("EGL_KHR_create_context_no_error")) {
        attribs.put(EGL_CONTEXT_OPENGL_NO_ERROR_KHR);
        attribs.put(EGL14.EGL_TRUE);
      }
    }
    while (attribs.hasRemaining()) {
      attribs.put(EGL10.EGL_NONE);
    }

    EGLContext context = egl.eglCreateContext(display, eglConfig, sharedContext, attribs.array());
    if ((context == null || context == EGL10.EGL_NO_CONTEXT)
        && eglContextClientVersion > MIN_REQUIRED_CONTEXT_CLIENT_VERSION) {
      Log.w(
          TAG,
          "Failed to create EGL context with version "
              + eglContextClientVersion
              + ", will try "
              + MIN_REQUIRED_CONTEXT_CLIENT_VERSION);

      attribs.array()[1] = MIN_REQUIRED_CONTEXT_CLIENT_VERSION;
      context = egl.eglCreateContext(display, eglConfig, sharedContext, attribs.array());
    }

    return context;
  }

  @Override
  public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
    egl.eglDestroyContext(display, context);
  }

  @Override
  public EGLSurface createWindowSurface(
      EGL10 egl, EGLDisplay display, EGLConfig config, Object nativeWindow) {
    EGLSurface result = null;
    int[] attribsList = null;
    if (useProtected && supportsProtectedContent(egl, display)) {
      attribsList = new int[] {EGL_PROTECTED_CONTENT_EXT, EGL14.EGL_TRUE, EGL10.EGL_NONE};
    }
    try {
      result = egl.eglCreateWindowSurface(display, config, nativeWindow, attribsList);
    } catch (IllegalArgumentException e) {
      // This exception indicates that the surface flinger surface
      // is not valid. This can happen if the surface flinger surface has
      // been torn down, but the application has not yet been
      // notified via SurfaceTexture.Callback.surfaceDestroyed.
      // In theory the application should be notified first,
      // but in practice sometimes it is not. See b/4588890.
      Log.e(TAG, "eglCreateWindowSurface", e);
    }
    return result;
  }

  @Override
  public void destroySurface(EGL10 egl, EGLDisplay display, EGLSurface surface) {
    egl.eglDestroySurface(display, surface);
  }

  /**
   * Sets the EGL version to use.
   */
  public void setEGLContextClientVersion(int version) {
    eglContextClientVersion = version;
  }

  /**
   * Returns whether the EGL driver supports EGL_EXT_protected_content.
   */
  private boolean supportsProtectedContent(EGL10 egl, EGLDisplay display) {
    String extensions = egl.eglQueryString(display, EGL10.EGL_EXTENSIONS);
    return extensions.contains("EGL_EXT_protected_content");
  }

}
