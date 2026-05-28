/*
 * Copyright 2024 Google LLC
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

import android.opengl.GLES20;

/** Defines a viewport rectangle. */
public class Viewport {
  public int x;
  public int y;
  public int width;
  public int height;

  /**
   * Sets a new viewport based on the provided coordinates and dimensions.
   *
   * @param x Horizontal pixel coordinate of the bottom-left corner of the viewport.
   * @param y Vertical pixel coordinate of the bottom-left corner of the viewport.
   * @param width Width of the viewport in pixels.
   * @param height Height of the viewport in pixels.
   */
  public void setViewport(int x, int y, int width, int height) {
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
  }

  /** Sets the current OpenGL viewport based on the current viewport settings. */
  public void setGLViewport() {
    GLES20.glViewport(x, y, width, height);
  }

  /** Sets the current OpenGL scissor rect based on the current viewport settings. */
  public void setGLScissor() {
    GLES20.glScissor(x, y, width, height);
  }

  /**
   * Stores the viewport parameters into an int array.
   *
   * @param array Array to store values in.
   * @param offset Offset into the array to start writing values in.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void getAsArray(int[] array, int offset) {
    // Ensure the result fits.
    if (offset + 4 > array.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    array[offset] = x;
    array[offset + 1] = y;
    array[offset + 2] = width;
    array[offset + 3] = height;
  }

  /**
   * Returns a string containing a concise, human-readable description of this object.
   *
   * @return A printable representation of this object.
   */
  @Override
  public String toString() {
    return new StringBuilder()
        .append("{\n")
        .append("  x: " + x + ",\n")
        .append("  y: " + y + ",\n")
        .append("  width: " + width + ",\n")
        .append("  height: " + height + ",\n")
        .append("}")
        .toString();
  }

  @Override
  public boolean equals(Object obj) {
    if (obj == this) {
      return true;
    }
    if (!(obj instanceof Viewport)) {
      return false;
    }
    Viewport other = (Viewport) obj;
    return x == other.x && y == other.y && width == other.width && height == other.height;
  }

  @Override
  public int hashCode() {
    return Integer.valueOf(x).hashCode()
        ^ Integer.valueOf(y).hashCode()
        ^ Integer.valueOf(width).hashCode()
        ^ Integer.valueOf(height).hashCode();
  }
}
