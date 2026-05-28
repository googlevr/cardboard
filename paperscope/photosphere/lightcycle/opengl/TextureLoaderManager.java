// Copyright 2013 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.graphics.Bitmap;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.Callback;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.Tile;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.TileProvider;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;

/**
 * Creates {@link DelayedTextureLoader} instances and manages the loading
 * semaphore.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class TextureLoaderManager {
  /** Controls how many bitmap creations are happening at once. */
  private Semaphore bitmapLoadingSemaphore;

  private final TileProvider tileProvider;
  private final List<DelayedTextureLoader> loaders = new ArrayList<DelayedTextureLoader>();

  /**
   * This instance is given to all texture loaders. It's called when any of them loaded a texture
   * bitmap or when textures are performing a fade-in, and thus require a re-render of the scene.
   */
  private final Callback<Void> triggerRenderCallbackInternal;

  /**
   * This is set from externally. If set, it is called whenever one of the loaders has finished
   * loading a texture bitmap or needs rendering to do an ongoing texture fade-in.
   */
  private Callback<Void> triggerRenderCallback;

  /**
   * Initializes a new manager using the given tile provider.
   */
  public TextureLoaderManager(TileProvider tileProvider) {
    this.tileProvider = tileProvider;
    triggerRenderCallbackInternal =
        new Callback<Void>() {
          @Override
          public void onCallback(Void response) {
            if (triggerRenderCallback != null) {
              triggerRenderCallback.onCallback(response);
            }
          }
        };
    reset();
  }

  /**
   * Resets this factory by canceling all pending texture loading threads.
   */
  public void reset() {
    for (DelayedTextureLoader loader : loaders) {
      loader.cancel();
    }
    loaders.clear();
    bitmapLoadingSemaphore = new Semaphore(2);
  }

  /**
   * Sets the callback that is notified when the scene needs to be re-rendered.
   * E.g. if an any texture has finished loading a texture bitmap. Notifying the
   * callback should then be used to trigger a re-rendering of the scene in
   * order to display the newly loaded texture.
   *
   * @param callback the callback to set.
   */
  public void setTriggerRenderCallback(Callback<Void> callback) {
    triggerRenderCallback = callback;
  }

  /**
   * Returns a delayed texture loader that will return null as a texture until
   * the texture is loaded (which happens asynchronously).
   *
   * @return A new delayed texture loader.
   */
  public TextureProvider getDelayedLoader(final int x, final int y) {
    DelayedTextureLoader loader =
        new DelayedTextureLoader(this.bitmapLoadingSemaphore, triggerRenderCallbackInternal) {
          @Override
          protected Bitmap loadBitmap() {
            Tile tile = tileProvider.getTile(x, tileProvider.getTileCountY() - y - 1);
            return tile.bitmap;
          }
        };
    loaders.add(loader);
    return loader;
  }
}
