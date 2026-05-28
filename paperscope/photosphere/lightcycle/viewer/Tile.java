// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer;

import android.graphics.Bitmap;

/**
 * A tile of a panorama image.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class Tile {
  /** The bitmap of the tile. */
  public final Bitmap bitmap;
  /** The x-coordinate of the tile. */
  public final int x;
  /** The y-coordinate of the tile. */
  public final int y;
  /** The width of the tile in pixels. */
  public final int width;
  /** The height of the tile in pixels. */
  public final int height;

  /**
   * Create a new tile instance.
   *
   * @param bitmap the bitmap of the tile
   * @param x the x-coordinate of the tile
   * @param y the y-coordinate of the tile
   * @param width the width of the tile in pixels
   * @param height the height of the tile in pixels
   */
  public Tile(Bitmap bitmap, int x, int y, int width, int height) {
    this.bitmap = bitmap;
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
  }
}
