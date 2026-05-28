// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer;

import android.content.Context;
import android.graphics.RectF;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.Constants;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.math.Vector3;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.OpenGLException;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.PartialSphere;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.TextureLoaderManager;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.shaders.TransparencyShader;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.Callback;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.LG;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 *
 * This class renders all objects in the 3D panorama viewer scene. The viewing
 * angle is controlled via set functions.
 *
 * Note that this renderer assumes that the associated GLSurfaceView renders
 * continuously, avoiding the need for explicit GLSurfaceView.requestRender()
 * calls when the underlying texture changes.
 *
 * NOTE(dcoz): edited to remove dependency on PanoramaView and get rid of the
 * cubemap dependency which we don't use.
 *
 * @author settinger@google.com (Scott Ettinger)
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class PanoramaViewRenderer implements GLSurfaceView.Renderer {
  private static final String TAG = PanoramaViewRenderer.class.getSimpleName();

  /** Auto-spin rate in degrees per frame at 90 degree FOV. */
  private static final float FINAL_PANORAMA_OPACITY = 1.0f;

  private static final float PANORAMA_FADE_IN_ALPHA = 0.05f;

  // Rendering state.
  private boolean objectsInitialized = false;
  private PanoramaImage image;

  // View state.
  private float panoramaOpacity = 1;

  // Rendering objects.
  private PartialSphere panoramaSphere;
  private TextureLoaderManager textureLoaderManager;

  // Shaders.
  private TransparencyShader panoSphereShader;

  // Transforms.
  private float[] projection = new float[16];
  private float[] clientModelView = new float[16];
  private float[] modelView = new float[16];
  private float[] vPMatrix = new float[16];
  private float[] offsetMatrix = new float[16];

  // Heading angle offset in degrees.
  private float headingOffsetDeg;

  // Sensor data.
  private Callback<Void> onInitializedCallback = null;

  private boolean initializationRequired = false;

  private RectF imageBoundsDeg = new RectF();

  /**
   * Instantiates a new renderer. Allows the cubemap resource id to be specified
   * when this class is used outside Lightcycle code
   *
   * @param image the image that will be displayed by this renderer
   * @param context the android context associated with this renderer
   */
  public PanoramaViewRenderer(PanoramaImage image, Context context) {
    textureLoaderManager = new TextureLoaderManager(image.getTileProvider());
    this.setPanoramaImage(image);
    Matrix.setIdentityM(offsetMatrix, 0);
    Matrix.setIdentityM(clientModelView, 0);
  }

  /** Sets the model view matrix. */
  public void setModelViewMatrix(float[] modelViewMatrix) {
    System.arraycopy(modelViewMatrix, 0, clientModelView, 0, clientModelView.length);
  }

  /** Sets the projection matrix. */
  public void setProjectionMatrix(float[] projectionMatrix) {
    System.arraycopy(projectionMatrix, 0, projection, 0, projection.length);
  }

  /**
   * Sets the heading offset in degrees, which should be taken into account when a new image is
   * loaded.
   */
  public void setHeadingOffset(float headingOffsetDeg) {
    this.headingOffsetDeg = headingOffsetDeg;
  }

  private void setPanoramaImage(PanoramaImage image) {
    this.image = image;
    textureLoaderManager.reset();

    float widthDegrees = (float) Math.toDegrees(this.image.getPanoWidthRad());
    imageBoundsDeg.left = (float) (Math.toDegrees(this.image.getOffsetLeftRad() - Math.PI));
    imageBoundsDeg.right = imageBoundsDeg.left + widthDegrees;

    float heightDegrees = (float) Math.toDegrees(this.image.getPanoHeightRad());
    imageBoundsDeg.top = 90f - (float) Math.toDegrees(this.image.getOffsetTopRad());
    imageBoundsDeg.bottom = imageBoundsDeg.top - heightDegrees;
  }

  /**
   * Sets the callback that gets fired when the scene is initialized.
   */
  public void setOnInitializedCallback(Callback<Void> callback) {
    onInitializedCallback = callback;
    if (objectsInitialized) {
      callback.onCallback(null);
    }
  }

  public void destroy() {
    textureLoaderManager.reset();
  }

  private void initFrame() {
    GLES20.glEnable(GLES20.GL_DEPTH_TEST);
  }

  /**
   * Calculate the model view and projection matrices based on the current pitch
   * and yaw.
   */
  private synchronized void updateMatrices() {
    // NOTE(dcoz): directly use client-provided model view and projection matrices.

    // Multiply client modelview matrix by the offset matrix.
    Matrix.multiplyMM(modelView, 0, clientModelView, 0, offsetMatrix, 0);
    Matrix.multiplyMM(vPMatrix, 0, projection, 0, modelView, 0);
  }

  /** Draw all objects in the scene. */
  private void drawScene() {
    updateMatrices();

    // Clear the buffers and set the viewport.
    initFrame();

    try {
      // NOTE(dcoz): we don't need a cubemap since we're displaying a full 360 panorama.
      // Draw environment cube.
      // mTexturedCube.draw(vPMatrix);

      // Draw the wireframe sphere.
      GLES20.glLineWidth(1);
      GLES20.glEnable(GLES20.GL_BLEND);
      GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

      // Draw the panorama.
      panoSphereShader.bind();
      panoSphereShader.setAlpha(panoramaOpacity);

      // NOTE(eparker): Use dummy values for lookAt and minVisibilityDot since
      // they are only used for culling but isVisible() in CurvedTile.java has
      // been hacked to always return true so all tiles are drawn each frame
      // currently.  If we decide we need culling then we'll need to compute
      // lookAt minVisibilityDot properly, but the way the old code was
      // computing them was incorrect.
      Vector3 lookAt = new Vector3(0, 0, 1);
      float minVisibilityDot = 0.0f;
      panoramaSphere.draw(vPMatrix, lookAt, minVisibilityDot);

    } catch (OpenGLException e) {
      e.printStackTrace();
    }
  }

  /**
   * Render a frame when requested by the system.
   * NOTE(dcoz): this function assumes the gl viewport was set beforehand by the caller.
   */
  @Override
  public void onDrawFrame(GL10 gl) {
    if (!objectsInitialized) {
      return;
    }

    if (initializationRequired) {
      image.getTileProvider().setMaximumTextureSize(getMaxTextureSize());
      panoramaSphere.setImage(image, textureLoaderManager);
      initializationRequired = false;
    }

    // Update the Panorama fade opacity.
    panoramaOpacity += (FINAL_PANORAMA_OPACITY - panoramaOpacity) * PANORAMA_FADE_IN_ALPHA;

    // Draw the scene oriented with the user's motion.
    if (objectsInitialized) {
      drawScene();
    }
  }

  /**
   * Set the new image and trigger a re-initialization of textures and geometry.
   */
  public void setImage(PanoramaImage image) {
    // Before loading a new image, compute a heading offset so that the new image is always centered
    // where the user was looking at.
    final float mHeadingOffset = headingOffsetDeg - image.getPanoMetadata().poseHeadingDegrees;
    Matrix.setIdentityM(offsetMatrix, 0);
    Matrix.rotateM(offsetMatrix, 0, mHeadingOffset, 0, 1, 0);

    this.image = image;
    textureLoaderManager = new TextureLoaderManager(image.getTileProvider());
    initializationRequired = true;
  }

  /**
   * Initialize all rendering objects when the surface is initially created.
   */
  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    if (image == null) {
      Log.d(TAG, "Image file not set. Cannot initialize rendering.");
      return;
    }

    if (!objectsInitialized) {
      try {
        // Initialize all rendering objects.
        initRendering();
      } catch (OpenGLException e) {
        e.printStackTrace();
      }
    }
    LG.d("Rendering init completed.");
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {}

  /**
   * Initializes rendering.
   * <p>
   * Note: Needs to be called from the OpenGL rendering thread.
   */
  private void initRendering() throws OpenGLException {
    initializationRequired = false;

    // Create shaders.
    panoSphereShader = new TransparencyShader();

    // Create the filled panorama Sphere
    float kRadius = 4.9f;
    image.getTileProvider().setMaximumTextureSize(getMaxTextureSize());
    panoramaSphere = new PartialSphere(kRadius, textureLoaderManager);
    panoramaSphere.setImage(image, textureLoaderManager);
    panoramaSphere.setShader(panoSphereShader);

    // Misc. setup.
    GLES20.glClearColor(Constants.BACKGROUND_BLACK[0],
        Constants.BACKGROUND_BLACK[1], Constants.BACKGROUND_BLACK[2],
        Constants.BACKGROUND_BLACK[3]);

    objectsInitialized = true;

    if (onInitializedCallback != null) {
      onInitializedCallback.onCallback(null);
    }
  }

  /**
   * Get the maximum supported texture size.
   *
   * @return the maximum supported texture size in pixels.
   */
  private int getMaxTextureSize() {
    int[] result = new int[1];
    GLES20.glGetIntegerv(GLES20.GL_MAX_TEXTURE_SIZE, result, 0);
    return result[0];
  }
}
