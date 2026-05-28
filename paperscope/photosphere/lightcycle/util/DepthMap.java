package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util;

import android.util.Base64;
import android.util.Log;
import java.io.ByteArrayInputStream;
import java.io.IOException;

/**
 * Contains a low-res depth map for pixels of a streetview panorama.
 *
 * This code is a self-contained, simplified version of
 * java/com/google/android/apps/gmm/streetview/internal/DepthMap.java.
 *
 * Contrary to the original code, all the per-pixel depth data is computed at startup rather than
 * on-demand, as we expect that data to be queried often by clients.
 */
public class DepthMap {
  private static final String TAG = "ps.DepthMap";

  // Depth value that is used when we don't have depth information for some pixel.
  // TODO(dcoz): refine this.
  public static final float INFINITE_DEPTH_METERS = 100.0f;

  // Fields below correspond to data encoded in the encoded depthmap string coming from
  // StreetView servers, ie, a set of plane equations, and a plane index for each pixel of the
  // depthmap.
  private int numPlanes;
  private int mapWidth;
  private int mapHeight;
  private byte[] planeIndices;
  private Plane[] planes;

  // Contains the decoded depth for each cell in the depthmap, derived from the plane data above.
  // This format is easier to use for clients.
  private float[] depthMap;

  // A simple plane class, (x, y, z) points of the plane satisfy x * nx + y * ny + z * nz = d.
  private static class Plane {
    public final float nx;
    public final float ny;
    public final float nz;
    public final float d;

    public Plane(float nx, float ny, float nz, float d) {
      this.nx = nx;
      this.ny = ny;
      this.nz = nz;
      this.d = d;
    }

    // Get distance between this plane and the origin, along the (x, y, z) ray.
    public float getDepthForRay(float x, float y, float z) {
      return d / (x * nx + y * ny + z * nz);
    }
  }

  // Instantiate depthmap from an uncompressed depthmap string typically fetched from the
  // StreetView server with the &dmz=1 parameter in the URL.
  public DepthMap(String uncompressedDepthMap) throws IOException {
    if (!decodeUncompressedDepthMap(uncompressedDepthMap)) {
      throw new IOException("Error decoding provided depth map");
    }

    computePerPixelDepthMap();
  }

  /**
   * Get depth in meters for point located at (x, y) in the depth map image.
   * (x, y) are [0, 1] normalized coordinates.
   */
  public float getDepth(double x, double y) {
    int cellX = (int) (x * mapWidth);
    int cellY = (int) (y * mapHeight);

    cellX = Math.max(0, Math.min(mapWidth - 1, cellX));
    cellY = Math.max(0, Math.min(mapHeight - 1, cellY));

    // TODO(dcoz): add interpolation?
    return getDepthAtCell(cellX, cellY);
  }

  public float getDepthAtCell(int x, int y) {
    return depthMap[y * mapWidth + x];
  }

  private void setDepth(int x, int y, float depth) {
    depthMap[y * mapWidth + x] = depth;
  }

  // Converts plane-index-per-pixel data to a more friendly depth-per-pixel data.
  private void computePerPixelDepthMap() {
    assert planeIndices != null;
    depthMap = new float [mapWidth * mapHeight];

    for (int y = 0; y < mapHeight; ++y) {
      for (int x = 0; x < mapWidth; ++x) {
        int planeIndex = planeIndices[y * mapWidth + x];

        if (planeIndex <= 0) {
          setDepth(x, y, INFINITE_DEPTH_METERS);
        } else {
          assert planeIndex < numPlanes;
          Plane plane = planes[planeIndex];

          // Convert (x, y) coordinates into lat, lng coordinates on a sphere.
          double lng = getLngFromX(x * 1.0 / mapWidth);
          double lat = getLatFromY(y * 1.0 / mapHeight);

          // Compute depth along the ray that points to this (lat, lng) point.
          float dx = (float) (Math.sin(lat) * Math.sin(lng));
          float dy = (float) (Math.sin(lat) * Math.cos(lng));
          float dz = (float) -Math.cos(lat);
          float depth = Math.abs(plane.getDepthForRay(dx, dy, dz));

          setDepth(x, y, depth);
        }
      }
    }

  }

  // Decodes the raw uncompressed depthmap string.
  private boolean decodeUncompressedDepthMap(String uncompressedDepthMap) {
    // First, base64 decode the string.
    final byte[] base64Decoded = Base64.decode(uncompressedDepthMap, Base64.URL_SAFE);

    // Read this data with Little Endian input stream.
    LEDataInputStream is = new LEDataInputStream(new ByteArrayInputStream(base64Decoded));

    try {
      // Read header and depthmap size.
      int headerSize = is.readUnsignedByte();
      assert headerSize == 8;
      numPlanes = is.readUnsignedShort();
      mapWidth = is.readUnsignedShort();
      mapHeight = is.readUnsignedShort();

      int planeOffset = is.readUnsignedByte();
      assert planeOffset == 8;

      planeIndices = new byte[mapWidth * mapHeight];
      is.readFully(planeIndices);

      planes = new Plane[numPlanes];
      float nx;
      float ny;
      float nz;
      float d;

      // Read all plane equations.
      for (int i = 0; i < numPlanes; ++i) {
        nx = is.readFloat();
        ny = is.readFloat();
        nz = is.readFloat();
        d = is.readFloat();
        planes[i] = new Plane(nx, ny, nz, d);
      }
    } catch (Exception e) {
      Log.w(TAG, "Error reading depth map", e);
      return false;
    } finally {
      try {
        is.close();
      } catch (IOException e) {
        Log.w(TAG, "error closing IS ", e);
        return false;
      }
    }
    return true;
  }

  // Utility function to convert from normalized [0, 1] x [0, 1] image rectangular
  // coordinates to [0, 2 PI] x [0, PI] lat, lng coordinates on a sphere.
  public static double getLngFromX(double x) {
    return x * Math.PI * 2;
  }

  public static double getLatFromY(double y) {
    return y * Math.PI;
  }
}
