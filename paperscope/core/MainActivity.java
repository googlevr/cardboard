package com.google.vr.cardboard.paperscope.demo;

import android.content.Intent;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.Toast;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;
import com.google.android.material.appbar.AppBarLayout;
import com.google.android.material.appbar.AppBarLayout.Behavior;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;
import com.google.cardboard.sdk.CardboardViewApi;
import com.google.vr.cardboard.paperscope.demo.about.AboutActivity;
import com.google.vr.cardboard.paperscope.demo.magicwindow.MagicWindowActivity;
import org.jspecify.annotations.Nullable;

/**
 * This is the Activity where the user can see a list of Cardboard compatible apps to launch, along
 * with the provided Cardboard Demo.
 */
public class MainActivity extends AppCompatActivity {
  static {
    System.loadLibrary("cardboard_jni");
  }

  private long nativeApp;

  private static final String TAG = MainActivity.class.getSimpleName();

  /** Index of MyLibrary fragment in the tab. */
  private static final int TAB_INDEX_MY_LIBRARY = 0;

  /** Number of fragments in the tab. */
  private static final int NUM_FRAGMENTS = 1;

  /** The URL for the Google Privacy Policy. */
  private static final Uri GOOGLE_PRIVACY_URL =
      Uri.parse("http://www.google.com/policies/privacy/");

  /** The URL for the Google Cardboard Contact Us page. */
  private static final Uri GOOGLE_CONTACT_URL =
      Uri.parse(
          "https://support.google.com/cardboard?hl=en&sjid=7565740170206509904-NC#topic=6295044");

  private CoordinatorLayout coordinatorLayout;
  private CoordinatorLayout.LayoutParams layoutParams;
  private AppBarLayout appBarLayout;
  private AppBarLayout.Behavior behavior;
  private ViewPager2 pager;

  private CardboardViewApi cardboardViewApi;

  public static <T> T checkNotNull(@Nullable T reference) {
    if (reference == null) {
      throw new NullPointerException();
    }
    return reference;
  }

  @Override
  public void onCreate(Bundle savedInstance) {
    super.onCreate(savedInstance);

    nativeApp = nativeOnCreate(getAssets());

    setContentView(R.layout.my_library);

    Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
    setSupportActionBar(toolbar);

    coordinatorLayout = checkNotNull((CoordinatorLayout) findViewById(R.id.coordinator));
    appBarLayout = checkNotNull((AppBarLayout) findViewById(R.id.appbar));
    layoutParams = (CoordinatorLayout.LayoutParams) appBarLayout.getLayoutParams();
    layoutParams.setBehavior(new Behavior());
    behavior = (AppBarLayout.Behavior) checkNotNull(layoutParams.getBehavior());

    ViewCompat.setOnApplyWindowInsetsListener(
        coordinatorLayout,
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

    pager = checkNotNull((ViewPager2) findViewById(R.id.pager));
    pager.setAdapter(new TabsAdapter(this));
    pager.registerOnPageChangeCallback(
        new ViewPager2.OnPageChangeCallback() {
          @Override
          public void onPageSelected(int position) {
            // Reset toolbar's position when switching pages. This is done by simulating a quick
            // fling.
            behavior.onNestedFling(
                /* coordinatorLayout= */ coordinatorLayout,
                /* child= */ appBarLayout,
                /* target= */ null,
                /* velocityX= */ 0,
                /* velocityY= */ -1000,
                /* consumed= */ true);
          }
        });

    TabLayout tabLayout = checkNotNull((TabLayout) findViewById(R.id.tabs));
    new TabLayoutMediator(
            tabLayout,
            pager,
            (tab, position) -> {
              tab.setText(
                  switch (position) {
                    case TAB_INDEX_MY_LIBRARY -> getString(R.string.app_grid_view_name);
                    default ->
                        throw new IllegalArgumentException(
                            "Invalid fragment index. index=" + position);
                  });
            })
        .attach();

    cardboardViewApi = new CardboardViewApi(this);
  }

  @Override
  public void onResume() {
    super.onResume();

    if (!cardboardViewApi.hasSavedDeviceParams()) {
      Intent intent = new Intent(this, MagicWindowActivity.class);
      startActivity(intent);
    }
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.my_library_settings_menu, menu);
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    int id = item.getItemId();
    if (id == R.id.action_switch_viewer) {
      nativeSwitchViewer(nativeApp);
      return true;
    } else if (id == R.id.action_about) {
      startActivity(new Intent(this, AboutActivity.class));
      return true;
    } else if (id == R.id.action_privacy_policy) {
      try {
        startActivity(new Intent(Intent.ACTION_VIEW, GOOGLE_PRIVACY_URL));
      } catch (android.content.ActivityNotFoundException e) {
        Toast.makeText(this, R.string.view_web_page_failed, Toast.LENGTH_SHORT).show();
      }
      return true;
    } else if (id == R.id.contact_us) {
      try {
        startActivity(new Intent(Intent.ACTION_VIEW, GOOGLE_CONTACT_URL));
      } catch (android.content.ActivityNotFoundException e) {
        Toast.makeText(this, R.string.view_web_page_failed, Toast.LENGTH_SHORT).show();
      }
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  private native long nativeOnCreate(AssetManager assetManager);

  private native void nativeSwitchViewer(long nativeApp);

  /** The adapter for the two fragments. */
  private static class TabsAdapter extends FragmentStateAdapter {

    TabsAdapter(AppCompatActivity activity) {
      super(activity);
    }

    @Override
    public Fragment createFragment(int index) {
      return switch (index) {
        case TAB_INDEX_MY_LIBRARY -> new MyLibraryFragment();
        default -> throw new IllegalArgumentException("Invalid fragment index. index=" + index);
      };
    }

    @Override
    public int getItemCount() {
      return NUM_FRAGMENTS;
    }
  }
}
