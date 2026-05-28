// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util;

/**
 * A simple general purpose callback interface.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 *
 * @param <T> The type of the callback response instance.
 */
public interface Callback<T> {

  /**
   * The callback method.
   */
  public void onCallback(T response);
}
