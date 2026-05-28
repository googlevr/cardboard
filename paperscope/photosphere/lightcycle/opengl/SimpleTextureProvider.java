// Copyright 2013 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

/**
 * A simple provider which wraps around the given GLTexture.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class SimpleTextureProvider implements TextureProvider {
  private final GLTexture texture;

  public SimpleTextureProvider(GLTexture texture) {
    this.texture = texture;
  }

  @Override
  public GLTexture getTexture() {
    return texture;
  }

  @Override
  public void recycle() {
    texture.recycle();
  }

  @Override
  public void createTexture() {
    // Nothing to do.
  }

  @Override
  public float getAlphaForRendering() {
    return 1;
  }
}
