package com.google.vr.cardboard.paperscope.demo;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import androidx.palette.graphics.Palette;

/**
 * This class holds the information necessary for constructing the supported apps list in the {@link
 * GridView}.
 */
public class BasicAppInfo {
  private static final int ICON_SIZE_PIXELS = 100;
  private String packageName;
  private String appName;
  private Intent startIntent;
  private Drawable icon;
  private Palette palette;

  public BasicAppInfo(String packageName, String appName, Intent startIntent, Drawable icon) {
    this.packageName = packageName;
    this.appName = appName;
    this.startIntent = startIntent;
    this.icon = icon;
    this.palette =
        Palette.from(convertToBitmap(icon, ICON_SIZE_PIXELS, ICON_SIZE_PIXELS)).generate();
  }

  private Bitmap convertToBitmap(Drawable drawable, int widthPixels, int heightPixels) {
    Bitmap mutableBitmap = Bitmap.createBitmap(widthPixels, heightPixels, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(mutableBitmap);
    drawable.setBounds(0, 0, widthPixels, heightPixels);
    drawable.draw(canvas);
    return mutableBitmap;
  }

  public String getPackageName() {
    return packageName;
  }

  public String getAppName() {
    return appName;
  }

  public Intent getStartIntent() {
    return startIntent;
  }

  public Drawable getIcon() {
    return icon;
  }

  public Palette getPalette() {
    return palette;
  }
}
