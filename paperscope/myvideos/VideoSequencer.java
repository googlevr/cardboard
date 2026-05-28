package com.google.vr.cardboard.paperscope.demo.myvideos;

import android.content.Context;
import android.util.Pair;
import com.google.cardboard.sdk.CardboardView;
import com.google.common.flogger.FluentLogger;
import com.google.vr.cardboard.paperscope.demo.myvideos.VideoFetcher.VideoInfo;
import java.util.ArrayList;
import java.util.List;

/**
 * Sequencer for fetching and playing videos.
 */
public class VideoSequencer implements MyVideosPlayer.Listener {

  private static final FluentLogger logger = FluentLogger.forEnclosingClass();

  /**
   * When an error happens, we skip to the next video. To ensure we don't hit the server too much
   * if something goes wrong, we limit the number of skips.
   */
  private static final int MAX_CONSECUTIVE_ERRORS_ALLOWED = 20;

  private final CardboardView cardboardView;
  private final MyVideosRenderer renderer;
  private final MyVideosPlayer player;
  private final VideoFetcher videoFetcher;
  private final ImageFetcher imageFetcher;

  private List<VideoInfo> videoList;
  private int currentVideoIndex;
  private int numThumbnailsFetched;
  private int consecutiveErrorCount;

  public VideoSequencer(Context context, CardboardView cardboardView, MyVideosRenderer renderer) {
    this.cardboardView = cardboardView;
    this.renderer = renderer;

    player = new MyVideosPlayer(context, this /* Listener */, renderer);
    videoFetcher = new VideoFetcher(context, new VideoListCallback());
    imageFetcher = new ImageFetcher(context, cardboardView, renderer);

    renderer.setSurfaceTextureListener(player);
  }

  public void startPlayback() {
    if (videoList != null && !videoList.isEmpty()) {
      currentVideoIndex = currentVideoIndex < 0 ? 0 : currentVideoIndex;
      player.play(videoList.get(currentVideoIndex));
    } else {
      imageFetcher.reset();
      currentVideoIndex = -1;
      maybeLoadMoreVideos();
    }
  }

  public void stop() {
    if (player != null) {
      player.stop();
    }
  }

  public void togglePause() {
    if (player != null) {
      player.togglePause();
    }
  }

  public void play() {
    if (player != null) {
      player.play();
    }
  }

  public void pause() {
    if (player != null) {
      player.pause();
    }
  }

  public void release() {
    player.release();
  }

  public void playTargetedVideo() {
    // Might be null if the trigger is pulled before any video is loaded.
    if (videoList == null) {
      return;
    }

    final int videoIndex = renderer.getActiveThumbnailId();
    if (videoIndex < 0 || videoIndex >= videoList.size()) {
      logger.atSevere().log("Video index out of bounds");
      return;
    }

    cardboardView.queueEvent(new Runnable() {
        @Override
        public void run() {
          renderer.onThumbnailSelected(videoIndex);
        }
    });
    playVideoByIndex(videoIndex);
  }

  /**
   * Load local videos.
   */
  private void maybeLoadMoreVideos() {
    if (videoList == null) {
      videoFetcher.getLocalVideos();
    }
  }

  private void playNext() {
    if (videoList != null) {
      playVideoByIndex((currentVideoIndex + 1) % videoList.size());
    }
  }

  private void playVideoByIndex(int index) {
    if (videoList == null) {
      return;
    }

    if (index < 0 || index >= videoList.size()) {
      logger.atSevere().log(
          "Tried to play video at index: %d , but video list size is %d", index, videoList.size());
      return;
    }

    currentVideoIndex = index;
    player.play(videoList.get(index));
    renderer.setCurrentVideoIndex(index);
  }

  private void fetchThumbnails(List<VideoInfo> videoList, int startIndex) {
    int index = startIndex;
    List<Pair<Integer, VideoInfo>> thumbnailUrls = new ArrayList<>(videoList.size());
    for (VideoInfo videoInfo : videoList) {
      if (numThumbnailsFetched++ >= renderer.getMaxThumbnailCount()) {
        break;
      }
      thumbnailUrls.add(Pair.create(index++, videoInfo));
    }
    imageFetcher.downloadImages(thumbnailUrls);
  }

  // MyVideosPlayer.Listener

  @Override
  public void onVideoEnded() {
    playNext();
  }

  @Override
  public void onPlaybackError(int what, int extra) {
    if (consecutiveErrorCount++ < MAX_CONSECUTIVE_ERRORS_ALLOWED) {
      playNext();
    } else {
      // TODO(kvaalen): Show a visual error message.
      player.stop();
    }
  }

  @Override
  public void onPlaybackSucceeded() {
    consecutiveErrorCount = 0;
  }

  /**
   * Callback listening to streams from a GData request and plays the appropriate one.
   */
  private class VideoListCallback implements VideoFetcher.Callback {

    @Override
    public void onResponse(List<VideoInfo> videoList) {
      if (!videoList.isEmpty()) {
        int thumbnailStartIndex;
        if (VideoSequencer.this.videoList == null) {
          thumbnailStartIndex = 0;
          VideoSequencer.this.videoList = videoList;
          playVideoByIndex(0);
        } else {
          thumbnailStartIndex = VideoSequencer.this.videoList.size();
          // Append the results so that we can request several categories.
          VideoSequencer.this.videoList.addAll(videoList);
        }
        fetchThumbnails(videoList, thumbnailStartIndex);
        maybeLoadMoreVideos();
      } else {
        onError(new Exception("Video list is empty"));
      }
    }

    @Override
    public void onError(Exception exception) {
      // TODO(kvaalen): Show some type of visual error.
      logger.atSevere().withCause(exception).log("Error getting videos");
    }

  }

}
