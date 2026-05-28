package com.google.vr.cardboard.paperscope.demo.myvideos;

import com.google.cardboard.sdk.CardboardView;
import com.google.vr.cardboard.paperscope.demo.common.DemoBaseController;

/** Demo that allows the user to view My Videos. */
public class MyVideosController extends DemoBaseController<MyVideosRenderer> {
  private final VideoSequencer sequencer;

  public MyVideosController(CardboardView cardboardView) {
    super(cardboardView);
    renderer = new MyVideosRenderer(cardboardView.getContext());
    sequencer = new VideoSequencer(cardboardView.getContext(), cardboardView, renderer);
    this.cardboardView.setRenderer(renderer);
  }

  @Override
  public void onResume() {
    super.onResume();
    sequencer.startPlayback();
  }

  @Override
  public void onPause() {
    super.onPause();
    sequencer.stop();
  }

  @Override
  public void shutdown() {
    super.shutdown();
    sequencer.release();
  }

  @Override
  public void enableOnTriggerEventListener() {
    cardboardView.setOnTriggerEvent(
        () -> {
          if (renderer != null && sequencer != null) {
            if (renderer.isScreenOnlyTargeted()) {
              sequencer.togglePause();
            } else {
              sequencer.playTargetedVideo();
            }
          }
        });
  }
}
