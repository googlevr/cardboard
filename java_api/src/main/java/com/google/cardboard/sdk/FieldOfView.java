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

import android.opengl.Matrix;
import java.util.Arrays;
import java.util.List;
import org.jspecify.annotations.Nullable;

/**
 * Encapsulates a field of view composed of 4 half angles (left, right, bottom, top) as would be
 * passed to glFrustum.
 */
public class FieldOfView {

  // Default (Cardboard V2.2) half angles in degrees for the maximum field of view.
  // A consequence of using left/right terminology over inner/outer is that
  // left and right default angles must be identical.
  private static final float CARDBOARD_V2_2_MAX_FOV_LEFT_RIGHT = 60.0f;
  private static final float CARDBOARD_V2_2_MAX_FOV_BOTTOM = 60.0f;
  private static final float CARDBOARD_V2_2_MAX_FOV_TOP = 60.0f;

  // Parameters for Carboard V1.0.
  private static final float CARDBOARD_V1_MAX_FOV_LEFT_RIGHT = 40.0f;
  private static final float CARDBOARD_V1_MAX_FOV_BOTTOM = 40.0f;
  private static final float CARDBOARD_V1_MAX_FOV_TOP = 40.0f;

  private float left;
  private float right;
  private float bottom;
  private float top;

  public FieldOfView() {
    left = CARDBOARD_V2_2_MAX_FOV_LEFT_RIGHT;
    right = CARDBOARD_V2_2_MAX_FOV_LEFT_RIGHT;
    bottom = CARDBOARD_V2_2_MAX_FOV_BOTTOM;
    top = CARDBOARD_V2_2_MAX_FOV_TOP;
  }

  /** Returns parameters for Cardboard V1.0.0 */
  public static FieldOfView cardboardV1FieldOfView() {
    FieldOfView params = new FieldOfView();
    params.setAngles(
        CARDBOARD_V1_MAX_FOV_LEFT_RIGHT,
        CARDBOARD_V1_MAX_FOV_LEFT_RIGHT,
        CARDBOARD_V1_MAX_FOV_BOTTOM,
        CARDBOARD_V1_MAX_FOV_TOP);

    return params;
  }

  /**
   * Creates a new field of view object with the provided params.
   *
   * @param left The left field of view half-angle in degrees.
   * @param right The right field of view half-angle in degrees.
   * @param bottom The bottom field of view half-angle in degrees.
   * @param top The top field of view half-angle in degrees.
   */
  public FieldOfView(float left, float right, float bottom, float top) {
    setAngles(left, right, bottom, top);
  }

  /**
   * Constructs a new field of view object copying the contents from another.
   *
   * @param other The other FieldOfView to copy from.
   */
  public FieldOfView(FieldOfView other) {
    copy(other);
  }

  /**
   * @hide Creates a field of view object from the contents of protocol buffer field.
   * @param angles Angle array from protocol buffer field.
   * @return The new field of view object or {@code null} in case of error.
   */
  public static @Nullable FieldOfView parseFromProtobuf(float[] angles) {
    if (angles.length != 4) {
      return null;
    }

    return new FieldOfView(angles[0], angles[1], angles[2], angles[3]);
  }

  /**
   * @hide Creates a field of view object from the contents of protocol buffer field.
   * @param angles Angle array from protocol buffer field.
   * @return The new field of view object or {@code null} in case of error.
   */
  public static @Nullable FieldOfView parseFromProtobuf(List<Float> angles) {
    if (angles.size() != 4) {
      return null;
    }

    return new FieldOfView(angles.get(0), angles.get(1), angles.get(2), angles.get(3));
  }

  /**
   * @hide Encodes the current field of view information as array for protocol buffer field.
   * @return Angle array.
   */
  public float[] toProtobuf() {
    return new float[] {left, right, bottom, top};
  }

  /**
   * @hide Encodes the current field of view information as list for protocol buffer field.
   * @return Angle list.
   */
  public List<Float> toProtobufAsList() {
    return Arrays.asList(left, right, bottom, top);
  }

  /**
   * Copies the contents of another FieldOfView into this one.
   *
   * @param other The FieldOfView object to copy from.
   */
  public void copy(FieldOfView other) {
    this.left = other.left;
    this.right = other.right;
    this.bottom = other.bottom;
    this.top = other.top;
  }

  /**
   * Sets the four half-angles of the field of view.
   *
   * @param left The left field of view half-angle in degrees.
   * @param right The right field of view half-angle in degrees.
   * @param bottom The bottom field of view half-angle in degrees.
   * @param top The top field of view half-angle in degrees.
   */
  public void setAngles(float left, float right, float bottom, float top) {
    this.left = left;
    this.right = right;
    this.bottom = bottom;
    this.top = top;
  }

  /**
   * Sets the left field of view half-angle in degrees.
   *
   * @param left The left field of view half-angle in degrees.
   */
  public void setLeft(float left) {
    this.left = left;
  }

  /**
   * Returns the left field of view half-angle in degrees.
   *
   * @return The left field of view half-angle in degrees.
   */
  public float getLeft() {
    return left;
  }

  /**
   * Sets the right field of view half-angle in degrees.
   *
   * @param right The right field of view half-angle in degrees.
   */
  public void setRight(float right) {
    this.right = right;
  }

  /**
   * Returns the right field of view half-angle in degrees.
   *
   * @return The right field of view half-angle in degrees.
   */
  public float getRight() {
    return right;
  }

  /**
   * Sets the bottom field of view half-angle in degrees.
   *
   * @param bottom The bottom field of view half-angle in degrees.
   */
  public void setBottom(float bottom) {
    this.bottom = bottom;
  }

  /**
   * Returns the bottom field of view half-angle in degrees.
   *
   * @return The bottom field of view half-angle in degrees.
   */
  public float getBottom() {
    return bottom;
  }

  /**
   * Sets the top field of view half-angle in degrees.
   *
   * @param top The top field of view half-angle in degrees.
   */
  public void setTop(float top) {
    this.top = top;
  }

  /**
   * Returns the top field of view half-angle in degrees.
   *
   * @return The top field of view half-angle in degrees.
   */
  public float getTop() {
    return top;
  }

  /**
   * Generates a perspective projection matrix from this object.
   *
   * @param near The near plane.
   * @param far The far plane.
   * @param perspective The perspective matrix to fill.
   * @param offset The offset into the perspective array to write the matrix to.
   * @throws IllegalArgumentException If there is not enough space to write the result.
   */
  public void toPerspectiveMatrix(float near, float far, float[] perspective, int offset) {
    // Ensure the result fits.
    if (offset + 16 > perspective.length) {
      throw new IllegalArgumentException("Not enough space to write the result");
    }

    float l = (float) (-Math.tan(Math.toRadians(left)) * near);
    float r = (float) (Math.tan(Math.toRadians(right)) * near);
    float b = (float) (-Math.tan(Math.toRadians(bottom)) * near);
    float t = (float) (Math.tan(Math.toRadians(top)) * near);
    Matrix.frustumM(perspective, offset, l, r, b, t, near, far);
  }

  /**
   * Compares this instance with the specified object and indicates if they are equal.
   *
   * @param other The object to compare this instance with.
   * @return {@code true} if the objects are equal, {@code false} otherwise.
   */
  // TODO(b/37774152): implement hashCode() (go/equals-hashcode-lsc)
  @SuppressWarnings("EqualsHashCode")
  @Override
  public boolean equals(Object other) {
    if (other == null) {
      return false;
    }

    if (other == this) {
      return true;
    }

    if (!(other instanceof FieldOfView)) {
      return false;
    }

    FieldOfView o = (FieldOfView) other;
    return (this.left == o.left && right == o.right && bottom == o.bottom && top == o.top);
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
        .append("  left: " + left + ",\n")
        .append("  right: " + right + ",\n")
        .append("  bottom: " + bottom + ",\n")
        .append("  top: " + top + ",\n")
        .append("}")
        .toString();
  }
}
