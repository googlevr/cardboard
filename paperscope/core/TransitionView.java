package com.google.vr.cardboard.paperscope.demo;

import android.content.Context;
import android.graphics.Point;
import android.graphics.drawable.ColorDrawable;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;

/** The transition view, which is displayed before entering VR mode. */
public class TransitionView extends FrameLayout {

  /** Interface for listening to events from the transition view. */
  public interface Listener {
    public void onSwitchButtonClicked();

    public void onExitButtonClicked();
  }

  public static final int TRANSITION_BACKGROUND_COLOR = 0xFF455A64;

  /**
   * Duration (in milliseconds), of the animation used to dismiss the view after landspace left
   * detected.
   */
  public static final int TRANSITION_ANIMATION_DURATION_MS = 500;

  /**
   * Amount of time (in milliseconds) to delay the dismissal of the transition view, if the device
   * is initially in landscape left, so the user can still see the transition briefly before
   * entering VR.
   */
  public static final int ALREADY_LANDSCAPE_LEFT_TRANSITION_DELAY_MS = 2000;

  /**
   * Tolerance (in degrees) for landscape device orientation detection. This should be a relatively
   * low tolerance as it is used for auto-dismissal of this view when in landscape. If this is too
   * high, then the transition view may dismiss before the phone is in the viewer.
   */
  private static final int LANDSCAPE_TOLERANCE_DEGREES = 5;

  private final Context context;
  private OrientationEventListener orientationEventListener;
  private Listener transitionViewListener;

  private static boolean isLandscapeLeft(int orientationDegrees) {
    return Math.abs(orientationDegrees - 270) < LANDSCAPE_TOLERANCE_DEGREES;
  }

  public TransitionView(Context context, Listener listener) {
    super(context);

    this.context = context;
    transitionViewListener = listener;
    setBackground(new ColorDrawable(TRANSITION_BACKGROUND_COLOR));
    inflateContentView(R.layout.transition_view);
    // Rotate the view -90 degrees to simulate portrait
    setRotation(-90);
  }

  private void inflateContentView(int layoutId) {
    removeAllViews();

    LayoutInflater.from(getContext()).inflate(layoutId, this, true);
    View switchActionView = findViewById(R.id.transition_switch_action);
    switchActionView.setOnClickListener(
        new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            // When the switch action is tapped, launched directly in to the change viewer
            transitionViewListener.onSwitchButtonClicked();
          }
        });

    ImageButton backButton = (ImageButton) findViewById(R.id.back_button);
    backButton.setOnClickListener(
        new OnClickListener() {
          @Override
          public void onClick(View v) {
            transitionViewListener.onExitButtonClicked();
          }
        });

    // Dismiss the view if the user clicks on the transition icon.
    ImageView transitionIcon = (ImageView) findViewById(R.id.transition_icon);
    transitionIcon.setOnClickListener(
        new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            fadeOutAndRemove(/* delay= */ false);
          }
        });
  }

  @SuppressWarnings("deprecation") // This is ignored because our app minSdkVersion is 16.
  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();

    // Get screen dimensions
    Display display =
        ((WindowManager) this.context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
    Point size = new Point();
    display.getSize(size);
    // Landscape width = real screen width, height = screen height
    int width = size.x;
    display.getRealSize(size);
    int height = size.y;

    // After rotation, we want the "portrait" layout:
    ViewGroup.LayoutParams params = getLayoutParams();
    if (params == null) {
      params = new ViewGroup.LayoutParams(height, width);
    } else {
      params.width = height;
      params.height = width;
    }
    setLayoutParams(params);

    // Move it back into position (translate after rotation)
    setTranslationX((width - height) / 2f);
    setTranslationY((height - width) / 2f);

    startOrientationMonitor();
  }

  private void fadeOutAndRemove(boolean delay) {
    // Once we are ready to fade out, immediately stop and invalidate the orientation listener.
    stopOrientationMonitor();

    Animation existingFadeOut = getAnimation();
    if (existingFadeOut != null) {
      // Don't reschedule an animation if an existing one is running or will start before the new
      // one.
      if (delay || existingFadeOut.getStartOffset() == 0) {
        return;
      }
      // The clearAnimation() call will trigger the listener's onAnimationEnd method, which
      // will in turn detach the TransitionView. Avoid this by first clearing the listener.
      existingFadeOut.setAnimationListener(null);
      clearAnimation();
    }

    Animation newFadeOut = new AlphaAnimation(/* fromAlpha= */ 1, /* toAlpha= */ 0);
    newFadeOut.setInterpolator(new AccelerateInterpolator());
    newFadeOut.setRepeatCount(0);
    newFadeOut.setDuration(TRANSITION_ANIMATION_DURATION_MS);
    if (delay) {
      newFadeOut.setStartOffset(ALREADY_LANDSCAPE_LEFT_TRANSITION_DELAY_MS);
    }

    newFadeOut.setAnimationListener(
        new Animation.AnimationListener() {
          @Override
          public void onAnimationStart(Animation animation) {}

          @Override
          public void onAnimationEnd(Animation animation) {
            TransitionView.this.setVisibility(View.GONE);
            ((ViewGroup) getParent()).removeView(TransitionView.this);
          }

          @Override
          public void onAnimationRepeat(Animation animation) {}
        });

    startAnimation(newFadeOut);
  }

  private void startOrientationMonitor() {
    if (orientationEventListener != null) {
      return;
    }

    orientationEventListener =
        new OrientationEventListener(getContext()) {
          @Override
          public void onOrientationChanged(int orientation) {
            if (isLandscapeLeft(orientation)) {
              fadeOutAndRemove(/* delay= */ false);
            }
          }
        };

    orientationEventListener.enable();
  }

  private void stopOrientationMonitor() {
    if (orientationEventListener == null) {
      return;
    }

    orientationEventListener.disable();
    orientationEventListener = null;
  }
}
