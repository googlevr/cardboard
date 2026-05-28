// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer;

import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.DepthMap;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.PanoMetadata;

import java.io.IOException;

/**
 * Contains information about how to load and render a panorama image.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class PanoramaImage {
  private static final String TAG = "ps.PanoramaImage";

  /** Provides the tiles of the image. */
  private final TileProvider tileProvider;
  /** The width of the pano in radians. */
  private float panoWidthRad;
  /** The height of the pano in radians. */
  private float panoHeightRad;
  /** The size of the quadratic tiles in radians. */
  private float tileSizeRad;
  /** The offset of the upper edge from the north pole in radians. */
  private float panoOffsetTopRad;
  /** The offset of the left edge from the left-hand seam in radians. */
  private float panoOffsetLeftRad;
  /** The width of the last column in radians. */
  private float lastColumnWidthRad;
  /** The height of the last row in radians. */
  private float lastRowHeightRad;

  private PanoMetadata panoMetadata;

  public PanoramaImage(TileProvider tileProvider, PanoMetadata panoMetadata) {
    this.tileProvider = tileProvider;
    this.panoMetadata = panoMetadata;

    this.panoWidthRad = (float) ((this.panoMetadata.croppedAreaWidth
        / (float) this.panoMetadata.fullPanoWidth) * Math.PI * 2);
    this.panoHeightRad = (float) ((this.panoMetadata.croppedAreaHeight
        / (float) this.panoMetadata.fullPanoHeight) * Math.PI);

    this.panoOffsetLeftRad = (float) ((this.panoMetadata.croppedAreaLeft
        / (float) this.panoMetadata.fullPanoWidth) * Math.PI * 2);
    this.panoOffsetTopRad = (float) ((this.panoMetadata.croppedAreaTop
        / (float) this.panoMetadata.fullPanoHeight) * Math.PI);
  }

  /**
   * Builds the pano depthmap associated to the image using the given uncompressed depth map string.
   */
  public void buildPanoDepthMap(String uncompressedDepthMap) throws IOException {
    panoMetadata.setDepthMap(new DepthMap(uncompressedDepthMap));
  }

  public DepthMap getDepthMap() {
    return panoMetadata.depthMap;
  }

  public PanoMetadata getPanoMetadata() {
    return panoMetadata;
  }

  /**
   * Compute all properties of the image.
   */
  public void init() {

    // If the image was scaled without updating the metadata, the absolute
    // values are wrong. This also means that the angular size per tile is not
    // correct, as the tile size stays the same. If the image was scaled to half
    // size, the angular size will be twice as large.
    float scale = this.panoMetadata.imageWidth
        / (float) this.panoMetadata.croppedAreaWidth;

    // TODO(settinger): determine how to handle the dependency of requiring
    // getTile() to be called on the GL thread before calling
    // getScale() or getTileSize() for early SDK versions.

    // Scale by the downsampling factor applied by the tile provider.
    scale *= this.tileProvider.getScale();

    // It doesn't matter if we use imageWidth/360, or imageHeight/180,
    // the number must be the same.
    this.tileSizeRad = ((float) (
        (tileProvider.getTileSize() / (float) this.panoMetadata.fullPanoWidth)
        * Math.PI * 2)) / scale;

    // Compute the angular size of last column and last row tiles, which can be smaller than the
    // other ones.
    this.lastColumnWidthRad = panoWidthRad - tileSizeRad * (tileProvider.getTileCountX() - 1);
    this.lastRowHeightRad = panoHeightRad - tileSizeRad * (tileProvider.getTileCountY() - 1);
  }

  /**
   * @return The tile provider for retrieving the tiles of this image.
   */
  public TileProvider getTileProvider() {
    return this.tileProvider;
  }

  /**
   * @return The width of the panorama in radians.
   */
  public float getPanoWidthRad() {
    return this.panoWidthRad;
  }

  /**
   * @return The height of the panorama in radians.
   */
  public float getPanoHeightRad() {
    return this.panoHeightRad;
  }

  /**
   * @return The size of a panorama tile in radians.
   */
  public float getTileSizeRad() {
    return tileSizeRad;
  }

  /**
   * @return The offset of the top-left corner of the cropped region in radians.
   */
  public float getOffsetTopRad() {
    return this.panoOffsetTopRad;
  }

  /**
   * @return The offset of the top-left corner of the cropped region in radians.
   */
  public float getOffsetLeftRad() {
    return this.panoOffsetLeftRad;
  }

  /**
   * @return The width of the last column in radians.
   */
  public float getLastColumnWidthRad() {
    return this.lastColumnWidthRad;
  }

  /**
   * @return The height of the last row in radians.
   */
  public float getLastRowHeightRad() {
    return this.lastRowHeightRad;
  }

  /**
   * Set the maximum texture size supported by the hardware.
   *
   * @param dimension the maximum texture size in pixels.
   */
  public void setMaximumTextureSize(int dimension) {
    this.tileProvider.setMaximumTextureSize(dimension);
  }

  /**
   * @return the degrees per pixel of this panorama image.
   */
  public double getDegreesPerPixel() {
    return Math.toDegrees((double) panoWidthRad / panoMetadata.imageWidth);
  }
}
