package com.google.vr.cardboard.paperscope.demo.myvideos;

import com.google.vr.cardboard.paperscope.demo.R;

import static java.lang.Math.max;
import static java.lang.Math.min;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import android.os.SystemClock;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.CardboardView.Eye;
import com.google.cardboard.sdk.HeadTransform;
import com.google.cardboard.sdk.Viewport;
import com.google.common.base.VerifyException;
import com.google.common.flogger.FluentLogger;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.egl.EGLConfig;

/** Renderer that displays a main video screen and a grid of video thumbnails around the viewer. */
public class MyVideosRenderer
    implements CardboardView.Renderer, SurfaceTexture.OnFrameAvailableListener {

  /** Listener for surface texture created events . */
  public interface SurfaceTextureListener {
    void onSurfaceTextureCreated(SurfaceTexture surfaceTexture);
  }

  private volatile int activeThumbnail = -1;
  private final float[] thumbnailDistances;
  private final float[] thumbnailBrightness;
  private final float[] thumbnailAlpha;
  private volatile int nowPlayingIndex = -1;

  private static final FluentLogger logger = FluentLogger.forEnclosingClass();

  private static final int NUM_SPRITE_TEXTURES = 7;
  private static final int NUM_THUMBNAIL_TEXTURES = 80;

  private static final int FLOAT_SIZE_BYTES = 4;
  private static final int TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
  private static final int TRIANGLE_VERTICES_DATA_POS_OFFSET = 0;
  private static final int TRIANGLE_VERTICES_DATA_UV_OFFSET = 3;

  // All distances in meters unless otherwise specified.

  private static final float SCENE_MULTIPLIER = 1;
  private static final float CAMERA_DISTANCE = 5 * SCENE_MULTIPLIER;

  private static final int FBO_WIDTH = 8;
  private static final int FBO_HEIGHT = 8;
  private boolean enableFramebuffer = false;
  private ByteBuffer framePixels;

  // Red, green and blue values for the glow.
  private final float[] glowValues;

  // Distance from camera to thumbnail, thumbnail sphere radius.
  private static final float DEFAULT_THUMBNAIL_DISTANCE = 4 * CAMERA_DISTANCE;
  private static final float ACTIVE_THUMBNAIL_DISTANCE = 2.5f * CAMERA_DISTANCE;
  private static final float Z_NEAR = 1;
  private static final float Z_FAR = DEFAULT_THUMBNAIL_DISTANCE * 2 * SCENE_MULTIPLIER;
  private static final float HALF_VIDEO_WIDTH = 2.8f * SCENE_MULTIPLIER;
  private static final float FLOOR_TILE_SIZE = 4f * SCENE_MULTIPLIER;
  private static final float VIDEO_WIDTH = HALF_VIDEO_WIDTH * 2;

  // The default video aspect ratio. This is used to display the video glow, which acts like a
  // a TV frame.
  private static final float DEFAULT_ASPECT_RATIO = (16 / 9f);
  private static final float VIDEO_HEIGHT = VIDEO_WIDTH / DEFAULT_ASPECT_RATIO;
  private static final float HALF_VIDEO_HEIGHT = VIDEO_HEIGHT / 2;
  private static final float CAMERA_HEIGHT = 0;
  // Angle between sphere slices/rows.
  private static final float ROW_ANGLE = 18;
  // Number of sphere rows. N translates to center row + N-1 rows on top and N-1 on bottom.
  private static final int ROWS_COUNT = 3;
  // Angle reserved for screen, thumbnails wouldn't be drawn within it.
  private static final float SCREEN_ANGLE = 100f;
  // Approximate screen angles used to tell if the user is targeting the screen when pulling the
  // magnet. Don't target the side edges of the screen.
  private static final float TARGET_SCREEN_YAW = SCREEN_ANGLE - 40f;
  private static final float TARGET_SCREEN_PITCH_UP = 45f;
  private static final float TARGET_SCREEN_PITCH_DOWN = -20f;
  private static final float TARGET_SEARCH_YAW = SCREEN_ANGLE - 80f;
  private static final float TARGET_SEARCH_PITCH_UP = -30f;
  private static final float TARGET_SEARCH_PITCH_DOWN = -45f;
  private static final float HALF_THUMBNAIL_WIDTH = 3.05f * SCENE_MULTIPLIER;
  private static final float THUMBNAIL_WIDTH = HALF_THUMBNAIL_WIDTH * 2;
  private static final float THUMBNAIL_HEIGHT = THUMBNAIL_WIDTH * (9 / 16f);
  private static final float HALF_THUMBNAIL_HEIGHT = THUMBNAIL_HEIGHT / 2;
  private static final float SEARCH_SIZE = HALF_THUMBNAIL_HEIGHT / 4f;

  private static final float[] videoVerticesData = {
    //              X,                  Y, Z,      U, V
    -HALF_VIDEO_WIDTH, HALF_VIDEO_HEIGHT, 0, 1, 1, HALF_VIDEO_WIDTH, HALF_VIDEO_HEIGHT, 0, 0, 1,
    -HALF_VIDEO_WIDTH, -HALF_VIDEO_HEIGHT, 0, 1, 0, HALF_VIDEO_WIDTH, -HALF_VIDEO_HEIGHT, 0, 0, 0,
  };

  private static final float[] thumbnailVerticesData = {
    //                  X,                      Y, Z,      U, V
    -HALF_THUMBNAIL_WIDTH, HALF_THUMBNAIL_HEIGHT, 0, 1, 1, HALF_THUMBNAIL_WIDTH,
        HALF_THUMBNAIL_HEIGHT, 0, 0, 1,
    -HALF_THUMBNAIL_WIDTH, -HALF_THUMBNAIL_HEIGHT, 0, 1, 0, HALF_THUMBNAIL_WIDTH,
        -HALF_THUMBNAIL_HEIGHT, 0, 0, 0,
  };

  private static final float[] floorVerticesData = {
    //        X,            Y, Z,      U, V
    -FLOOR_TILE_SIZE, FLOOR_TILE_SIZE, 0, 1, 1, FLOOR_TILE_SIZE, FLOOR_TILE_SIZE, 0, 0, 1,
    -FLOOR_TILE_SIZE, -FLOOR_TILE_SIZE, 0, 1, 0, FLOOR_TILE_SIZE, -FLOOR_TILE_SIZE, 0, 0, 0,
  };

  private static final float[] lightboxVerticesData = {
    // X,Y,Z,      U, V
    1, 1, 0, 1, 1, -1, 1, 0, 0, 1, 1, -1, 0, 1, 0, -1, -1, 0, 0, 0,
  };

  private static final float[] searchVerticesData = {
    // X,Y,Z,      U, V
    SEARCH_SIZE,
    SEARCH_SIZE,
    0,
    0,
    1,
    -SEARCH_SIZE,
    SEARCH_SIZE,
    0,
    1,
    1,
    SEARCH_SIZE,
    -SEARCH_SIZE,
    0,
    0,
    0,
    -SEARCH_SIZE,
    -SEARCH_SIZE,
    0,
    1,
    0,
  };

  // The seek bar is a thin bar just under the video. It will get scaled in the X axis to be the
  // right width.
  private static final float[] seekBarVerticesData = {
    //          X,                          Y, Z,      U, V
    0,
    -HALF_VIDEO_HEIGHT * 1.01f,
    0,
    0,
    1,
    -VIDEO_WIDTH,
    -HALF_VIDEO_HEIGHT * 1.01f,
    0,
    1,
    1,
    0,
    -HALF_VIDEO_HEIGHT * 1.02f,
    0,
    0,
    0,
    -VIDEO_WIDTH,
    -HALF_VIDEO_HEIGHT * 1.02f,
    0,
    1,
    0,
  };

  private static final float DEFAULT_THUMBNAIL_BRIGHTNESS = 0.5f;
  private static final float ACTIVE_THUMBNAIL_BRIGHTNESS = 1.1f;
  private static final float NOW_PLAYING_THUMBNAIL_BRIGHTNESS = 0.4f;
  private static final float MIN_VIDEO_GLOW_BRIGHTNESS = 0.2f;
  // The red, green, and blue brightness values for the glow when no video is playing.
  // It has a red tint to match the red background when there is no video.
  private static final float[] startVideoGlowBrightness = {1f, .2f, .5f};

  // Full screen quad shader for drawing of a texture using texture coordinates as screen space
  // vertex coordinates.
  private static final String FSQ_VERTEX_SHADER =
      """
      const vec2 xy_rescale = vec2(0.5, 0.5);
      attribute vec4 aPosition;
      varying vec2 vTextureCoord;
      void main() {
        vTextureCoord = xy_rescale + aPosition.xy * xy_rescale;
        gl_Position = vec4(aPosition.xy, 0.0, 1.0);
      }\
      """;

  private static final String FSQ_FRAGMENT_SHADER =
      """
      #extension GL_OES_EGL_image_external : require
      precision mediump float;
      varying vec2 vTextureCoord;
      uniform samplerExternalOES imageTexture;
      void main() {
        vec4 color = texture2D(imageTexture, vTextureCoord);
        gl_FragColor = color + vec4(vTextureCoord.x * 0.1, vTextureCoord.y * 0.1, 0.0, 1.0);
      }
      """;

  private static final String VERTEX_SHADER =
      """
      uniform mat4 uMvpMatrix;
      uniform mat4 uStMatrix;
      attribute vec4 aPosition;
      attribute vec4 aTextureCoord;
      varying vec2 vTextureCoord;
      void main() {
        gl_Position = uMvpMatrix * aPosition;
        vTextureCoord = (uStMatrix * aTextureCoord).xy;
      }
      """;

  private static final String FRAGMENT_SHADER =
      """
      precision mediump float;
      varying vec2 vTextureCoord;
      uniform sampler2D imageTexture;
      uniform vec4 uBrightness;
      void main() {
        gl_FragColor = texture2D(imageTexture, vTextureCoord) * uBrightness;
      }
      """;

  // Fragment shader using the samplerExternalOES so that it can use video as a texture.
  private static final String VIDEO_FRAGMENT_SHADER =
      """
      #extension GL_OES_EGL_image_external : require
      precision mediump float;
      varying vec2 vTextureCoord;
      uniform samplerExternalOES sTexture;
      void main() {
        gl_FragColor = texture2D(sTexture, vTextureCoord);
      }
      """;

  /** Struct holding a program and its handlers. */
  private static class ProgramHolder {
    int program;
    int uMvpMatrix;
    int uStMatrix;
    int aPosition;
    int aTexture;
    int uBrightness;
  }

  private static class FsqProgramHolder {
    int program;
    int aPosition;
  }

  private final FloatBuffer videoVertices;
  private final FloatBuffer thumbnailVertices;
  private final FloatBuffer lightboxVertices;
  private final FloatBuffer seekBarVertices;
  private final FloatBuffer floorVertices;
  private final FloatBuffer searchVertices;

  private final float[] mvpMatrix = new float[16];
  private final float[] tmpMatrix = new float[16];
  private final float[] videoStMatrix = new float[16];
  private final float[] spriteStMatrix = new float[16];

  // Model matrix
  private final float[] modelMatrix = new float[16];
  // View matrix
  private final float[] viewMatrix = new float[16];
  private final float[] lookAtMatrix = new float[16];
  private final float[] headViewMatrix = new float[16];

  // We use a separate program for the video texture since it uses its own fragment shader.
  private ProgramHolder videoProgramHolder;
  // Program used for all image based textures.
  private ProgramHolder spriteProgramHolder;
  private FsqProgramHolder fsqProgramHolder;

  private int[] textureIds;
  private int[] thumbnailTextureIds;
  private int transitionBackgroundTextureId;
  private int noVideoBackgroundTextureId;
  private int videoGlowTextureId;
  private int nowPlayingTextureId;
  private int seekBarTextureId;
  private int videoTextureId;
  private int floorTextureId;

  private SurfaceTexture surfaceTexture;
  private boolean surfaceNeedsUpdate = false;
  private long initialTime;
  private float animationTime;

  private static final int GL_TEXTURE_EXTERNAL_OES = 0x8D65;

  private final Context context;
  private final Bitmap[] thumbnailBitmaps;

  private SurfaceTextureListener surfaceTextureListener;

  private boolean isPlaybackStarting;
  private float seekBarRatio;

  private final float[] eulerAngles = new float[3];
  private float lastYaw = 0.0f;
  private float lastPitch = 0.0f;
  private float targetScreenYaw;
  private float screenYaw;
  private boolean isScreenTargeted;
  private boolean isSearchTargeted;

  private int[] fboTargetTextureIds;
  private int fboTargetTextureId;
  private int framebuffer;
  private int frameCounter = 0;

  private boolean isListening;

  // Whether there is no videos to display, in which case we show a "no video" background
  // explaining to the user that they can take videos to view them here.
  private boolean noVideos = true;

  // Video aspect ratio. Will be updated according to video played.
  private float videoAspectRatio = DEFAULT_ASPECT_RATIO;

  public MyVideosRenderer(Context context) {
    this.context = context;

    videoVertices =
        ByteBuffer.allocateDirect(videoVerticesData.length * FLOAT_SIZE_BYTES)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    videoVertices.put(videoVerticesData).position(0);

    thumbnailVertices =
        ByteBuffer.allocateDirect(thumbnailVerticesData.length * FLOAT_SIZE_BYTES)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    thumbnailVertices.put(thumbnailVerticesData).position(0);

    lightboxVertices =
        ByteBuffer.allocateDirect(lightboxVerticesData.length * FLOAT_SIZE_BYTES)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    lightboxVertices.put(lightboxVerticesData).position(0);

    seekBarVertices =
        ByteBuffer.allocateDirect(seekBarVerticesData.length * FLOAT_SIZE_BYTES)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    seekBarVertices.put(seekBarVerticesData).position(0);

    floorVertices =
        ByteBuffer.allocateDirect(floorVerticesData.length * FLOAT_SIZE_BYTES)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    floorVertices.put(floorVerticesData).position(0);

    searchVertices =
        ByteBuffer.allocateDirect(searchVerticesData.length * FLOAT_SIZE_BYTES)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    searchVertices.put(searchVerticesData).position(0);

    thumbnailBitmaps = new Bitmap[NUM_THUMBNAIL_TEXTURES];
    thumbnailDistances = new float[NUM_THUMBNAIL_TEXTURES];
    thumbnailBrightness = new float[NUM_THUMBNAIL_TEXTURES];
    thumbnailAlpha = new float[NUM_THUMBNAIL_TEXTURES];
    glowValues = new float[3];

    isPlaybackStarting = true;
  }

  public synchronized void setVideoAspectRatio(float videoAspectRatio) {
    // We don't want to go over DEFAULT_ASPECT_RATIO otherwise the video would go over the video
    // frame.
    this.videoAspectRatio = min(DEFAULT_ASPECT_RATIO, videoAspectRatio);
  }

  public synchronized float getVideoAspectRatio() {
    return videoAspectRatio;
  }

  public void setSurfaceTextureListener(SurfaceTextureListener surfaceTextureListener) {
    this.surfaceTextureListener = surfaceTextureListener;
  }

  public void setIsBuffering(boolean isBuffering) {}

  public void setIsPlaybackStarting(boolean isPlaybackStarting) {
    this.isPlaybackStarting = isPlaybackStarting;
    setGlowValues(
        startVideoGlowBrightness[0], startVideoGlowBrightness[1], startVideoGlowBrightness[2]);
  }

  private void setGlowValues(float r, float g, float b) {
    glowValues[0] = r;
    glowValues[1] = g;
    glowValues[2] = b;
  }

  public void setCurrentVideoIndex(int videoIndex) {
    this.nowPlayingIndex = videoIndex;
  }

  public void setProgressRatio(float seekBarRatio) {
    this.seekBarRatio = seekBarRatio;
  }

  public int getMaxThumbnailCount() {
    return NUM_THUMBNAIL_TEXTURES;
  }

  public synchronized int getActiveThumbnailId() {
    return activeThumbnail;
  }

  public boolean isScreenOnlyTargeted() {
    return isScreenTargeted && activeThumbnail == -1;
  }

  public boolean isSearchTargeted() {
    return isSearchTargeted;
  }

  public void centerScreen() {
    targetScreenYaw = lastYaw + screenYaw - 180f;
  }

  private synchronized void setNoVideos(boolean noVideos) {
    this.noVideos = noVideos;
  }

  private synchronized boolean hasNoVideos() {
    return noVideos;
  }

  public void addThumbnail(Bitmap bitmap, int index) {
    if (index >= NUM_THUMBNAIL_TEXTURES) {
      logger.atSevere().log("Adding thumbnail with an index too high");
      return;
    }

    // All thumbnails at the quality we request have letterboxing. Crop it out.
    int cropAmount = bitmap.getHeight() / 8;
    Bitmap croppedBitmap =
        Bitmap.createBitmap(
            bitmap, 0, cropAmount, bitmap.getWidth(), bitmap.getHeight() - 2 * cropAmount);
    // Textures sizes need to be a power of 2.
    Bitmap scaledBitmap = Bitmap.createScaledBitmap(croppedBitmap, 256, 256, false);
    thumbnailBitmaps[index] = scaledBitmap;
    loadThumbnailTexture(index);

    bitmap.recycle();
    croppedBitmap.recycle();
  }

  public void onThumbnailSelected(int id) {
    if (id < NUM_THUMBNAIL_TEXTURES && thumbnailBitmaps[id] != null && activeThumbnail != -1) {
      // If the user selects a thumbnail, show that it is selected by making it pop out and zoom
      // again.
      thumbnailDistances[id] = DEFAULT_THUMBNAIL_DISTANCE;
    }
  }

  @Override
  public void onNewFrame(HeadTransform headTransform) {
    headTransform.getHeadView(headViewMatrix, 0);
    // Yaw must be remapped due to sphere being upside down.
    headTransform.getEulerAngles(eulerAngles, 0);
    lastYaw = 180f - (float) Math.toDegrees(-eulerAngles[1]) - screenYaw;
    lastYaw = lastYaw % 360f;
    if (lastYaw < 0f) {
      lastYaw += 360f;
    }
    lastPitch = (float) Math.toDegrees(eulerAngles[0]);
    isScreenTargeted =
        lastYaw > 180f - (TARGET_SCREEN_YAW / 2)
            && lastYaw < 180f + (TARGET_SCREEN_YAW / 2)
            && lastPitch < TARGET_SCREEN_PITCH_UP
            && lastPitch > TARGET_SCREEN_PITCH_DOWN;
    isSearchTargeted =
        lastYaw > 180f - (TARGET_SEARCH_YAW / 2)
            && lastYaw < 180f + (TARGET_SEARCH_YAW / 2)
            && lastPitch < TARGET_SEARCH_PITCH_UP
            && lastPitch > TARGET_SEARCH_PITCH_DOWN;

    synchronized (this) {
      if (surfaceNeedsUpdate) {
        surfaceTexture.updateTexImage();
        surfaceTexture.getTransformMatrix(videoStMatrix);
        surfaceNeedsUpdate = false;
      }
    }
    animationTime = SystemClock.uptimeMillis() - initialTime;

    if (enableFramebuffer) {
      frameCounter++;
      if (frameCounter == 5) {
        frameCounter = 0;
        getAverageFrameColor();
      }
    }
  }

  @Override
  public void onDrawEye(Eye eye) {
    eye.applyHeadView(headViewMatrix);

    GLES20.glEnable(GLES20.GL_BLEND);
    GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

    drawFloor(eye);
    drawVideoGlow(eye);
    drawVideo(eye);
    drawSeekBar(eye);
    drawThumbnailSphere(eye, 46);
  }

  @Override
  public void onFinishFrame(Viewport viewport) {}

  private void drawThumbnail(
      int id, boolean active, float thumbnailAngle, float rowAngle, Eye eye) {
    if (thumbnailBitmaps[id] == null) {
      // Not thumbnail for this id.
      return;
    }

    int textureId = thumbnailTextureIds[id];
    createGeometry(spriteProgramHolder, thumbnailVertices, textureId);

    // Slow up-down movement animation.
    float angleAnimationDiff = (float) Math.sin(animationTime / 4000f + (id * 10f)) * 1.5f;

    setBaseModelMatrix();
    Matrix.rotateM(modelMatrix, 0, thumbnailAngle, 0, 1, 0);
    Matrix.rotateM(modelMatrix, 0, rowAngle + angleAnimationDiff, 1, 0, 0);

    float thumbnailDistance = updateAndGetThumbnailDistance(id, active);
    Matrix.translateM(modelMatrix, 0, 0, 0, thumbnailDistance);

    // Flip the model for the background so that we can have it upright regardless of the stMatrix
    Matrix.rotateM(modelMatrix, 0, 180, 1, 0, 0);

    Matrix.multiplyMM(viewMatrix, 0, eye.getEyeView(), 0, lookAtMatrix, 0);
    Matrix.multiplyMM(tmpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
    Matrix.multiplyMM(mvpMatrix, 0, eye.getPerspective(Z_NEAR, Z_FAR), 0, tmpMatrix, 0);

    GLES20.glUniformMatrix4fv(spriteProgramHolder.uMvpMatrix, 1, false, mvpMatrix, 0);
    GLES20.glUniformMatrix4fv(spriteProgramHolder.uStMatrix, 1, false, spriteStMatrix, 0);

    float alpha = updateAndGetThumbnailAlpha(id);
    float brightness = alpha * updateAndGetThumbnailBrightness(id, active);
    GLES20.glUniform4f(spriteProgramHolder.uBrightness, brightness, brightness, brightness, 1f);

    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");

    // TODO(kvaalen): merge this now playing gl drawing with the above thumbnail gl drawing.
    //     They do the same thing, except that the now playing is smaller and lower.
    if (id == nowPlayingIndex) {

      createGeometry(spriteProgramHolder, thumbnailVertices, nowPlayingTextureId);

      // Slow up-down movement animation.
      angleAnimationDiff = (float) Math.sin(animationTime / 4000f + (id * 10f)) * 1.5f;

      setBaseModelMatrix();
      Matrix.rotateM(modelMatrix, 0, thumbnailAngle, 0, 1, 0);
      Matrix.rotateM(modelMatrix, 0, rowAngle + angleAnimationDiff, 1, 0, 0);

      Matrix.translateM(modelMatrix, 0, 0, 0, thumbnailDistance * 0.95f);

      // Flip the model for the background so that we can have it upright regardless of the stMatrix
      Matrix.rotateM(modelMatrix, 0, 180, 1, 0, 0);

      Matrix.scaleM(modelMatrix, 0, 0.6f, 0.6f, 0.6f);

      Matrix.multiplyMM(viewMatrix, 0, eye.getEyeView(), 0, lookAtMatrix, 0);
      Matrix.multiplyMM(tmpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
      Matrix.multiplyMM(mvpMatrix, 0, eye.getPerspective(Z_NEAR, Z_FAR), 0, tmpMatrix, 0);

      GLES20.glUniformMatrix4fv(spriteProgramHolder.uMvpMatrix, 1, false, mvpMatrix, 0);
      GLES20.glUniformMatrix4fv(spriteProgramHolder.uStMatrix, 1, false, spriteStMatrix, 0);
      GLES20.glUniform4f(spriteProgramHolder.uBrightness, 1.2f, 1.2f, 1.2f, 1.2f);

      GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
      checkGlError("glDrawArrays");
    }
  }

  private float updateAndGetThumbnailAlpha(int id) {
    // TODO(kvaalen): Make these animations depend on absolute time, not on the frame rate.
    thumbnailAlpha[id] = thumbnailAlpha[id] + ((1f - thumbnailAlpha[id]) / 30f);
    return thumbnailAlpha[id];
  }

  private float updateAndGetThumbnailDistance(int id, boolean isActive) {
    if (thumbnailDistances[id] == 0f) {
      thumbnailDistances[id] = DEFAULT_THUMBNAIL_DISTANCE;
    }

    float targetThumbnailDistance =
        isActive ? ACTIVE_THUMBNAIL_DISTANCE : DEFAULT_THUMBNAIL_DISTANCE;

    thumbnailDistances[id] =
        thumbnailDistances[id] + ((targetThumbnailDistance - thumbnailDistances[id]) / 10f);

    return thumbnailDistances[id];
  }

  private float updateAndGetThumbnailBrightness(int id, boolean isActive) {
    if (thumbnailBrightness[id] == 0f) {
      thumbnailBrightness[id] = DEFAULT_THUMBNAIL_BRIGHTNESS;
    }

    float targetThumbnailBrightness =
        isActive ? ACTIVE_THUMBNAIL_BRIGHTNESS : DEFAULT_THUMBNAIL_BRIGHTNESS;

    targetThumbnailBrightness =
        (nowPlayingIndex == id) ? NOW_PLAYING_THUMBNAIL_BRIGHTNESS : targetThumbnailBrightness;

    thumbnailBrightness[id] =
        thumbnailBrightness[id] + ((targetThumbnailBrightness - thumbnailBrightness[id]) / 10);

    return thumbnailBrightness[id];
  }

  private void drawThumbnailSphere(Eye eye, int thumbnails) {
    GLES20.glUseProgram(spriteProgramHolder.program);
    checkGlError("glUseProgram");

    int decreasePerRow = 2;
    int unusedOnAllRows = 0;
    for (int row = 0; row < ROWS_COUNT; row++) {
      unusedOnAllRows += decreasePerRow * (row + 1);
    }

    int thumbnailPerRow = (thumbnails + unusedOnAllRows) / ((ROWS_COUNT - 1) * 2 + 1);

    // Number of thumbnail on this row would be decreased by this value.
    int unusedOnCurrentRow = 0;
    int thumbnailId = 0;
    float rowAngle = 0;
    int newActiveThumbnail = -1;
    for (int row = 0; row < ROWS_COUNT; row++) {
      int tiles = thumbnailPerRow - unusedOnCurrentRow;

      for (int x = 0; x < tiles; x++) {
        float angleRegion = 360f - SCREEN_ANGLE;

        float anglesPerTile = angleRegion / (tiles - 1);
        float thumbnailAngle = (anglesPerTile * x) + (SCREEN_ANGLE / 2);
        if (thumbnailAngle > 360f) {
          thumbnailAngle -= 360f;
        }

        float minRowAngle = -rowAngle - (ROW_ANGLE / 2f);
        float maxRowAngle = -rowAngle + (ROW_ANGLE / 2f);
        boolean rowActive = (lastPitch >= minRowAngle) && (lastPitch < maxRowAngle);

        float minAngle = thumbnailAngle - (anglesPerTile / 2f) - 180f;
        float maxAngle = thumbnailAngle + (anglesPerTile / 2f) - 180f;

        float adjustedMinAngle = minAngle < 0 ? minAngle + 360 : minAngle;
        float adjustedMaxAngle = minAngle < 0 ? maxAngle + 360 : maxAngle;
        boolean columnActive = (lastYaw >= adjustedMinAngle) && (lastYaw < adjustedMaxAngle);

        drawThumbnail(thumbnailId, columnActive && rowActive, thumbnailAngle, rowAngle, eye);
        if (columnActive && rowActive) {
          newActiveThumbnail = thumbnailId;
        }
        thumbnailId += 1;

        // Draw bottom of the sphere, but because view is upsidedown - this is actually top half.
        if (row != 0) {
          rowActive = false;
          minRowAngle = rowAngle - (ROW_ANGLE / 2f);
          maxRowAngle = rowAngle + (ROW_ANGLE / 2f);
          rowActive = (lastPitch >= minRowAngle) && (lastPitch < maxRowAngle);
          drawThumbnail(thumbnailId, columnActive && rowActive, thumbnailAngle, -rowAngle, eye);
          if (columnActive && rowActive) {
            newActiveThumbnail = thumbnailId;
          }
          thumbnailId += 1;
        }
      }
      rowAngle += ROW_ANGLE;
      unusedOnCurrentRow += 2;
    }

    synchronized (this) {
      activeThumbnail = newActiveThumbnail;
    }
  }

  /**
   * Create the base model matrix and set it in MMAtrix. The base model matrix is the rotation
   * matrix to place the screen in the location where it's thumbnail was.
   */
  private void setBaseModelMatrix() {
    screenYaw = screenYaw + ((targetScreenYaw - screenYaw) / 800);
    Matrix.setRotateM(modelMatrix, 0, screenYaw, 0, 1, 0);
  }

  public void setListenStatusActive(boolean active) {
    isListening = active;
  }

  public boolean isListening() {
    return isListening;
  }

  private void drawVideo(Eye eye) {
    float[] stMatrix;
    ProgramHolder programHolder;
    int textureId;

    // If the video hasn't yet started.
    if (isPlaybackStarting) {
      stMatrix = spriteStMatrix;
      programHolder = spriteProgramHolder;

      if (hasNoVideos()) {
        textureId = noVideoBackgroundTextureId;
      } else {
        // There's at least one video, so if we're here it means we're starting playback of
        // another video, so show a transition texture.
        textureId = transitionBackgroundTextureId;
      }
    } else {
      stMatrix = videoStMatrix;
      programHolder = videoProgramHolder;
      textureId = videoTextureId;
    }

    GLES20.glUseProgram(programHolder.program);
    checkGlError("glUseProgram");
    createGeometry(programHolder, videoVertices, textureId);

    setBaseModelMatrix();

    // Scale the video texture according to its aspect ratio.
    float horizontalScaling = getVideoAspectRatio() / DEFAULT_ASPECT_RATIO;
    Matrix.scaleM(modelMatrix, 0, horizontalScaling, 1.0f, 1.0f);

    Matrix.translateM(modelMatrix, 0, 0, 0, CAMERA_DISTANCE);

    if (isPlaybackStarting) {
      // Flip the model for the background so that we can have it upright regardless of the stMatrix
      Matrix.rotateM(modelMatrix, 0, 180, 1, 0, 0);
      GLES20.glUniform4f(spriteProgramHolder.uBrightness, 1, 1, 1, 1);
    }

    Matrix.multiplyMM(viewMatrix, 0, eye.getEyeView(), 0, lookAtMatrix, 0);
    Matrix.multiplyMM(tmpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
    Matrix.multiplyMM(mvpMatrix, 0, eye.getPerspective(Z_NEAR, Z_FAR), 0, tmpMatrix, 0);

    GLES20.glUniformMatrix4fv(programHolder.uMvpMatrix, 1, false, mvpMatrix, 0);
    GLES20.glUniformMatrix4fv(programHolder.uStMatrix, 1, false, stMatrix, 0);

    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");
  }

  private void drawFloor(Eye eye) {
    GLES20.glUseProgram(spriteProgramHolder.program);
    checkGlError("glUseProgram");
    createGeometry(spriteProgramHolder, floorVertices, floorTextureId);

    setBaseModelMatrix();

    Matrix.rotateM(modelMatrix, 0, 90, 1, 0, 0);
    Matrix.translateM(modelMatrix, 0, 0, 0, CAMERA_DISTANCE + 1.0f);

    GLES20.glUniform4f(spriteProgramHolder.uBrightness, 1, 1, 1, 1);

    Matrix.multiplyMM(viewMatrix, 0, eye.getEyeView(), 0, lookAtMatrix, 0);
    Matrix.multiplyMM(tmpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
    Matrix.multiplyMM(mvpMatrix, 0, eye.getPerspective(Z_NEAR, Z_FAR), 0, tmpMatrix, 0);

    GLES20.glUniformMatrix4fv(spriteProgramHolder.uMvpMatrix, 1, false, mvpMatrix, 0);
    GLES20.glUniformMatrix4fv(spriteProgramHolder.uStMatrix, 1, false, spriteStMatrix, 0);

    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");
  }

  private void drawSeekBar(Eye eye) {
    GLES20.glUseProgram(spriteProgramHolder.program);
    checkGlError("glUseProgram");
    createGeometry(spriteProgramHolder, seekBarVertices, seekBarTextureId);

    setBaseModelMatrix();
    Matrix.translateM(modelMatrix, 0, 0, 0, CAMERA_DISTANCE);
    Matrix.translateM(modelMatrix, 0, HALF_VIDEO_WIDTH, 0, 0);
    Matrix.scaleM(modelMatrix, 0, seekBarRatio, 1f, 1f);

    Matrix.multiplyMM(viewMatrix, 0, eye.getEyeView(), 0, lookAtMatrix, 0);
    Matrix.multiplyMM(mvpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
    Matrix.multiplyMM(mvpMatrix, 0, eye.getPerspective(Z_NEAR, Z_FAR), 0, mvpMatrix, 0);

    GLES20.glUniformMatrix4fv(spriteProgramHolder.uMvpMatrix, 1, false, mvpMatrix, 0);
    GLES20.glUniformMatrix4fv(spriteProgramHolder.uStMatrix, 1, false, spriteStMatrix, 0);
    GLES20.glUniform4f(spriteProgramHolder.uBrightness, 1, 1, 1, 1);

    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");
  }

  /** Draws the glowing border around the video. */
  private void drawVideoGlow(Eye eye) {
    GLES20.glUseProgram(spriteProgramHolder.program);
    checkGlError("glUseProgram");
    createGeometry(spriteProgramHolder, videoVertices, videoGlowTextureId);

    setBaseModelMatrix();
    Matrix.translateM(modelMatrix, 0, 0, 0, CAMERA_DISTANCE);
    float glowTextureScale = 0.2f;
    Matrix.scaleM(modelMatrix, 0, 1f + glowTextureScale, 1f + glowTextureScale * (16f / 9f), 1f);
    Matrix.multiplyMM(viewMatrix, 0, eye.getEyeView(), 0, lookAtMatrix, 0);
    Matrix.multiplyMM(tmpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
    Matrix.multiplyMM(mvpMatrix, 0, eye.getPerspective(Z_NEAR, Z_FAR), 0, tmpMatrix, 0);

    GLES20.glUniformMatrix4fv(spriteProgramHolder.uMvpMatrix, 1, false, mvpMatrix, 0);
    GLES20.glUniformMatrix4fv(spriteProgramHolder.uStMatrix, 1, false, spriteStMatrix, 0);

    GLES20.glUniform4f(
        spriteProgramHolder.uBrightness, glowValues[0], glowValues[1], glowValues[2], 1f);

    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");
  }

  private void createGeometry(ProgramHolder programHolder, FloatBuffer vertices, int textureId) {
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    int glTextureType =
        textureId == videoTextureId ? GL_TEXTURE_EXTERNAL_OES : GLES20.GL_TEXTURE_2D;
    GLES20.glBindTexture(glTextureType, textureId);

    vertices.position(TRIANGLE_VERTICES_DATA_POS_OFFSET);
    GLES20.glVertexAttribPointer(
        programHolder.aPosition,
        3,
        GLES20.GL_FLOAT,
        false,
        TRIANGLE_VERTICES_DATA_STRIDE_BYTES,
        vertices);
    checkGlError("glVertexAttribPointer aPosition");
    GLES20.glEnableVertexAttribArray(programHolder.aPosition);
    checkGlError("glEnableVertexAttribArray aPosition handle");

    vertices.position(TRIANGLE_VERTICES_DATA_UV_OFFSET);
    GLES20.glVertexAttribPointer(
        programHolder.aTexture,
        3,
        GLES20.GL_FLOAT,
        false,
        TRIANGLE_VERTICES_DATA_STRIDE_BYTES,
        vertices);
    checkGlError("glVertexAttribPointer aTexture handle");
    GLES20.glEnableVertexAttribArray(programHolder.aTexture);
    checkGlError("glEnableVertexAttribArray aTexture handle");
  }

  private void createFsqGeometry(
      FsqProgramHolder programHolder, FloatBuffer vertices, int textureId) {
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    int glTextureType =
        textureId == videoTextureId ? GL_TEXTURE_EXTERNAL_OES : GLES20.GL_TEXTURE_2D;
    GLES20.glBindTexture(glTextureType, textureId);

    vertices.position(TRIANGLE_VERTICES_DATA_POS_OFFSET);
    GLES20.glVertexAttribPointer(
        programHolder.aPosition,
        3,
        GLES20.GL_FLOAT,
        false,
        TRIANGLE_VERTICES_DATA_STRIDE_BYTES,
        vertices);
    checkGlError("glVertexAttribPointer aPosition");
    GLES20.glEnableVertexAttribArray(programHolder.aPosition);
    checkGlError("glEnableVertexAttribArray aPosition handle");
  }

  private void getAverageFrameColor() {
    if (isPlaybackStarting) {
      return;
    }

    // Save the current framebuffer so we can restore it at the end of this method
    // (necessary for this code to work with distortion correction enabled).
    int[] params = new int[1];
    GLES20.glGetIntegerv(GLES20.GL_FRAMEBUFFER_BINDING, params, 0);
    int currentFrameBuffer = params[0];

    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer);
    GLES20.glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);
    GLES20.glScissor(0, 0, FBO_WIDTH, FBO_HEIGHT);

    checkGlError("glBindFramebuffer");
    GLES20.glClearColor(0f, 0f, 0f, 1f);
    GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

    GLES20.glUseProgram(fsqProgramHolder.program);
    checkGlError("glUseProgram");
    createFsqGeometry(fsqProgramHolder, lightboxVertices, videoTextureId);
    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");

    framePixels.clear();
    GLES20.glReadPixels(
        0, 0, FBO_WIDTH, FBO_HEIGHT, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, framePixels);
    checkGlError("glReadPixels");

    setGlowValues(0, 0, 0);
    while (framePixels.hasRemaining()) {
      glowValues[0] += framePixels.get() & 0xFF;
      glowValues[1] += framePixels.get() & 0xFF;
      glowValues[2] += framePixels.get() & 0xFF;
      framePixels.get(); // Read the alpha to advance the buffer.
    }

    for (int i = 0; i < 3; i++) {
      // Clamp the value so there is always some glow.
      glowValues[i] =
          max(
              MIN_VIDEO_GLOW_BRIGHTNESS,
              glowValues[i] / 255f / (FBO_WIDTH * FBO_HEIGHT * 3.0f) * 2.0f);
    }

    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, currentFrameBuffer);
  }

  @Override
  public void onSurfaceChanged(int width, int height) {
    // Nothing to do. Handled by GvrView.
  }

  @Override
  public void onSurfaceCreated(EGLConfig config) {
    initialTime = SystemClock.uptimeMillis();
    videoProgramHolder = createProgramHolder(VIDEO_FRAGMENT_SHADER, false);
    spriteProgramHolder = createProgramHolder(FRAGMENT_SHADER, true);
    fsqProgramHolder = createFsqProgramHolder(FSQ_VERTEX_SHADER, FSQ_FRAGMENT_SHADER);

    Matrix.setLookAtM(
        lookAtMatrix, 0, 0, CAMERA_HEIGHT, 0, 0, CAMERA_HEIGHT, CAMERA_DISTANCE, 0, 1, 0);
    Matrix.setIdentityM(videoStMatrix, 0);
    Matrix.setIdentityM(spriteStMatrix, 0);

    textureIds = new int[NUM_SPRITE_TEXTURES];
    GLES20.glGenTextures(NUM_SPRITE_TEXTURES, textureIds, 0);

    // Create the texture used for transitions between videos.
    transitionBackgroundTextureId = textureIds[0];
    createResourceTexture(transitionBackgroundTextureId, R.raw.transition_bg);

    // Create the texture used when there is no video.
    noVideoBackgroundTextureId = textureIds[1];
    createResourceTexture(noVideoBackgroundTextureId, R.raw.novideos_bg);

    // Create the texture used by the video background logo.
    videoGlowTextureId = textureIds[2];
    createResourceTexture(videoGlowTextureId, R.raw.glow_bg);

    // Create the texture used by the video background logo.
    nowPlayingTextureId = textureIds[3];
    createResourceTexture(nowPlayingTextureId, R.raw.playing);

    // Create the texture used by the seekbar.
    seekBarTextureId = textureIds[4];
    createResourceTexture(seekBarTextureId, R.raw.seekbar);

    // Create the texture used for video playback.
    videoTextureId = textureIds[5];
    GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, videoTextureId);

    // Create the texture used for floor.
    floorTextureId = textureIds[6];
    createResourceTexture(floorTextureId, R.raw.floor);

    checkGlError("glBindTexture mTextureID");

    GLES20.glTexParameterf(
        GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
    GLES20.glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

    surfaceTexture = new SurfaceTexture(videoTextureId);
    surfaceTexture.setOnFrameAvailableListener(this);

    surfaceTextureListener.onSurfaceTextureCreated(surfaceTexture);

    synchronized (this) {
      surfaceNeedsUpdate = false;
    }

    fboTargetTextureId = createFboTargetTexture(FBO_WIDTH, FBO_HEIGHT);
    framebuffer = createFrameBuffer(fboTargetTextureId);
    if (framebuffer != -1) {
      framePixels = ByteBuffer.allocateDirect(FBO_WIDTH * FBO_HEIGHT * 4);
      framePixels.order(ByteOrder.nativeOrder());
      enableFramebuffer = true;
    }

    // Update the thumbnails that have been loaded so far.
    thumbnailTextureIds = new int[NUM_THUMBNAIL_TEXTURES];
    GLES20.glGenTextures(NUM_THUMBNAIL_TEXTURES, thumbnailTextureIds, 0);
    for (int i = 0; i < NUM_THUMBNAIL_TEXTURES; i++) {
      if (thumbnailBitmaps[i] != null) {
        loadThumbnailTexture(i);
      }
    }
  }

  private int createFboTargetTexture(int width, int height) {
    fboTargetTextureIds = new int[1];
    GLES20.glGenTextures(1, fboTargetTextureIds, 0);
    int texture = fboTargetTextureIds[0];
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
    GLES20.glTexImage2D(
        GLES20.GL_TEXTURE_2D,
        0,
        GLES20.GL_RGBA,
        width,
        height,
        0,
        GLES20.GL_RGBA,
        GLES20.GL_UNSIGNED_BYTE,
        null);

    GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
    GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
    return texture;
  }

  private int createFrameBuffer(int fboTargetTextureId) {
    // Save the current framebuffer so we can restore it at the end of this method
    // (necessary for this code to work with distortion correction enabled).
    int[] params = new int[1];
    GLES20.glGetIntegerv(GLES20.GL_FRAMEBUFFER_BINDING, params, 0);
    int currentFrameBuffer = params[0];

    int[] framebuffers = new int[1];
    GLES20.glGenFramebuffers(1, framebuffers, 0);
    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffers[0]);

    int[] renderbuffers = new int[1];
    GLES20.glGenRenderbuffers(1, renderbuffers, 0);

    GLES20.glFramebufferTexture2D(
        GLES20.GL_FRAMEBUFFER,
        GLES20.GL_COLOR_ATTACHMENT0,
        GLES20.GL_TEXTURE_2D,
        fboTargetTextureId,
        0);
    int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);

    if (status != GLES20.GL_FRAMEBUFFER_COMPLETE) {
      logger.atSevere().log("Framebuffer is not complete: %x", status);
      return -1;
    }

    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, currentFrameBuffer);
    return framebuffers[0];
  }

  private void loadThumbnailTexture(int index) {
    if (thumbnailTextureIds == null) {
      // The thumbnail textures will get loaded later once the thumbnailTextureIds are generated.
      return;
    }

    int textureId = thumbnailTextureIds[index];
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);

    GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
    GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);

    GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, thumbnailBitmaps[index], 0);
  }

  private void createResourceTexture(int textureId, int resourceId) {
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);

    GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
    GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);

    InputStream is = context.getResources().openRawResource(resourceId);
    Bitmap bitmap;
    try {
      bitmap = BitmapFactory.decodeStream(is);
    } finally {
      try {
        is.close();
      } catch (IOException e) {
        // Ignore.
      }
    }

    GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
    bitmap.recycle();
  }

  @Override
  public synchronized void onFrameAvailable(SurfaceTexture surface) {
    // We received a frame, so it means we have at least one valid video.
    setNoVideos(false);
    surfaceNeedsUpdate = true;
  }

  /** Creates a GL program and the associated handlers from a given fragment shader. */
  private ProgramHolder createProgramHolder(String fragmentShader, boolean supportsBrightness) {
    ProgramHolder holder = new ProgramHolder();
    holder.program = createProgram(VERTEX_SHADER, fragmentShader);
    if (holder.program == 0) {
      throw new VerifyException("Could not create program");
    }

    holder.aPosition = GLES20.glGetAttribLocation(holder.program, "aPosition");
    checkGlError("glGetAttribLocation aPosition");
    if (holder.aPosition == -1) {
      throw new VerifyException("Could not get attrib location for aPosition");
    }
    holder.aTexture = GLES20.glGetAttribLocation(holder.program, "aTextureCoord");
    checkGlError("glGetAttribLocation aTextureCoord");
    if (holder.aTexture == -1) {
      throw new VerifyException("Could not get attrib location for aTextureCoord");
    }

    holder.uMvpMatrix = GLES20.glGetUniformLocation(holder.program, "uMvpMatrix");
    checkGlError("glGetUniformLocation uMvpMatrix");
    if (holder.uMvpMatrix == -1) {
      throw new VerifyException("Could not get attrib location for uMvpMatrix");
    }

    holder.uStMatrix = GLES20.glGetUniformLocation(holder.program, "uStMatrix");
    checkGlError("glGetUniformLocation uStMatrix");
    if (holder.uStMatrix == -1) {
      throw new VerifyException("Could not get attrib location for uStMatrix");
    }

    if (supportsBrightness) {
      holder.uBrightness = GLES20.glGetUniformLocation(holder.program, "uBrightness");
      checkGlError("glGetUniformLocation uBrightness");
      if (holder.uBrightness == -1) {
        throw new VerifyException("Could not get attrib location for uBrightness");
      }
    }

    return holder;
  }

  private FsqProgramHolder createFsqProgramHolder(String vertexShader, String fragmentShader) {
    FsqProgramHolder holder = new FsqProgramHolder();
    holder.program = createProgram(vertexShader, fragmentShader);
    if (holder.program == 0) {
      throw new VerifyException("Could not create program");
    }

    holder.aPosition = GLES20.glGetAttribLocation(holder.program, "aPosition");
    checkGlError("glGetAttribLocation aPosition");
    if (holder.aPosition == -1) {
      throw new VerifyException("Could not get attrib location for aPosition");
    }
    return holder;
  }

  private int loadShader(int shaderType, String source) {
    int shader = GLES20.glCreateShader(shaderType);
    if (shader != 0) {
      GLES20.glShaderSource(shader, source);
      GLES20.glCompileShader(shader);
      int[] compiled = new int[1];
      GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
      if (compiled[0] == 0) {
        logger.atSevere().log(
            "Could not compile shader %s  %s", shaderType, GLES20.glGetShaderInfoLog(shader));
        GLES20.glDeleteShader(shader);
        shader = 0;
      }
    }
    return shader;
  }

  private int createProgram(String vertexSource, String fragmentSource) {
    int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
      return 0;
    }
    int pixelShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
    if (pixelShader == 0) {
      return 0;
    }

    int program = GLES20.glCreateProgram();
    if (program != 0) {
      GLES20.glAttachShader(program, vertexShader);
      checkGlError("glAttachShader");
      GLES20.glAttachShader(program, pixelShader);
      checkGlError("glAttachShader");
      GLES20.glLinkProgram(program);
      int[] linkStatus = new int[1];
      GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
      if (linkStatus[0] != GLES20.GL_TRUE) {
        logger.atSevere().log("Could not link program: %s", GLES20.glGetProgramInfoLog(program));
        GLES20.glDeleteProgram(program);
        program = 0;
      }
    }
    return program;
  }

  private void checkGlError(String op) {
    int error;
    while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
      logger.atSevere().log("%s : glError: %s", op, error);
      throw new VerifyException(op + ": glError " + error);
    }
  }

  @Override
  public void onRendererShutdown() {
    GLES20.glDeleteTextures(NUM_SPRITE_TEXTURES, textureIds, 0);
    GLES20.glDeleteTextures(NUM_THUMBNAIL_TEXTURES, thumbnailTextureIds, 0);
    GLES20.glDeleteTextures(1, fboTargetTextureIds, 0);

    GLES20.glDeleteProgram(videoProgramHolder.program);
    GLES20.glDeleteProgram(spriteProgramHolder.program);
    GLES20.glDeleteProgram(fsqProgramHolder.program);
  }
}
