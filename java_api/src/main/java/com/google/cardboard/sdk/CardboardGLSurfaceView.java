/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.content.Context;
import android.opengl.EGL14;
import android.opengl.EGLExt;
import android.opengl.GLDebugHelper;
import android.opengl.GLSurfaceView.EGLConfigChooser;
import android.opengl.GLSurfaceView.EGLContextFactory;
import android.opengl.GLSurfaceView.EGLWindowSurfaceFactory;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import java.io.Writer;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

/**
 * A port to CardboardOSS of Gvr's surface, to compare against GLSurfaceView.
 * Local changes to Gvr's version, on top of the listed below to the GLSurfaceView fork:
 * - Removal of EglReadyListener instance member and all code related to it.
 *   Reasoning: it allowed to native code to reuse the GL context in use within Java code and it was
 *   no longer needed.
 * - Removal of EglReadyListener.Listener interface implementation in the GlThread.
 *   Reasoning: it worked in tandem with the removed instance of EglReadyListener and it was no
 *   longer needed.
 *
 * A port of Android's GLSurfaceView, to be used by VR applications.
 *
 * /////////////////////////// ATTENTION ///////////////////////////
 * Latest GLSurfaceView Change-id: 3054d010327f8888919d7e0a97d63773dd015681.
 *
 * Other than whitespace, we purposefully follow Android's style conventions, not the
 * Google3 style guide, in order to make syncs easier.
 *
 * Local changes to GLSurfaceView include:
 * - Delinting whitespace using git5 fix.
 * - Removing SystemProperties checks to confirm the code is running with GLES20 support.
 * - Copying helper functions from EGLLogWrapper to format error messages.
 * - Replacing trace functions with their app-space, API compatible equivalents.
 * - Fixing several lint errors for final modifiers and variable naming.
 * - Cherry pick of changes from change-id I2bb5b475433ebff8ca82db385e228fef11e32e20
 *     that removes support for pre-Android-3.0 GL driver limitations.
 * - Remove global locking behavior (only necessary for pre-Android-3.0 functionality).
 * - Add support to preserve the GL thread onDetachedFromWindow.
 * - Expose requestExitAndWait() to allow explicit request for the GL thread to exit.
 * - Expose isDetachedFromWindow() to subclasses.
 * - Listen to events from EglReadyListener to allow for context sharing.
 * - New function onPause(Runnable). The Runnable will be executed after the last render call,
 *   but before the rendering thread is paused.
 * - New function onSurfaceDestroyed(Runnable). Handles surfaceDestroyed and executes the Runnable
 *   before the surface and GL context are destroyed.
 *
 * An implementation of SurfaceView that uses the dedicated surface for
 * displaying OpenGL rendering.
 * <p>
 * A CardboardGLSurfaceView provides the following features:
 * <p>
 * <ul>
 * <li>Manages a surface, which is a special piece of memory that can be
 * composited into the Android view system.
 * <li>Manages an EGL display, which enables OpenGL to render into a surface.
 * <li>Accepts a user-provided Renderer object that does the actual rendering.
 * <li>Renders on a dedicated thread to decouple rendering performance from the
 * UI thread.
 * <li>Supports both on-demand and continuous rendering.
 * <li>Optionally wraps, traces, and/or error-checks the renderer's OpenGL calls.
 * </ul>
 *
 * <div class="special reference">
 * <h3>Developer Guides</h3>
 * <p>For more information about how to use OpenGL, read the
 * <a href="{@docRoot}guide/topics/graphics/opengl.html">OpenGL</a> developer guide.</p>
 * </div>
 *
 * <h3>Using CardboardGLSurfaceView</h3>
 * <p>
 * Typically you use CardboardGLSurfaceView by subclassing it and overriding one or more of the
 * View system input event methods. If your application does not need to override event
 * methods then CardboardGLSurfaceView can be used as-is. For the most part
 * CardboardGLSurfaceView behavior is customized by calling "set" methods rather than by subclassing.
 * For example, unlike a regular View, drawing is delegated to a separate Renderer object which
 * is registered with the CardboardGLSurfaceView
 * using the {@link #setRenderer(Renderer)} call.
 * <p>
 * <h3>Initializing CardboardGLSurfaceView</h3>
 * All you have to do to initialize a CardboardGLSurfaceView is call {@link #setRenderer(Renderer)}.
 * However, if desired, you can modify the default behavior of CardboardGLSurfaceView by calling one or
 * more of these methods before calling setRenderer:
 * <ul>
 * <li>{@link #setDebugFlags(int)}
 * <li>{@link #setEGLConfigChooser(boolean)}
 * <li>{@link #setEGLConfigChooser(EGLConfigChooser)}
 * <li>{@link #setEGLConfigChooser(int, int, int, int, int, int)}
 * <li>{@link #setGLWrapper(GLWrapper)}
 * </ul>
 * <p>
 * <h4>Specifying the android.view.Surface</h4>
 * By default CardboardGLSurfaceView will create a PixelFormat.RGB_888 format surface. If a translucent
 * surface is required, call getHolder().setFormat(PixelFormat.TRANSLUCENT).
 * The exact format of a TRANSLUCENT surface is device dependent, but it will be
 * a 32-bit-per-pixel surface with 8 bits per component.
 * <p>
 * <h4>Choosing an EGL Configuration</h4>
 * A given Android device may support multiple EGLConfig rendering configurations.
 * The available configurations may differ in how may channels of data are present, as
 * well as how many bits are allocated to each channel. Therefore, the first thing
 * CardboardGLSurfaceView has to do when starting to render is choose what EGLConfig to use.
 * <p>
 * By default CardboardGLSurfaceView chooses a EGLConfig that has an RGB_888 pixel format,
 * with at least a 16-bit depth buffer and no stencil.
 * <p>
 * If you would prefer a different EGLConfig
 * you can override the default behavior by calling one of the
 * setEGLConfigChooser methods.
 * <p>
 * <h4>Debug Behavior</h4>
 * You can optionally modify the behavior of CardboardGLSurfaceView by calling
 * one or more of the debugging methods {@link #setDebugFlags(int)},
 * and {@link #setGLWrapper}. These methods may be called before and/or after setRenderer, but
 * typically they are called before setRenderer so that they take effect immediately.
 * <p>
 * <h4>Setting a Renderer</h4>
 * Finally, you must call {@link #setRenderer} to register a {@link Renderer}.
 * The renderer is
 * responsible for doing the actual OpenGL rendering.
 * <p>
 * <h3>Rendering Mode</h3>
 * Once the renderer is set, you can control whether the renderer draws
 * continuously or on-demand by calling
* {@link #setRenderMode}. The default is continuous rendering.
 * <p>
 * <h3>Activity Life-cycle</h3>
 * A CardboardGLSurfaceView must be notified when the activity is paused and resumed. CardboardGLSurfaceView clients
 * are required to call {@link #onPause()} when the activity pauses and
 * {@link #onResume()} when the activity resumes. These calls allow CardboardGLSurfaceView to
 * pause and resume the rendering thread, and also allow CardboardGLSurfaceView to release and recreate
 * the OpenGL display.
 * <p>
 * <h3>Handling events</h3>
 * <p>
 * To handle an event you will typically subclass CardboardGLSurfaceView and override the
 * appropriate method, just as you would with any other View. However, when handling
 * the event, you may need to communicate with the Renderer object
 * that's running in the rendering thread. You can do this using any
 * standard Java cross-thread communication mechanism. In addition,
 * one relatively easy way to communicate with your renderer is
 * to call
 * {@link #queueEvent(Runnable)}. For example:
 * <pre class="prettyprint">
 * class MyCardboardGLSurfaceView extends CardboardGLSurfaceView {
 *
 *     private MyRenderer mMyRenderer;
 *
 *     public void start() {
 *         mMyRenderer = ...;
 *         setRenderer(mMyRenderer);
 *     }
 *
 *     public boolean onKeyDown(int keyCode, KeyEvent event) {
 *         if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
 *             queueEvent(new Runnable() {
 *                 // This method will be called on the rendering
 *                 // thread:
 *                 public void run() {
 *                     mMyRenderer.handleDpadCenter();
 *                 }});
 *             return true;
 *         }
 *         return super.onKeyDown(keyCode, event);
 *     }
 * }
 * </pre>
 *
 * @hide
 */
public class CardboardGLSurfaceView extends SurfaceView implements SurfaceHolder.Callback2 {
  private static final String TAG = "CardboardGLSurfaceView";
  private static final boolean LOG_ATTACH_DETACH = false;
  private static final boolean LOG_THREADS = false;
  private static final boolean LOG_PAUSE_RESUME = false;
  private static final boolean LOG_SURFACE = false;
  private static final boolean LOG_RENDERER = false;
  private static final boolean LOG_RENDERER_DRAW_FRAME = false;
  private static final boolean LOG_EGL = false;
  // Context flag that says whether the context has error reporting disabled.
  // https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_no_error.txt
  private static final int GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR = 0x8;

  /**
   * The renderer only renders
   * when the surface is created, or when {@link #requestRender} is called.
   *
   * @see #getRenderMode()
   * @see #setRenderMode(int)
   * @see #requestRender()
   */
  public static final int RENDERMODE_WHEN_DIRTY = 0;
  /**
   * The renderer is called
   * continuously to re-render the scene.
   *
   * @see #getRenderMode()
   * @see #setRenderMode(int)
   */
  public static final int RENDERMODE_CONTINUOUSLY = 1;

  /**
   * The EGL context communicates through a normal swap chain. Multiple
   * buffers are made available to the application, and eglSwapBuffers()
   * is called by this view at the end of every frame draw.
   *
   * This is the default swap mode.
   *
   * @see setSwapMode(int)
   */
  public static final int SWAPMODE_QUEUED = 0;

  /**
   * The EGL context is placed in single buffer mode, where the application
   * and the display concurrently access the same graphical buffer. eglSwapBuffers()
   * is called once, the first frame that is drawn after the swap mode is changed,
   * and then not called again. Applications are expected to call glFlush() manually
   * to commit their draw commands.
   *
   * @see setSwapMode(int)
   */
  public static final int SWAPMODE_SINGLE = 1;

  /**
   * The EGL context communicates through a normal swap chain, but eglSwapBuffers()
   * is not called on behalf of the application. This allows the application to
   * execute onDrawFrame(), without the view assuming that a frame will be submitted.
   *
   * @see setSwapMode(int)
   */
  public static final int SWAPMODE_MANUAL = 2;

  /**
   * Check glError() after every GL call and throw an exception if glError indicates
   * that an error has occurred. This can be used to help track down which OpenGL ES call
   * is causing an error.
   *
   * @see #getDebugFlags
   * @see #setDebugFlags
   */
  public static final int DEBUG_CHECK_GL_ERROR = 1;

  /**
   * Log GL calls to the system log at "verbose" level with tag "CardboardGLSurfaceView".
   *
   * @see #getDebugFlags
   * @see #setDebugFlags
   */
  public static final int DEBUG_LOG_GL_CALLS = 2;

  /**
   * Standard View constructor. In order to render something, you
   * must call {@link #setRenderer} to register a renderer.
   */
  public CardboardGLSurfaceView(Context context) {
    super(context);
    init();
  }

  /**
   * Standard View constructor. In order to render something, you
   * must call {@link #setRenderer} to register a renderer.
   */
  public CardboardGLSurfaceView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init();
  }

  @Override
  protected void finalize() throws Throwable {
    try {
      if (mGLThread != null) {
        // GLThread may still be running if this view was never
        // attached to a window.
        mGLThread.requestExitAndWait();
      }
    } finally {
      super.finalize();
    }
  }

  private void init() {
    // Install a SurfaceHolder.Callback so we get notified when the
    // underlying surface is created and destroyed
    SurfaceHolder holder = getHolder();
    holder.addCallback(this);
    // setFormat is done by SurfaceView in SDK 2.3 and newer. Uncomment
    // this statement if back-porting to 2.2 or older:
    // holder.setFormat(PixelFormat.RGB_565);
    //
    // setType is not needed for SDK 2.0 or newer. Uncomment this
    // statement if back-porting this code to older SDKs.
    // holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
  }

  /**
   * Set the glWrapper. If the glWrapper is not null, its
   * {@link GLWrapper#wrap(GL)} method is called
   * whenever a surface is created. A GLWrapper can be used to wrap
   * the GL object that's passed to the renderer. Wrapping a GL
   * object enables examining and modifying the behavior of the
   * GL calls made by the renderer.
   * <p>
   * Wrapping is typically used for debugging purposes.
   * <p>
   * The default value is null.
   * @param glWrapper the new GLWrapper
   */
  public void setGLWrapper(GLWrapper glWrapper) {
    mGLWrapper = glWrapper;
  }

  /**
   * Set the debug flags to a new value. The value is
   * constructed by OR-together zero or more
   * of the DEBUG_CHECK_* constants. The debug flags take effect
   * whenever a surface is created. The default value is zero.
   * @param debugFlags the new debug flags
   * @see #DEBUG_CHECK_GL_ERROR
   * @see #DEBUG_LOG_GL_CALLS
   */
  public void setDebugFlags(int debugFlags) {
    mDebugFlags = debugFlags;
  }

  /**
   * Get the current value of the debug flags.
   * @return the current value of the debug flags.
   */
  public int getDebugFlags() {
    return mDebugFlags;
  }

  /**
   * Control whether the EGL context is preserved when the CardboardGLSurfaceView is paused and
   * resumed.
   * <p>
   * If set to true, then the EGL context may be preserved when the CardboardGLSurfaceView is paused.
   * Whether the EGL context is actually preserved or not depends upon whether the
   * Android device that the program is running on can support an arbitrary number of EGL
   * contexts or not. Devices that can only support a limited number of EGL contexts must
   * release the  EGL context in order to allow multiple applications to share the GPU.
   * <p>
   * If set to false, the EGL context will be released when the CardboardGLSurfaceView is paused,
   * and recreated when the CardboardGLSurfaceView is resumed.
   * <p>
   *
   * The default is false.
   *
   * @param preserveOnPause preserve the EGL context when paused
   */
  public void setPreserveEGLContextOnPause(boolean preserveOnPause) {
    mPreserveEGLContextOnPause = preserveOnPause;
  }

  /**
   * @return true if the EGL context will be preserved when paused
   */
  public boolean getPreserveEGLContextOnPause() {
    return mPreserveEGLContextOnPause;
  }

  /**
   * Set the renderer associated with this view. Also starts the thread that
   * will call the renderer, which in turn causes the rendering to start.
   * <p>This method should be called once and only once in the life-cycle of
   * a CardboardGLSurfaceView.
   * <p>The following CardboardGLSurfaceView methods can only be called <em>before</em>
   * setRenderer is called:
   * <ul>
   * <li>{@link #setEGLConfigChooser(boolean)}
   * <li>{@link #setEGLConfigChooser(EGLConfigChooser)}
   * <li>{@link #setEGLConfigChooser(int, int, int, int, int, int)}
   * </ul>
   * <p>
   * The following CardboardGLSurfaceView methods can only be called <em>after</em>
   * setRenderer is called:
   * <ul>
   * <li>{@link #getRenderMode()}
   * <li>{@link #onPause()}
   * <li>{@link #onResume()}
   * <li>{@link #queueEvent(Runnable)}
   * <li>{@link #requestRender()}
   * <li>{@link #setRenderMode(int)}
   * </ul>
   *
   * @param renderer the renderer to use to perform OpenGL drawing.
   */
  public void setRenderer(Renderer renderer) {
    checkRenderThreadState();
    if (mEGLConfigChooser == null) {
      mEGLConfigChooser = new SimpleEGLConfigChooser(true);
    }
    if (mEGLContextFactory == null) {
      mEGLContextFactory = new DefaultContextFactory();
    }
    if (mEGLWindowSurfaceFactory == null) {
      mEGLWindowSurfaceFactory = new DefaultWindowSurfaceFactory();
    }
    mRenderer = renderer;
    mGLThread = new GLThread(mThisWeakRef);
    mGLThread.start();
  }

  /**
   * Install a custom EGLContextFactory.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(Renderer)}
   * is called.
   * <p>
   * If this method is not called, then by default
   * a context will be created with no shared context and
   * with a null attribute list.
   */
  public void setEGLContextFactory(EGLContextFactory factory) {
    checkRenderThreadState();
    mEGLContextFactory = factory;
  }

  /**
   * Install a custom EGLWindowSurfaceFactory.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(Renderer)}
   * is called.
   * <p>
   * If this method is not called, then by default
   * a window surface will be created with a null attribute list.
   */
  public void setEGLWindowSurfaceFactory(EGLWindowSurfaceFactory factory) {
    checkRenderThreadState();
    mEGLWindowSurfaceFactory = factory;
  }

  /**
   * Install a custom EGLConfigChooser.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(Renderer)}
   * is called.
   * <p>
   * If no setEGLConfigChooser method is called, then by default the
   * view will choose an EGLConfig that is compatible with the current
   * android.view.Surface, with a depth buffer depth of
   * at least 16 bits.
   * @param configChooser
   */
  public void setEGLConfigChooser(EGLConfigChooser configChooser) {
    checkRenderThreadState();
    mEGLConfigChooser = configChooser;
  }

  /**
   * Install a config chooser which will choose a config
   * as close to 16-bit RGB as possible, with or without an optional depth
   * buffer as close to 16-bits as possible.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(Renderer)}
   * is called.
   * <p>
   * If no setEGLConfigChooser method is called, then by default the
   * view will choose an RGB_888 surface with a depth buffer depth of
   * at least 16 bits.
   *
   * @param needDepth
   */
  public void setEGLConfigChooser(boolean needDepth) {
    setEGLConfigChooser(new SimpleEGLConfigChooser(needDepth));
  }

  /**
   * Install a config chooser which will choose a config
   * with at least the specified depthSize and stencilSize,
   * and exactly the specified redSize, greenSize, blueSize and alphaSize.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(Renderer)}
   * is called.
   * <p>
   * If no setEGLConfigChooser method is called, then by default the
   * view will choose an RGB_888 surface with a depth buffer depth of
   * at least 16 bits.
   *
   */
  public void setEGLConfigChooser(
      int redSize, int greenSize, int blueSize, int alphaSize, int depthSize, int stencilSize) {
    setEGLConfigChooser(
        new ComponentSizeChooser(redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize));
  }

  /**
   * Inform the default EGLContextFactory and default EGLConfigChooser
   * which EGLContext client version to pick.
   * <p>Use this method to create an OpenGL ES 2.0-compatible context.
   * Example:
   * <pre class="prettyprint">
   *     public MyView(Context context) {
   *         super(context);
   *         setEGLContextClientVersion(2); // Pick an OpenGL ES 2.0 context.
   *         setRenderer(new MyRenderer());
   *     }
   * </pre>
   * <p>Note: Activities which require OpenGL ES 2.0 should indicate this by
   * setting @lt;uses-feature android:glEsVersion="0x00020000" /> in the activity's
   * AndroidManifest.xml file.
   * <p>If this method is called, it must be called before {@link #setRenderer(Renderer)}
   * is called.
   * <p>This method only affects the behavior of the default EGLContexFactory and the
   * default EGLConfigChooser. If
   * {@link #setEGLContextFactory(EGLContextFactory)} has been called, then the supplied
   * EGLContextFactory is responsible for creating an OpenGL ES 2.0-compatible context.
   * If
   * {@link #setEGLConfigChooser(EGLConfigChooser)} has been called, then the supplied
   * EGLConfigChooser is responsible for choosing an OpenGL ES 2.0-compatible config.
   * @param version The EGLContext client version to choose. Use 2 for OpenGL ES 2.0
   */
  public void setEGLContextClientVersion(int version) {
    checkRenderThreadState();
    mEGLContextClientVersion = version;
  }

  /**
   * Set the rendering mode. When renderMode is
   * RENDERMODE_CONTINUOUSLY, the renderer is called
   * repeatedly to re-render the scene. When renderMode
   * is RENDERMODE_WHEN_DIRTY, the renderer only rendered when the surface
   * is created, or when {@link #requestRender} is called. Defaults to RENDERMODE_CONTINUOUSLY.
   * <p>
   * Using RENDERMODE_WHEN_DIRTY can improve battery life and overall system performance
   * by allowing the GPU and CPU to idle when the view does not need to be updated.
   * <p>
   * This method can only be called after {@link #setRenderer(Renderer)}
   *
   * @param renderMode one of the RENDERMODE_X constants
   * @see #RENDERMODE_CONTINUOUSLY
   * @see #RENDERMODE_WHEN_DIRTY
   */
  public void setRenderMode(int renderMode) {
    mGLThread.setRenderMode(renderMode);
  }

  /**
   * Get the current rendering mode. May be called
   * from any thread. Must not be called before a renderer has been set.
   * @return the current rendering mode.
   * @see #RENDERMODE_CONTINUOUSLY
   * @see #RENDERMODE_WHEN_DIRTY
   */
  public int getRenderMode() {
    return mGLThread.getRenderMode();
  }

  /**
   * Request that the renderer render a frame.
   * This method is typically used when the render mode has been set to
   * {@link #RENDERMODE_WHEN_DIRTY}, so that frames are only rendered on demand.
   * May be called
   * from any thread. Must not be called before a renderer has been set.
   */
  public void requestRender() {
    mGLThread.requestRender();
  }

  /**
   * Set the swap mode.
   *
   * <p>When swapMode is SWAPMODE_QUEUED, the view behaves as the default GLSurfaceView
   * implementation -- frames are buffered, and eglSwapBuffers() is automatically executed after
   * each call to Renderer.onDrawFrame().
   *
   * <p>When swapMode is SWAPMODE_SINGLE, the application and the display concurrently access the
   * same graphical buffer, eglSwapBuffers() is not called continuously, and applications are
   * expected to synchronize with the display, and to submit their draw commands manually by calling
   * glFlush() or glFinish().
   *
   * <p>When swapMode is SWAPMODE_MANUAL, frames are buffered, but eglSwapBuffers() is not called.
   * Applications are expected to call it themselves.
   *
   * <p>Note that SWAPMODE_SINGLE requires two extensions to function properly --
   * EGL_KHR_mutable_render_buffer and EGL_ANDROID_front_buffer_auto_refresh.
   * EGL_KHR_mutable_render_buffer, in particular, requires bits to be set in the EGL config during
   * EGL context construction. So applications that specify an EGLContextFactory will need to
   * initialize the context appropriately. With the correct extensions and configurations, the
   * behavior of this extension is undefined.
   *
   * <p>This method can only be called after {@link #setRenderer(Renderer)}
   *
   * @param swapMode one of the SWAPMODE_X constants
   * @see #SWAPMODE_QUEUED
   * @see #SWAPMODE_SINGLE
   * @see #SWAPMODE_MANUAL
   */
  public void setSwapMode(int swapMode) {
    if (swapMode == SWAPMODE_SINGLE
        && Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
      Log.e(
          TAG,
          "setSwapMode(SWAPMODE_SINGLE) requires Jellybean MR1 (EGL14 dependency).");
      return;
    }
    mGLThread.setSwapMode(swapMode);
  }

  /**
   * Set whether to preserve the GL thread when {@link #onDetachedFromWindow()} is called.
   * <p>Defaults to false.
   * <p>If this method is called, it must be called before
   * {@link #setRenderer(Renderer)} is called.
   */
  public void setPreserveGlThreadOnDetachedFromWindow(boolean preserveGlThread) {
    checkRenderThreadState();
    mPreserveGlThreadOnDetachedFromWindow = preserveGlThread;
  }

  /**
   * This method is part of the SurfaceHolder.Callback interface, and is
   * not normally called or subclassed by clients of CardboardGLSurfaceView.
   */
  @Override
  public void surfaceCreated(SurfaceHolder holder) {
    mGLThread.surfaceCreated();
  }

  /**
   * This method is part of the SurfaceHolder.Callback interface, and is
   * not normally called or subclassed by clients of CardboardGLSurfaceView.
   */
  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {
    // Surface will be destroyed when we return
    onSurfaceDestroyed(null);
  }

  /**
   * Handles destruction of the target surface of this CardboardGLSurfaceView and executes an event just
   * before the surface is destroyed. This is useful for cleanup and prevents an extremely rare race
   * condition where a render call is made after an event added with {@link queueEvent} is
   * processed, but before the super call is made. Both the passed Runnable and the required state
   * changes on the rendering thread are completed before this function returns.
   * @param r Runnable to execute just before destroying the surface. There will be no onDraw()
   *     callbacks between this Runnable's execution and surface destruction. Can be null.
   */
  public void onSurfaceDestroyed(Runnable r) {
    mGLThread.surfaceDestroyed(r);
  }

  /**
   * This method is part of the SurfaceHolder.Callback interface, and is
   * not normally called or subclassed by clients of CardboardGLSurfaceView.
   */
  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    mGLThread.onWindowResize(w, h);
  }

  /**
   * This method is part of the SurfaceHolder.Callback interface, and is
   * not normally called or subclassed by clients of CardboardGLSurfaceView.
   */
  @Override
  public void surfaceRedrawNeeded(SurfaceHolder holder) {
    mGLThread.requestRenderAndWait();
  }

  /**
   * Inform the view that the activity is paused. The owner of this view must
   * call this method when the activity is paused. Calling this method will
   * pause the rendering thread.
   * Must not be called before a renderer has been set.
   */
  public void onPause() {
    mGLThread.onPause(null);
  }

  /**
   * Same as {@link #onPause()}, but executes a runnable just before pausing the GL thread. This
   * prevents an extremely rare race condition where the GL thread executes a render call between
   * queueEvent() and onPause(). Both the passed Runnable and the required state changes on the GL
   * thread are completed before this function returns.
   * @param r Runnable to execute just before pausing the thread. There will be no onDraw()
   *     callbacks between this Runnable's execution and the thread being paused. Can be null.
   */
  public void onPause(Runnable r) {
    mGLThread.onPause(r);
  }

  /**
   * Inform the view that the activity is resumed. The owner of this view must
   * call this method when the activity is resumed. Calling this method will
   * recreate the OpenGL display and resume the rendering
   * thread.
   * Must not be called before a renderer has been set.
   */
  public void onResume() {
    mGLThread.onResume();
  }

  /**
   * Queue a runnable to be run on the GL rendering thread.
   *
   * <p>This can be used to communicate with the Renderer on the rendering thread. Must not be
   * called before a renderer has been set.
   *
   * <p>Most methods that interact with the GL thread, such as {@link #onPause()},
   * {@link #onResume()}, {@link #surfaceDestroyed}, etc. are event barriers, i.e., all events
   * queued before these calls are executed before the GL thread state change is performed. This is
   * enforced by synchronizing on mGLThreadManager and processing all events before applying any
   * state changes in the GL thread render loop.
   *
   * @param r the runnable to be run on the GL rendering thread.
   */
  public void queueEvent(Runnable r) {
    mGLThread.queueEvent(r);
  }

  /**
   * Requests and waits for the GL thread to exit.
   * Must not be called on the GL thread.
   */
  public void requestExitAndWait() {
    mGLThread.requestExitAndWait();
  }

  /**
   * This method is used as part of the View class and is not normally
   * called or subclassed by clients of CardboardGLSurfaceView.
   */
  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    if (LOG_ATTACH_DETACH) {
      Log.d(TAG, "onAttachedToWindow reattach =" + mDetached);
    }
    if (mDetached && (mRenderer != null) && !mPreserveGlThreadOnDetachedFromWindow) {
      int renderMode = RENDERMODE_CONTINUOUSLY;
      int swapMode = SWAPMODE_QUEUED;
      if (mGLThread != null) {
        renderMode = mGLThread.getRenderMode();
        swapMode = mGLThread.getSwapMode();
      }
      mGLThread = new GLThread(mThisWeakRef);
      if (renderMode != RENDERMODE_CONTINUOUSLY) {
        mGLThread.setRenderMode(renderMode);
      }
      if (swapMode != SWAPMODE_QUEUED) {
        mGLThread.setSwapMode(swapMode);
      }
      mGLThread.start();
    }
    mDetached = false;
  }

  @Override
  protected void onDetachedFromWindow() {
    if (LOG_ATTACH_DETACH) {
      Log.d(TAG, "onDetachedFromWindow");
    }
    if (mGLThread != null && !mPreserveGlThreadOnDetachedFromWindow) {
      requestExitAndWait();
    }
    mDetached = true;
    super.onDetachedFromWindow();
  }

  /**
   * Whether the view has received an onDetachedFromWindow() callback.
   *
   * <p>Note: this can only be called from the UI thread.
   */
  protected boolean isDetachedFromWindow() {
    return mDetached;
  }

  // ----------------------------------------------------------------------

  /**
   * An interface used to wrap a GL interface.
   * <p>Typically
   * used for implementing debugging and tracing on top of the default
   * GL interface. You would typically use this by creating your own class
   * that implemented all the GL methods by delegating to another GL instance.
   * Then you could add your own behavior before or after calling the
   * delegate. All the GLWrapper would do was instantiate and return the
   * wrapper GL instance:
   * <pre class="prettyprint">
   * class MyGLWrapper implements GLWrapper {
   *     GL wrap(GL gl) {
   *         return new MyGLImplementation(gl);
   *     }
   *     static class MyGLImplementation implements GL,GL10,GL11,... {
   *         ...
   *     }
   * }
   * </pre>
   * @see #setGLWrapper(GLWrapper)
   */
  public interface GLWrapper {
    /**
     * Wraps a gl interface in another gl interface.
     * @param gl a GL interface that is to be wrapped.
     * @return either the input argument or another GL object that wraps the input argument.
     */
    GL wrap(GL gl);
  }

  private class DefaultContextFactory implements EGLContextFactory {
    private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    private static final String LOG_TAG = TAG + "::DefaultContextFactory";

    @Override
    public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig config) {
      int[] attribList = {EGL_CONTEXT_CLIENT_VERSION, mEGLContextClientVersion, EGL10.EGL_NONE};

      return egl.eglCreateContext(
          display,
          config,
          EGL10.EGL_NO_CONTEXT,
          mEGLContextClientVersion != 0 ? attribList : null);
    }

    @Override
    public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
      if (!egl.eglDestroyContext(display, context)) {
        Log.e("DefaultContextFactory", "display:" + display + " context: " + context);
        if (LOG_THREADS) {
          Log.i(LOG_TAG, "tid=" + Thread.currentThread().getId());
        }
        EglHelper.throwEglException("eglDestroyContext", egl.eglGetError());
      }
    }
  }

  private static class DefaultWindowSurfaceFactory implements EGLWindowSurfaceFactory {
    private static final String LOG_TAG = TAG + "::DefaultWindowSurfaceFactory";

    @Override
    public EGLSurface createWindowSurface(
        EGL10 egl, EGLDisplay display, EGLConfig config, Object nativeWindow) {
      EGLSurface result = null;
      try {
        result = egl.eglCreateWindowSurface(display, config, nativeWindow, null);
      } catch (IllegalArgumentException e) {
        // This exception indicates that the surface flinger surface
        // is not valid. This can happen if the surface flinger surface has
        // been torn down, but the application has not yet been
        // notified via SurfaceHolder.Callback.surfaceDestroyed.
        // In theory the application should be notified first,
        // but in practice sometimes it is not. See b/4588890
        Log.e(LOG_TAG, "eglCreateWindowSurface", e);
      }
      return result;
    }

    @Override
    public void destroySurface(EGL10 egl, EGLDisplay display, EGLSurface surface) {
      egl.eglDestroySurface(display, surface);
    }
  }

  private abstract class BaseConfigChooser implements EGLConfigChooser {
    private static final String LOG_TAG = TAG + "::BaseConfigChooser";

    public BaseConfigChooser(int[] configSpec) {
      mConfigSpec = filterConfigSpec(configSpec);
    }

    @Override
    public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
      int[] numConfig = new int[1];
      if (!egl.eglChooseConfig(display, mConfigSpec, null, 0, numConfig)) {
        for (int i = 1; i < mConfigSpec.length; ++i) {
          if (mConfigSpec[i - 1] == EGL10.EGL_RENDERABLE_TYPE
              && mConfigSpec[i] == EGLExt.EGL_OPENGL_ES3_BIT_KHR) {
            Log.w(LOG_TAG, "Failed to choose GLES 3 config, will try 2.");
            mConfigSpec[i] = EGL14.EGL_OPENGL_ES2_BIT;
            return chooseConfig(egl, display);
          }
        }
        throw new IllegalArgumentException("eglChooseConfig failed");
      }

      int numConfigs = numConfig[0];

      if (numConfigs <= 0) {
        throw new IllegalArgumentException("No configs match configSpec");
      }

      EGLConfig[] configs = new EGLConfig[numConfigs];
      if (!egl.eglChooseConfig(display, mConfigSpec, configs, numConfigs, numConfig)) {
        throw new IllegalArgumentException("eglChooseConfig#2 failed");
      }
      EGLConfig config = chooseConfig(egl, display, configs);
      if (config == null) {
        throw new IllegalArgumentException("No config chosen");
      }
      return config;
    }

    abstract EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, EGLConfig[] configs);

    protected int[] mConfigSpec;

    private int[] filterConfigSpec(int[] configSpec) {
      if (mEGLContextClientVersion != 2 && mEGLContextClientVersion != 3) {
        return configSpec;
      }
      /* We know none of the subclasses define EGL_RENDERABLE_TYPE.
       * And we know the configSpec is well formed.
       */
      int len = configSpec.length;
      int[] newConfigSpec = new int[len + 2];
      System.arraycopy(configSpec, 0, newConfigSpec, 0, len - 1);
      newConfigSpec[len - 1] = EGL10.EGL_RENDERABLE_TYPE;
      if (mEGLContextClientVersion == 2) {
        newConfigSpec[len] = EGL14.EGL_OPENGL_ES2_BIT; /* EGL_OPENGL_ES2_BIT */
      } else {
        newConfigSpec[len] = EGLExt.EGL_OPENGL_ES3_BIT_KHR; /* EGL_OPENGL_ES3_BIT_KHR */
      }
      newConfigSpec[len + 1] = EGL10.EGL_NONE;
      return newConfigSpec;
    }
  }

  /**
   * Choose a configuration with exactly the specified r,g,b,a sizes,
   * and at least the specified depth and stencil sizes.
   */
  private class ComponentSizeChooser extends BaseConfigChooser {
    public ComponentSizeChooser(
        int redSize, int greenSize, int blueSize, int alphaSize, int depthSize, int stencilSize) {
      super(
          new int[] {
            EGL10.EGL_RED_SIZE,
            redSize,
            EGL10.EGL_GREEN_SIZE,
            greenSize,
            EGL10.EGL_BLUE_SIZE,
            blueSize,
            EGL10.EGL_ALPHA_SIZE,
            alphaSize,
            EGL10.EGL_DEPTH_SIZE,
            depthSize,
            EGL10.EGL_STENCIL_SIZE,
            stencilSize,
            EGL10.EGL_NONE
          });
      mValue = new int[1];
      mRedSize = redSize;
      mGreenSize = greenSize;
      mBlueSize = blueSize;
      mAlphaSize = alphaSize;
      mDepthSize = depthSize;
      mStencilSize = stencilSize;
    }

    @Override
    public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, EGLConfig[] configs) {
      for (EGLConfig config : configs) {
        int d = findConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0);
        int s = findConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);
        if ((d >= mDepthSize) && (s >= mStencilSize)) {
          int r = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
          int g = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
          int b = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
          int a = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);
          if ((r == mRedSize) && (g == mGreenSize) && (b == mBlueSize) && (a == mAlphaSize)) {
            return config;
          }
        }
      }
      return null;
    }

    private int findConfigAttrib(
        EGL10 egl, EGLDisplay display, EGLConfig config, int attribute, int defaultValue) {

      if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
        return mValue[0];
      }
      return defaultValue;
    }

    private int[] mValue;
    // Subclasses can adjust these values:
    protected int mRedSize;
    protected int mGreenSize;
    protected int mBlueSize;
    protected int mAlphaSize;
    protected int mDepthSize;
    protected int mStencilSize;
  }

  /**
   * This class will choose a RGB_888 surface with
   * or without a depth buffer.
   *
   */
  private class SimpleEGLConfigChooser extends ComponentSizeChooser {
    public SimpleEGLConfigChooser(boolean withDepthBuffer) {
      super(8, 8, 8, 0, withDepthBuffer ? 16 : 0, 0);
    }
  }

  /**
   * An EGL helper class.
   */
  private static class EglHelper {
    private static final String LOG_TAG = TAG + "::EglHelper";

    // This constant is defined in EGL_ANDROID_front_buffer_auto_refresh, an
    // Android-specific EGL extension.
    public static final int EGL_FRONT_BUFFER_AUTO_REFRESH = 0x314C;

    public EglHelper(WeakReference<CardboardGLSurfaceView> cardboardGLSurfaceViewWeakRef) {
      mCardboardGLSurfaceViewWeakRef = cardboardGLSurfaceViewWeakRef;
    }

    public void start() {
      if (LOG_EGL) {
        Log.w(LOG_TAG, "start() tid=" + Thread.currentThread().getId());
      }
      if (mEgl == null) {
        initialize();
      }
      // Destroy the previous EGL context if it exists, so that we can release the old share group
      // and create a new one with the recently-created app context.
      // TODO(b/37944681): Create automated test that exercises this condition.
      if (mEglContext != null) {
        mEgl.eglDestroyContext(mEglDisplay, mEglContext);
        mEglContext = null;
      }

      if (mPendingEglContext == null) {
        // TODO(b/37944681): Create automated test that exercises this condition.
        createPendingEglContext();
      }
      mEglContext = mPendingEglContext;
      mEglDisplay = mPendingEglDisplay;
      mPendingEglContext = null;
      mPendingEglDisplay = null;
    }

    // Creates a new context, shared with the app context if necessary, and stashes it.
    // The new context is considered "pending" until the run loop has a chance to grab it.
    public void renewPendingEglContext() {
      if (mEgl == null) {
        initialize();
      }
      // If a pending EDS context already exists, destroy and recreate it to ensure that we're
      // in a share group with the most recently created application context.
      // TODO(b/37944681): Create automated test that exercises this condition.
      if (mPendingEglContext != null) {
        mEgl.eglDestroyContext(mEglDisplay, mPendingEglContext);
      }
      createPendingEglContext();
    }

    private void initialize() {
      /*
       * Get an EGL instance
       */
      mEgl = (EGL10) EGLContext.getEGL();

      /*
       * Get to the default display.
       */
      mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

      if (mEglDisplay == EGL10.EGL_NO_DISPLAY) {
        throw new RuntimeException("eglGetDisplay failed");
      }

      /*
       * We can now initialize EGL for that display
       */
      int[] version = new int[2];
      if (!mEgl.eglInitialize(mEglDisplay, version)) {
        throw new RuntimeException("eglInitialize failed");
      }
      mEglSurface = null;
    }

    // Chooses and stashes the latest EGL config, creates a new EGL context (in the app's share
    // group if necessary) and returns a new context.
    private void createPendingEglContext() {
      // Obtain the current display and ensure that it has been initialized.
      mPendingEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
      if (mPendingEglDisplay == EGL10.EGL_NO_DISPLAY) {
        throw new RuntimeException("eglGetDisplay failed");
      }
      int[] version = new int[2];
      if (!mEgl.eglInitialize(mPendingEglDisplay, version)) {
        throw new RuntimeException("eglInitialize failed");
      }
      // The surface view provides the config chooser and context factory.
      EGLContext context;
      CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
      if (view == null) {
        mEglConfig = null;
        context = null;
      } else {
        mEglConfig = view.mEGLConfigChooser.chooseConfig(mEgl, mPendingEglDisplay);
        context = view.mEGLContextFactory.createContext(
            mEgl, mPendingEglDisplay, mEglConfig);
      }
      if (context == null || context == EGL10.EGL_NO_CONTEXT) {
        context = null;
        // Android does not provide us a way to be notified when an EGL context becomes invalid,
        // it only provides lifetime events for the Surface.  For the best robustness, we handle
        // EGL_BAD_CONTEXT gracefully by clearing out the invalid context, then waiting for the app
        // to renew it.
        int err = mEgl.eglGetError();
        if (err == EGL10.EGL_BAD_CONTEXT) {
          Log.e(
              LOG_TAG,
              "Stashed EGL context has become invalid and can no longer be used for sharing.");
          mPendingEglContext = null;
          mPendingEglDisplay = null;
          mEglConfig = null;
          return;
        } else {
          throwEglException("createPendingEglContext", err);
        }
      }
      if (LOG_EGL) {
        Log.w(
            LOG_TAG,
            "createPendingEglContext " + context + " tid=" + Thread.currentThread().getId());
      }
      mPendingEglContext = context;
    }

    /**
     * Create an egl surface for the current SurfaceHolder surface. If a surface
     * already exists, destroy it before creating the new surface.
     *
     * @return true if the surface was created successfully.
     */
    public boolean createSurface() {
      if (LOG_EGL) {
        Log.w(LOG_TAG, "createSurface()  tid=" + Thread.currentThread().getId());
      }
      /*
       * Check preconditions.
       */
      if (mEgl == null) {
        throw new RuntimeException("egl not initialized");
      }
      if (mEglDisplay == null) {
        throw new RuntimeException("eglDisplay not initialized");
      }
      if (mEglConfig == null) {
        throw new RuntimeException("mEglConfig not initialized");
      }

      /*
       *  The window size has changed, so we need to create a new
       *  surface.
       */
      destroySurfaceImp();

      /*
       * Create an EGL surface we can render into.
       */
      CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
      if (view != null) {
        mEglSurface =
            view.mEGLWindowSurfaceFactory.createWindowSurface(
                mEgl, mEglDisplay, mEglConfig, view.getHolder());
      } else {
        mEglSurface = null;
      }

      if (mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE) {
        int error = mEgl.eglGetError();
        if (error == EGL10.EGL_BAD_NATIVE_WINDOW) {
          Log.e(LOG_TAG, "createWindowSurface returned EGL_BAD_NATIVE_WINDOW.");
        }
        return false;
      }

      /*
       * Before we can issue GL commands, we need to make sure
       * the context is current and bound to a surface.
       */
      if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
        /*
         * Could not make the context current, probably because the underlying
         * SurfaceView surface has been destroyed.
         */
        logEglErrorAsWarning(LOG_TAG, "eglMakeCurrent", mEgl.eglGetError());
        return false;
      }

      return true;
    }

    /**
     * Create a GL object for the current EGL context.
     * @return
     */
    GL createGL() {

      GL gl = mEglContext.getGL();
      CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
      if (view != null) {
        if (view.mGLWrapper != null) {
          gl = view.mGLWrapper.wrap(gl);
        }

        if ((view.mDebugFlags & (DEBUG_CHECK_GL_ERROR | DEBUG_LOG_GL_CALLS)) != 0) {
          int configFlags = 0;
          Writer log = null;
          if ((view.mDebugFlags & DEBUG_CHECK_GL_ERROR) != 0) {
            configFlags |= GLDebugHelper.CONFIG_CHECK_GL_ERROR;
          }
          if ((view.mDebugFlags & DEBUG_LOG_GL_CALLS) != 0) {
            log = new LogWriter();
          }
          gl = GLDebugHelper.wrap(gl, configFlags, log);
        }
      }
      return gl;
    }

    /**
     * Display the current render surface.
     * @return the EGL error code from eglSwapBuffers.
     */
    public int swap() {
      if (!mEgl.eglSwapBuffers(mEglDisplay, mEglSurface)) {
        return mEgl.eglGetError();
      }
      return EGL10.EGL_SUCCESS;
    }

    public void destroySurface() {
      if (LOG_EGL) {
        Log.w(LOG_TAG, "destroySurface()  tid=" + Thread.currentThread().getId());
      }
      destroySurfaceImp();
    }

    public void setEglSurfaceAttrib(int attribute, int value) {
      if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
        Log.e(LOG_TAG, "Cannot call eglSurfaceAttrib. API version is too low.");
        return;
      }

      // TODO(adamgousetis): Is there a way to convert from EGL10 to EGL14?
      android.opengl.EGLDisplay display = EGL14.eglGetCurrentDisplay();
      android.opengl.EGLSurface surface = EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW);
      boolean success = EGL14.eglSurfaceAttrib(display, surface, attribute, value);
      if (!success) {
        Log.e(LOG_TAG, "eglSurfaceAttrib() failed. attribute=" + attribute + " value=" + value);
      }
    }

    private void destroySurfaceImp() {
      if (mEglSurface != null && mEglSurface != EGL10.EGL_NO_SURFACE) {
        mEgl.eglMakeCurrent(
            mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
        CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
        if (view != null) {
          view.mEGLWindowSurfaceFactory.destroySurface(mEgl, mEglDisplay, mEglSurface);
        }
        mEglSurface = null;
      }
    }

    public void finish() {
      if (LOG_EGL) {
        Log.w(LOG_TAG, "finish() tid=" + Thread.currentThread().getId());
      }
      if (mEglContext != null) {
        CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
        if (view != null) {
          view.mEGLContextFactory.destroyContext(mEgl, mEglDisplay, mEglContext);
        }
        mEglContext = null;
      }
      if (mEglDisplay != null) {
        mEgl.eglTerminate(mEglDisplay);
        mEglDisplay = null;
      }
    }

    private void throwEglException(String function) {
      throwEglException(function, mEgl.eglGetError());
    }

    public static void throwEglException(String function, int error) {
      String message = formatEglError(function, error);
      if (LOG_THREADS) {
        Log.e(
            LOG_TAG,
            "throwEglException tid=" + Thread.currentThread().getId() + " " + message);
      }
      throw new RuntimeException(message);
    }

    public static void logEglErrorAsWarning(String tag, String function, int error) {
      Log.w(tag, formatEglError(function, error));
    }

    public static String formatEglError(String function, int error) {
      return function + " failed: " + getErrorString(error);
    }

    // This function was copied from EGLLogWrapper
    private static String getHex(int value) {
      return "0x" + Integer.toHexString(value);
    }

    // This function was copied from EGLLogWrapper
    private static String getErrorString(int error) {
      switch (error) {
        case EGL11.EGL_SUCCESS:
          return "EGL_SUCCESS";
        case EGL11.EGL_NOT_INITIALIZED:
          return "EGL_NOT_INITIALIZED";
        case EGL11.EGL_BAD_ACCESS:
          return "EGL_BAD_ACCESS";
        case EGL11.EGL_BAD_ALLOC:
          return "EGL_BAD_ALLOC";
        case EGL11.EGL_BAD_ATTRIBUTE:
          return "EGL_BAD_ATTRIBUTE";
        case EGL11.EGL_BAD_CONFIG:
          return "EGL_BAD_CONFIG";
        case EGL11.EGL_BAD_CONTEXT:
          return "EGL_BAD_CONTEXT";
        case EGL11.EGL_BAD_CURRENT_SURFACE:
          return "EGL_BAD_CURRENT_SURFACE";
        case EGL11.EGL_BAD_DISPLAY:
          return "EGL_BAD_DISPLAY";
        case EGL11.EGL_BAD_MATCH:
          return "EGL_BAD_MATCH";
        case EGL11.EGL_BAD_NATIVE_PIXMAP:
          return "EGL_BAD_NATIVE_PIXMAP";
        case EGL11.EGL_BAD_NATIVE_WINDOW:
          return "EGL_BAD_NATIVE_WINDOW";
        case EGL11.EGL_BAD_PARAMETER:
          return "EGL_BAD_PARAMETER";
        case EGL11.EGL_BAD_SURFACE:
          return "EGL_BAD_SURFACE";
        case EGL11.EGL_CONTEXT_LOST:
          return "EGL_CONTEXT_LOST";
        default:
          return getHex(error);
      }
    }

    private WeakReference<CardboardGLSurfaceView> mCardboardGLSurfaceViewWeakRef;
    EGL10 mEgl;
    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLConfig mEglConfig;
    EGLContext mEglContext;
    EGLContext mPendingEglContext;
    EGLDisplay mPendingEglDisplay;
  }

  /**
   * A generic GL Thread. Takes care of initializing EGL and GL. Delegates
   * to a Renderer instance to do the actual drawing. Can be configured to
   * render continuously or on request.
   *
   * All potentially blocking synchronization is done through the
   * mGLThreadManager object. This avoids multiple-lock ordering issues.
   *
   */
  static class GLThread extends Thread {
    private static final String LOG_TAG = TAG + "::GLThread";
    private static final String LOG_MAIN_THREAD_TAG = TAG + "::MainThread";

    GLThread(WeakReference<CardboardGLSurfaceView> cardboardGLSurfaceViewWeakRef) {
      super();
      mWidth = 0;
      mHeight = 0;
      mRequestRender = true;
      mRenderMode = RENDERMODE_CONTINUOUSLY;
      mRequestedSwapMode = SWAPMODE_QUEUED;
      mWantRenderNotification = false;
      mCardboardGLSurfaceViewWeakRef = cardboardGLSurfaceViewWeakRef;
    }

    @Override
    public void run() {
      setName("GLThread " + getId());
      if (LOG_THREADS) {
        Log.i(LOG_TAG, "starting tid=" + getId());
      }

      try {
        guardedRun();
      } catch (InterruptedException e) {
        // fall thru and exit normally
      } finally {
        mGLThreadManager.threadExiting(this);
      }
    }

    /*
     * This private method should only be called inside a
     * synchronized(mGLThreadManager) block.
     */
    private void stopEglSurfaceLocked() {
      if (mHaveEglSurface) {
        mHaveEglSurface = false;
        mEglHelper.destroySurface();
      }
    }

    /*
     * This private method should only be called inside a
     * synchronized(mGLThreadManager) block.
     */
    private void stopEglContextLocked() {
      if (mHaveEglContext) {
        mEglHelper.finish();
        mHaveEglContext = false;
        mGLThreadManager.releaseEglContextLocked(this);
      }
    }

    private void guardedRun() throws InterruptedException {
      mEglHelper = new EglHelper(mCardboardGLSurfaceViewWeakRef);
      mHaveEglContext = false;
      mHaveEglSurface = false;
      mWantRenderNotification = false;

      try {
        GL10 gl = null;
        boolean createEglContext = false;
        boolean createEglSurface = false;
        boolean createGlInterface = false;
        boolean lostEglContext = false;
        boolean sizeChanged = false;
        boolean wantRenderNotification = false;
        boolean doRenderNotification = false;
        boolean askedToReleaseEglContext = false;
        boolean applySwapMode = false;
        int swapMode = SWAPMODE_QUEUED;
        int w = 0;
        int h = 0;
        Runnable event = null;

        while (true) {
          synchronized (mGLThreadManager) {
            while (true) {
              if (mShouldExit) {
                return;
              }

              if (!mEventQueue.isEmpty()) {
                event = mEventQueue.remove(0);
                break;
              }

              // Update the pause state.
              boolean pausing = false;
              if (mPaused != mRequestPaused) {
                pausing = mRequestPaused;
                mPaused = mRequestPaused;
                mGLThreadManager.notifyAll();
                if (LOG_PAUSE_RESUME) {
                  Log.i(LOG_TAG, "mPaused is now " + mPaused + " tid=" + getId());
                }
              }

              // Do we need to give up the EGL context?
              if (mShouldReleaseEglContext) {
                if (LOG_SURFACE) {
                  Log.i(LOG_TAG, "releasing EGL context because asked to tid=" + getId());
                }
                stopEglSurfaceLocked();
                stopEglContextLocked();
                mShouldReleaseEglContext = false;
                askedToReleaseEglContext = true;
              }

              // Have we lost the EGL context?
              if (lostEglContext) {
                stopEglSurfaceLocked();
                stopEglContextLocked();
                lostEglContext = false;
              }

              // When pausing, release the EGL surface:
              if (pausing && mHaveEglSurface) {
                if (LOG_SURFACE) {
                  Log.i(LOG_TAG, "releasing EGL surface because paused tid=" + getId());
                }
                stopEglSurfaceLocked();
              }

              // When pausing, optionally release the EGL Context:
              if (pausing && mHaveEglContext) {
                CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
                boolean preserveEglContextOnPause =
                    view == null ? false : view.mPreserveEGLContextOnPause;
                if (!preserveEglContextOnPause) {
                  stopEglContextLocked();
                  if (LOG_SURFACE) {
                    Log.i(LOG_TAG, "releasing EGL context because paused tid=" + getId());
                  }
                }
              }

              // Have we lost the SurfaceView surface?
              if ((!mHasSurface) && (!mWaitingForSurface)) {
                if (LOG_SURFACE) {
                  Log.i(LOG_TAG, "noticed surfaceView surface lost tid=" + getId());
                }
                if (mHaveEglSurface) {
                  stopEglSurfaceLocked();
                }
                mWaitingForSurface = true;
                mSurfaceIsBad = false;
                mGLThreadManager.notifyAll();
              }

              // Have we acquired the surface view surface?
              if (mHasSurface && mWaitingForSurface) {
                if (LOG_SURFACE) {
                  Log.i(LOG_TAG, "noticed surfaceView surface acquired tid=" + getId());
                }
                mWaitingForSurface = false;
                mGLThreadManager.notifyAll();
              }

              if (doRenderNotification) {
                if (LOG_SURFACE) {
                  Log.i(LOG_TAG, "sending render notification tid=" + getId());
                }
                mWantRenderNotification = false;
                doRenderNotification = false;
                mRenderComplete = true;
                mGLThreadManager.notifyAll();
              }

              // Ready to draw?
              if (readyToDraw()) {

                // If we don't have an EGL context, try to acquire one.
                if (!mHaveEglContext) {
                  if (askedToReleaseEglContext) {
                    askedToReleaseEglContext = false;
                  } else {
                    try {
                      mEglHelper.start();
                    } catch (RuntimeException t) {
                      mGLThreadManager.releaseEglContextLocked(this);
                      throw t;
                    }
                    if (mEglHelper.mEglContext != null) {
                      mHaveEglContext = true;
                      createEglContext = true;
                      mGLThreadManager.notifyAll();
                    }
                  }
                }

                if (mHaveEglContext && !mHaveEglSurface) {
                  mHaveEglSurface = true;
                  createEglSurface = true;
                  createGlInterface = true;
                  sizeChanged = true;
                }

                if (mHaveEglSurface) {
                  if (mSizeChanged) {
                    sizeChanged = true;
                    w = mWidth;
                    h = mHeight;
                    mWantRenderNotification = true;
                    if (LOG_SURFACE) {
                      Log.i(LOG_TAG, "noticing that we want render notification tid=" + getId());
                    }

                    // Destroy and recreate the EGL surface.
                    createEglSurface = true;

                    mSizeChanged = false;
                  }
                  mRequestRender = false;
                  mGLThreadManager.notifyAll();
                  if (mWantRenderNotification) {
                    wantRenderNotification = true;
                  }
                  applySwapMode = mRequestedSwapMode != swapMode;
                  swapMode = mRequestedSwapMode;
                  break;
                }
              }

              // By design, this is the only place in a GLThread thread where we wait().
              if (LOG_THREADS) {
                Log.i(
                    LOG_TAG,
                    "waiting tid="
                        + getId()
                        + " mHaveEglContext: "
                        + mHaveEglContext
                        + " mHaveEglSurface: "
                        + mHaveEglSurface
                        + " mFinishedCreatingEglSurface: "
                        + mFinishedCreatingEglSurface
                        + " mPaused: "
                        + mPaused
                        + " mHasSurface: "
                        + mHasSurface
                        + " mSurfaceIsBad: "
                        + mSurfaceIsBad
                        + " mWaitingForSurface: "
                        + mWaitingForSurface
                        + " mWidth: "
                        + mWidth
                        + " mHeight: "
                        + mHeight
                        + " mRequestRender: "
                        + mRequestRender
                        + " mRenderMode: "
                        + mRenderMode
                        + " mRequestedSwapMode: "
                        + mRequestedSwapMode);
              }
              mGLThreadManager.wait();
            }
          } // end of synchronized(mGLThreadManager)

          if (event != null) {
            event.run();
            event = null;
            continue;
          }

          if (createEglSurface) {
            if (LOG_SURFACE) {
              Log.w(LOG_TAG, "egl createSurface");
            }
            if (mEglHelper.createSurface()) {
              synchronized (mGLThreadManager) {
                mFinishedCreatingEglSurface = true;
                mGLThreadManager.notifyAll();
              }
            } else {
              synchronized (mGLThreadManager) {
                mFinishedCreatingEglSurface = true;
                mSurfaceIsBad = true;
                mGLThreadManager.notifyAll();
              }
              continue;
            }
            createEglSurface = false;
            // If we made a new surface, the swap mode defaults to queued.
            swapMode = SWAPMODE_QUEUED;
          }

          if (createGlInterface) {
            gl = (GL10) mEglHelper.createGL();

            createGlInterface = false;
          }

          if (createEglContext) {
            if (LOG_RENDERER) {
              Log.w(LOG_TAG, "onSurfaceCreated");
            }
            CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
            if (view != null) {
              view.mRenderer.onSurfaceCreated(gl, mEglHelper.mEglConfig);
            }
            createEglContext = false;
          }

          if (sizeChanged) {
            if (LOG_RENDERER) {
              Log.w(LOG_TAG, "onSurfaceChanged(" + w + ", " + h + ")");
            }
            CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
            if (view != null) {
              view.mRenderer.onSurfaceChanged(gl, w, h);
            }
            sizeChanged = false;
          }

          if (applySwapMode) {
            mEglHelper.setEglSurfaceAttrib(
                EGL14.EGL_RENDER_BUFFER,
                swapMode == SWAPMODE_SINGLE ? EGL14.EGL_SINGLE_BUFFER : EGL14.EGL_BACK_BUFFER);
            mEglHelper.setEglSurfaceAttrib(
                EglHelper.EGL_FRONT_BUFFER_AUTO_REFRESH,
                swapMode == SWAPMODE_SINGLE ? EGL14.EGL_TRUE : EGL14.EGL_FALSE);
          }

          if (LOG_RENDERER_DRAW_FRAME) {
            Log.w(LOG_TAG, "onDrawFrame tid=" + getId());
          }
          {
            CardboardGLSurfaceView view = mCardboardGLSurfaceViewWeakRef.get();
            if (view != null) {
              view.mRenderer.onDrawFrame(gl);
            }
          }

          if (applySwapMode || swapMode == SWAPMODE_QUEUED) {
            int swapError = mEglHelper.swap();
            switch (swapError) {
              case EGL10.EGL_SUCCESS:
                break;
              case EGL11.EGL_CONTEXT_LOST:
                if (LOG_SURFACE) {
                  Log.i(LOG_TAG, "egl context lost tid=" + getId());
                }
                lostEglContext = true;
                break;
              default:
                // Other errors typically mean that the current surface is bad,
                // probably because the SurfaceView surface has been destroyed,
                // but we haven't been notified yet.
                // Log the error to help developers understand why rendering stopped.
                EglHelper.logEglErrorAsWarning(LOG_TAG, "eglSwapBuffers", swapError);

                // b/29648121: Swap will return egl errors even when everything is working fine.
                // Because we won't call swap more than a few times when in the alternate swap
                // modes, we ignore the error instead of killing the GL thread.
                if (swapMode == SWAPMODE_QUEUED) {
                  synchronized (mGLThreadManager) {
                    mSurfaceIsBad = true;
                    mGLThreadManager.notifyAll();
                  }
                }
                break;
            }
          }

          if (wantRenderNotification) {
            doRenderNotification = true;
            wantRenderNotification = false;
          }
        }

      } finally {
        /*
         * clean-up everything...
         */
        synchronized (mGLThreadManager) {
          stopEglSurfaceLocked();
          stopEglContextLocked();
        }
      }
    }

    public boolean ableToDraw() {
      return mHaveEglContext && mHaveEglSurface && readyToDraw();
    }

    private boolean readyToDraw() {
      // Keep spinning until all of the following have become true.
      return (!mPaused)
          && mHasSurface
          && (!mSurfaceIsBad)
          && (mWidth > 0)
          && (mHeight > 0)
          && (mRequestRender || (mRenderMode == RENDERMODE_CONTINUOUSLY));
    }

    public void setRenderMode(int renderMode) {
      if (!((RENDERMODE_WHEN_DIRTY <= renderMode) && (renderMode <= RENDERMODE_CONTINUOUSLY))) {
        throw new IllegalArgumentException("renderMode");
      }
      synchronized (mGLThreadManager) {
        mRenderMode = renderMode;
        mGLThreadManager.notifyAll();
      }
    }

    public void setSwapMode(int swapMode) {
      if (!((SWAPMODE_QUEUED <= swapMode) && (swapMode <= SWAPMODE_MANUAL))) {
        throw new IllegalArgumentException("swapMode");
      }
      synchronized (mGLThreadManager) {
        mRequestedSwapMode = swapMode;
        mGLThreadManager.notifyAll();
      }
    }

    public int getRenderMode() {
      synchronized (mGLThreadManager) {
        return mRenderMode;
      }
    }

    public int getSwapMode() {
      synchronized (mGLThreadManager) {
        return mRequestedSwapMode;
      }
    }

    public void requestRender() {
      synchronized (mGLThreadManager) {
        mRequestRender = true;
        mGLThreadManager.notifyAll();
      }
    }

    public void requestRenderAndWait() {
      synchronized (mGLThreadManager) {
        // If we are already on the GL thread, this means a client callback
        // has caused reentrancy, for example via updating the SurfaceView parameters.
        // We will return to the client rendering code, so here we don't need to
        // do anything.
        if (Thread.currentThread() == this) {
          return;
        }

        mWantRenderNotification = true;
        mRequestRender = true;
        mRenderComplete = false;

        mGLThreadManager.notifyAll();

        while (!mExited && !mPaused && !mRenderComplete && ableToDraw()) {
          try {
            mGLThreadManager.wait();
          } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void surfaceCreated() {
      synchronized (mGLThreadManager) {
        if (LOG_THREADS) {
          Log.i(LOG_TAG, "surfaceCreated tid=" + getId());
        }
        mHasSurface = true;
        mFinishedCreatingEglSurface = false;
        mGLThreadManager.notifyAll();
        while (mWaitingForSurface && !mFinishedCreatingEglSurface && !mExited) {
          try {
            mGLThreadManager.wait();
          } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void surfaceDestroyed(Runnable r) {
      synchronized (mGLThreadManager) {
        if (LOG_THREADS) {
          Log.i(LOG_TAG, "surfaceDestroyed tid=" + getId());
        }
        mHasSurface = false;
        if (r != null) {
          mEventQueue.add(r);
        }
        mGLThreadManager.notifyAll();
        while ((!mWaitingForSurface) && (!mExited)) {
          try {
            mGLThreadManager.wait();
          } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void onPause(Runnable r) {
      synchronized (mGLThreadManager) {
        if (LOG_PAUSE_RESUME) {
          Log.i(LOG_TAG, "onPause tid=" + getId());
        }
        mRequestPaused = true;
        if (r != null) {
          mEventQueue.add(r);
        }
        mGLThreadManager.notifyAll();
        while ((!mExited) && (!mPaused)) {
          if (LOG_PAUSE_RESUME) {
            Log.i(LOG_MAIN_THREAD_TAG, "onPause waiting for mPaused.");
          }
          try {
            mGLThreadManager.wait();
          } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void onResume() {
      synchronized (mGLThreadManager) {
        if (LOG_PAUSE_RESUME) {
          Log.i(LOG_TAG, "onResume tid=" + getId());
        }
        mRequestPaused = false;
        mRequestRender = true;
        mRenderComplete = false;
        mGLThreadManager.notifyAll();
        while ((!mExited) && mPaused && (!mRenderComplete)) {
          if (LOG_PAUSE_RESUME) {
            Log.i(LOG_MAIN_THREAD_TAG, "onResume waiting for !mPaused.");
          }
          try {
            mGLThreadManager.wait();
          } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void onWindowResize(int w, int h) {
      synchronized (mGLThreadManager) {
        mWidth = w;
        mHeight = h;
        mSizeChanged = true;
        mRequestRender = true;
        mRenderComplete = false;

        // If we are already on the GL thread, this means a client callback
        // has caused reentrancy, for example via updating the SurfaceView parameters.
        // We need to process the size change eventually though and update our EGLSurface.
        // So we set the parameters and return so they can be processed on our
        // next iteration.
        if (Thread.currentThread() == this) {
          return;
        }

        mGLThreadManager.notifyAll();

        // Wait for thread to react to resize and render a frame
        while (!mExited && !mPaused && !mRenderComplete && ableToDraw()) {
          if (LOG_SURFACE) {
            Log.i(
                LOG_MAIN_THREAD_TAG,
                "onWindowResize waiting for render complete from tid=" + getId());
          }
          try {
            mGLThreadManager.wait();
          } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void requestExitAndWait() {
      // don't call this from GLThread thread or it is a guaranteed
      // deadlock!
      synchronized (mGLThreadManager) {
        mShouldExit = true;
        mGLThreadManager.notifyAll();
        while (!mExited) {
          try {
            mGLThreadManager.wait();
          } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void requestReleaseEglContextLocked() {
      mShouldReleaseEglContext = true;
      mGLThreadManager.notifyAll();
    }

    /**
     * Queue an "event" to be run on the GL rendering thread.
     * @param r the runnable to be run on the GL rendering thread.
     */
    public void queueEvent(Runnable r) {
      if (r == null) {
        throw new IllegalArgumentException("r must not be null");
      }
      synchronized (mGLThreadManager) {
        mEventQueue.add(r);
        mGLThreadManager.notifyAll();
      }
    }

    // Once the thread is started, all accesses to the following member
    // variables are protected by the mGLThreadManager monitor
    private boolean mShouldExit;
    private boolean mExited;
    private boolean mRequestPaused;
    private boolean mPaused;
    private boolean mHasSurface;
    private boolean mSurfaceIsBad;
    private boolean mWaitingForSurface;
    private boolean mHaveEglContext;
    private boolean mHaveEglSurface;
    private boolean mFinishedCreatingEglSurface;
    private boolean mShouldReleaseEglContext;
    private int mWidth;
    private int mHeight;
    private int mRenderMode;
    private int mRequestedSwapMode;
    private boolean mRequestRender;
    private boolean mWantRenderNotification;
    private boolean mRenderComplete;
    private ArrayList<Runnable> mEventQueue = new ArrayList<Runnable>();
    private boolean mSizeChanged = true;

    // End of member variables protected by the mGLThreadManager monitor.

    private EglHelper mEglHelper;

    /**
     * Set once at thread construction time, nulled out when the parent view is garbage
     * called. This weak reference allows the CardboardGLSurfaceView to be garbage collected while
     * the GLThread is still alive.
     */
    private WeakReference<CardboardGLSurfaceView> mCardboardGLSurfaceViewWeakRef;

    private static class GLThreadManager {
      private static final String TAG = "GLThreadManager";

      public synchronized void threadExiting(GLThread thread) {
        if (LOG_THREADS) {
          Log.i(LOG_TAG, "exiting tid=" + thread.getId());
        }
        thread.mExited = true;
        notifyAll();
      }

      /*
       * Releases the EGL context. Requires that we are already in the
       * mGLThreadManager monitor when this is called.
       */
      public void releaseEglContextLocked(GLThread thread) {
        notifyAll();
      }
    }

    private final GLThreadManager mGLThreadManager = new GLThreadManager();

  }

  static class LogWriter extends Writer {

    @Override
    public void close() {
      flushBuilder();
    }

    @Override
    public void flush() {
      flushBuilder();
    }

    @Override
    public void write(char[] buf, int offset, int count) {
      for (int i = 0; i < count; i++) {
        char c = buf[offset + i];
        if (c == '\n') {
          flushBuilder();
        } else {
          mBuilder.append(c);
        }
      }
    }

    private void flushBuilder() {
      if (mBuilder.length() > 0) {
        Log.v(TAG, mBuilder.toString());
        mBuilder.delete(0, mBuilder.length());
      }
    }

    private StringBuilder mBuilder = new StringBuilder();
  }

  private void checkRenderThreadState() {
    if (mGLThread != null) {
      throw new IllegalStateException("setRenderer has already been called for this instance.");
    }
  }

  private final WeakReference<CardboardGLSurfaceView> mThisWeakRef =
      new WeakReference<CardboardGLSurfaceView>(this);
  private GLThread mGLThread;
  private Renderer mRenderer;
  private boolean mDetached;
  private EGLConfigChooser mEGLConfigChooser;
  private EGLContextFactory mEGLContextFactory;
  private EGLWindowSurfaceFactory mEGLWindowSurfaceFactory;
  private GLWrapper mGLWrapper;
  private int mDebugFlags;
  private int mEGLContextClientVersion;
  private boolean mPreserveEGLContextOnPause;
  private boolean mPreserveGlThreadOnDetachedFromWindow;
}
