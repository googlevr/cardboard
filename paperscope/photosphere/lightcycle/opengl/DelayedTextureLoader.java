// Copyright 2013 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.graphics.Bitmap;
import android.util.Log;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl.GLTexture.TextureType;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.Callback;
import java.util.concurrent.Semaphore;
import org.jspecify.annotations.Nullable;

/**
 * Loads a texture when requested. The loading itself is done asynchronously.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public abstract class DelayedTextureLoader implements TextureProvider {
  private static final String TAG = DelayedTextureLoader.class.getSimpleName();

  /** The fade-in duration in milliseconds. */
  private static final long FADE_IN_DURATION_MILLIS = 200;

  /** Limits the number of active texture bitmap loading/decoding processes. */
  private final Semaphore semaphore;

  private final Callback<Void> triggerRenderCallback;

  private GLTexture glTexture;
  private Bitmap bitmap;
  private boolean loading = false;
  private Thread bitmapLoadingThread;

  /** We use this to slowly fade in newly loaded textures. */
  private float alpha = 0;

  /** Timestamp of the fade-in start. */
  private long startFadeInTimeMillis = -1;

  /**
   * Instantiates a new {@link DelayedTextureLoader}.
   *
   * @param bitmapLoadingSemaphore a shared semaphore to limit the amount of
   *        bitmaps being loaded.
   * @param triggerRenderCallback notified when rendering is required (i.e. a
   *        bitmap finished loading and needs to be pushed to video memory or a
   *        fade-in is in progress.
   */
  public DelayedTextureLoader(
      Semaphore bitmapLoadingSemaphore, Callback<Void> triggerRenderCallback) {
    semaphore = bitmapLoadingSemaphore;
    this.triggerRenderCallback = triggerRenderCallback;
  }

  /**
   * If this is the first time getTexture is called, this will trigger the asynchronous loading of
   * the texture bitmap. While the bitmap is loading this method will return null.
   *
   * <p>Once the texture bitmap is loaded, the next call to this method will create the texture
   * object and return it. All subsequent calls will then return the previously created texture.
   *
   * @return Either a texture or null, of the texture is not loaded yet.
   */
  @Override
  public @Nullable GLTexture getTexture() {
    // If the texture is loaded, return it. Nothing more to do.
    if (glTexture != null) {
      return glTexture;
    }

    // Return null while loading the bitmap.
    if (loading) {
      return null;
    }
    loading = true;

    // A thread that waits for a semaphore permit to become available
    // to load the bitmap.
    bitmapLoadingThread =
        new Thread() {
          @Override
          public void run() {
            super.run();
            try {
              semaphore.acquire();
            } catch (InterruptedException e) {
              // Return without loading the bitmap.
              return;
            }
            bitmap = loadBitmap();
            bitmapLoadingThread = null;
            triggerRenderCallback.onCallback(null);
          }
        };
    bitmapLoadingThread.start();
    return null;
  }

  /**
   * If this loader has a pending bitmap loader thread, it will be cancelled.
   */
  public void cancel() {
    if (bitmapLoadingThread != null) {
      bitmapLoadingThread.interrupt();
      bitmapLoadingThread = null;
      bitmap = null;
    }
  }

  /**
   * Recycles the texture.
   */
  @Override
  public void recycle() {
    if (glTexture != null) {
      glTexture.recycle();
      glTexture = null;
      alpha = 0;
      startFadeInTimeMillis = -1;
    }
  }

  @Override
  public void createTexture() {
    if (bitmap != null) {
      createTextureAndReleaseBitmap();
    }
  }

  @Override
  public float getAlphaForRendering() {
    // No texture yet.
    if (glTexture == null) {
      return 0;
    }

    // Fade-in complete.
    if (alpha == 1) {
      return 1;
    }

    // First time this method is called with a valid texture being present marks
    // the starting time.
    if (startFadeInTimeMillis < 0) {
      startFadeInTimeMillis = System.currentTimeMillis();
    }

    alpha = (System.currentTimeMillis() - startFadeInTimeMillis) / (float) FADE_IN_DURATION_MILLIS;
    alpha = Math.min(alpha, 1);

    // Make sure we re-render as long as the tile has not been faded in
    // completely.
    triggerRenderCallback.onCallback(null);
    return alpha;
  }

  /**
   * Subclasses need to implement this method to load the texture bitmap.
   *
   * @return The texture bitmap.
   */
  protected abstract Bitmap loadBitmap();

  /**
   * Loads the bitmap into video memory and then recycles the bitmap to free up
   * main memory. Also releases the semaphore.
   */
  private void createTextureAndReleaseBitmap() {
    glTexture = new GLTexture(TextureType.Standard);
    try {
      // TODO(haeberling): Modify this to use texSubImage2D instead of
      // texImage2D to prevent potential framerate drops.
      glTexture.loadBitmap(bitmap);
    } catch (OpenGLException e) {
      Log.e(TAG, "Could not load texture");
    } finally {
      // Once the bitmap is pushed to graphics memory, recycle it and
      // release the semaphore.
      semaphore.release();
      bitmap.recycle();
      bitmap = null;
      loading = false;
    }
  }
}
