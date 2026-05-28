// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer;

/**
 * Provides tiles of an image.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public interface TileProvider {

  /**
   * Returns the tile at the given coordinate.
   *
   * @param x the x-coordinate of the tile.
   * @param y the y-coordinate of the tile.
   * @return The tile at the given position or null.
   */
  public Tile getTile(int x, int y);

  /**
   * @return The size of the quadratic tiles (in pixels).
   */
  public int getTileSize();

  /**
   * @return How many tiles there are horizontally.
   */
  public int getTileCountX();

  /**
   * @return How many tiles there are vertically.
   */
  public int getTileCountY();

  /**
   * @return The width of the rightmost column in pixels.
   */
  public int getLastColumnWidth();

  /**
   * @return The height of the last row in pixels.
   */
  public int getLastRowHeight();

  /**
   * Set the maximum texture size supported by the hardware.
   * @param dimension the maximum texture size in pixels.
   */
  public void setMaximumTextureSize(int dimension);

  /**
   * Get the scale factor applied to all tile images.
   * @return the scale factor.
   */
  public float getScale();

}
