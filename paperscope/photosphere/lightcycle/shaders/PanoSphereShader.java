// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.shaders;

import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.OpenGLException;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.Shader;

/**
 * @author settinger@google.com (Scott Ettinger)
 *
 * Shader used to render the panorama sphere with a fixed transparency.  It also
 *  renders any area within the texture with zero intensity as fully 
 *  transparent. 
 *
 */
public class PanoSphereShader extends Shader {

  public PanoSphereShader() throws OpenGLException {
    mProgram = createProgram(vertexShader, fragmentShader);

    mVertexIndex = getAttribute(mProgram, "aPosition");
    mTextureCoordIndex = getAttribute(mProgram, "aTextureCoord");
    mTransformIndex = getUniform(mProgram, "uMvpMatrix");
    //mSamplerIndex = getUniform(mProgram, "sTexture");
  }

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
          + "varying vec2 vTexCoord;                             \n"
          + "uniform sampler2D sTexture;                         \n"
          + "void main()                                         \n"
          + "{                                                   \n"
          + "  vec4 texcolor;                                    \n"
          + "  texcolor = texture2D( sTexture, vTexCoord );      \n"
          + "  texcolor.a = 0.85;                                \n"
          + "  if (texcolor.r < .0001) texcolor.a = 0.0;         \n"
          + "  gl_FragColor = texcolor;                          \n"
          + "}                                                   \n";
}
