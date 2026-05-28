// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.graphics.Bitmap;
import android.opengl.GLES20;
import android.opengl.GLUtils;

/**
 * OpenGL texture management object. It supports creating a texture from a
 * bitmap.
 *
 * @author settinger@google.com (Scott Ettinger)
 */
public class GLTexture {

  /**
   * Type to define the type of texture to create. A standard texture uses
   * bilinear interpolation while a nearest neighbor texture does not.
   */
  public enum TextureType {
    Standard, NearestNeighbor
  }

  private int textureIndex = -1;

  public GLTexture() {}

  // TODO(settinger): re-factor this to make creating a texture more consistent
  // with one create function that takes parameters for
  // interpolation and mip-mapping and takes an optional
  // bitmap.
  public GLTexture(TextureType type) {
    textureIndex =
        switch (type) {
          case Standard -> GLTexture.createStandardTexture();
          case NearestNeighbor -> GLTexture.createNNTexture();
          default -> GLTexture.createStandardTexture();
        };
  }

  /** Get the OpenGL texture index. */
  public int getIndex() {
    return textureIndex;
  }

  /** Clear the texture and clear the associated resources. */
  public void recycle() {
    int[] textures = new int[] {textureIndex};
    GLES20.glDeleteTextures(1, textures, 0);
    textureIndex = -1;
  }

  /** Bind the texture to the shader. */
  public void bind(Shader shader) throws OpenGLException {
    if (textureIndex < 0) {
      throw new OpenGLException("Trying to bind without a loaded texture");
      // return;
    }
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureIndex);
    OpenGLException.logError("glBindTexture");
  }

  /** Create a texture from a bitmap object. */
  public void loadBitmap(Bitmap bitmap) throws OpenGLException {
    int[] textures = new int[1];
    GLES20.glGenTextures(1, textures, 0);
    textureIndex = textures[0];
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureIndex);

    // Use linear interpolation for up-sampling and down-sampling
    GLES20.glTexParameterf(
        GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
    GLES20.glTexParameterf(
        GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

    // Clamp all textures to their edge
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
        GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
        GLES20.GL_CLAMP_TO_EDGE);

    GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
    OpenGLException.logError("Texture : loadBitmap");
    bitmap.recycle();
  }

  /** Create a texture with Linear interpolation sampling. */
  public static int createStandardTexture() {
    int[] textures = new int[1];
    GLES20.glGenTextures(1, textures, 0);

    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);

    // Disable MIP-mapping here.
    GLES20.glTexParameterf(
        GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);

    // Use linear interpolation for up sampling
    GLES20.glTexParameterf(
        GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

    // Clamp all textures to their edge
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
        GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
        GLES20.GL_CLAMP_TO_EDGE);
    return textures[0];
  }

  /** Create a texture with nearest neighbor sampling. */
  public static int createNNTexture() {
    int[] textures = new int[1];
    GLES20.glGenTextures(1, textures, 0);

    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);

    // Disable MIP-mapping here.
    GLES20.glTexParameterf(
        GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);

    // Use linear interpolation for up sampling
    GLES20.glTexParameterf(
        GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);

    // Clamp all textures to their edge
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
        GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
        GLES20.GL_CLAMP_TO_EDGE);
    return textures[0];
  }

  /** Set the OpenGL texture id for this texture. */
  public void setIndex(int id) {
    textureIndex = id;
  }

}
