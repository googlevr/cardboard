// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.shaders;

import android.opengl.GLES20;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.OpenGLException;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.Shader;

/**
 * @author settinger@google.com (Scott Ettinger)
 *
 * Shader with variable transparency.  This shader simply sets the alpha value
 * for all fragments to a variable value.
 *
 */
public class TransparencyShader extends Shader {

  private int alphaFactorIndex = 0;

  public TransparencyShader() throws OpenGLException {
    mProgram = createProgram(vertexShader, fragmentShader);
    mVertexIndex = getAttribute(mProgram, "aPosition");
    mTextureCoordIndex = getAttribute(mProgram, "aTextureCoord");
    mTransformIndex = getUniform(mProgram, "uMvpMatrix");
    alphaFactorIndex = getUniform(mProgram, "uAlphaFactor");

    // Set default values.
    this.bind();
    GLES20.glUniform1f(alphaFactorIndex, 0.9f);
  }

  //----------------------- Parameter set methods ---------------------------

  public void setAlpha(float offset) {
    GLES20.glUniform1f(alphaFactorIndex, offset);
  }

  // ---------------------------- Shader code --------------------------------

  private final String vertexShader =
      "uniform mat4 uMvpMatrix;                   \n"
          + "attribute vec4 aPosition;                   \n"
          + "attribute vec2 aTextureCoord;               \n"
          + "varying vec2 vTexCoord;                     \n"
          + "void main()                                 \n"
          + "{                                           \n"
          + "   gl_Position = uMvpMatrix * aPosition;    \n"
          + "   vTexCoord = aTextureCoord;               \n"
          + "}                                           \n";

  private String fragmentShader =
      "precision highp float;                            \n"
          + "uniform float uAlphaFactor;                         \n"
          + "varying vec2 vTexCoord;                             \n"
          + "uniform sampler2D sTexture;                         \n"
          + "void main()                                         \n"
          + "{                                                   \n"
          + "  vec4 texcolor;                                    \n"
          + "  texcolor = texture2D( sTexture, vTexCoord );      \n"
          + "  texcolor.a = uAlphaFactor;                        \n"
          + "  gl_FragColor = texcolor;                          \n"
          + "}                                                   \n";
}
