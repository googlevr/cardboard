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
package com.google.cardboard.sdk;

import com.google.cardboard.sdk.nativetypes.EyeTextureDescription;
import com.google.cardboard.sdk.nativetypes.Mesh;

/** Cardboard SDK Distortion Renderer API. */
public class DistortionRenderer {

  // Opaque native pointer to the native DistortionRenderer instance.
  // This object is owned by this class instance and passed to the native methods.
  private long nativeDistortionRenderer;

  /**
   * Creates a new distortion renderer object.
   *
   * <p>Preconditions: Must be called from the rendering thread.
   */
  public DistortionRenderer() {
    nativeDistortionRenderer = nativeDistortionRendererCreate();
  }

  /**
   * Releases memory created at construct time by the native distortion renderer object. This method
   * should be executed just once after construct time, then it will not have effect and a new
   * instance must be instantiated. If this function is not called, additional resources will be
   * consumed.
   *
   * <p>Preconditions: Must be called from the rendering thread.
   */
  public void close() {
    nativeDistortionRendererDestroy(nativeDistortionRenderer);
    nativeDistortionRenderer = 0;
  }

  /**
   * Calls {@code close()} in case that method is mistakenly not called.
   *
   * <p>Preconditions: Must be called from the rendering thread.
   */
  @Override
  protected void finalize() throws Throwable {
    // TODO(b/146156526): Remove finalize() usage from Java API.
    close();
    super.finalize();
  }

  /**
   * Sets Distortion Mesh for a particular eye.
   *
   * <p>Preconditions: Must be called from the rendering thread.
   *
   * @param[in] mesh Distortion mesh.
   * @param[in] eye Desired eye.
   */
  public void setMesh(Mesh mesh, int eye) {
    nativeDistortionRendererSetMesh(nativeDistortionRenderer, mesh, eye);
  }

  /**
   * Renders eye textures to display.
   *
   * <p>Preconditions: Must be called from the rendering thread.
   *
   * @param[in] targetDisplay Target display.
   * @param[in] x x coordinate of the rectangle's lower left corner to draw into.
   * @param[in] y y coordinate of the rectangle's lower left corner to draw into.
   * @param[in] width Size in pixels of rectangle's width to draw into.
   * @param[in] height Size in pixels of rectangle's height to draw into.
   * @param[in] leftEye Left eye texture description.
   * @param[in] rightEye Right eye texture description.
   */
  public void renderEyeToDisplay(
      long targetDisplay,
      int x,
      int y,
      int width,
      int height,
      EyeTextureDescription leftEye,
      EyeTextureDescription rightEye) {
    nativeDistortionRendererRenderEyeToDisplay(
        nativeDistortionRenderer, targetDisplay, x, y, width, height, leftEye, rightEye);
  }

  private native long nativeDistortionRendererCreate();

  private native void nativeDistortionRendererDestroy(long nativeDistortionRenderer);

  private native void nativeDistortionRendererSetMesh(
      long nativeDistortionRenderer, Mesh mesh, int eye);

  private native void nativeDistortionRendererRenderEyeToDisplay(
      long nativeDistortionRenderer,
      long targetDisplay,
      int x,
      int y,
      int width,
      int height,
      EyeTextureDescription leftEye,
      EyeTextureDescription rightEye);
}
