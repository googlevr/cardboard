// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util;

import java.io.InputStream;

/**
 * Creates input streams.
 *
 * TODO(haeberling): Move to common package when possible.
 *
 * @author haeberling@google.com (Sascha Haeberling)
 */
public interface InputStreamFactory {
    /**
     * @return The newly created {@link InputStream}.
     */
    public InputStream create();
}
