package com.google.vr.cardboard.paperscope.demo;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.widget.BaseAdapter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * This class provides a list of supported apps to be displayed by the {@link MyLibraryFragment}
 * fragment. Apps can advertise cardboard support by adding the following category section to a
 * given intent filter in their AndroidManifest.xml: <queries> <intent> <action
 * android:name="android.intent.action.MAIN" /> <category
 * android:name="com.google.intent.category.CARDBOARD" /> </intent> </queries>
 */
final class SupportedAppProvider {
  private static SupportedAppProvider instance;
  private static final String LOG_TAG = SupportedAppProvider.class.getName();
  private static final String CARDBOARD_SUPPORT_CATEGORY_INTENT_TAG =
      "com.google.intent.category.CARDBOARD";
  private List<BasicAppInfo> installedApps;

  private SupportedAppProvider() {
    installedApps = new ArrayList<BasicAppInfo>();
  }

  public static SupportedAppProvider getInstance() {
    if (instance == null) {
      instance = new SupportedAppProvider();
    }
    return instance;
  }

  @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
  private BasicAppInfo buildCardboardDemoAppInfo(Context context) {
    Intent intent = new Intent(context, CardboardLauncherActivity.class);

    String packageName = context.getPackageName();
    BasicAppInfo appInfo =
        new BasicAppInfo(
            packageName,
            context.getResources().getString(R.string.bundled_demos_label),
            intent,
            context.getResources().getDrawable(R.drawable.cardboard_demo));

    return appInfo;
  }

  public List<BasicAppInfo> getInstalledApps() {
    return installedApps;
  }

  private void sortAppListByName(List<BasicAppInfo> apps) {
    Comparator<BasicAppInfo> comparator =
        new Comparator<BasicAppInfo>() {
          @Override
          public int compare(BasicAppInfo first, BasicAppInfo second) {
            return first.getAppName().compareTo(second.getAppName());
          }
        };
    Collections.sort(apps, comparator);
  }

  private List<ResolveInfo> getCardboardActivities(PackageManager packageManager) {
    Intent cardboardIntent = new Intent(Intent.ACTION_MAIN);
    cardboardIntent.addCategory(CARDBOARD_SUPPORT_CATEGORY_INTENT_TAG);
    return packageManager.queryIntentActivities(cardboardIntent, 0);
  }

  private BasicAppInfo createBasicAppInfoFromResolveInfoAndIntent(
      PackageManager pm, ResolveInfo resolveInfo, Intent intent) {
    final String packageName = resolveInfo.activityInfo.packageName;
    if (packageName == null) {
      Log.w(LOG_TAG, "packageName not set in activityInfo");
      return null;
    }

    CharSequence appName = resolveInfo.loadLabel(pm);
    if (appName == null) {
      Log.w(LOG_TAG, "application label not found for " + packageName);
      return null;
    }

    Drawable icon = pm.getApplicationIcon(resolveInfo.activityInfo.applicationInfo);
    if (icon == null) {
      Log.w(LOG_TAG, "application icon not found for " + packageName);
      return null;
    }

    return new BasicAppInfo(packageName, appName.toString(), intent, icon);
  }

  private BasicAppInfo createBasicAppInfoFromResolveInfo(
      PackageManager pm, ResolveInfo resolveInfo) {
    // Create an intent that will launch the input activity.
    final ActivityInfo activityInfo = resolveInfo.activityInfo;
    Intent intent = new Intent();
    intent.setClassName(activityInfo.applicationInfo.packageName, activityInfo.name);

    return createBasicAppInfoFromResolveInfoAndIntent(pm, resolveInfo, intent);
  }

  private void appendInstalledAppsWithCardboardSupportIntent(
      PackageManager packageManager, Context context, Map<String, BasicAppInfo> supportedAppsMap) {
    List<ResolveInfo> cardboardApps = getCardboardActivities(packageManager);
    for (ResolveInfo resolveInfo : cardboardApps) {

      // Check that the package name is not already in the list, if it was most likely we already
      // added the intent for an activity in the same package that matched better. Therefore we are
      // skipping the next one.
      // This also avoid somebody abusing this "API" by declaring multiple activities from the same
      // app.
      if (supportedAppsMap.containsKey(resolveInfo.activityInfo.applicationInfo.packageName)) {
        continue;
      }

      // Check to make sure the activity is not in our own APK.
      if (resolveInfo.activityInfo.applicationInfo.packageName.equalsIgnoreCase(
          context.getPackageName())) {
        continue;
      }

      // Bundle the app name with its icon and start intent.
      BasicAppInfo basicAppInfo = createBasicAppInfoFromResolveInfo(packageManager, resolveInfo);

      // Add the app info if valid to the list of apps.
      if (basicAppInfo != null) {
        supportedAppsMap.put(basicAppInfo.getPackageName(), basicAppInfo);
      }
    }
  }

  private List<BasicAppInfo> getSupportedAppList(PackageManager packageManager, Context context) {
    // Map of package name and basicAppInfo that will be used to ensure uniqueness.
    Map<String, BasicAppInfo> supportedAppsMap = new LinkedHashMap<String, BasicAppInfo>();
    // Add apps that have cardboard intent.
    appendInstalledAppsWithCardboardSupportIntent(packageManager, context, supportedAppsMap);

    List<BasicAppInfo> supportedApps = new ArrayList<BasicAppInfo>(supportedAppsMap.values());
    sortAppListByName(supportedApps);
    return supportedApps;
  }

  public void populateSupportedAppsList(
      PackageManager packageManager, Context context, BaseAdapter adapter) {
    installedApps.clear();
    installedApps.add(buildCardboardDemoAppInfo(context));
    installedApps.addAll(getSupportedAppList(packageManager, context));

    adapter.notifyDataSetChanged();
  }
}
