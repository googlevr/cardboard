package com.google.vr.cardboard.paperscope.demo;

import com.google.vr.cardboard.paperscope.demo.R;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.PopupMenu;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import com.google.vr.cardboard.paperscope.demo.settings.SettingsActivity;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * A Google Cardboard demo application.
 *
 * <p>This is the main Activity for the demo application. It initializes a GLSurfaceView to allow
 * rendering.
 */
public class CardboardLauncherActivity extends AppCompatActivity
    implements PopupMenu.OnMenuItemClickListener {
  static {
    System.loadLibrary("cardboard_jni");
  }

  private static final String TAG = CardboardLauncherActivity.class.getSimpleName();

  // Opaque native pointer to the native CardboardApp instance.
  // This object is owned by the CardboardLauncherActivity instance
  // and passed to the native methods.
  private long nativeApp;
  private TransitionView transitionView;
  private TransitionView.Listener transitionViewListener;
  private GLSurfaceView glView;

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void onCreate(Bundle savedInstance) {
    super.onCreate(savedInstance);

    nativeApp = nativeOnCreate(getAssets());

    setContentView(R.layout.activity_vr);
    glView = findViewById(R.id.surface_view);
    glView.setEGLContextClientVersion(2);
    Renderer renderer = new Renderer();
    glView.setRenderer(renderer);
    glView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    glView.setOnTouchListener(
        (v, event) -> {
          if (nativeShouldLaunchDemo(nativeApp)) {
            launchDemo(nativeDemoToLaunch(nativeApp));
          }
          if (event.getAction() == MotionEvent.ACTION_DOWN) {
            // Signal a trigger event.
            glView.queueEvent(
                () -> {
                  nativeOnTriggerEvent(nativeApp);
                });
            return true;
          }
          return false;
        });

    // TODO(b/139010241): Avoid that action and status bar are displayed when pressing settings
    // button.
    setImmersiveSticky();

    // Forces screen to max brightness.
    WindowManager.LayoutParams layout = getWindow().getAttributes();
    layout.screenBrightness = 1.f;
    getWindow().setAttributes(layout);

    // Prevents screen from dimming/locking.
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    // Buttons listeners for close and settings.
    ImageButton closeButton = findViewById(R.id.ui_back_button);
    closeButton.setOnClickListener(
        (view) -> {
          Log.d(TAG, "Close button clicked");
          closeSample();
        });
    ImageButton settingsButton = findViewById(R.id.ui_settings_button);
    settingsButton.setOnClickListener(
        (view) -> {
          Log.d(TAG, "Settings button clicked");
          showSettings(view);
        });

    // Create TransitionView Listener to pass to the Transition View
    transitionViewListener =
        new TransitionView.Listener() {
          @Override
          public void onSwitchButtonClicked() {
            Intent intent = new Intent(CardboardLauncherActivity.this, SettingsActivity.class);
            startActivity(intent);
          }

          @Override
          public void onExitButtonClicked() {
            closeSample();
          }
        };

    // Create the full-screen custom view
    transitionView = new TransitionView(this, transitionViewListener);

    // Set layout params to fill the screen
    FrameLayout.LayoutParams params =
        new FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

    // Add the TransitionView custom view to the activity
    addContentView(transitionView, params);
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

    glView.onResume();
    nativeOnResume(nativeApp);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    nativeOnDestroy(nativeApp);
    nativeApp = 0;
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
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

  /** Callback for when close button is pressed. */
  public void closeSample() {
    Log.d(TAG, "Leaving VR sample");
    finish();
  }

  /** Callback for when settings_menu button is pressed. */
  public void showSettings(View view) {
    PopupMenu popup = new PopupMenu(this, view);
    MenuInflater inflater = popup.getMenuInflater();
    inflater.inflate(R.menu.settings_menu, popup.getMenu());
    popup.setOnMenuItemClickListener(this);
    popup.show();
  }

  @Override
  public boolean onMenuItemClick(MenuItem item) {
    if (item.getItemId() == R.id.switch_viewer) {
      switchViewer();
      return true;
    }
    return false;
  }

  private void setImmersiveSticky() {
    WindowInsetsControllerCompat controller =
        new WindowInsetsControllerCompat(getWindow(), getWindow().getDecorView());
    controller.hide(WindowInsetsCompat.Type.systemBars());
    controller.setSystemBarsBehavior(
        WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
  }

  private void launchDemo(String demoName) {
    String className = "";
    if (demoName.equals("exhibit")) {
      className = "com.google.vr.cardboard.paperscope.demo.exhibit.ExhibitActivity";
    } else if (demoName.equals("mymovies")) {
      className = "com.google.vr.cardboard.paperscope.demo.myvideos.MyVideosActivity";
    } else if (demoName.equals("photospheres")) {
      className = "com.google.vr.cardboard.paperscope.demo.photosphere.PhotoSphereActivity";
    }
    try {
      Class<?> activityClass = Class.forName(className);
      Intent intent = new Intent(this, activityClass);
      startActivity(intent);
      nativeDemoLaunched(nativeApp);
    } catch (ClassNotFoundException e) {
      e.printStackTrace();
    }
  }

  public void switchViewer() {
    nativeSwitchViewer(nativeApp);
  }

  private native long nativeOnCreate(AssetManager assetManager);

  private native void nativeOnDestroy(long nativeApp);

  private native void nativeOnSurfaceCreated(long nativeApp);

  private native void nativeOnDrawFrame(long nativeApp);

  private native void nativeOnTriggerEvent(long nativeApp);

  private native void nativeOnPause(long nativeApp);

  private native void nativeOnResume(long nativeApp);

  private native void nativeSetScreenParams(long nativeApp, int width, int height);

  private native void nativeSwitchViewer(long nativeApp);

  private native boolean nativeShouldLaunchDemo(long nativeApp);

  private native String nativeDemoToLaunch(long nativeApp);

  private native void nativeDemoLaunched(long nativeApp);
}
