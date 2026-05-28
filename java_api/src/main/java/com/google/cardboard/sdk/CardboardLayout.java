/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.cardboard.sdk;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageButton;

/**
 * This class holds a view which results from inflating {@code R.layout.ui_layer}. Provides an easy
 * way to set and control the Cardboard layout. This class also includes some methods to set the
 * visibility of the inner views (back and settings buttons, and alignment marker), making it
 * suitable for both monocular and stereo layouts. Setters for the buttons' callbacks are also
 * provided within this class.
 */
public class CardboardLayout {
  private final Handler handler;
  private final FrameLayout alignmentMarker;
  private final ImageButton backButton;
  private final ImageButton settingsButton;
  private final View view;

  /**
   * Creates a new Cardboard Layout object.
   *
   * <p>Inflates and adds the {@code R.layout.ui_layer}. It also loads all the inner views.
   *
   * @param[in] context Used to build a LayoutInflater and inflate the layout.
   * @param[in] parent A ViewGroup to get the LayoutParams from. It won't be attached to it.
   */
  public CardboardLayout(Context context, ViewGroup parent) {
    handler = new Handler(Looper.getMainLooper());

    LayoutInflater inflater =
        (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    view = inflater.inflate(R.layout.cardboard_ui, parent, false);

    alignmentMarker = (FrameLayout) view.findViewById(R.id.ui_alignment_marker);
    backButton = (ImageButton) view.findViewById(R.id.ui_back_button);
    settingsButton = (ImageButton) view.findViewById(R.id.ui_settings_button);
  }

  public View getView() {
    return view;
  }

  /** Shows all widgets from the layout. */
  public void showAll() {
    showAlignmentMarker();
    showBackButton();
    showSettingsButton();
  }

  /** Hides all widgets from the layout. */
  public void hideAll() {
    hideAlignmentMarker();
    hideBackButton();
    hideSettingsButton();
  }

  /** Shows alignment marker widget. */
  public void showAlignmentMarker() {
    handler.post(
        () -> {
          alignmentMarker.setVisibility(View.VISIBLE);
          alignmentMarker.bringToFront();
        });
  }

  /** Hides alignment marker widget. */
  public void hideAlignmentMarker() {
    handler.post(() -> alignmentMarker.setVisibility(View.INVISIBLE));
  }

  /** Shows back button (X) widget. */
  public void showBackButton() {
    handler.post(
        () -> {
          backButton.setVisibility(View.VISIBLE);
          backButton.bringToFront();
        });
  }

  /** Hides back button (X) widget. */
  public void hideBackButton() {
    handler.post(() -> backButton.setVisibility(View.INVISIBLE));
  }

  /** Shows settings button (gear) widget. */
  public void showSettingsButton() {
    handler.post(
        () -> {
          settingsButton.setVisibility(View.VISIBLE);
          settingsButton.bringToFront();
        });
  }

  /** Hides settings button (gear) widget. */
  public void hideSettingsButton() {
    handler.post(() -> settingsButton.setVisibility(View.INVISIBLE));
  }

  /**
   * Sets a {@code Runnable} as the back button event handler.
   *
   * @param[in] listener A {@code Runnable} to be executed when the back button is clicked.
   */
  public void setOnBackButtonClick(Runnable listener) {
    backButton.setOnClickListener((View v) -> listener.run());
  }

  /**
   * Sets a {@code Runnable} as the settings button event handler.
   *
   * @param[in] listener A {@code Runnable} to be executed when the settings button is clicked.
   */
  public void setOnSettingskButtonClick(Runnable listener) {
    settingsButton.setOnClickListener((View v) -> listener.run());
  }
}
