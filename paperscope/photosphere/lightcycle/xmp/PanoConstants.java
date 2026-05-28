// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.xmp;

import com.adobe.xmp.XMPException;
import com.adobe.xmp.XMPMetaFactory;

/**
 * Namespace and fields for panorama in XMP.
 * TODO(bwuest): This should not be forked from lightcycle, but rather shared as a library, can
 * be included through LightCycleCoreLib.
 */
public class PanoConstants {
  public static final String GOOGLE_PANO_NAMESPACE = "http://ns.google.com/photos/1.0/panorama/";
  public static final String PANO_PREFIX = "GPano";

  // Properties for Google panorama.
  // Whether to show the image in a panorama viewer, the property type is boolean.
  public static final String PROPERTY_USE_PANORAMA_VIEWER = "UsePanoramaViewer";

  // Projection type used in the image file, the property type is open choice.
  public static final String PROPERTY_PROJECTION_TYPE = "ProjectionType";
  public static final String PROPERTY_VALUE_EQUIRECTANGULAR = "equirectangular";
  public static final String PROPERTY_VALUE_CUBIC = "cubic";

  // Date and time for the first and last image created in the panorama.
  public static final String FIRST_PHOTO_DATE = "FirstPhotoDate";
  public static final String LAST_PHOTO_DATE = "LastPhotoDate";

  // The count of source photos to create panorama.
  public static final String SOURCE_PHOTOS_COUNT = "SourcePhotosCount";

  /** Pose heading in degrees. */
  public static final String POSE_HEADING_DEGREES = "PoseHeadingDegrees";
  /** Pose pitch in degrees. */
  public static final String POSE_PITCH_DEGREES = "PosePitchDegrees";
  /** Pose roll in degrees. */
  public static final String POSE_ROLL_DEGREES = "PoseRollDegrees";

  // Cropped area metadata.
  public static final String CROPPED_AREA_IMAGE_WIDTH_PIXELS =
      "CroppedAreaImageWidthPixels";
  public static final String CROPPED_AREA_IMAGE_HEIGHT_PIXELS =
      "CroppedAreaImageHeightPixels";
  public static final String CROPPED_AREA_FULL_PANO_WIDTH_PIXELS =
      "FullPanoWidthPixels";
  public static final String CROPPED_AREA_FULL_PANO_HEIGHT_PIXELS =
      "FullPanoHeightPixels";
  public static final String CROPPED_AREA_LEFT =
      "CroppedAreaLeftPixels";
  public static final String CROPPED_AREA_TOP =
      "CroppedAreaTopPixels";

  static {
    try {
      XMPMetaFactory.getSchemaRegistry().registerNamespace(GOOGLE_PANO_NAMESPACE, PANO_PREFIX);
    } catch (XMPException e) {
      e.printStackTrace();
    }
  }
}
