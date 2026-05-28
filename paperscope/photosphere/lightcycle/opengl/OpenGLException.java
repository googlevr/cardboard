// Copyright 2011 Google Inc. All Rights Reserved.

// Exception class for OpenGL routines.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.opengl.GLES20;
import android.opengl.GLU;
import android.util.Log;

/**
 * @author settinger@google.com (Scott Ettinger)
 *
 * Class to handle Exceptions related to OpenGL.
 *
 */
public class OpenGLException extends Exception {

  private static final String TAG = "LightCycle";
  public OpenGLException(String exception) {
    super(exception);
    Log.e(TAG, exception, this);
  }

  public OpenGLException(String exception, String log) {
    super(exception);
    Log.e(TAG, exception + " : " + log, this);
  }

  public static void vGL() throws OpenGLException {
    logError("");
  }

  public static void logError(String op) throws OpenGLException {
    int error;
    while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
      throw new OpenGLException(op + ": glError " + GLU.gluErrorString(error) +
          " " + error);
    }
  }

}
