// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.opengl.GLES20;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.math.Vector3;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

/**
 * Contains one texture as well as all the vertices the texture is mapped on. In
 * the simplest case, this is just a square, but usually contains a bunch of
 * vertices to e.g. describe a part of a sphere the texture should be mapped
 * onto.
 *
 * TODO(haeberling): This should ideally be a DrawableGL.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class CurvedTile {
  /** The ID of the texture that is assigned to this tile. */
  public final int textureId;

  private final int numVertices;
  private final int numIndices;

  private int vertIndex = 0;
  private FloatBuffer vertices;
  private FloatBuffer texCoords;
  private ShortBuffer indices;

  /**
   * This contains all the vertices of this tile. The data is the same as in
   * 'vertices' but optimized for faster access and normalized in order to do
   * culling quickly.
   */
  private Vector3[] normVertexArray;

  // TODO: b/352536125 - Remove this field for it is never read.
  private boolean visible = false;

  /**
   * Instantiates a new {@link CurvedTile} instance.
   *
   * @param textureId the ID of the texture to use.
   * @param tesselationFactor defines by how much the geometry of the tile is
   *        sub-divided. A factor of 1 means there is no sub-division as a tile
   *        has 2x2 vertices, 2 means 3x3, 3 means 4x4 etc.
   */
  public CurvedTile(int textureId, int tesselationFactor) {
    this.textureId = textureId;

    final int verticesPerEdge = tesselationFactor + 1;
    this.numVertices = verticesPerEdge * verticesPerEdge;

    // 3 = vertices per triangle, 2 = trianges per quad.
    this.numIndices = (verticesPerEdge - 1) * (verticesPerEdge - 1) * 3 * 2;
    this.initBuffers();

    // Add the indices.
    int index = 0;
    for (int y = 0; y < verticesPerEdge - 1; ++y) {
      int rowStart = y * verticesPerEdge;
      for (int x = 0; x < verticesPerEdge - 1; ++x) {
        int vertexIndex = rowStart + x;

        /**
         * Add two triangles to define a quad. Given the following indices, the
         * order in which they are added is: 0, 1, 2 2, 1, 3.
         *
         * <pre>
         (2)-----(3)
          | \     |
          |  \    |
          |   \   |
          |    \  |
          |     \ |
         (0)-----(1)
         </pre>
         */
        this.indices.put(index++, (short) (vertexIndex + 0));
        this.indices.put(index++, (short) (vertexIndex + 1));
        this.indices.put(index++, (short) (vertexIndex + verticesPerEdge));
        this.indices.put(index++, (short) (vertexIndex + verticesPerEdge));
        this.indices.put(index++, (short) (vertexIndex + 1));
        this.indices.put(index++, (short) (vertexIndex + verticesPerEdge + 1));
      }
    }

    // Add the texture coordinates. The texture will span the whole geometry, so
    // texture coordinates are simply linearly interpolated.
    index = 0;
    for (int y = 0; y < verticesPerEdge; ++y) {
      for (int x = 0; x < verticesPerEdge; ++x) {
        float u = x / (float) (verticesPerEdge - 1);
        float v = 1f - (y / (float) (verticesPerEdge - 1));
        this.texCoords.put(index++, u);
        this.texCoords.put(index++, v);
      }
    }
  }

  /**
   * Sets the given vertices as the vertex buffer of this instance. Removes all
   * previously added vertices.
   */
  public void setVertices(Vertex[] vertices) {
    // Each vertex is 3 coordinates of 4 bytes (float)
    this.vertices = ByteBuffer.allocateDirect(numVertices * 3 * 4)
        .order(ByteOrder.nativeOrder()).asFloatBuffer();
    vertIndex = 0;
    normVertexArray = new Vector3[vertices.length];

    for (int i = 0; i < vertices.length; ++i) {
      Vertex vertex = vertices[i];
      vertex.addToBuffer(this.vertices, vertIndex);
      vertIndex += 3;

      // Normalize so we can use the dot product in a range from -1 to 1 for
      // visibility testing later.
      Vector3 vector = new Vector3(vertex.x, vertex.y, vertex.z);
      vector.normalize();
      normVertexArray[i] = vector;
    }
  }

  /**
   * Draws this instance using the given shader.
   */
  public void draw(Shader shader) {
    this.vertices.position(0);
    shader.setVertices(this.vertices);

    this.texCoords.position(0);
    shader.setTexCoords(this.texCoords);

    this.indices.position(0);
    // TODO(haeberling): Use GL_TRIANGLE_STRIP at some point, for improved
    // performance.
    GLES20.glDrawElements(GLES20.GL_TRIANGLES, this.numIndices,
        GLES20.GL_UNSIGNED_SHORT, this.indices);
  }

  /**
   * Determines and sets the visibility of this tile based on the look-at vector
   * of the scene and the minimum visibility dot product.
   *
   * @param lookAt the normalized look-at vector.
   * @param minVisibilityDot the minimum visibility dot product between the
   *        look-at vector and a vertex so that the vertex is considered to be
   *        in view.
   */
  public void determineVisibility(Vector3 lookAt, float minVisibilityDot) {
    this.visible = false;

    for (int i = 0; i < this.normVertexArray.length; ++i) {
      if (this.normVertexArray[i].dot(lookAt) >= minVisibilityDot) {
        this.visible = true;
        return;
      }
    }
  }

  /**
   * @return Whether this tile is visible.
   */
  public boolean isVisible() {
    // NOTE(dcoz): Consider that tiles are always visible to avoid flickering effects with wider
    // field of view. We may want to revisit this if memory taken by textures is an issue.
    // return this.visible;
    return true;
  }

  /**
   * Initializes the buffers.
   */
  private void initBuffers() {
    // Each texture coordinate is 2 coordinates of 4 bytes
    this.texCoords = ByteBuffer.allocateDirect(numVertices * 2 * 4)
        .order(ByteOrder.nativeOrder()).asFloatBuffer();
    this.indices = ByteBuffer.allocateDirect(numIndices * 2)
        .order(ByteOrder.nativeOrder()).asShortBuffer();
    this.normVertexArray = new Vector3[0];
  }
}
