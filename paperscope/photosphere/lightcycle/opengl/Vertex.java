// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import java.nio.FloatBuffer;

/**
 * Simple vertex data class.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class Vertex {
  public final float x;
  public final float y;
  public final float z;

  public Vertex(float x, float y, float z) {
    this.x = x;
    this.y = y;
    this.z = z;
  }

  /**
   * Add this vertex to the specified vertex buffer.
   *
   * @param buffer the buffer to add this vertex to.
   * @param offset the offset.
   */
  public void addToBuffer(FloatBuffer buffer, int offset) {
    buffer.put(offset, x);
    buffer.put(offset + 1, y);
    buffer.put(offset + 2, z);
  }
}
