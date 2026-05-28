package com.google.vr.cardboard.paperscope.demo.photosphere;

import android.content.Context;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.HeadTransform;
import com.google.cardboard.sdk.Viewport;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.PanoramaImage;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.PanoramaViewRenderer;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * A wrapper around a PanoramaViewRenderer that implements CardboardView.Renderer so that we can use
 * it in a CardboardView.
 */
final class PhotoSphereRenderer implements CardboardView.Renderer {
  private static final String TAG = PhotoSphereRenderer.class.getSimpleName();
  private static final float Z_NEAR = 0.1f;
  private static final float Z_FAR = 100.0f;

  private PanoramaViewRenderer renderer;
  private Context context;
  private final float[] headViewMatrix = new float[16];

  public PhotoSphereRenderer(Context context) {
    this.context = context;
  }

  public void setPanoramaImage(PanoramaImage image) {
    if (renderer == null) {
      renderer = new PanoramaViewRenderer(image, context);
    } else {
      renderer.setImage(image);
    }
  }

  @Override
  public void onNewFrame(HeadTransform headTransform) {
    if (renderer == null) {
      return;
    }
    // Store the head view matrix.
    headTransform.getHeadView(headViewMatrix, 0);

    // Use the Euler angles to get the yaw.
    float[] eulerAngles = new float[3];
    headTransform.getEulerAngles(eulerAngles, 0);
    double yaw = eulerAngles[1]; // Yaw is the second element.

    // The heading offset is used to rotate the panorama when a new one is loaded.
    renderer.setHeadingOffset((float) Math.toDegrees(yaw));
  }

  @Override
  public void onDrawEye(CardboardView.Eye eye) {
    if (renderer == null) {
      return;
    }

    // Apply the head view matrix to the eye. This is the crucial step.
    eye.applyHeadView(headViewMatrix);

    // Set the model view and projection matrices for this eye.
    renderer.setModelViewMatrix(eye.getEyeView());
    renderer.setProjectionMatrix(eye.getPerspective(Z_NEAR, Z_FAR));

    renderer.onDrawFrame((GL10) null);
  }

  @Override
  public void onFinishFrame(Viewport viewport) {
    // No need to do anything here.
  }

  @Override
  public void onSurfaceChanged(int width, int height) {
    if (renderer == null) {
      return;
    }

    renderer.onSurfaceChanged((GL10) null, width, height);
  }

  @Override
  public void onSurfaceCreated(EGLConfig config) {
    if (renderer == null) {
      return;
    }

    renderer.onSurfaceCreated((GL10) null, config);
  }

  @Override
  public void onRendererShutdown() {
    renderer = null;
    context = null;
  }
}
