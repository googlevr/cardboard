package com.google.vr.cardboard.paperscope.demo;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.palette.graphics.Palette;
import java.util.List;

/** Fragment where the list of Cardboard supported apps is generated and displayed to the user. */
public class MyLibraryFragment extends Fragment {
  private static final String LOG_TAG = MyLibraryFragment.class.getName();
  private static final int DEMO_ICON_PADDING_COLOR = 0xFFE0E0E0;
  private PackageManager packageManager;
  private GridView gridView;
  private AppAdapter adapter;
  private Activity activity;
  private SupportedAppProvider supportedAppProvider;

  @Override
  public View onCreateView(
      LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.my_library_fragment, container, false);
  }

  @Override
  public void onAttach(Context context) {
    super.onAttach(context);

    try {
      activity = (MainActivity) context;
    } catch (ClassCastException e) {
      ClassCastException exception =
          new ClassCastException(context.toString() + " must be an instance of MainActivity");
      exception.initCause(e);
      throw exception;
    }
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    packageManager = getContext().getPackageManager();
    supportedAppProvider = SupportedAppProvider.getInstance();
    adapter = new AppAdapter();
    gridView = (GridView) view.findViewById(R.id.gridview);
      gridView.setAdapter(adapter);
      supportedAppProvider.populateSupportedAppsList(packageManager, activity, adapter);
  }

  /** The adapter for the grid view that provides the view of each of the grid items. */
  private class AppAdapter extends BaseAdapter {

    @Override
    public int getCount() {
      return supportedAppProvider.getInstalledApps().size();
    }

    @Override
    @Nullable
    public Object getItem(int position) {
      return null;
    }

    @Override
    public long getItemId(int position) {
      return 0;
    }

    private void updateAppItemFromBasicAppInfo(View appItemView, BasicAppInfo appInfo) {
      TextView appNameView = (TextView) appItemView.findViewById(R.id.app_name);
      appNameView.setText(appInfo.getAppName());
      ImageView iconView = (ImageView) appItemView.findViewById(R.id.app_icon);
      iconView.setImageDrawable(appInfo.getIcon());

      appItemView.setOnClickListener(
          new OnClickListener() {
            @Override
            public void onClick(View view) {
              try {
                activity.startActivity(appInfo.getStartIntent());
              } catch (ActivityNotFoundException e) {
                Toast.makeText(activity, R.string.error_opening_app, Toast.LENGTH_SHORT).show();
                Log.e(LOG_TAG, "Unable to launch activity " + appInfo.getPackageName() + ": " + e);
              }
            }
          });
    }

    private int getIconPaddingColor(Palette palette, int defaultColor) {
      return palette.getLightMutedColor(
          palette.getMutedColor(
              palette.getLightVibrantColor(palette.getVibrantColor(defaultColor))));
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      if (convertView == null) {
        convertView = activity.getLayoutInflater().inflate(R.layout.app_grid_card, parent, false);
      }
      List<BasicAppInfo> installedApps = supportedAppProvider.getInstalledApps();
      updateAppItemFromBasicAppInfo(convertView, installedApps.get(position));
      ImageView iconView = (ImageView) convertView.findViewById(R.id.app_icon);
      int color =
          getIconPaddingColor(
              installedApps.get(position).getPalette(),
              ContextCompat.getColor(activity, R.color.transparent));

      // Special case the "Cardboard Demos" icon padding color
      if (installedApps.get(position).getPackageName().equals(activity.getPackageName())) {
        // Light grey color surrounding the "Cardboard Demos" icon. Spec is here:
        // https://folio.googleplex.com/ericamorse/Cardboard/November-release/spec#%2Fmy-library-spec.png
        color = DEMO_ICON_PADDING_COLOR;
      }

      iconView.setBackgroundColor(color);
      return convertView;
    }
  }
}
