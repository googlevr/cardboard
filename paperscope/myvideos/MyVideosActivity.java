package com.google.vr.cardboard.paperscope.demo.myvideos;

import com.google.vr.cardboard.paperscope.demo.common.DemoBaseActivity;
import com.google.vr.cardboard.paperscope.demo.common.DemoBaseController;

/** Demo that allows the user to view videos stored on their device. */
public class MyVideosActivity extends DemoBaseActivity {
  @Override
  protected DemoBaseController<MyVideosRenderer> createController() {
    return new MyVideosController(cardboardView);
  }
}
