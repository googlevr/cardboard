package com.google.vr.cardboard.paperscope.demo.myvideos;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Handler;
import android.view.Surface;
import com.google.common.flogger.FluentLogger;
import com.google.vr.cardboard.paperscope.demo.myvideos.VideoFetcher.VideoInfo;

/** Playback controller for local videos. */
public class MyVideosPlayer implements MyVideosRenderer.SurfaceTextureListener {

  /** Simple listener interface for video playback events. */
  public interface Listener {
    void onVideoEnded();
    void onPlaybackError(int what, int extra);
    void onPlaybackSucceeded();
  }

  private static final FluentLogger logger = FluentLogger.forEnclosingClass();

  private final Context context;
  private final Listener listener;
  private final MyVideosRenderer renderer;
  private final AudioManager audioManager;
  private final ProgressTracker progressTracker;

  private MediaPlayer mediaPlayer;
  private SurfaceTexture surfaceTexture;
  private boolean isSurfaceCreated;
  private Uri pendingStreamUri;
  private boolean isPaused;

  public MyVideosPlayer(Context context, Listener listener, MyVideosRenderer renderer) {
    this.context = context;
    this.listener = listener;
    this.renderer = renderer;
    audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    progressTracker = new ProgressTracker();
  }

  @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
  public void stop() {
    progressTracker.stopTracking();
    // Release the player instead of stopping so that an async prepare gets stopped.
    releasePlayer();
    audioManager.abandonAudioFocus(null);
  }

  public boolean isPaused() {
    return isPaused;
  }

  public void togglePause() {
    if (mediaPlayer != null) {
      if (isPaused) {
        mediaPlayer.start();
      } else {
        mediaPlayer.pause();
      }
      isPaused = !isPaused;
    }
  }

  public void play() {
    if (mediaPlayer != null && isPaused) {
      togglePause();
    }
  }

  public void pause() {
    if (mediaPlayer != null && !isPaused) {
      togglePause();
    }
  }

  public void play(VideoInfo video) {
    startPlaybackStream(video.getVideoStreamUri());
  }

  public void release() {
    progressTracker.stopTracking();
    releasePlayer();
  }

  public void releasePlayer() {
    if (mediaPlayer != null) {
      isPaused = false;
      mediaPlayer.release();
      mediaPlayer = null;
    }
  }

  private void startPlaybackStream(Uri streamUri) {
    renderer.setIsPlaybackStarting(true);
    renderer.setIsBuffering(true);
    if (!isSurfaceCreated) {
      pendingStreamUri = streamUri;
      return;
    }

    pendingStreamUri = null;

    if (mediaPlayer != null) {
      releasePlayer();
    }
    mediaPlayer = createMediaPlayer();

    try {
      mediaPlayer.setDataSource(context, streamUri);
    } catch (Exception e) {
      logger.atSevere().log(
          "MediaPlayer setMediaDataSource exception with stream: %s %s", streamUri, e.getMessage());
      return;
    }

    // Prepare and start the method asynchronously, otherwise it blocks the main thread.
    mediaPlayer.setOnPreparedListener(
        new MediaPlayer.OnPreparedListener() {
          @Override
          public void onPrepared(MediaPlayer mediaPlayer) {
            if (mediaPlayer.getVideoHeight() == 0) {
              logger.atSevere().log("Video looks invalid, its height is zero.");
              return;
            }

            float videoAspectRatio =
                ((float) mediaPlayer.getVideoWidth()) / mediaPlayer.getVideoHeight();
            renderer.setVideoAspectRatio(videoAspectRatio);
            mediaPlayer.start();
          }
        });

    try {
      mediaPlayer.prepareAsync();
      progressTracker.startTracking();
    } catch (IllegalStateException e) {
      logger.atSevere().log("MediaPlayer prepare() failed %s", e.getMessage());
      return;
    }
  }

  @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
  private MediaPlayer createMediaPlayer() {
    MediaPlayer mediaPlayer = new MediaPlayer();
    mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
    mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
        @Override
        public void onCompletion(MediaPlayer mediaPlayer) {
          listener.onVideoEnded();
        }
    });
    mediaPlayer.setOnErrorListener(
        new MediaPlayer.OnErrorListener() {
          @Override
          public boolean onError(MediaPlayer mediaPlayer, int what, int extra) {
            logger.atSevere().log("MediaPlayer error - what: %s, extra: %s", what, extra);
            // TODO(kvaalen): Have more fine grained control over the listener events.
            listener.onPlaybackError(what, extra);
            return true;
          }
        });
    mediaPlayer.setOnInfoListener(
        new MediaPlayer.OnInfoListener() {
          @Override
          public boolean onInfo(MediaPlayer mediaPlayer, int what, int extra) {
            switch (what) {
              case MediaPlayer.MEDIA_INFO_BUFFERING_START -> renderer.setIsBuffering(true);
              case MediaPlayer.MEDIA_INFO_BUFFERING_END -> {
                renderer.setIsBuffering(false);
                listener.onPlaybackSucceeded();
              }
              case MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START -> {
                renderer.setIsPlaybackStarting(false);
                renderer.setIsBuffering(false);
              }
            }
            return true;
          }
        });

    Surface surface = new Surface(surfaceTexture);
    mediaPlayer.setSurface(surface);
    surface.release();

    return mediaPlayer;
  }

  // MyVideosStereoRenderer.SurfaceTextureListener

  @Override
  public void onSurfaceTextureCreated(SurfaceTexture surfaceTexture) {
    isSurfaceCreated = true;
    this.surfaceTexture = surfaceTexture;
    if (pendingStreamUri != null) {
      startPlaybackStream(pendingStreamUri);
    }
  }

  /**
   * Sends pings to check the progress of the video every so often.
   */
  private class ProgressTracker {

    private static final int DELAY_MILLIS = 100;

    private final Handler handler;
    private final Runnable ping;

    private boolean lastIsBuffering;
    private boolean lastIsPlaybackStarting;

    @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
    public ProgressTracker() {
      handler = new Handler();
      ping = new Runnable() {
          @Override
          public void run() {
            ping();
          }
      };
    }

    public void startTracking() {
      handler.removeCallbacksAndMessages(null);
      lastIsBuffering = true;
      lastIsPlaybackStarting = true;
      ping();
    }

    public void stopTracking() {
      handler.removeCallbacksAndMessages(null);
    }

    private void ping() {
      checkIsPlaybackStarted();
      checkBufferingStatus();
      float ratio;
      if (mediaPlayer != null && mediaPlayer.isPlaying()) {
        int pos = mediaPlayer.getCurrentPosition();
        int duration = mediaPlayer.getDuration();
        ratio = pos / (float) duration;
      } else {
        ratio = 0f;
      }
      renderer.setProgressRatio(ratio);
      handler.postDelayed(ping, DELAY_MILLIS);
    }

    /**
     * For some reason the Galaxy Note 2 doesn't get the onInfo events from the MediaPlayer.
     * This check is a fallback to detect if a buffering spinner should be shown.
     * TODO(kvaalen): Figure out why the Galaxy Note 2 doesn't get onInfo events and remove this
     *     when it's fixed.
     */
    private void checkBufferingStatus() {
      boolean isBuffering = mediaPlayer == null || !mediaPlayer.isPlaying();
      if (isBuffering == lastIsBuffering) {
        return;
      }

      renderer.setIsBuffering(isBuffering && !isPaused);
      lastIsBuffering = isBuffering;
    }

    /**
     * For some reason the Galaxy Note 2 doesn't get the onInfo events from the MediaPlayer.
     * This check is a fallback to detect if the splash screen should be shown.
     * TODO(kvaalen): Figure out why the Galaxy Note 2 doesn't get onInfo events and remove this
     *     when it's fixed.
     */
    private void checkIsPlaybackStarted() {
      boolean isPlaybackStarting =
          !isPaused
          && (mediaPlayer == null
              || !mediaPlayer.isPlaying()
              || mediaPlayer.getCurrentPosition() == 0);
      if (isPlaybackStarting == lastIsPlaybackStarting) {
        return;
      }

      renderer.setIsPlaybackStarting(isPlaybackStarting);
      lastIsPlaybackStarting = isPlaybackStarting;
    }

  }

}
