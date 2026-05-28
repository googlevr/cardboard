// Copyright 2010 Google Inc. All Rights Reserved.

// Shader base class.

// Base class for shaders.  It provides handle indices for vertices,
// texture coordinates, and transforms along with methods for connecting
// data buffers for rendering and for shader creation and binding.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.content.Context;
import android.opengl.GLES20;
import java.io.IOException;
import java.io.InputStream;
import java.nio.FloatBuffer;

/**
 * @author settinger@google.com (Scott Ettinger)
 *
 * OpenGL shader base class object.  It handles the common operations for
 * shaders.
 *
 */
public abstract class Shader {

  // Attribute Handles.
  protected int mVertexIndex = -1;
  protected int mTextureCoordIndex = -1;
  protected int mTransformIndex = -1;
  protected int mSamplerIndex = -1;

  // Shader program index.
  protected int mProgram;

  // ----------- Gets for handle indices ---------------------

  public int getVertexIndex() {
    return mVertexIndex;
  }

  public int getTextureCoordIndex() {
    return mTextureCoordIndex;
  }

  public int getTransformIndex() {
    return mTransformIndex;
  }

  public int getSamplerIndex() {
    return mSamplerIndex;
  }

  // ------------------- Sets for rendering data ---------------------------

  public void setVertices(FloatBuffer vertices) {
    // If the shader does not take vertices as input then exit.
    if (mVertexIndex < 0) {
      return;
    }

    // Set the vertex data.
    GLES20.glVertexAttribPointer(mVertexIndex, 3,  GLES20.GL_FLOAT,
      false,  4 * 3, vertices);
    GLES20.glEnableVertexAttribArray(mVertexIndex);
  }

  public void setTexCoords(FloatBuffer texCoords) {
    // If the shader does not take texture coordinates as input then exit.
    if (mTextureCoordIndex < 0)  {
      return;
    }

    // Set the texture coordinate data.
    GLES20.glVertexAttribPointer(mTextureCoordIndex, 2, GLES20.GL_FLOAT, false,
      0, texCoords);
    GLES20.glEnableVertexAttribArray (mTextureCoordIndex);
  }

  public void setTransform(float[] transform) {
    // If the shader does not take texture coordinates as input then exit.
    if (mTransformIndex < 0) {
      return;
    }

    GLES20.glUniformMatrix4fv(mTransformIndex, 1, false, transform, 0);
  }

  // ------------------------ Misc ----------------------------------------

  // Activate the default texture unit.
  public void activateTexture() {
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
  }

  // Activate the shader
  public void bind() {
    GLES20.glUseProgram(mProgram);
  }

  // De-activate the shader.
  public void unbind() {
    GLES20.glUseProgram(0);
  }

  // -------------------- Shader Creation methods ----------------------------

  // Create a shader program from source strings.
  public static int createProgram(String vertexSource, String fragmentSource)
      throws OpenGLException {
    int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
    int pixelShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);

    int program = GLES20.glCreateProgram();
    if (program == 0) {
      throw new OpenGLException("Unable to create program");
    }
    GLES20.glAttachShader(program, vertexShader);
    OpenGLException.logError("glAttachShader");
    GLES20.glAttachShader(program, pixelShader);
    OpenGLException.logError("glAttachShader");
    GLES20.glLinkProgram(program);
    int[] linkStatus = new int[1];
    GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);

    if (linkStatus[0] != GLES20.GL_TRUE) {
      GLES20.glDeleteProgram(program);
      throw new OpenGLException("Could not link program",
          GLES20.glGetProgramInfoLog(program));
    }
    return program;
  }

  // Load the shader to the hardware.
  protected static int loadShader(int shaderType,
                                  String source) throws OpenGLException {
    int shader = GLES20.glCreateShader(shaderType);
    if (shader == 0) {
      throw new OpenGLException("Unable to create shader");
    }

    GLES20.glShaderSource(shader, source);
    GLES20.glCompileShader(shader);
    int[] compiled = new int[1];
    GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
    if (compiled[0] == 0) {
      String shaderInfoLog = GLES20.glGetShaderInfoLog(shader);
      GLES20.glDeleteShader(shader);
      throw new OpenGLException("Unable to compile shader " + shaderType,
          shaderInfoLog);
    }
    return shader;
  }

  // Get a given shader attribute index.
  protected int getAttribute(int program, String name) throws OpenGLException {
    int handle = GLES20.glGetAttribLocation(program, name);
    if (handle == -1) {
      throw new OpenGLException("Unable to find " + name + " in shader");
    }
    OpenGLException.logError("glGetAttribLocation " + name);
    return handle;
  }

  // Get a given shader uniform index.
  protected int getUniform(int program, String name) throws OpenGLException {
    int handle = GLES20.glGetUniformLocation(program, name);
    if (handle == -1) {
      throw new OpenGLException("Unable to find " + name + " in shader");
    }
    OpenGLException.logError("glGetUniformLocation " + name);
    return handle;
  }

  // Create a program given resource data IDs containing shader source code.
  public static int createProgram(Context context,
                                  int vertexSourceId,
                                  int fragmentSourceId) throws OpenGLException {
    return createProgram(
      getRawResourceString(context, vertexSourceId),
      getRawResourceString(context, fragmentSourceId));
   }

  // Get a string from a raw resource
  protected static String getRawResourceString(Context context,
                                               int resourceId) {
    InputStream ins = context.getResources().openRawResource(resourceId);
    StringBuilder buffer = new StringBuilder();
    try {
      // TODO(settinger): fix this to not use a fixed buffer size.
      byte[] tmp = new byte[1024];
      for (int i; (i = ins.read(tmp)) != -1;) {
        buffer.append(new String(tmp, 0, i));
      }
      return buffer.toString();
    } catch (IOException e) {
      e.printStackTrace();
      return new String("");
    }
  }

}
