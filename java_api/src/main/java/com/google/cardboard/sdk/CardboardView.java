/*
 * Copyright 2019 Google LLC
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

import static android.opengl.GLU.gluErrorString;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.EGLWindowSurfaceFactory;
import android.opengl.Matrix;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import com.google.cardboard.sdk.nativetypes.EyeTextureDescription;
import com.google.cardboard.sdk.nativetypes.EyeType;
import com.google.cardboard.sdk.nativetypes.Mesh;
import com.google.cardboard.sdk.utils.MathUtils;
import java.nio.IntBuffer;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Wrapper around {@code CardboardViewApi} and {@code GLSurfaceView}. Cardboard SDK provides stereo
 * rendering only but this class offers stereo and monocular rendering seamlessly by reusing the
 * same {@code GLSurfaceView}.
 */
public class CardboardView extends FrameLayout {
  /** Defines an interface of an Eye, a useful bundle to the renderer when drawing. */
  public static class Eye {
    public static final int LEFT = EyeType.LEFT;
    public static final int RIGHT = EyeType.RIGHT;
    public static final int MONOCULAR = 2;

    /** Default vertical FOV for monocular mode. */
    static final float MONOCULAR_FOV_Y_DEGREES = 45.0f;

    /**
     * Holds the eye type, one value in the set {{@code LEFT}, {@code RIGHT}, {@code MONOCULAR}}.
     */
    private final int eyeType;

    /** Holds a reference to {@code CardboardViewApi} to compute eye matrices. */
    private final CardboardViewApi cardboardViewApi;

    /** Holds the Transformation of the from the head after the head was rotated */
    private final float[] eyeViewMatrix;

    /** Holds {@code LensDistortion} eye from head matrix for a device */
    private final float[] eyeFromHeadMatrix = new float[16];

    /** Holds {@code LensDistortion} eye projection matrix for a device */
    private final float[] projectionMatrix = new float[16];

    public Eye(int eyeType, CardboardViewApi cardboardViewApi) {
      this.eyeType = eyeType;
      eyeViewMatrix = new float[16];
      this.cardboardViewApi = cardboardViewApi;
      resetEyeFromHeadMatrix();
    }

    public void resetEyeFromHeadMatrix() {
      if (this.cardboardViewApi == null) {
        return;
      }

      if (this.eyeType == MONOCULAR) {
        this.cardboardViewApi.getEyeFromHeadMatrix(EyeType.LEFT, eyeFromHeadMatrix);
      } else {
        this.cardboardViewApi.getEyeFromHeadMatrix(this.eyeType, eyeFromHeadMatrix);
      }
    }

    /**
     * Gets the eye type.
     *
     * @return An integer describing the which eye this class represents. It can be one of {{@code
     *     LEFT}, {@code RIGHT}, {@code MONOCULAR}}.
     */
    public int getEyeType() {
      return eyeType;
    }

    /**
     * Gets the eye from head matrix.
     *
     * @return A float 4x4 matrix representing the eye from head matrix.
     */
    public float[] getEyeFromHead() {
      return eyeFromHeadMatrix;
    }

    public void applyHeadView(float[] headView) {
//      resetEyeFromHeadMatrix();
      Matrix.multiplyMM(eyeViewMatrix, 0, eyeFromHeadMatrix, 0, headView, 0);
    }

    /**
     * Returns a matrix that transforms from the camera to the current eye. This transform is the
     * multiplication of the head rotation and the eye from head transformation.
     *
     * @return A 4x4 column-major representing the transformation from the camera to this eye.
     */
    public float[] getEyeView() {
      return eyeViewMatrix;
    }

    /**
     * Gets the perspective matrix for the input z-axis clipping planes.
     *
     * @param[in] zNear Near clip plane z-axis coordinate.
     * @param[in] zFar Far clip plane z-axis coordinate.
     * @return A float 4x4 matrix representing the perspective matrix.
     */
    public float[] getPerspective(float zNear, float zFar) {
      if (eyeType == MONOCULAR) {
        computeMonocularPerspectiveMatrix(zNear, zFar);
      } else {
        cardboardViewApi.getEyeProjectionMatrix(eyeType, zNear, zFar, projectionMatrix);
      }
      return projectionMatrix;
    }

    /**
     * Constructs a float array with device's field of view angles.
     *
     * @return A float array whose elements are "[left, right, bottom, top]" angles.
     */
    public float[] getFieldOfView() {
      final float[] fov = new float[4];
      fov[0] = cardboardViewApi.getFieldOfViewParamsLeft();
      fov[1] = cardboardViewApi.getFieldOfViewParamsRight();
      fov[2] = cardboardViewApi.getFieldOfViewParamsBottom();
      fov[3] = cardboardViewApi.getFieldOfViewParamsTop();
      return fov;
    }

    /**
     * Computes in {@code projectionMatrix} the projection matrix for monocular rendering mode.
     *
     * <p>Screen size is extracted from {@code cardboardViewApi} and from it, the viewport aspect
     * ratio is derived. A fixed vertical angular span is used in conjunction with the z-axis
     * clipping planes.
     *
     * @param zNear Near clip plane z-axis coordinate.
     * @param zFar Far clip plane z-axis coordinate.
     */
    private void computeMonocularPerspectiveMatrix(float zNear, float zFar) {
      // For monocular rendering always use a fixed vertical FOV and calculate the horizontal FOV
      // correspondingly using the aspect ratio of the screen.
      float aspectRatio =
          cardboardViewApi.getScreenParamsWidth()
              / (float) cardboardViewApi.getScreenParamsHeight();
      Matrix.perspectiveM(projectionMatrix, 0, MONOCULAR_FOV_Y_DEGREES, aspectRatio, zNear, zFar);
    }
  }

  /**
   * Defines a rendering interface to be consumed by the {@code GLSurfaceView} when set.
   *
   * <p>Note: all callbacks will be executed in the rendering thread.
   */
  public interface Renderer {
    /**
     * Called when a new frame is about to be drawn.
     *
     * <p>Allows to differenciate between rendering eye views and different frames. Any per-frame
     * operations not specific to a single view should happen here.
     *
     * @param headTransform The head transformation in the new frame. It is a 4x4 matrix.
     */
    public void onNewFrame(HeadTransform headTransform);

    /**
     * Requests to draw the contents from the point of view of an eye.
     *
     * <p>When monocular rendering mode is set, {@code onDrawEye()} will be called once per frame
     * with monocular eye information. When stereo rendering mode is set, this method is called for
     * the left eye first and then for the right eye.
     *
     * @param eye The eye type and matrix information.
     */
    public void onDrawEye(Eye eye);

    /** Called before a frame is finished. */
    public void onFinishFrame(Viewport viewport);

    /**
     * Called when there is a change in the surface dimensions.
     *
     * @param width New width of the surface for a single eye in pixels.
     * @param height New height of the surface for a single eye in pixels.
     */
    public void onSurfaceChanged(int width, int height);

    /**
     * Called when the surface is created or recreated.
     *
     * @param config The EGL configuration used when creating the surface.
     */
    public void onSurfaceCreated(EGLConfig config);

    /**
     * Called on the renderer thread when the thread is shutting down.
     *
     * <p>Allows releasing GL resources and performing shutdown operations in the renderer thread.
     * Called only if there was a previous call to {@link #onSurfaceCreated(EGLConfig)
     * onSurfaceCreated}.
     */
    public void onRendererShutdown();
  }

  /** Aggregate type to hold eye specific stereo eye data. */
  private static class EyeData {
    /** Eye configuration. */
    public final Eye eye;

    /** Cardboard SDK eye textrure description. */
    public final EyeTextureDescription textureDescription;

    public EyeData(int eyeType, CardboardViewApi cardboardViewApi) {
      eye = new Eye(eyeType, cardboardViewApi);
      textureDescription = new EyeTextureDescription();
    }
  }

  /**
   * Interface of a safe surface view.
   *
   * <p>This interface is two fold: provides by means of the adapter pattern a common interface to
   * interact with {@code CardboardGLSurfaceView} and {@code GLSurfaceView}, and subclasses them to
   * call a shutdown runnable when detached from the window.
   */
  private static interface SafeSurfaceView {
    /**
     * The following list of methods are almost all that {@code GLView} offers. {@code
     * GLSurfaceView} implements such interface, but {@code CardboardGLSurfaceView} doesn't.
     */
    public void setEGLContextClientVersion(int version);

    public void setRenderer(android.opengl.GLSurfaceView.Renderer renderer);

    public void setRenderMode(int mode);

    public void onPause();

    public void onResume();

    public void setPreserveEGLContextOnPause(boolean preserveOnPause);

    public void setOnTouchListener(android.view.View.OnTouchListener listener);

    public void setLayoutParams(ViewGroup.LayoutParams glViewLayoutParams);
    /** The following list of methods are part of the {@code GLSurfaceView} public API. */
    public void setEGLConfigChooser(
        int redSize, int greenSize, int blueSize, int alphaSize, int depthSize, int stencilSize);

    public void setEGLWindowSurfaceFactory(EGLWindowSurfaceFactory factory);

    public void queueEvent(Runnable r);
    /**
     * The following two methods conform the extended "safe" interface of the surface to execute a
     * runnable when the view is detached.
     */
    public void setOnViewDetachedRunnable(Runnable onViewDetachedRunnable);

    public boolean isViewAttached();

    public void requestRender();
  }

  /**
   * Implementation of {@code SafeSurfaceView} for {@code CardboardGLSurfaceView}.
   *
   * <p>Most of the methods forwards calls to {@code CardboardGLSurfaceView} implementation. The
   * rest offers mechanisms to call a runnable when the view is detached from the window.
   */
  private static class SafeCardboardGLSurfaceView extends CardboardGLSurfaceView
      implements SafeSurfaceView {
    private boolean isViewAttached;
    private Runnable onViewDetachedRunnable;

    /**
     * Constructs a SafeCardboardGLSurfaceView.
     *
     * @param[in] context Unused parameter within this class, required by parent class. It is
     *     generally an Activity instance or wraps one.
     */
    public SafeCardboardGLSurfaceView(Context context) {
      super(context);
      isViewAttached = false;
    }

    @Override
    protected void onDetachedFromWindow() {
      if (onViewDetachedRunnable != null) {
        onViewDetachedRunnable.run();
      }
      isViewAttached = false;
      super.onDetachedFromWindow();
    }

    @Override
    protected void onAttachedToWindow() {
      super.onAttachedToWindow();
      isViewAttached = true;
    }

    @Override
    public void setOnViewDetachedRunnable(Runnable onViewDetachedRunnable) {
      this.onViewDetachedRunnable = onViewDetachedRunnable;
    }

    @Override
    public boolean isViewAttached() {
      return isViewAttached;
    }
  }

  /**
   * Implementation of {@code SafeSurfaceView} for {@code GLSurfaceView}.
   *
   * <p>Most of the methods forwards calls to {@code GLSurfaceView} implementation. The rest offers
   * mechanisms to call a runnable when the view is detached from the window.
   */
  private static class SafeGLSurfaceView extends GLSurfaceView implements SafeSurfaceView {
    private boolean isViewAttached;
    private Runnable onViewDetachedRunnable;

    /**
     * Constructs a SafeGLSurfaceView.
     *
     * @param[in] context Unused parameter within this class, required by parent class. It is
     *     generally an Activity instance or wraps one.
     */
    public SafeGLSurfaceView(Context context) {
      super(context);
      isViewAttached = false;
    }

    @Override
    protected void onDetachedFromWindow() {
      if (onViewDetachedRunnable != null) {
        onViewDetachedRunnable.run();
      }
      isViewAttached = false;
      super.onDetachedFromWindow();
    }

    @Override
    protected void onAttachedToWindow() {
      super.onAttachedToWindow();
      isViewAttached = true;
    }

    @Override
    public void setOnViewDetachedRunnable(Runnable onViewDetachedRunnable) {
      this.onViewDetachedRunnable = onViewDetachedRunnable;
    }

    @Override
    public boolean isViewAttached() {
      return isViewAttached;
    }
  }

  /** Thread with its own Looper to serve Choreographer. */
  private static class ChoreographerThread extends Thread {
    private volatile Handler handler;

    @Override
    public void run() {
      Looper.prepare();
      handler =
          new Handler(Looper.myLooper()) {
            @Override
            public void handleMessage(Message msg) {}
          };
      Looper.loop();
    }

    /**
     * Posts a message to Choreographer to request the rendering of @p view upon the next VSYNC
     * callback.
     *
     * @param[in] view A SafeSurfaceView to request the rendering.
     */
    public synchronized void requestRenderOnVsyncEvent(final SafeSurfaceView view) {
      if (handler != null) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
          handler.post(
              () -> Choreographer.getInstance().postFrameCallback((t) -> view.requestRender()));
        } else {
          handler.post(
              () -> Choreographer.getInstance().postVsyncCallback((t) -> view.requestRender()));
        }
      }
    }

    /**
     * When {@code handler} is not null, quits the Looper making it not possible to continue sending
     * messages.
     */
    public synchronized void stopLooper() {
      if (handler != null) {
        handler.getLooper().quit();
        handler = null;
      }
    }
  }

  private static final String TAG = CardboardView.class.getSimpleName();

  /** Timeout in milliseconds to wait for the Choreographer thread to join. */
  private static final int CHOREOGRAPHER_THREAD_JOIN_TIMEOUT_MS = 100;

  /** Tracks when screen parameters changed. */
  private boolean screenParamsChanged;

  /** Tracks when device parameters changed. */
  private boolean deviceParamsChanged;

  /** Holds left eye specific data. */
  private EyeData leftEyeData;

  /** Holds right eye specific data. */
  private EyeData rightEyeData;

  /** Holds monocular eye specific data. */
  private Eye monocularEye;

  /** OpenGL depth render buffer. */
  private final int[] depthRenderBuffer = new int[1];

  /** OpenGL frame buffer. */
  private final int[] framebuffer = new int[1];

  /** OpenGL texture. */
  private final int[] texture = new int[1];

  /** Cardboard SDK API. */
  private CardboardViewApi cardboardViewApi;

  /** OpenGL Surface. */
  private SafeSurfaceView glView;

  /** Custom client renderer. */
  private Renderer renderer;

  /** Stereo rendering mode status. */
  private boolean stereoRenderMode;

  /** Cardboard layout manager. */
  private CardboardLayout cardboardLayout;

  /**
   * Alias of {@code stereoRenderMode} updated when the frame is rendered. It avoids any change in
   * the rendering mode while the frame is drawn.
   */
  private boolean currentStereoRenderMode;

  private long lastLogTime;

  /** Tracks whether a shutdown sequence has started. */
  private volatile boolean shutdownCalled = false;

  /** Tracks whether to use Gvr GlSurfaceView or Android's. */
  private static boolean useCardboardGlSurfaceView = false;

  /** Thread to execute Choreographer VSync callbacks. */
  private ChoreographerThread choreographerThread;

  /**
   * Selects which GlSurfaceView to use.
   *
   * <p>Consumers of this class should call this class method before constructing an instance. By
   * default, the Android GlSurfaceView will be preferred (wrapped with the SafeGlSurfaceView class)
   * if this function is not called.
   *
   * @param[in] useCardboardGlSurfaceView True when Gvr GlSurfaceView must be used, else false.
   */
  public static void setUseCardboardGlSurfaceView(boolean useCardboardGlSurfaceView) {
    CardboardView.useCardboardGlSurfaceView = useCardboardGlSurfaceView;
    Log.w(TAG, "CardboardSV(" + useCardboardGlSurfaceView + ")");
  }

  /**
   * Constructs a CardboardView.
   *
   * @param[in] context The current Context. It is generally an Activity instance or wraps one. The
   *     following is the list of methods that will be queried: getFilesDir(), getPackageManager(),
   *     getResources(), getSystemService(Context.WINDOW_SERVICE), startActivity(Intent),
   *     getContentResolver(), getDisplay().
   */
  public CardboardView(Context context) {
    super(context);
    Log.w(TAG, "CW(C)");
    initialize(context);
  }

  /**
   * Constructs a CardboardView.
   *
   * @param[in] context The current Context. It is generally an Activity instance or wraps one. The
   *     following is the list of methods that will be queried: getFilesDir(), getPackageManager(),
   *     getResources(), getSystemService(Context.WINDOW_SERVICE), startActivity(Intent),
   *     getContentResolver(), getDisplay().
   * @param[in] attrs The custom AttributeSet.
   */
  public CardboardView(Context context, AttributeSet attributeSet) {
    super(context, attributeSet);
    Log.w(TAG, "CW(C,A)");
    initialize(context);
  }

  /**
   * Initializes a CardboardView.
   *
   * <p>Constructs the {@code CardboardViewApi} and the {@code GLSurfaceView}. For the latter, a
   * custom {@code GLSurfaceView.Renderer} is created to forward calls to private methods that will
   * handle the management of the GL resources and {@code CardboardViewApi}.
   *
   * <p>Monocular rendering is configured, meaning that {@code stereoRenderMode} is initialized with
   * false.
   *
   * @param[in] context The current Context. It is generally an Activity instance or wraps one. See
   *     {@code CardboardView(Context)} for more details.
   */
  private void initialize(Context context) {
    cardboardViewApi = new CardboardViewApi(context);

    renderer = null;

    // Sets and adds GLSurfaceView to CardboardView layout.
    if (useCardboardGlSurfaceView) {
      Log.w(TAG, "Using Cardboard Surface");
      glView = new SafeCardboardGLSurfaceView(context);
    } else {
      Log.w(TAG, "Using Android Surface");
      glView = new SafeGLSurfaceView(context);
    }

    glView.setEGLContextClientVersion(2);
    // The following line makes the surface to preserve the context between resumed-paused state
    // transitions.
    // GvrView sets it which makes dependent and already integrated code assume that the Gl
    // context will be preserved.
    // When it is set to false (or default is used, i.e. false), client code will experiment calls
    // to Renderer::onSurfaceCreated() after resuming the GlSurfaceView from a prior pause state.
    // The callback is used to request a Gl context creation because the surface has been created
    // for the first time or re-created.
    //
    // See b/186201897 to understand what this line could cause if not set.
    // See {@code GlSurfaceView::setPreserveEGLContextOnPause()} for further details.
    glView.setPreserveEGLContextOnPause(true);
    ViewGroup.LayoutParams glViewLayoutParams =
        new ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
    glView.setLayoutParams(glViewLayoutParams);
    addView((SurfaceView) glView);

    // Sets and adds CardboardLayout to CardboardView layout.
    cardboardLayout = new CardboardLayout(context, this);
    setStereoRenderMode(false);
    screenParamsChanged = false;
    deviceParamsChanged = false;

    addView(cardboardLayout.getView());
  }

  /**
   * Sets the renderer that will be called from {@code GLSurfaceView}'s callbacks.
   *
   * @param[in] renderer The CardboardView.Renderer implementation to be consumed by GLSurfaceView.
   */
  public void setRenderer(Renderer renderer) {
    Log.w(TAG, "SR");
    this.renderer = renderer;
    lastLogTime = SystemClock.uptimeMillis();
    glView.setRenderer(
        new GLSurfaceView.Renderer() {
          @Override
          public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
            CardboardView.this.onSurfaceCreated(eglConfig);
          }

          @Override
          public void onSurfaceChanged(GL10 gl10, int width, int height) {
            CardboardView.this.onSurfaceChanged(width, height);
          }

          @Override
          public void onDrawFrame(GL10 gl10) {
            CardboardView.this.onDrawFrame();
            if (!shutdownCalled && choreographerThread != null) {
              choreographerThread.requestRenderOnVsyncEvent(glView);
            }
          }
        });
    glView.setRenderMode(CardboardGLSurfaceView.RENDERMODE_WHEN_DIRTY);
    choreographerThread = new ChoreographerThread();
    choreographerThread.start();
    choreographerThread.requestRenderOnVsyncEvent(glView);
  }

  /**
   * Gets the enabled rendering mode.
   *
   * @return true When stereo rendering mode is enabled. Otherwise, monocular rendering is enabled.
   */
  public boolean isStereoRenderModeEnabled() {
    return stereoRenderMode;
  }

  /**
   * Sets the enabled rendering mode.
   *
   * <p>This method can be called from any thread. However, the change (if any) will be effective
   * next time the {@code GLSurfaceView} in the rendering thead executes {@code onDrawFrame()}
   * callback.
   *
   * @param[in] enable When true, stereo rendering mode will be enabled. Otherwise, monocular
   *     rendering becomes effective.
   */
  public void setStereoRenderMode(boolean enable) {
    Log.w(TAG, "SSRMode(" + enable + ")");

    stereoRenderMode = enable;
    screenParamsChanged = true;

    // Setups the layout according to the selected mode.
    if (enable) {
      cardboardLayout.showAll();
    } else {
      cardboardLayout.hideAll();
    }
  }

  /**
   * Pauses the {@code HeadTracker} module and the {@code GLSurfaceView}.
   *
   * <p>Generally, this method should be called from the Activity that contains it at {@code
   * onPause()} callback.
   *
   * <p>Preconditions: Must not be called before a renderer has been set as it forwards the call to
   * the {@code GLSurfaceView}.
   */
  public void onPause() {
    Log.w(TAG, "OP");
    cardboardViewApi.pauseHeadTracker();
    glView.onPause();
  }

  /**
   * Resumes the {@code HeadTracker} module and the {@code GLSurfaceView}.
   *
   * <p>Generally, this method should be called from the Activity that contains it at {@code
   * onResume()} callback.
   *
   * <p>Preconditions: Must not be called before a renderer has been set as it forwards the call to
   * the {@code GLSurfaceView}.
   */
  public void onResume() {
    Log.w(TAG, "OR()");
    // Parameters may have changed.
    deviceParamsChanged = true;
    glView.onResume();
    cardboardViewApi.resumeHeadTracker();
  }

  /**
   * Loads device parameter configuration. See {@code CardboardViewApi::loadDeviceParams()} for an
   * in detail explanation of how and where parameters are obtained.
   */
  public void loadDeviceParams() {
    Log.w(TAG, "LDP");
    // Parameters may have changed.
    deviceParamsChanged = true;
    cardboardViewApi.loadDeviceParams();
  }

  /** Settings (gear) button callback. */
  public void onSettingsButtonClick() {
    Log.w(TAG, "OSBC");
    cardboardViewApi.scanViewerQrCode();
  }

  /** Scans viewer parameters. */
  public void scanViewerQrCode() {
    Log.w(TAG, "SVQRC");
    cardboardViewApi.scanViewerQrCode();
  }

  /**
   * Closes all taken resources by {@code CardboardViewApi}.
   *
   * <p>Generally, this method should be called from the Activity that contains it at {@code
   * onDestroy()} callback.
   */
  public void onDestroy() {
    Log.w(TAG, "OD");
    shutdownCalled();
    if (choreographerThread != null) {
      try {
        choreographerThread.stopLooper();
        choreographerThread.join(CHOREOGRAPHER_THREAD_JOIN_TIMEOUT_MS);
      } catch (SecurityException e) {
        Log.e(TAG, "OD-SE");
      } catch (InterruptedException e) {
        Log.e(TAG, "OD-IE");
      } finally {
        choreographerThread = null;
      }
    }
    cardboardViewApi.close();
  }

  /** Forwards the call to the underlying GL view. */
  public void setEGLConfigChooser(
      int redSize, int greenSize, int blueSize, int alphaSize, int depthSize, int stencilSize) {
    glView.setEGLConfigChooser(redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize);
  }

  /** Forwards the call to the underlying GL view. */
  public void setEGLWindowSurfaceFactory(EGLWindowSurfaceFactory factory) {
    glView.setEGLWindowSurfaceFactory(factory);
  }

  /** Returns whether the GL view is attached. */
  public boolean isGlViewAttached() {
    return glView.isViewAttached();
  }

  /** Sets a runnable that should be executed when the GL view is detached. */
  public void setOnViewDetachedRunnable(Runnable onViewDetachedRunnable) {
    Log.w(TAG, "SOVDR");
    glView.setOnViewDetachedRunnable(onViewDetachedRunnable);
  }

  /**
   * Sets a {@code Runnable} to be executed in {@code GLSurfaceView}'s GL Thread.
   *
   * <p>Preconditions: Must not be called before a renderer has been set as it forwards the call to
   * the {@code GLSurfaceView}.
   *
   * @param[in] runnable Runnable to execute.
   */
  public void queueEvent(Runnable runnable) {
    glView.queueEvent(runnable);
  }

  /**
   * Sets a {@code Runnable} as the back button event handler.
   *
   * @param[in] listener A {@code Runnable} to be executed when the back button is clicked.
   */
  public void setOnBackButtonClick(Runnable listener) {
    Log.w(TAG, "SOBBC");
    cardboardLayout.setOnBackButtonClick(listener);
  }

  /**
   * Sets a {@code Runnable} as the settings button event.
   *
   * @param[in] listener A {@code Runnable} to be executed when the settings button is clicked.
   */
  public void setOnSettingsButtonClick(Runnable listener) {
    Log.w(TAG, "SOSBC");
    cardboardLayout.setOnSettingskButtonClick(listener);
  }

  /**
   * Sets a {@code Runnable} to be executed when the viewer's trigger is pressed (screen is
   * touched).
   *
   * @param[in] runnable Runnable to execute.
   */
  public void setOnTriggerEvent(Runnable runnable) {
    Log.w(TAG, "SOTE");
    glView.setOnTouchListener(
        (v, event) -> {
          if (runnable != null && event.getAction() == MotionEvent.ACTION_DOWN) {
            runnable.run();
            v.performClick();
            return true;
          }
          return false;
        });
  }

  /**
   * Flags from *any* thread that the shutdown sequence has started.
   *
   * <p>This method does not replace {@code CardboardView::onDestroy()} which must be still called
   * to properly close resources. However, application code can use this flag to prevent {@code
   * onSurfaceCreated()}, {@code onSurfaceChanged()} and {@code onDrawFrame()} callbacks from
   * running next time the GlSurfaceView executes them before the enqueued run ({@code
   * CardboardView::onDestroy()} is typically enqueued in the Gl Thread).
   */
  public void shutdownCalled() {
    shutdownCalled = true;
  }

  /** Updates device parameters and GL resources if needed. */
  private boolean updateDeviceParams() {
    // Checks if screen or device parameters changed.
    if (!screenParamsChanged && !deviceParamsChanged) {
      return true;
    }

    // Turns down both flags. Note when they change:
    // - screenParamsChanged will be set when the surface changes ({@code onSurfaceChange()}) or
    //   when the stereoscopic view changes ({@code setStereoRenderMode()}).
    // - deviceParamsChanged will be set when the application resumes after a pause state
    //   ({@code onResume()}) under the assumption that parameters may have changed by other
    //   application (Android R or below) and when a request to {@code loadDeviceParams()} is
    //   performed.
    screenParamsChanged = false;
    deviceParamsChanged = false;

    // Gets saved device parameters.
    // Note: the following call does not touch {@code deviceParamsChange}.
    cardboardViewApi.loadDeviceParams();

    // When using monocular rendering, we must ensure to teardown previously configured GL resources
    // for stereo rendering. Monocular rendering setup is trivial, so no configuration is needed.
    if (!currentStereoRenderMode) {
      glTeardown();
      // Recreates the eye data for monocular rendering.
      monocularEye = new Eye(Eye.MONOCULAR, cardboardViewApi);
      return true;
    }

    // Gets distortion meshes.
    Mesh leftMesh = cardboardViewApi.getDistortionMesh(EyeType.LEFT);
    Mesh rightMesh = cardboardViewApi.getDistortionMesh(EyeType.RIGHT);

    // Recreates the eye data.
    leftEyeData = new EyeData(Eye.LEFT, cardboardViewApi);
    rightEyeData = new EyeData(Eye.RIGHT, cardboardViewApi);

    glSetup();

    cardboardViewApi.initializeRenderThread();

    // Sets distortion meshes.
    cardboardViewApi.setMesh(leftMesh, EyeType.LEFT);
    cardboardViewApi.setMesh(rightMesh, EyeType.RIGHT);

    checkGlError("updateDeviceParams");

    return true;
  }

  /**
   * Setups GLES20 components.
   *
   * <p>Modifies the OpenGL global state. In particular: - glGet(GL_TEXTURE_BINDING_2D) -
   * glGet(GL_FRAMEBUFFER_BINDING)
   */
  private void glSetup() {
    if (framebuffer[0] != 0) {
      glTeardown();
    }

    // Creates render texture.
    GLES20.glGenTextures(1, IntBuffer.wrap(texture));
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture[0]);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexImage2D(
        GLES20.GL_TEXTURE_2D,
        0,
        GLES20.GL_RGB,
        cardboardViewApi.getScreenParamsWidth(),
        cardboardViewApi.getScreenParamsHeight(),
        0,
        GLES20.GL_RGB,
        GLES20.GL_UNSIGNED_BYTE,
        null);

    // Sets eyes texture description.
    leftEyeData.textureDescription.texture = (long) texture[0];
    leftEyeData.textureDescription.leftU = 0f;
    leftEyeData.textureDescription.rightU = 0.5f;
    leftEyeData.textureDescription.topV = 1f;
    leftEyeData.textureDescription.bottomV = 0f;

    rightEyeData.textureDescription.texture = (long) texture[0];
    rightEyeData.textureDescription.leftU = 0.5f;
    rightEyeData.textureDescription.rightU = 1f;
    rightEyeData.textureDescription.topV = 1f;
    rightEyeData.textureDescription.bottomV = 0f;

    // Generate depth buffer to perform depth test.
    GLES20.glGenRenderbuffers(1, IntBuffer.wrap(depthRenderBuffer));
    GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, depthRenderBuffer[0]);
    GLES20.glRenderbufferStorage(
        GLES20.GL_RENDERBUFFER,
        GLES20.GL_DEPTH_COMPONENT16,
        cardboardViewApi.getScreenParamsWidth(),
        cardboardViewApi.getScreenParamsHeight());
    checkGlError("Create Render buffer");

    // Creates the render target.
    GLES20.glGenFramebuffers(1, IntBuffer.wrap(framebuffer));
    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer[0]);
    GLES20.glFramebufferTexture2D(
        GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, texture[0], 0);
    GLES20.glFramebufferRenderbuffer(
        GLES20.GL_FRAMEBUFFER,
        GLES20.GL_DEPTH_ATTACHMENT,
        GLES20.GL_RENDERBUFFER,
        depthRenderBuffer[0]);

    checkGlError("glSetup");
  }

  /** Teardowns GLES20 components. */
  private void glTeardown() {
    if (framebuffer[0] == 0) {
      return;
    }

    GLES20.glDeleteRenderbuffers(1, IntBuffer.wrap(depthRenderBuffer));
    depthRenderBuffer[0] = 0;
    GLES20.glDeleteFramebuffers(1, IntBuffer.wrap(framebuffer));
    framebuffer[0] = 0;
    GLES20.glDeleteTextures(1, IntBuffer.wrap(texture));
    texture[0] = 0;

    checkGlError("glTeardown");
  }

  /**
   * Gets the pose matrix.
   *
   * <p>If the head tracker query results in a no-op because it is not initialized, this function
   * will return an identity transform.
   *
   * @return 4x4 pose matrix as a float array.
   */
  private float[] getPose() {
    float[] posePosition = {0.0f, 0.0f, 0.0f};
    float[] poseOrientation = {0.0f, 0.0f, 0.0f, 1.0f};
    cardboardViewApi.getPose(posePosition, poseOrientation);
    return MathUtils.getMatrixFromPose(posePosition, poseOrientation);
  }

  /** Inner callback that just redirects to {@code renderer} the call. */
  private void onSurfaceCreated(EGLConfig eglConfig) {
    if (shutdownCalled) {
      Log.w(TAG, "OSCRS");
      return;
    }
    if (renderer != null) {
      renderer.onSurfaceCreated(eglConfig);
    }
  }

  /**
   * Inner callback that updates {@code cardboardViewApi} screen size and forwards the call to the
   * {@code renderer}.
   *
   * @param width New width of the surface in pixels.
   * @param height New height of the surface in pixels.
   */
  private void onSurfaceChanged(int width, int height) {
    Log.w(TAG, "OSC(" + width + ", " + height + ")");
    if (shutdownCalled) {
      Log.w(TAG, "OSCS");
      return;
    }
    cardboardViewApi.setScreenParams(width, height);
    screenParamsChanged = true;

    if (renderer != null) {
      try {
        renderer.onSurfaceChanged(width, height);
      } catch (Throwable throwable) {
        errorLogCurrentState();
        throw throwable;
      }
    }
  }

  /** Logs every at roughly 1Hz that the renderer is active and the selected renderer mode. */
  private void logOnDrawFrame() {
    final long currentTime = SystemClock.uptimeMillis();
    if ((currentTime - lastLogTime) < 1000) {
      return;
    }
    lastLogTime = currentTime;
    Log.w(TAG, "DF");
  }

  /**
   * Inner callback that manages the draw frame event.
   *
   * <p>Saves the value of {@code stereoRenderMode} in {@code currentStereoRenderMode} and uses the
   * latter during the span of this method and preserve the rendering mode while this method is
   * executed. Note that it can be changed from the GUI thread while this one runs on the rendering
   * one.
   *
   * <p>Updates device parameters if necessary and retrieves the head pose to call {@code
   * Renderer.onNewFrame()}. Then, the frame buffer is cleared, viewports are set and the client
   * rendering via {@code Render.onDrawEye()} happens. Finally, {@code Renderer.onFinishFrame()} is
   * called to free any GL resources.
   *
   * <p>When {@code currentStereoRenderMode} is true, left and right eyes are rendered using
   * Cardboard SDK. Otherwise, calls to Open GL are done to clean the viewport and channels and only
   * left eye is rendered.
   */
  private void onDrawFrame() {
    if (shutdownCalled) {
      Log.w(TAG, "DFS");
      return;
    }
    // Guarantees that stereoRenderMode is preserved while onDrawFrame() is executed.
    currentStereoRenderMode = stereoRenderMode;

    logOnDrawFrame();

    if (!updateDeviceParams()) {
      return;
    }

    // Updates Head Pose.
    if (renderer != null) {
      try {
        HeadTransform headTransform = new HeadTransform();
        System.arraycopy(getPose(), 0, headTransform.getHeadView(), 0, 16);

        renderer.onNewFrame(headTransform);
      } catch (Throwable throwable) {
        errorLogCurrentState();
        throw throwable;
      }
    }

    // Draws the frames.
    if (currentStereoRenderMode) {
      onRenderStereoFrame();
    } else {
      onRenderMonocularFrame();
    }

    // Finishes the frame.
    if (renderer != null) {
      try {
        Viewport viewport = new Viewport();
        viewport.setViewport(
            0,
            0,
            cardboardViewApi.getScreenParamsWidth(),
            cardboardViewApi.getScreenParamsHeight());
        renderer.onFinishFrame(viewport);
      } catch (Throwable throwable) {
        errorLogCurrentState();
        throw throwable;
      }
    }
    checkGlError("onDrawFrame");
  }

  /**
   * Renders left and right eye using Cardboard SDK.
   *
   * <p>When {@code currentStereoRenderMode} is false, it does nothing.
   *
   * <p>Modifies the OpenGL global state. In particular: - glGet(GL_FRAMEBUFFER_BINDING) -
   * glGet(GL_VIEWPORT)
   */
  private void onRenderStereoFrame() {
    if (!currentStereoRenderMode) {
      return;
    }

    // Binds framebuffer.
    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer[0]);
    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

    // Draws eyes views.
    if (renderer != null) {
      GLES20.glViewport(
          0,
          0,
          cardboardViewApi.getScreenParamsWidth() / 2,
          cardboardViewApi.getScreenParamsHeight());
      try {
        renderer.onDrawEye(leftEyeData.eye);
      } catch (Throwable throwable) {
        errorLogCurrentState();
        throw throwable;
      }

      GLES20.glViewport(
          cardboardViewApi.getScreenParamsWidth() / 2,
          0,
          cardboardViewApi.getScreenParamsWidth() / 2,
          cardboardViewApi.getScreenParamsHeight());
      try {
        renderer.onDrawEye(rightEyeData.eye);
      } catch (Throwable throwable) {
        errorLogCurrentState();
        throw throwable;
      }
    }

    // Renders eye textures to display.
    cardboardViewApi.renderEyeToDisplay(
        leftEyeData.textureDescription, rightEyeData.textureDescription);
  }

  /**
   * Renders left eye without any distortion.
   *
   * <p>When {@code currentStereoRenderMode} is true, it does nothing.
   *
   * <p>Modifies the OpenGL global state. In particular: - glGet(GL_CLEAR_COLOR_VALUE) -
   * glGet(GL_VIEWPORT)
   */
  private void onRenderMonocularFrame() {
    if (currentStereoRenderMode) {
      return;
    }

    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
    GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (renderer != null) {
      GLES20.glViewport(
          0, 0, cardboardViewApi.getScreenParamsWidth(), cardboardViewApi.getScreenParamsHeight());
      try {
        renderer.onDrawEye(monocularEye);
      } catch (Throwable throwable) {
        errorLogCurrentState();
        throw throwable;
      }
    }
  }

  /**
   * Checks GLES20.glGetError and fails quickly if the state isn't GL_NO_ERROR.
   *
   * @param label Label to report in case of error.
   */
  private static void checkGlError(String label) {
    int error = GLES20.glGetError();
    int lastError;
    if (error != GLES20.GL_NO_ERROR) {
      do {
        lastError = error;
        Log.w(TAG, label + ": glError " + gluErrorString(lastError));
        error = GLES20.glGetError();
      } while (error != GLES20.GL_NO_ERROR);

      throw new RuntimeException("glError " + gluErrorString(lastError));
    }
  }

  /** Logs the state of this class instance. */
  private void errorLogCurrentState() {
    Log.w(
        TAG,
        "screenParamsChanged: "
            + screenParamsChanged
            + " | deviceParamsChanged: "
            + deviceParamsChanged
            + " | stereoRenderMode: "
            + stereoRenderMode
            + " | currentStereoRenderMode: "
            + currentStereoRenderMode
            + " | renderer != null: "
            + (renderer != null));
  }
}
