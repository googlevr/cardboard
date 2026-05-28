// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.math;


/*
 * Copyright (C) 2010 The Android Open Source Project
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

/** Simple 3D vector class. Handles basic vector math for 3D vectors. */
public final class Vector3 {
  public float x;
  public float y;
  public float z;

  public static final Vector3 ZERO = new Vector3(0, 0, 0);

  public Vector3() {
  }

  public Vector3(float xValue, float yValue, float zValue) {
    set(xValue, yValue, zValue);
  }

  public Vector3(Vector3 other) {
    set(other);
  }

  public final void add(Vector3 other) {
    x += other.x;
    y += other.y;
    z += other.z;
  }

  public final void add(float otherX, float otherY, float otherZ) {
    x += otherX;
    y += otherY;
    z += otherZ;
  }

  public final void subtract(Vector3 other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
  }

  public final void multiply(float magnitude) {
    x *= magnitude;
    y *= magnitude;
    z *= magnitude;
  }

  public final void multiply(Vector3 other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
  }

  public final void divide(float magnitude) {
    if (magnitude != 0.0f) {
      x /= magnitude;
      y /= magnitude;
      z /= magnitude;
    }
  }

  public final void set(Vector3 other) {
    x = other.x;
    y = other.y;
    z = other.z;
  }

  public final void set(float xValue, float yValue, float zValue) {
    x = xValue;
    y = yValue;
    z = zValue;
  }

  public final float dot(Vector3 other) {
    return (x * other.x) + (y * other.y) + (z * other.z);
  }

  public final float length() {
    return (float) Math.sqrt(length2());
  }

  public final float length2() {
    return (x * x) + (y * y) + (z * z);
  }

  public final float distance2(Vector3 other) {
    float dx = x - other.x;
    float dy = y - other.y;
    float dz = z - other.z;
    return (dx * dx) + (dy * dy) + (dz * dz);
  }

  public final float normalize() {
    final float magnitude = length();

    if (magnitude != 0.0f) {
      x /= magnitude;
      y /= magnitude;
      z /= magnitude;
    }

    return magnitude;
  }

  public final void zero() {
    set(0.0f, 0.0f, 0.0f);
  }

  @Override
  public String toString() {
    return "" + x + ", " + y + ", " + z;
  }

  public float[] toFloatArray() {
    return new float[]{x, y, z};
  }

  public Vector3 crossProduct(Vector3 v) {
    Vector3 r = new Vector3();
    r.x = y * v.z - z * v.y;
    r.y = z * v.x - x * v.z;
    r.z = x * v.y - y * v.x;
    return r;
  }

}
