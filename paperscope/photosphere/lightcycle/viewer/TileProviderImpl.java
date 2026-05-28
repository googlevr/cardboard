// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.BitmapRegionDecoder;
import android.graphics.Rect;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import org.jspecify.annotations.Nullable;

/**
 * The default implementation of the {@link TileProvider} interface that loads
 * tiles from an image input stream.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
@TargetApi(10)
public class TileProviderImpl implements TileProvider {
  @SuppressWarnings("unused")
  private static final String TAG = TileProviderImpl.class.getSimpleName();
  /**
   * The size of the tiles in pixels. 1024 seems to be an efficient value for the
   * BitmapRegionDecoder.
   */
  private static final int TILE_SIZE = 1024;

  // The maximum image size after downsampling.
  private static final int MAX_DOWNSAMPLED_IMAGE_SIZE = 5000;
  private final int tileCountX;
  private final int tileCountY;
  private final int lastColumnWidth;
  private final int lastRowHeight;

  // How much we downsample the original image.
  // If the input image is too big, we downsample it to make it more manageable.
  private int downSamplingFactor = 1;

  private final BitmapRegionDecoder decoder;

  /**
   * Creates a {@link TileProvider} from a file.
   *
   * @param imageFile the image file to use for tiling
   * @return A {@link TileProvider} or null, if the file could not be read.
   */
  public static @Nullable TileProvider fromFile(File imageFile) {
    try {
      return new TileProviderImpl(new FileInputStream(imageFile));
    } catch (FileNotFoundException ex) {
      Log.e(TAG, "File not found", ex);
    }
    return null;
  }

  /**
   * Instantiates and initializes a new tile provider.
   *
   * @param image stream to use for tiling.
   */
  @SuppressWarnings("deprecation")
  public TileProviderImpl(InputStream image) {
    try {
      this.decoder = BitmapRegionDecoder.newInstance(image, false);
    } catch (IOException e) {
      throw new RuntimeException("Could not create decoder", e);
    }

    // Compute the downsampling factor. The bigger the picture, the more we downsample it so that
    // we're always below MAX_DOWNSAMPLED_IMAGE_SIZE.
    this.downSamplingFactor = 1 + this.decoder.getWidth() / MAX_DOWNSAMPLED_IMAGE_SIZE;

    // Compute the size of the image after downsampling.
    int downSampledWidth = this.decoder.getWidth() / downSamplingFactor;
    int downSampledHeight = this.decoder.getHeight() / downSamplingFactor;

    double horizontalTileCount = downSampledWidth / (double) TILE_SIZE;
    this.tileCountX = (int) Math.ceil(horizontalTileCount);

    int lastColumnWidthPixels = downSampledWidth % TILE_SIZE;

    this.lastColumnWidth =
        lastColumnWidthPixels > 0 ? lastColumnWidthPixels : TILE_SIZE;

    double verticalTileCount = (downSampledHeight) / (double) TILE_SIZE;
    this.tileCountY = (int) Math.ceil(verticalTileCount);
    int lastRowHeightPixels = downSampledHeight % TILE_SIZE;
    this.lastRowHeight =
        lastRowHeightPixels > 0 ? lastRowHeightPixels : TILE_SIZE;
  }

  @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
  @SuppressLint("NewApi")
  @Override
  public synchronized @Nullable Tile getTile(int x, int y) {
    BitmapFactory.Options options = new BitmapFactory.Options();
    options.inPreferredConfig = Config.ARGB_8888;
    options.inPreferQualityOverSpeed = true;
    // Use our sampling factor here to sample the bitmap.
    options.inSampleSize = downSamplingFactor;


    // TODO(haeberling): Try to use level 11 API to recycle bitmaps.
    Bitmap bitmap = this.decoder.decodeRegion(
        getImageRegionForTileCoordinate(x, y), options);
    if (bitmap == null) {
      return null;
    }
    return new Tile(bitmap, x, y, TILE_SIZE, TILE_SIZE);
  }

  @Override
  public int getTileSize() {
    return TILE_SIZE;
  }

  @Override
  public int getTileCountX() {
    return tileCountX;
  }

  @Override
  public int getTileCountY() {
    return tileCountY;
  }

  @Override
  public int getLastColumnWidth() {
    return lastColumnWidth;
  }

  @Override
  public int getLastRowHeight() {
    return lastRowHeight;
  }

 /**
   * Returns the region for the tile at the given tile coordinate.
   * The coordinates are expressed in the original image coordinates before downsampling.
   */
  private Rect getImageRegionForTileCoordinate(int tileIndexX, int tileIndexY) {
    int originalTileSize = TILE_SIZE * downSamplingFactor;
    int left = tileIndexX * originalTileSize;
    int top = tileIndexY * originalTileSize;
    int right = left + originalTileSize;
    int bottom = top + originalTileSize;

    // For the last column and row we need to substract the padding, so the
    // tiles will be smaller than the default tile size.
    if (tileIndexX == this.tileCountX - 1) {
      right -= (originalTileSize - lastColumnWidth * downSamplingFactor);
    }
    if (tileIndexY == this.tileCountY - 1) {
      bottom -= (originalTileSize - lastRowHeight * downSamplingFactor);
    }
    return new Rect(left, top, right, bottom);
  }

  @Override
  public void setMaximumTextureSize(int dimension) {
    // Nothing to do here.
  }

  @Override
  public float getScale() {
    return 1.0f / downSamplingFactor;
  }
}
