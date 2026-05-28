// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.shaders;

import android.opengl.GLES20;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.OpenGLException;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.Shader;

/**
 * Shader with variable transparency.  This shader multiplies the existing
 * alpha value in the texture by a variable alpha value.  This is useful for
 * compositing textures with transparency already baked into the texture.
 *
 * @author settinger@google.com (Scott Ettinger)
 */
public class ScaledTransparencyShader extends Shader {

  private int alphaFactorIndex = 0;

  public ScaledTransparencyShader() throws OpenGLException {
    mProgram = createProgram(vertexShader, fragmentShader);
    mVertexIndex = getAttribute(mProgram, "aPosition");
    mTextureCoordIndex = getAttribute(mProgram, "aTextureCoord");
    mTransformIndex = getUniform(mProgram, "uMvpMatrix");
    alphaFactorIndex = getUniform(mProgram, "uAlphaFactor");

    // Set default values.
    this.bind();
    GLES20.glUniform1f(alphaFactorIndex, 1f);
  }

  // ----------------------- Parameter set methods ---------------------------

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
      "precision mediump float;                            \n"
          + "uniform float uAlphaFactor;                         \n"
          + "varying vec2 vTexCoord;                             \n"
          + "uniform sampler2D sTexture;                         \n"
          + "void main()                                         \n"
          + "{                                                   \n"
          + "  gl_FragColor = texture2D( sTexture, vTexCoord);   \n"
          + "  gl_FragColor.a = gl_FragColor.a * uAlphaFactor;   \n"
          + "}                                                   \n";
}
