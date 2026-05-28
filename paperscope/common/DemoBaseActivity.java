package com.google.vr.cardboard.paperscope.demo.common;

import com.google.vr.cardboard.paperscope.demo.R;

import android.content.Intent;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.view.OrientationEventListener;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.CardboardView.Renderer;
import com.google.common.flogger.FluentLogger;
import com.google.vr.cardboard.paperscope.demo.settings.SettingsActivity;

/** Base activity class for the demos. */
public abstract class DemoBaseActivity extends AppCompatActivity {
  private static final FluentLogger logger = FluentLogger.forEnclosingClass();
  protected DemoBaseController<? extends Renderer> demoActivityController;
  private OrientationEventListener orientationEventListener;
  protected CardboardView cardboardView;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.demo_base_hud);
    cardboardView = findViewById(R.id.cardboard_view);

    demoActivityController = createController();
    initializeOrientationEventListener();

    demoActivityController.setOnCloseButtonListener(this::finish);
    demoActivityController.setOnSettingsButtonListener(this::showSettings);
    demoActivityController.enableOnTriggerEventListener();
  }

  @Override
  protected void onResume() {
    super.onResume();
    demoActivityController.onResume();

    if (orientationEventListener != null) {
      orientationEventListener.enable();
    }
  }

  @Override
  protected void onPause() {
    super.onPause();
    demoActivityController.onPause();

    if (orientationEventListener != null) {
      orientationEventListener.disable();
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    demoActivityController.shutdown();
    orientationEventListener = null;
  }

  protected abstract DemoBaseController<? extends Renderer> createController();

  private void initializeOrientationEventListener() {
    orientationEventListener =
        new OrientationEventListener(this) {
          @Override
          public void onOrientationChanged(int orientation) {
            if (demoActivityController != null
                && demoActivityController.shouldFinishActivity(orientation)) {
              logger.atInfo().log("Finishing activity due to orientation change.");
              finish();
            }
          }
        };
  }

  private void showSettings() {
    Intent intent = new Intent(this, SettingsActivity.class);
    startActivity(intent);
  }
}
