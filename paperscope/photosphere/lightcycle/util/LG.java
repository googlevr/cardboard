// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util;

import android.util.Log;

/**
 * @author settinger@google.com (Scott Ettinger)
 *
 * Logger class that outputs the class and line number of the caller.  The
 * output can be slow, so it should be used in non-performance critical paths
 * or for debug only.
 */
public class LG {

  public static final boolean DEBUG_LOGGING_ENABLED = false;

  public static final boolean VERBOSE = true;
  public static final String TAG = "LightCycle-debug";

  // Output class and line number to the log if Verbose is set to true.
  public static void d(String message) {
    if (!DEBUG_LOGGING_ENABLED) {
      return;
    }
    if (VERBOSE) {
      String className =
        Thread.currentThread().getStackTrace()[3].getClassName();
      String shortClass =
        className.substring(className.lastIndexOf(".") + 1);
      String method =
        Thread.currentThread().getStackTrace()[3].getMethodName();
      int line =
        Thread.currentThread().getStackTrace()[3].getLineNumber();

      Log.v(TAG, shortClass + "." + method + "():" +  line + " : " +
            message);
    } else {
      Log.v(TAG, message);
    }
  }

}

