package com.google.vr.cardboard.paperscope.demo.myvideos;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.util.Pair;
import android.util.Size;
import com.google.cardboard.sdk.CardboardView;
import com.google.common.flogger.FluentLogger;
import com.google.vr.cardboard.paperscope.demo.myvideos.VideoFetcher.VideoInfo;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Simple image downloader in a background thread.
 */
public class ImageFetcher {

  private static final FluentLogger logger = FluentLogger.forEnclosingClass();
  private static final int DISPLAY_DELAY_MILLIS = 50;
  private static final int THUMBNAIL_WIDTH_SIZE = 512;
  private static final int THUMBNAIL_HEIGHT_SIZE = 384;

  private final CardboardView cardboardView;
  private final MyVideosRenderer renderer;
  private final Handler handler;
  private int fetchCount;
  private ExecutorService executor = Executors.newSingleThreadExecutor();

  private final ContentResolver contentResolver;

  public ImageFetcher(
      Context context, final CardboardView cardboardView, final MyVideosRenderer renderer) {
    this.cardboardView = cardboardView;
    this.renderer = renderer;
    contentResolver = context.getContentResolver();
    handler = new ImageHandler(this);
  }

  @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
  private static class ImageHandler extends Handler {
    private final WeakReference<ImageFetcher> imageFetcherReference;

    ImageHandler(ImageFetcher imageFetcher) {
      imageFetcherReference = new WeakReference<>(imageFetcher);
    }

    @Override
    public void handleMessage(Message msg) {
      ImageFetcher imageFetcher = imageFetcherReference.get();
      if (imageFetcher == null) {
        return;
      }

      if (msg.obj instanceof Bitmap) {
        final Bitmap bitmap = (Bitmap) msg.obj;
        final int index = msg.what;
        // Delay the loading of each thumbnail by a bit so that all the GL textures don't get
        // created at the same time, causing stuttering.
        // TODO(kvaalen): it's still possible for multiple thumbnails to be added at the same
        // time
        //     since these times are relative to when the thumbnail loaded and are not
        // absolute.
        postDelayed(
            () -> {
              // Use queueEvent to have it run on the GL thread.
              imageFetcher.cardboardView.queueEvent(
                  () -> imageFetcher.renderer.addThumbnail(bitmap, index));
            },
            imageFetcher.fetchCount * ((long) DISPLAY_DELAY_MILLIS));
        imageFetcher.fetchCount++;
      } else {
        logger.atSevere().log("Object isn't a bitmap");
      }
    }
  }

  /**
   * @param indexAndPaths Pairs of file path strings and their index in the list of videos loaded.
   */
  public void downloadImages(final List<Pair<Integer, VideoInfo>> indexAndVideosInfo) {
    Runnable runnable =
        () -> {
          for (Pair<Integer, VideoInfo> indexAndVideoInfo : indexAndVideosInfo) {
            if (Thread.currentThread().isInterrupted()) {
              return;
            }
            doDownload(indexAndVideoInfo.first, indexAndVideoInfo.second);
          }
        };
    executor.execute(runnable);
  }

  /**
   * Creates the bitmap for the thumbnail for the video loaded. Once the bitmap is created, it is
   * queued for rendering by sending it to the handler.
   *
   * <p>Based on the API level, the bitmap is created from different sources. When the API level is
   * below Android Q´s the bitmap is created using the absolute path of the video. From Android Q,
   * the bitmap is created using the Uri of the video.
   *
   * @param index Index of the video file in the loaded videos list.
   * @param videoInfo Structure containing the file path string and the URI of the loaded video.
   */
  private void doDownload(int index, VideoInfo videoInfo) {
    Bitmap bitmap = null;
    Uri videoUri = videoInfo.getVideoStreamUri();
    Size thumbnailSize = new Size(THUMBNAIL_WIDTH_SIZE, THUMBNAIL_HEIGHT_SIZE);
    try {
      bitmap = contentResolver.loadThumbnail(videoUri, thumbnailSize, null);
    } catch (IOException e) {
      logger.atSevere().log(
          "Error getting bitmap from Uri %s : %s", videoUri.getPath(), e.getMessage());
    }

    if (!Thread.currentThread().isInterrupted()) {
      handler.obtainMessage(index, bitmap).sendToTarget();
    }
  }

  public void reset() {
    // Note: This also resets the thumbnail loading delay in the handler.
    fetchCount = 0;
    executor.shutdownNow();
    executor = Executors.newSingleThreadExecutor();
    // Cancel all previous thumbnail update messages in the handler, since they are now obsolete.
    handler.removeCallbacksAndMessages(null);
  }

}
