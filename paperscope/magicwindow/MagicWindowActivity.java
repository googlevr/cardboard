package com.google.vr.cardboard.paperscope.demo.magicwindow;

import com.google.vr.cardboard.paperscope.demo.R;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.TextView;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This is the main Activity for the demo application. It shows a monoscopic view of a VR world that
 * is controlled by the user's head tracking.
 */
public class MagicWindowActivity extends AppCompatActivity {
  static {
    System.loadLibrary("cardboard_jni");
  }

  // Random request code for photos and videos.
  private static final int PHOTOS_AND_VIDEOS_REQUEST_CODE = 234;
  private static final String TAG = MagicWindowActivity.class.getSimpleName();

  private static final Uri CARDBOARD_MANUFACTURERS_URL =
      Uri.parse("https://developers.google.com/cardboard/manufacturers");

  private long nativeApp;
  private GLSurfaceView glView;
  private Snackbar permissionRationalSnackbar;
  private boolean alreadyAskedStoragePermission = false;

  @Override
  public void onCreate(Bundle savedInstance) {
    super.onCreate(savedInstance);

    nativeApp = nativeOnCreate(getAssets());

    setContentView(R.layout.magic_window);

    View container = findViewById(R.id.coordinator);
    ViewCompat.setOnApplyWindowInsetsListener(
        container,
        (v, windowInsets) -> {
          Insets insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars());

          MarginLayoutParams mlp = (MarginLayoutParams) v.getLayoutParams();
          mlp.leftMargin = insets.left;
          mlp.bottomMargin = insets.bottom;
          mlp.rightMargin = insets.right;
          v.setLayoutParams(mlp);

          return WindowInsetsCompat.CONSUMED;
        });

    glView = findViewById(R.id.surface_view);
    glView.setEGLContextClientVersion(2);
    Renderer renderer = new Renderer();
    glView.setRenderer(renderer);
    glView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

    TextView getOneTv = findViewById(R.id.get_cardboard);
    getOneTv.setOnClickListener(
        new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            Intent intent = new Intent(Intent.ACTION_VIEW, CARDBOARD_MANUFACTURERS_URL);
            startActivity(intent);
          }
        });

    FloatingActionButton nextFab = findViewById(R.id.next);
    nextFab.setOnClickListener(
        new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            if (nativeHasSavedDeviceParams(nativeApp)) {
              finish();
            } else {
              nativeScanViewer(nativeApp);
            }
          }
        });
  }

  @Override
  protected void onPause() {
    super.onPause();

    nativeOnPause(nativeApp);
    glView.onPause();
  }

  @Override
  protected void onResume() {
    super.onResume();

    if (nativeHasSavedDeviceParams(nativeApp)) {
      finish();
    }

    if (!hasMediaPermissions()) {
      requestMediaPermissions();
    }

    glView.onResume();
    nativeOnResume(nativeApp);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    nativeOnDestroy(nativeApp);
    nativeApp = 0;
  }

  /**
   * Gets whether UI should be shown with rationale before requesting a permission.
   *
   * @param permissions Permissions to request.
   * @return True if UI with rationale should be shown before requesting a permission. Otherwise,
   *     return false.
   */
  private boolean shouldShowRequestPermissionRationale(List<String> permissions) {
    for (String permission : permissions) {
      if (ActivityCompat.shouldShowRequestPermissionRationale(this, permission)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Gets the permissions required to access images and videos from the external storage.
   *
   * <p>Based on the API level, the permission to be requested may be different. When the API level
   * is below Android T READ_EXTERNAL_STORAGE have to be requested. From Android T on, due to
   * granular media permissions, READ_MEDIA_IMAGES and READ_MEDIA_VIDEO have to be requested.
   *
   * @return string array with permissions to be requested.
   */
  private List<String> getStoragePermissionsToRequest() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      return Collections.unmodifiableList(
          Arrays.asList(
              Manifest.permission.READ_MEDIA_IMAGES, Manifest.permission.READ_MEDIA_VIDEO));
    }

    return Collections.unmodifiableList(Arrays.asList(Manifest.permission.READ_EXTERNAL_STORAGE));
  }

  private void requestMediaPermissions() {
    List<String> permissions = getStoragePermissionsToRequest();
    if (shouldShowRequestPermissionRationale(permissions)) {
      if (permissionRationalSnackbar == null) {
        final View rootView = ((ViewGroup) findViewById(android.R.id.content)).getChildAt(0);

        permissionRationalSnackbar =
            Snackbar.make(
                    rootView, R.string.read_media_permission_rationale, Snackbar.LENGTH_INDEFINITE)
                .setAction(
                    android.R.string.ok,
                    new View.OnClickListener() {
                      @Override
                      public void onClick(View view) {
                        ActivityCompat.requestPermissions(
                            MagicWindowActivity.this,
                            permissions.toArray(new String[0]),
                            PHOTOS_AND_VIDEOS_REQUEST_CODE);
                        // After click, the current Snackbar will be automatically removed from the
                        // SnackbarManager so next time calling show() on it won't show it. As a
                        // result,
                        // we explicitly set it to be null so next time we will first create a new
                        // Snackbar then call show() on the new Snackbar.
                        permissionRationalSnackbar = null;
                      }
                    });
      }

      // Only call show() if the current Snackbar is not shown to avoid the animation.
      if (!permissionRationalSnackbar.isShown()) {
        permissionRationalSnackbar.show();
      }
    } else if (!alreadyAskedStoragePermission) {
      alreadyAskedStoragePermission = true;
      // Storage permission has not been granted yet. Request it directly.
      ActivityCompat.requestPermissions(
          this, permissions.toArray(new String[0]), PHOTOS_AND_VIDEOS_REQUEST_CODE);
    }
  }

  private boolean hasMediaPermissions() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      return ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_IMAGES)
              == PackageManager.PERMISSION_GRANTED
          && ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_VIDEO)
              == PackageManager.PERMISSION_GRANTED;
    } else {
      return ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
          == PackageManager.PERMISSION_GRANTED;
    }
  }

  private class Renderer implements GLSurfaceView.Renderer {
    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
      nativeOnSurfaceCreated(nativeApp);
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int width, int height) {
      nativeSetScreenParams(nativeApp, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
      nativeOnDrawFrame(nativeApp);
    }
  }

  private native long nativeOnCreate(AssetManager assetManager);

  private native void nativeOnDestroy(long nativeApp);

  private native void nativeOnResume(long nativeApp);

  private native void nativeOnPause(long nativeApp);

  private native void nativeOnSurfaceCreated(long nativeApp);

  private native void nativeSetScreenParams(long nativeApp, int width, int height);

  private native void nativeOnDrawFrame(long nativeApp);

  private native void nativeScanViewer(long nativeApp);

  private native boolean nativeHasSavedDeviceParams(long nativeApp);
}
