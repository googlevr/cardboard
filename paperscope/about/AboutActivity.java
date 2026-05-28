package com.google.vr.cardboard.paperscope.demo.about;

import com.google.vr.cardboard.paperscope.demo.R;

import static com.google.common.base.Preconditions.checkNotNull;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.widget.Toast;
import com.google.common.flogger.FluentLogger;

/** Activity for the About page. */
public class AboutActivity extends AppCompatActivity {

  private static final FluentLogger logger = FluentLogger.forEnclosingClass();

  private static final String MARKET_DETAIL = "market://details?id=";

  private static final String PLAYSTORE_LISTING = "http://play.google.com/store/apps/details?id=";

  private static final String PROD_PACKAGE_NAME = "com.google.samples.apps.cardboarddemo";

  private static final Uri GOOGLE_TOS_URL = Uri.parse("http://www.google.com/accounts/TOS");

  private static final Uri CARDBOARD_SAFETY_INFORMATION_URL =
      Uri.parse("http://www.google.com/get/cardboard/product-safety/");

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.about);

    ActionBar actionBar = getSupportActionBar();
    if (actionBar != null) {
      actionBar.setDisplayHomeAsUpEnabled(true);
    }
    // Rate the app.
    checkNotNull((View) findViewById(R.id.rate_the_app))
        .setOnClickListener(
            new OnClickListener() {
              @Override
              public void onClick(View v) {
                // First try to go directly to Play Store page using a playstore specific intent.
                // We don't use getPackageName() here since the dev package name is not present
                // in Play Store.
                Uri uri = Uri.parse(MARKET_DETAIL + PROD_PACKAGE_NAME);
                Intent goToPlayStore = new Intent(Intent.ACTION_VIEW, uri);
                try {
                  startActivity(goToPlayStore);
                } catch (ActivityNotFoundException e) {
                  // If that fails, fall back to a broader view webpage intent.
                  openWebPageOrShowErrorToast(
                      AboutActivity.this, Uri.parse(PLAYSTORE_LISTING + PROD_PACKAGE_NAME));
                }
              }
            });

    // Terms of service.
    checkNotNull((View) findViewById(R.id.terms_of_service))
        .setOnClickListener(
            new OnClickListener() {
              @Override
              public void onClick(View v) {
                openWebPageOrShowErrorToast(AboutActivity.this, GOOGLE_TOS_URL);
              }
            });

    // Safety.
    checkNotNull((View) findViewById(R.id.product_safety))
        .setOnClickListener(
            new OnClickListener() {
              @Override
              public void onClick(View v) {
                openWebPageOrShowErrorToast(AboutActivity.this, CARDBOARD_SAFETY_INFORMATION_URL);
              }
            });

    // Set app version.
    TextView appVersionString = checkNotNull((TextView) findViewById(R.id.app_version));
    appVersionString.setText(getVersionName());
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    if (item.getItemId() == android.R.id.home) {
      // This is the "Up" button
      finish();
      return true; // Event is handled
    }
    return super.onOptionsItemSelected(item);
  }

  /** Opens up the given web page or shows a toast on error. */
  public void openWebPageOrShowErrorToast(Context context, Uri uri) {
    try {
      context.startActivity(new Intent(Intent.ACTION_VIEW, uri));
    } catch (ActivityNotFoundException e) {
      Toast.makeText(context, context.getString(R.string.view_web_page_failed), Toast.LENGTH_SHORT)
          .show();
    }
  }

  /**
   * Returns the version name of the application, or an empty string on error. For dogfood or debug
   * builds, this method truncates the version name to the "X.Y.Z" pattern, removing any additional
   * suffixes.
   */
  private String getVersionName() {
    String appVersion = "";
    String packageName = getPackageName();
    PackageManager pm = getPackageManager();
    try {
      PackageInfo info;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        info = pm.getPackageInfo(packageName, PackageManager.PackageInfoFlags.of(0));
      } else {
        info = pm.getPackageInfo(packageName, 0);
      }
      if (info != null) {
        appVersion = info.versionName;
      }
    } catch (PackageManager.NameNotFoundException e) {
      logger.atSevere().withCause(e).log("Failed to get package info for %s", packageName);
    }

    // Production version strings follow a traditional "X.Y.Z" pattern.
    // Dogfood / debug builds follow the following pattern:
    // "X.Y.Z.Dogfood long_string_such_as_release_branch_name".
    // In order to display something that doesn't overflow for these builds, we strip out everything
    // after the first whitespace, if any.
    // Note that the full version name can always be accessed from the Android "App info" page.
    int spaceIndex = appVersion.indexOf(" ");
    if (spaceIndex == -1) {
      return appVersion;
    }
    return appVersion.substring(0, spaceIndex);
  }
}
