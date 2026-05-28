/*
 * Copyright 2019 Google LLC
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
package com.google.cardboard.sdk.nativetypes;

/** Cardboard SDK Mesh type. */
public class Mesh {
  // Indices buffer opaque native pointer.
  public long indices;
  // Number of indices.
  public int nIndices;
  // Vertices buffer opaque native pointer. 2 floats per vertex: x, y.
  public long vertices;
  // UV coordinates buffer opaque native pointer. 2 floats per uv: u, v.
  public long uvs;
  // Number of vertices.
  public int nVertices;

  /** Creates a new mesh object. */
  public Mesh(long indices, int nIndices, long vertices, long uvs, int nVertices) {
    this.indices = indices;
    this.nIndices = nIndices;
    this.vertices = vertices;
    this.uvs = uvs;
    this.nVertices = nVertices;
  }
}
