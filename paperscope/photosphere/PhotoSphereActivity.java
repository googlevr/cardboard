package com.google.vr.cardboard.paperscope.demo.photosphere;

import com.google.vr.cardboard.paperscope.demo.common.DemoBaseActivity;
import com.google.vr.cardboard.paperscope.demo.common.DemoBaseController;

/**
 * Demo that allows the user to view Photo Spheres stored on their device. Magnet pull toggles
 * through the Photo Spheres.
 */
public class PhotoSphereActivity extends DemoBaseActivity {
  private PhotoSphereController photoSphereController;

  @Override
  protected DemoBaseController<PhotoSphereRenderer> createController() {
    photoSphereController = new PhotoSphereController(cardboardView);
    photoSphereController.createPhotoSphere();
    return photoSphereController;
  }
}
