// Copyright 2013 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

/**
 * Provides and recycles a texture.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public interface TextureProvider {
  /**
   * Provides a texture or returns null if a texture cannot be provided at this
   * point.
   *
   * @return A texture or null.
   */
  public GLTexture getTexture();

  /**
   * Recycles the texture, if present.
   */
  public void recycle();

  /**
   * This should be called from the GL thread. if a bitmap is ready this creates
   * a valid texture from it. Otherwise this is a no-op.
   * <p>
   * It does NOT trigger the loading of the texture as {@link #getTexture()}
   * does. Call this method in the rendering loop to make sure textures are
   * created when the bitmap becomes available.
   */
  public void createTexture();

  /**
   * Returns the alpha that should be used for the texture. Implementations
   * might animate the alpha over time when new textures have been loaded.
   */
  public float getAlphaForRendering();
}
