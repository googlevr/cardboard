package com.google.vr.cardboard.paperscope.demo.myvideos;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;
import com.google.common.flogger.FluentLogger;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * A basic video fetcher for videos stored on the phone.
 * TODO(dcoz): test this class.
 */
public class VideoFetcher {

  private static final FluentLogger logger = FluentLogger.forEnclosingClass();

  /** Simple structure with information about a video stored on the phone file system. */
  public static class VideoInfo {

    /** Stream for the video. */
    private Uri videoStreamUri;

    /** The absolute file path to the video file. */
    private String videoAbsolutePath;

    public VideoInfo(Uri videoStreamUri, String videoAbsolutePath) {
      this.videoStreamUri = videoStreamUri;
      this.videoAbsolutePath = videoAbsolutePath;
    }

    public Uri getVideoStreamUri() {
      return videoStreamUri;
    }

    public String getVideoAbsolutePath() {
      return videoAbsolutePath;
    }
  }

  /**
   * Interface used to communicate video results back to requesters.
   */
  public interface Callback {
    /**
     * Called when a video list response was obtained successfully.
     */
    void onResponse(List<VideoInfo> response);

    /**
     * Called when an exception is caught while getting the response.
     */
    void onError(Exception exception);
  }

  private final Callback videoListCallback;
  private Cursor videoCursor;
  private Context context;

  public VideoFetcher(Context context, Callback videoListCallback) {
    this.context = context;
    this.videoListCallback = videoListCallback;
  }

  /**
   * Fetches the local videos stored on the phone. On success, the list of videos are sent to
   * the video callback specified in the constructor.
   */
  public void getLocalVideos() {
    // Prepare a query for local videos, sorted by date.
    // TODO(dcoz): figure out what we should do for videos that don't have the expected 'landscape'
    // aspect ratio.

    String[] projection = {
      MediaStore.Video.Media._ID, MediaStore.Video.Media.DATA,
    };
    videoCursor = context.getContentResolver().query(
        MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
        projection,
        null,
        null,
        MediaStore.MediaColumns.DATE_ADDED + " DESC");

    if (videoCursor == null) {
      logger.atWarning().log("Error querying local videos.");
      videoListCallback.onError(new IOException("Error querying local videos."));
    }

    int idColumn = videoCursor.getColumnIndex(MediaStore.Video.Media._ID);
    int dataColumn = videoCursor.getColumnIndex(MediaStore.Video.Media.DATA);

    List<VideoInfo> videoInfoList = new ArrayList<VideoInfo>();
    while (videoCursor.moveToNext()) {
      Uri contentUri = Uri.parse(
          android.provider.MediaStore.Video.Media.EXTERNAL_CONTENT_URI
              + "/" + videoCursor.getString(idColumn));

      videoInfoList.add(new VideoInfo(contentUri, videoCursor.getString(dataColumn)));
    }

    videoListCallback.onResponse(videoInfoList);
  }

}
