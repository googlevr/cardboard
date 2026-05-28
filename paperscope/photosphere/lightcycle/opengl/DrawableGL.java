// Copyright 2010 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.opengl.Matrix;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.math.Vector3;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Vector;

/**
 * Base class for all objects to be drawn. Each drawable contains storage for
 * its geometry, textures, and the transformation for its orientation in space.
 *
 *  Each drawable can hold a shader reference to be used for drawing.
 *
 *  Drawables can contain children to construct a scene graph. Each child is
 * drawn in the reference frame of the parent.
 *
 * @author settinger@google.com (Scott Ettinger)
 *
 */
public abstract class DrawableGL {
  // Geometry and texture storage.
  protected FloatBuffer mVertices = null;
  protected FloatBuffer mTexCoords = null;
  protected ShortBuffer mIndices = null;

  /** Storage for textures. */
  protected Vector<TextureProvider> mTextures = new Vector<TextureProvider>();

  /** Transformation. */
  protected float[] mLocalMatrix = new float[16];
  protected float[] mGlobalMatrix = new float[16];

  /** Shader. */
  protected Shader mShader = null;

  /** Graph tree. */
  protected HashSet<DrawableGL> mChildren = null;

  /** Basic state of the drawable. */
  protected final DrawableGL mParent;


  // ------------------------- Functions -------------------------------------

  public DrawableGL() {
    Matrix.setIdentityM(mLocalMatrix, 0);
    mParent = null;
  }

  public DrawableGL(DrawableGL parent) {
    Matrix.setIdentityM(mLocalMatrix, 0);
    mParent = parent;
  }

  // ----------------------------- Gets --------------------------------------

  public int getNumTextures() {
    return mTextures.size();
  }

  public Shader getShader() {
    return mShader;
  }

  // Get the transformation within the parent frame.
  public float[] getMatrix() {
    return mLocalMatrix;
  }

  // ----------------------------- Sets --------------------------------------

  public void setShader(Shader shader) {
    mShader = shader;
  }

  // ------------------------- Rendering methods -----------------------------

  // All drawing of the object in the local coordinate frame is done here
  public abstract void drawObject(float[] baseTransform) throws OpenGLException;

  /** Draw given a base transformation using the stored shader. */
  public void draw(float[] parentTransform) throws OpenGLException {
    // Compute the global matrix (parent transform applied to local transform)
    Matrix.multiplyMM(mGlobalMatrix, 0, parentTransform, 0, mLocalMatrix, 0);
    // Draw all children
    if (mChildren != null) {
      Iterator<DrawableGL> iter = mChildren.iterator();
      while (iter.hasNext()) {
        iter.next().draw(mGlobalMatrix);
      }
    }
    // Draw myself.
    drawObject(mGlobalMatrix);
  }

  /**
   * Draw given a base transformation using the stored shader. Also perform culling if applicable
   * using the given normalized look-at vector and minimum visibility dot product.
   */
  public void draw(float[] parentTransform, Vector3 lookAt, float minVisibilityDot)
      throws OpenGLException {
    cull(lookAt, minVisibilityDot);
    draw(parentTransform);
  }

  // ---------------------- Scene building methods ---------------------------

  // Add a child of the drawable.
  public void addChild(DrawableGL child) {
    if (mChildren == null) {
      mChildren = new HashSet<DrawableGL>();
    }
    mChildren.add(child);
  }

  public void remove() {
    // Remove children of this drawable.
    if (mChildren != null) {
      Iterator<DrawableGL> iter = mChildren.iterator();
      while (iter.hasNext()) {
        iter.next().remove();
      }
    }
    // Recycle any textures
    for (int i = 0; i < mTextures.size(); ++i) {
      mTextures.get(i).recycle();
    }

    // Remove this drawable from its parent.
    if (mParent != null) {
      mParent.removeChild(this);
    }
  }

  // Remove a given child drawable from the hashset.
  public void removeChild(DrawableGL child) {
    if (mChildren != null) {
      mChildren.remove(child);
    }
  }

  /**
   * Allows sub-classes to cull geometry that is not visible.
   *
   * @param lookAt the normalized look-at vector.
   * @param minVisibilityDot the minimum visibility dot product between the
   *        look-at vector and a vertex so that the vertex is considered to be
   *        in view.
   */
  protected void cull(Vector3 lookAt, float minVisibilityDot) {
    // Do nothing by default but allow sub-classes to override.
  }

  /**
   * Initializes the byte buffers for the vertices, texture coordinates and
   * indices.
   *
   * @param numVertices number of vertices.
   * @param numIndices number of indices.
   * @param allocateTextureCoords if true allocate texture coordinates per
   *        vertex.
   */
  protected void initGeometry(
      int numVertices, int numIndices, boolean allocateTextureCoords) {
    // Allocate buffer space.
    // Each vertex is 3 coordinates of 4 bytes (float)
    mVertices = ByteBuffer.allocateDirect(numVertices * 3 * 4)
        .order(ByteOrder.nativeOrder()).asFloatBuffer();

    mIndices = ByteBuffer.allocateDirect(numIndices * 2)
        .order(ByteOrder.nativeOrder()).asShortBuffer();

    if (allocateTextureCoords) {
      // Each texture coordinate is 2 coordinates of 4 bytes
      mTexCoords = ByteBuffer.allocateDirect(numVertices * 2 * 4)
          .order(ByteOrder.nativeOrder()).asFloatBuffer();
    }
  }

  /**
   * Put an index into the vertex buffer.
   *
   * @param vertexIndex - the index of the vertex to be added (in vertices - not
   *        bytes or floats)
   * @param x X coordinate.
   * @param y Y coordinate.
   * @param z Z coordinate.
   */
  protected void putVertex(int vertexIndex, float x, float y, float z) {
    int index = vertexIndex * 3;
    mVertices.put(index++, x);
    mVertices.put(index++, y);
    mVertices.put(index++, z);
  }

  /**
   * Add a value to the index buffer.
   *
   * @param position position in the buffer.
   * @param value index value to be added.
   */
  protected void putIndex(int position, short value) {
    mIndices.put(position, value);
  }

}
