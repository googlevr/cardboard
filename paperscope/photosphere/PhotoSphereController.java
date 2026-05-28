package com.google.vr.cardboard.paperscope.demo.photosphere;

import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore.Images.Media;
import android.provider.MediaStore.MediaColumns;
import com.google.cardboard.sdk.CardboardView;
import com.google.common.flogger.FluentLogger;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.vr.cardboard.paperscope.demo.common.DemoBaseController;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util.PanoMetadata;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.PanoramaImage;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.TileProvider;
import com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.viewer.TileProviderImpl;
import java.io.IOException;
import java.io.InputStream;
import org.jspecify.annotations.Nullable;

/** Demo that allows the user to view Photo Spheres. */
public class PhotoSphereController extends DemoBaseController<PhotoSphereRenderer> {
  private static final FluentLogger logger = FluentLogger.forEnclosingClass();
  private boolean hasValidPhotoSphere;
  private Uri contentUri;
  private Cursor cursor;
  private static final String NO_PHOTO_SPHERE_FOUND_ASSET = "photosphere/no_photo_sphere_found.jpg";

  public PhotoSphereController(CardboardView cardboardView) {
    super(cardboardView);
    if (cardboardView == null) {
      throw new IllegalArgumentException("CardboardView is null");
    }
    this.renderer = new PhotoSphereRenderer(cardboardView.getContext());
    this.cardboardView.setRenderer(renderer);
  }

  @Override
  public void enableOnTriggerEventListener() {
    cardboardView.setOnTriggerEvent(
        () -> {
          logger.atInfo().log("Trigger event received, switching photo.");
          if (renderer != null && hasValidPhotoSphere) {
            nextPhotoSphere();
          }
        });
  }

  public void createPhotoSphere() {
    logger.atInfo().log("createPhotoSphere");
    setUpCursor();
    hasValidPhotoSphere = nextPhotoSphere();

    if (!hasValidPhotoSphere) {
      loadDefaultPhotoSphere();
    }
  }

  private void setUpCursor() {
    String[] projection = {Media._ID, MediaColumns.DISPLAY_NAME};

    String selection = MediaColumns.DISPLAY_NAME + " LIKE 'PANO%'";
    cursor =
        cardboardView
            .getContext()
            .getContentResolver()
            .query(
                Media.EXTERNAL_CONTENT_URI,
                projection,
                selection,
                null,
                MediaColumns.DATE_ADDED + " DESC");
  }

  private void loadDefaultPhotoSphere() {
    if (renderer == null) {
      logger.atWarning().log(
          "Renderer must be initialized before loading the default photo sphere.");
      return;
    }

    PanoramaImage defaultPano = createPanoramaImage(NO_PHOTO_SPHERE_FOUND_ASSET);
    if (defaultPano != null) {
      renderer.setPanoramaImage(defaultPano);
    } else {
      logger.atWarning().log("Unable to create PanoramaImage from asset!");
    }
  }

  private @Nullable PanoramaImage createPanoramaImage(String assetPath) {
    TileProvider tileProvider = createTileProvider(assetPath);
    if (tileProvider == null) {
      return null;
    }

    PanoMetadata metadata = createPanoMetadata(assetPath);
    if (metadata == null) {
      return null;
    }
    // Reset the photo heading so that it's shown centered.
    metadata.setPoseHeadingDegrees(0);
    logger.atInfo().log("Metadata: %s", metadata);
    return new PanoramaImage(tileProvider, metadata);
  }

  private @Nullable PanoramaImage createPanoramaImage(Uri uri) {
    logger.atInfo().log("createPanoramaImage for uri: %s", uri);
    TileProvider tileProvider = createTileProvider(uri);
    PanoMetadata metadata = createPanoMetadata(uri);
    // Reset the photo heading so that it's shown centered.
    if (metadata != null) {
      metadata.setPoseHeadingDegrees(0);
      logger.atFine().log("Metadata: %s", metadata);
      if (tileProvider != null) {
        return new PanoramaImage(tileProvider, metadata);
      }
    }

    return null;
  }

  private @Nullable TileProvider createTileProvider(Uri uri) {
    try (InputStream in = cardboardView.getContext().getContentResolver().openInputStream(uri)) {
      return new TileProviderImpl(in);
    } catch (IOException | RuntimeException e) {
      logger.atWarning().withCause(e).log("Failed to create tile provider for uri: %s", uri);
      return null;
    }
  }

  private @Nullable TileProvider createTileProvider(String assetPath) {
    try (InputStream in = cardboardView.getContext().getAssets().open(assetPath)) {
      return new TileProviderImpl(in);
    } catch (IOException e) {
      logger.atWarning().withCause(e).log(
          "Failed to create tile provider for asset: %s", assetPath);
      return null;
    }
  }

  private PanoMetadata createPanoMetadata(String assetPath) {
    return PanoMetadata.parse(
        () -> {
          try {
            return cardboardView.getContext().getAssets().open(assetPath);
          } catch (IOException e) {
            logger.atWarning().withCause(e).log("Failed to open asset file: %s", assetPath);
            return null;
          }
        });
  }

  private PanoMetadata createPanoMetadata(Uri uri) {
    return PanoMetadata.parse(
        () -> {
          try {
            return cardboardView.getContext().getContentResolver().openInputStream(uri);
          } catch (IOException e) {
            logger.atWarning().withCause(e).log("Failed to open uri: %s", uri);
            return null;
          }
        });
  }

  @CanIgnoreReturnValue
  private boolean nextPhotoSphere() {
    boolean foundValidPhotoSphere = false;
    if (cursor != null && cursor.getCount() > 0) {
      boolean done = false;
      boolean cycled = false;
      while (!done) {
        if (!cursor.moveToNext()) {
          cursor.moveToFirst(); // Cycle back to beginning.
          if (cycled) {
            // Set done to true here so the while loop doesn't loop infinitely
            // in the case where there are photos in the cursor but
            // createPanoramaImage() fails on every photo.
            done = true;
          }
          cycled = true;
        }
        contentUri = Uri.parse(Media.EXTERNAL_CONTENT_URI + "/" + cursor.getString(0));
        logger.atInfo().log("Switching to pano %s at uri: %s", cursor.getString(1), contentUri);

        PanoramaImage pano = createPanoramaImage(contentUri);
        if (pano != null) {
          renderer.setPanoramaImage(pano);
          done = true;
          foundValidPhotoSphere = true;
        } else {
          logger.atWarning().log("Unable to create PanoramaImage!");
        }
      }
    } else {
      logger.atInfo().log("Cursor is empty");
    }

    return foundValidPhotoSphere;
  }
}
