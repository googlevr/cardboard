package com.google.vr.cardboard.paperscope.demo.settings;

import com.google.vr.cardboard.paperscope.demo.R;

import android.content.res.AssetManager;
import android.os.Bundle;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.Button;
import android.widget.TextView;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import com.google.cardboard.sdk.CardboardViewApi;
import com.google.common.base.Strings;

/** Activity for settings. */
public class SettingsActivity extends AppCompatActivity {
  static {
    System.loadLibrary("cardboard_jni");
  }

  private long nativeApp;
  private CardboardViewApi cardboardViewApi;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    nativeApp = nativeOnCreate(getAssets());

    setContentView(R.layout.settings);

    // From SDK 35+, Screen displays in Edge-to-edge mode, which means that the system bars are
    // drawn over the content. To make sure that the content is not covered by the system bars, we
    // need to add the system bars padding to the content.
    View container = findViewById(R.id.container);
    ViewCompat.setOnApplyWindowInsetsListener(
        container,
        (v, windowInsets) -> {
          Insets insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars());

          MarginLayoutParams mlp = (MarginLayoutParams) v.getLayoutParams();
          mlp.leftMargin = insets.left;
          mlp.bottomMargin = insets.bottom;
          mlp.topMargin = insets.top;
          mlp.rightMargin = insets.right;
          v.setLayoutParams(mlp);

          return WindowInsetsCompat.CONSUMED;
        });

    Button switchViewerBtn = findViewById(R.id.switch_headset);
    switchViewerBtn.setOnClickListener(
        new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            nativeSwitchViewer(nativeApp);
          }
        });

    ActionBar actionBar = getSupportActionBar();
    if (actionBar != null) {
      actionBar.setDisplayHomeAsUpEnabled(true);
    }

    cardboardViewApi = new CardboardViewApi(this);
  }

  @Override
  protected void onResume() {
    super.onResume();

    if (cardboardViewApi.hasSavedDeviceParams()) {
      updateViewerInfo();
    }
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    if (item.getItemId() == android.R.id.home) {
      // Home button clicked.
      finish();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  /** Updates the viewer info in the UI. */
  private void updateViewerInfo() {
    TextView viewerModel = findViewById(R.id.headset_model);
    viewerModel.setText(getViewerModel());
    TextView viewerVendor = findViewById(R.id.headset_vendor);
    viewerVendor.setText(getViewerVendor());
  }

  /** Returns the viewer model. */
  private String getViewerModel() {
    String viewerModel = cardboardViewApi.getSavedDeviceParams().getModel();
    if (Strings.isNullOrEmpty(viewerModel)) {
      return getString(R.string.unknown_headset_info);
    }
    return viewerModel;
  }

  /** Returns the viewer vendor. */
  private String getViewerVendor() {
    String viewerVendor = cardboardViewApi.getSavedDeviceParams().getVendor();
    if (Strings.isNullOrEmpty(viewerVendor)) {
      return getString(R.string.headset_vendor_prefix) + getString(R.string.unknown_headset_info);
    }
    return getString(R.string.headset_vendor_prefix) + viewerVendor;
  }

  private native long nativeOnCreate(AssetManager assetManager);

  private native void nativeSwitchViewer(long nativeApp);
}
