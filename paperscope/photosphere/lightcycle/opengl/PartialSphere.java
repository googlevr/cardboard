// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.opengl;

import android.util.Log;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.math.Vector3;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.shaders.TransparencyShader;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.DepthMap;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.PanoramaImage;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.TileProvider;
import java.util.ArrayList;
import java.util.List;

/**
 * A {@link DrawableGL} class to generate geometry and render a partial sphere.
 * It can also render a full sphere, as a full sphere is one kind of a partial
 * sphere.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public class PartialSphere extends DrawableGL {
  private static final String TAG = PartialSphere.class.getSimpleName();

  /**
   * The maximum angle we allow between columns in the sphere to avoid
   * artifacts.
   * NOTE(dcoz): original value was 0.12f. Use more vertice to make better use of the depthmap data.
   */
  private static final float MAX_ANGLE_STEP_RAD = 0.05f;

  /** The radius of the sphere. */
  private final float radius;

  /** The panorama image to display. */
  private PanoramaImage image;

  /** Provides the tiles from the original image. */
  private TileProvider tileProvider;

  /** All the curved tiles that make up this partial sphere. */
  private CurvedTile[][] curvedTiles;

  /** Creates texture providers. */
  private TextureLoaderManager textureLoaderManager;

  /** Limit culling rate. */
  private int cullCounter;

  /**
   * Constructs a new partial sphere.
   *
   * @param radius the radius of the sphere.
   * @param textureLoaderManager used to create the texture providers.
   */
  public PartialSphere(float radius, TextureLoaderManager textureLoaderManager) {
    this.radius = radius;
    this.textureLoaderManager = textureLoaderManager;
  }

  /**
   * Sets a new image, loads its textures and generates the geometry. This needs
   * to be called from the GL thread.
   *
   * @param image the image to display.
   */
  public void setImage(PanoramaImage image, TextureLoaderManager textureLoaderManager) {
    this.textureLoaderManager = textureLoaderManager;
    this.textureLoaderManager.reset();
    this.cullCounter = -1;
    this.image = image;
    this.tileProvider = image.getTileProvider();
    this.setupTextureProviders();
    image.init();
    this.generateGeometry(radius);
  }

  @Override
  public void drawObject(float[] transform) throws OpenGLException {
    // Select the shader.
    mShader.bind();

    // Draw all curved tiles.
    for (int x = 0; x < curvedTiles.length; ++x) {
      CurvedTile[] textureColumn = this.curvedTiles[x];
      for (int y = 0; y < textureColumn.length; ++y) {
        CurvedTile tile = textureColumn[y];
        TextureProvider textureProvider = mTextures.get(tile.textureId);

        // If a bitmap has been loaded make sure a texture is created from it.
        // Frees held semaphores in the delayed texture loaders if a bitmap
        // is loaded but no texture was created yet.
        textureProvider.createTexture();

        // Don't render tiles that are not in view.
        if (!tile.isVisible()) {
          continue;
        }

        GLTexture texture = textureProvider.getTexture();
        if (texture == null) {
          // Texture can be null if it could not be loaded or is delay loaded.
          continue;
        }
        texture.bind(mShader);
        mShader.setTransform(transform);

        // Apply fade-in transparency for tiles.
        float alpha = textureProvider.getAlphaForRendering();
        ((TransparencyShader) mShader).setAlpha(alpha);
        tile.draw(mShader);
      }
    }
    // Reset alpha back to 100%.
    ((TransparencyShader) mShader).setAlpha(1);
  }

  @Override
  protected void cull(Vector3 lookAt, float minVisibilityDot) {
    // Reduce CPU impact of culling by only calling it every 4th frame.
    if (++cullCounter % 4 != 0) {
      return;
    }
    cullCounter = 0;

    // Determine the visibility of each tile.
    for (int x = 0; x < curvedTiles.length; ++x) {
      CurvedTile[] textureColumn = this.curvedTiles[x];
      for (int y = 0; y < textureColumn.length; ++y) {
        CurvedTile tile = textureColumn[y];
        tile.determineVisibility(lookAt, minVisibilityDot);
      }
    }
  }

  /**
   * Generates the geometry for the partial sphere.
   *
   * @param radius the radius of the sphere to generate.
   */
  // TODO: b/352536125 - Revisit why radius is not used.
  private void generateGeometry(float radius) {
    final int tesselationFactor =
        (int) Math.ceil(this.image.getTileSizeRad() / MAX_ANGLE_STEP_RAD);
    Log.d(TAG, "tesselation factor: " + tesselationFactor);

    // The number of latitudes and longitudes of the partial sphere mesh.
    int numLatitudes =
        (this.tileProvider.getTileCountY()) * tesselationFactor + 1;
    int numLongitudes =
        (this.tileProvider.getTileCountX()) * tesselationFactor + 1;

    // The number of vertices and indices of the partial sphere.
    int numVertices = numLatitudes * numLongitudes;
    // Two triangles per quad = 6 indices.
    int numIndices = (numLatitudes - 1) * (numLongitudes - 1) * 6;
    this.initGeometry(numVertices, numIndices, true);

    // The angle between vertices of the mesh. The smaller the step, the finer
    // the mesh.
    float angleStepLat = this.image.getTileSizeRad() / tesselationFactor;
    float angleStepLon = this.image.getTileSizeRad() / tesselationFactor;

    // On the edges we might have denser geometry due to smaller tiles. Hence we
    // need to compute a separate angle step, which might be smaller.
    float angleStepLatEdge = angleStepLat
        * (this.image.getLastRowHeightRad() / this.image.getTileSizeRad());
    float angleStepLonEdge = angleStepLon
        * (this.image.getLastColumnWidthRad() / this.image.getTileSizeRad());

    // The offset is based on the meta-data of the pano. If the pano is cropped,
    // we need to know where to put the pano.
    final float offsetLat = this.image.getOffsetTopRad()
        + this.image.getPanoHeightRad() - (float) (Math.PI / 2f);
    final float offsetLon = -this.image.getOffsetLeftRad() - (float) Math.PI;

    // Up until the second to last tile column we size all columns equally. The
    // last tile column can be denser in vertices though, if the texture is not
    // divisible by the texture size. In case the texture is narrower, the
    // geometry also needs to be made narrower/denser, which will happen for all
    // vertex longitudes greater than this value.
    int secondToLastTileEndColumn = numLongitudes - tesselationFactor - 1;
    int secondTileStartRow = tesselationFactor;

    // Generate ALL the vertices!
    final float piDiv2 = (float) Math.PI / 2;
    final Vertex[][] vertices = new Vertex[numLongitudes][numLatitudes];
    for (int r = 0; r < numLatitudes; ++r) {
      float lat;

      // The bottom most tile row might need to be denser if the tiles in this
      // row are less tall than the normal tile size specifies.
      if (r < secondTileStartRow) {
        lat = r * angleStepLatEdge - offsetLat;
      } else {
        // All the rows above the first need to get lowered by the padding
        // amount, but are spaced with the default angle step value.
        lat = r * angleStepLat - offsetLat
            - (this.image.getTileSizeRad() - this.image.getLastRowHeightRad());
      }

      for (int c = 0; c < numLongitudes; ++c) {
        // All columns have the same angleStep...
        float lon = c * angleStepLon;

        // ... except for the vertices of the last tile column, which might be
        // denser.
        if (c > secondToLastTileEndColumn) {
          // As the base we calculate the longitude of the second to last tile
          // column.
          float prevLon = secondToLastTileEndColumn * angleStepLon;

          // We then use the (potentially smaller) angle step of the denser last
          // tile column to calculate the longitudes.
          lon = prevLon + ((c - secondToLastTileEndColumn) * angleStepLonEdge);
        }

        // Apply the longitudal offset.
        lon = lon - piDiv2 - offsetLon;

        // Compute vertex coordinates from the lat/long.
        final float sinlat = (float) Math.sin(lat);
        final float sinlon = (float) Math.sin(lon);
        final float coslat = (float) Math.cos(lat);
        final float coslon = (float) Math.cos(lon);

        // Push each vertex along the viewing ray according to its depth in the depthmap.
        final float depth = getDepthAtLatLon(lat, lon);
        final float x = coslat * coslon * depth;
        final float y = sinlat * depth;
        final float z = coslat * sinlon * depth;

        vertices[c][r] = new Vertex(x, y, z);
      }
    }

    // The number of textures we need for this partial sphere.
    final int textureCountX = this.tileProvider.getTileCountX();
    final int textureCountY = this.tileProvider.getTileCountY();

    // Create the curved tiles and assign the correct vertices to them.
    curvedTiles = new CurvedTile[textureCountX][textureCountY];
    for (int y = 0, v = 0; v < textureCountY; y += tesselationFactor, ++v) {
      for (int x = 0, u = 0; u < textureCountX; x += tesselationFactor, ++u) {
        curvedTiles[u][v] =
            new CurvedTile(v + (u * textureCountY), tesselationFactor);
        List<Vertex> vertexList = new ArrayList<Vertex>();

        // For each texture, add the relevant vertices.
        // TODO(haeberling): We should store vertices in a single buffer for
        // efficiency
        for (int yy = 0; yy < tesselationFactor + 1; ++yy) {
          for (int xx = 0; xx < tesselationFactor + 1; ++xx) {
            vertexList.add(vertices[x + xx][y + yy]);
          }
        }
        curvedTiles[u][v].setVertices(vertexList.toArray(new Vertex[0]));
      }
    }
  }

  /**
   * Returns depth for the image point located at (lat, lon) using the image depthmap.
   * If there is no depthmap for that image, a constant depth is used.
   */
  private float getDepthAtLatLon(float lat, float lon) {
    DepthMap depthMap = image.getDepthMap();
    if (depthMap == null) {
      return radius;
    }

    // Convert our (lat, lon) coordinates to the normalized coordinate frame used by the depthmap.
    lat = (float) (Math.PI / 2 - lat);
    lon = (float) (lon - Math.PI / 2);
    return depthMap.getDepth(lon / (Math.PI * 2), lat / (Math.PI));
  }

  /** Set up the delayed texture loaders. */
  private void setupTextureProviders() {
    if (this.tileProvider == null) {
      Log.e(TAG, "tile provider is null. Cannot load textures");
      return;
    }

    // Recycle current textures if present.
    for (TextureProvider texture : mTextures) {
      texture.recycle();
    }
    mTextures.clear();

    // Create the delayed textures in order.
    for (int x = 0; x < this.tileProvider.getTileCountX(); ++x) {
      for (int y = 0; y < this.tileProvider.getTileCountY(); ++y) {
        mTextures.add(textureLoaderManager.getDelayedLoader(x, y));
      }
    }
  }
}
