// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.LG;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Sprite object that can be rendered in either 2D or 3D.
 *
 * @author settinger@google.com (Scott Ettinger)
 */
public class Sprite extends DrawableGL {

  private static final String TAG = "LightCycle";
  private float depth = 4.0f;

  private Point imageDim = new Point();
  private float halfX;
  private float halfY;
  private float[] transform = new float[16];
  private float[] objectTransform = new float[16];
  private int numIndices;
  private int numVertices;
  private boolean initialized = false;

  public int getWidth() {
    return imageDim.x;
  }

  public int getHeight() {
    return imageDim.y;
  }

  /**
   * Set the texture to be used to render the sprite.
   *
   * @param texture GLTexture object to be used for rendering.
   */
  public void setTexture(GLTexture texture) {
    mTextures.set(0,  new SimpleTextureProvider(texture));
  }

  private void createRenderData() {
    // Only 2 triangles.
    numIndices = 6;
    numVertices = 4;

    // Allocate buffer space.
    mVertices =
        ByteBuffer.allocateDirect(numVertices * 3 * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    mTexCoords =
        ByteBuffer.allocateDirect(numVertices * 2 * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    mIndices =
        ByteBuffer.allocateDirect(numIndices * 2).order(ByteOrder.nativeOrder()).asShortBuffer();

    mVertices.clear();
    mTexCoords.clear();
    mIndices.clear();

    halfX = imageDim.x / 2.0f;
    halfY = imageDim.y / 2.0f;

    // Create texture coordinates.
    float[] texCoordData = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};

    for (int i = 0; i < texCoordData.length; ++i) {
      mTexCoords.put(i, texCoordData[i]);
    }

    // Create the triangle indices.
    short[] indices = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < indices.length; ++i) {
      mIndices.put(i, indices[i]);
    }

    // Set the initial sprite transform to identity
    Matrix.setIdentityM(objectTransform, 0);
  }

  private boolean initFromDrawable(Context context, int drawable) {
    // Create textures
    GLTexture texture = new GLTexture();
    mTextures.add(0, new SimpleTextureProvider(texture));

    // Load the image and create the texture.
    BitmapFactory.Options opts = new BitmapFactory.Options();
    opts.inScaled = false;
    Bitmap bitmap1 =
        BitmapFactory.decodeResource(context.getResources(), drawable, opts);
    if (bitmap1 == null) {
      return false;
    }

    imageDim.set(bitmap1.getWidth(), bitmap1.getHeight());

    try {
      mTextures.get(0).getTexture().loadBitmap(bitmap1);
    } catch (OpenGLException e) {
      e.printStackTrace();
    }
    bitmap1.recycle();

    // Create all geometry and texture coordinates for rendering the sprite.
    createRenderData();

    return true;
  }

  private boolean initFromBitmap(Bitmap bitmap) {
    // Create texture.
    GLTexture texture = new GLTexture();
    mTextures.add(0, new SimpleTextureProvider(texture));
    imageDim.set(bitmap.getWidth(), bitmap.getHeight());
    try {
      mTextures.get(0).getTexture().loadBitmap(bitmap);
    } catch (OpenGLException e) {
      e.printStackTrace();
      return false;
    }

    // Create all geometry and texture coordinates for rendering the sprite.
    createRenderData();
    return true;
  }

  /**
   * Set the current transform.
   *
   * @param transform
   */
  public void setTransform(float[] transform) {
    objectTransform = transform;
  }

  /**
   * Initialize to be drawn as a 2 dimensional image (orthographic). The image will be equal in size
   * in pixels to the given drawable.
   *
   * @param context - Android context.
   * @param drawable - bitmap to use as the sprite.
   * @param depth - depth for the sprite to be drawn.
   * @param scale - scale that the sprite will be drawn relative to the actual bitmap size.
   * @return true if initialization was successful.
   */
  public boolean init2D(Context context, int drawable, float depth, float scale) {

    if (!initFromDrawable(context, drawable)) {
      return false;
    }
    this.depth = depth;
    halfX *= scale;
    halfY *= scale;
    float[] vertexData = {
      -halfX,
      halfY,
      this.depth,
      halfX,
      halfY,
      this.depth,
      halfX,
      -halfY,
      this.depth,
      -halfX,
      -halfY,
      this.depth
    };

    for (int i = 0; i < vertexData.length; ++i) {
      mVertices.put(i, vertexData[i]);
    }
    initialized = true;
    return true;
  }

  /**
   * Initialize to be drawn as a 2 dimensional image (orthographic). The image
   * will be equal in size in pixels to the given bitmap.
   *
   * @param bitmap - bitmap to use as the sprite.
   * @param depth - depth for the sprite to be drawn.
   * @param scale - scale that the sprite will be drawn relative to the actual
   *        bitmap size.
   * @return true if initialization was successful.
   */
  public boolean init2D(Bitmap bitmap, float depth, float scale) {

    initFromBitmap(bitmap);
    this.depth = depth;
    halfX *= scale;
    halfY *= scale;
    float[] vertexData = {
      -halfX,
      halfY,
      this.depth,
      halfX,
      halfY,
      this.depth,
      halfX,
      -halfY,
      this.depth,
      -halfX,
      -halfY,
      this.depth
    };

    for (int i = 0; i < vertexData.length; ++i) {
      mVertices.put(i, vertexData[i]);
    }
    initialized = true;
    return true;
  }

  /**
   * Initialize the sprite to be drawn as a 2 dimensional image (orthographic)
   * given only its dimensions and depth. Before being drawn, a texture must be
   * specified by setTexture();
   *
   * @param width
   * @param height
   * @param depth
   */
  public void init2D(int width, int height, float depth) {
    imageDim.set(width, height);
    createRenderData();

    // Create the vertex coordinates.
    halfX = width / 2.0f;
    halfY = height / 2.0f;
    this.depth = depth;
    float[] vertexData = {
      -halfX,
      halfY,
      this.depth,
      halfX,
      halfY,
      this.depth,
      halfX,
      -halfY,
      this.depth,
      -halfX,
      -halfY,
      this.depth
    };

    for (int i = 0; i < vertexData.length; ++i) {
      mVertices.put(i, vertexData[i]);
    }
    initialized = true;
  }

  /**
   * Create the sprite to be drawn in 3-D as a flat surface. The scale is given
   * by the size that the larger of width or height will be scaled to in 3space.
   *
   * @param context - Android context
   * @param drawable - bitmap to use as the sprite.
   * @param largestDimension - the size in 3-space of the largest side of the
   *        sprite.
   * @param depth - depth for the sprite to be drawn.
   * @return true if the sprite is successfully initialized.
   */
  public boolean init3D(
      Context context, int drawable, float largestDimension, float depth) {

    if (!initFromDrawable(context, drawable)) {
      return false;
    }

    float xdim;
    float ydim;
    if (imageDim.x > imageDim.y) {
      xdim = largestDimension;
      ydim = largestDimension * imageDim.y / imageDim.x;
    } else {
      ydim = largestDimension;
      xdim = largestDimension * imageDim.x / imageDim.y;
    }

    this.depth = depth;
    float x2 = xdim / 2;
    float y2 = ydim / 2;
    float[] vertexData = {
      -x2, y2, this.depth, x2, y2, this.depth, x2, -y2, this.depth, -x2, -y2, this.depth
    };

    for (int i = 0; i < vertexData.length; ++i) {
      mVertices.put(i, vertexData[i]);
    }
    initialized = true;
    return true;
  }

  /**
   * Draw the sprite using the base class draw function. This function will bind
   * the shader upon every call. For drawing multiple sprites, call
   * drawNoBind().
   */
  @Override
  public void drawObject(float[] transform) throws OpenGLException {}

  /**
   * Draw the sprite in 2-D with u and v as the top, left coordinates of the
   * sprite. The transform parameter should be an orthographic transform with
   * 1:1 pixel units.
   *
   * @param transform - 4x4 transform matrix
   * @param u - left edge of sprite.
   * @param v - top edge of sprite.
   * @throws OpenGLException
   */
  public void draw(float[] transform, float u, float v) throws OpenGLException {

    if (!initialized) {
      Log.e(TAG, "Sprite not initialized.");
    }

    // Select the shader.
    mShader.bind();

    // Set shader and geometry.
    mVertices.position(0);
    mTexCoords.position(0);
    mShader.setVertices(mVertices);
    mShader.setTexCoords(mTexCoords);
    Matrix.translateM(this.transform, 0, transform, 0, u + halfX, v + halfY, 0);
    mShader.setTransform(this.transform);

    if (mTextures.isEmpty()) {
      LG.d("Error : no textures defined for Sprite");
      return;
    }
    mTextures.get(0).getTexture().bind(mShader);

    // Draw the message.
    mIndices.position(0);
    GLES20.glDrawElements(GLES20.GL_TRIANGLES, numIndices, GLES20.GL_UNSIGNED_SHORT, mIndices);
  }

  /**
   * Draw the sprite in 2-D with u and v as the top, left coordinates of the
   * sprite. The transform parameter should be an orthographic transform with
   * 1:1 pixel units.
   *
   * @param transform 4x4 transform matrix
   * @param u center U coordinate of the sprite.
   * @param v center V coordinate of the sprite.
   * @param rotationAngleDegrees the rotation angle for the sprite to be drawn.
   *        The sprite will be rotated about its center.
   * @throws OpenGLException
   */
  public void drawRotated(float[] transform, float u, float v,
      float rotationAngleDegrees, float scale) throws OpenGLException {

    if (!initialized) {
      Log.e(TAG, "Sprite not initialized.");
      return;
    }

    // Select the shader.
    if (mShader == null) {
      LG.d("The shader does not exist.");
      return;
    }
    mShader.bind();

    // Set shader and geometry.
    mVertices.position(0);
    mTexCoords.position(0);
    mShader.setVertices(mVertices);
    mShader.setTexCoords(mTexCoords);
    Matrix.translateM(this.transform, 0, transform, 0, u, v, 0);
    Matrix.rotateM(this.transform, 0, rotationAngleDegrees, 0, 0, 1.0f);
    if (scale != 1.0f) {
      Matrix.scaleM(this.transform, 0, scale, scale, scale);
    }
    mShader.setTransform(this.transform);

    if (mTextures.isEmpty()) {
      LG.d("Error : no textures defined for Sprite");
      return;
    }
    mTextures.get(0).getTexture().bind(mShader);

    // Draw the message.
    mIndices.position(0);
    GLES20.glDrawElements(GLES20.GL_TRIANGLES, numIndices, GLES20.GL_UNSIGNED_SHORT, mIndices);
  }

  /**
   * Draw the sprite in 2-D with u and v as the center of the sprite. The
   * transform parameter should be an orthographic transform with 1:1 pixel
   * units.
   *
   * @param transform 4x4 transform matrix
   * @param u center U coordinate of the sprite.
   * @param v center V coordinate of the sprite.
   * @param rotationAngleDegrees the rotation angle for the sprite to be drawn.
   *        The sprite will be rotated about its center.
   * @throws OpenGLException
   */
  public void drawRotatedCentered(
      float[] transform, float u, float v, float rotationAngleDegrees)
      throws OpenGLException {

    if (!initialized) {
      Log.e(TAG, "Sprite not initialized.");
      return;
    }

    // Select the shader.
    if (mShader == null) {
      LG.d("The shader does not exist.");
      return;
    }
    mShader.bind();

    // Set shader and geometry.
    mVertices.position(0);
    mTexCoords.position(0);
    mShader.setVertices(mVertices);
    mShader.setTexCoords(mTexCoords);
    Matrix.translateM(this.transform, 0, transform, 0, u + halfX, v + halfY, 0);
    Matrix.rotateM(this.transform, 0, rotationAngleDegrees, 0, 0, 1.0f);
    mShader.setTransform(this.transform);

    if (mTextures.isEmpty()) {
      LG.d("Error : no textures defined for Sprite");
      return;
    }
    mTextures.get(0).getTexture().bind(mShader);

    // Draw the message.
    mIndices.position(0);
    GLES20.glDrawElements(GLES20.GL_TRIANGLES, numIndices, GLES20.GL_UNSIGNED_SHORT, mIndices);
  }

  /**
   * Draw given a global transform without first binding to a shader. Use this
   * when drawing multiple copies of the same sprite.
   *
   * @param globalTransform - 4x4 transform matrix for the sprite.
   * @throws OpenGLException
   */
  public void drawNoBinding(float[] globalTransform) throws OpenGLException {

    if (!initialized) {
      Log.e(TAG, "Sprite not initialized.");
      return;
    }

    // Set geometry.
    mVertices.position(0);
    mTexCoords.position(0);
    mShader.setVertices(mVertices);
    mShader.setTexCoords(mTexCoords);
    Matrix.multiplyMM(transform, 0, globalTransform, 0, objectTransform, 0);
    mShader.setTransform(transform);

    if (mTextures.isEmpty()) {
      LG.d("Error : no textures defined for Sprite2d");
      return;
    }
    mTextures.get(0).getTexture().bind(mShader);

    // Draw the sprite.
    mIndices.position(0);
    GLES20.glDrawElements(GLES20.GL_TRIANGLES, numIndices, GLES20.GL_UNSIGNED_SHORT, mIndices);
  }

  protected static float computeScale(
      Context context, int drawable, float pctWidth, int surfaceWidth) {
    BitmapFactory.Options opts = new BitmapFactory.Options();
    opts.inJustDecodeBounds = true;
    Bitmap bitmap = BitmapFactory.decodeResource(
        context.getResources(), drawable, opts);

    float scale = pctWidth * surfaceWidth / opts.outWidth;
    // If the scale is close to 1:1, use 1:1.
    float tolerance = 0.1f;
    return (scale > 1.0f - tolerance && scale < 1.0f + tolerance) ? 1.0f
        : scale;
  }

  protected static float computeScale(
      Bitmap bitmap, float pctWidth, int surfaceWidth) {
    float scale = pctWidth * surfaceWidth / bitmap.getWidth();

    // If the scale is close to 1:1, use 1:1.
    float tolerance = 0.1f;
    return (scale > 1.0f - tolerance && scale < 1.0f + tolerance) ? 1.0f
        : scale;
  }

  /**
   * Factory to create a sprite given a drawable sized to be resolution
   * independent. It creates the sprite scaled to be a percentage of the OpenGL
   * surface width.
   *
   * @param context - Android context.
   * @param drawable - Android drawable.
   * @param pctWidth - width of the sprite as a percentage of the surface width.
   * @param surfaceWidth - width of the OpenGL surface.
   * @param depth - depth that the sprite should be displayed (typically -1).
   * @return the newly initialized Sprite object.
   */
  public static Sprite create2dScaled(Context context, int drawable,
      float pctWidth, int surfaceWidth, int depth) {
    float scale = computeScale(context, drawable, pctWidth, surfaceWidth);
    Sprite sprite = new Sprite();
    sprite.init2D(context, drawable, -1.0f, scale);
    return sprite;
  }

}
