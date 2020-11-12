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
package com.google.cardboard.sdk.qrcode;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.OutputStream;

/**
 * Provides an {@code OutputStream} to write to a {@code File}.
 *
 * <p>This class is used to inject mock streams and test {@code CardboardParamsUtils}.
 */
public class OutputStreamProvider {
  /** Interface to provide an {@code OutputStream} from a file. */
  public interface Provider {
    /**
     * Returns an {@code OutputStream} that wraps a file.
     *
     * @param[in] file A file to wrap with an {@code OutputStream}.
     * @return An {@code OutputStream}.
     * @throws FileNotFoundException When {@code file} cannot be openned.
     */
    OutputStream get(File file) throws FileNotFoundException;
  }

  /**
   * Default {@code Provider} implementation based on a {@code BufferedOutputStream}.
   */
  private static class BufferedProvider implements Provider {
    public BufferedProvider() {}

    @Override
    public OutputStream get(File file) throws FileNotFoundException {
      return new BufferedOutputStream(new FileOutputStream(file));
    }
  }

  /**
   * Default {@code Provider} implementation that returns a {@code BufferedOutputStream} from {@code
   * file}.
   */
  private static Provider provider = new BufferedProvider();

  private OutputStreamProvider() {}

  /**
   * Setter of a custom {@code Provider} implementation.
   *
   * @param[in] provider A custom {@code Provider} implementation.
   */
  public static void setProvider(Provider provider) {
    OutputStreamProvider.provider = provider;
  }

  /**
   * Getter of a default {@code Provider} that uses a {@code BufferedOutputStream}.
   *
   * @return A {@code Provider}.
   */
  public static Provider getDefaultProvider() {
    return new BufferedProvider();
  }

  /**
   * Gets an {@code OutputStream} wrapping a file.
   *
   * @param[in] file A file to wrap with an {@code OutputStream}.
   * @return An {@code OutputStream}.
   * @throws FileNotFoundException When {@code file} cannot be openned.
   */
  public static OutputStream get(File file) throws FileNotFoundException {
    return provider.get(file);
  }
}
