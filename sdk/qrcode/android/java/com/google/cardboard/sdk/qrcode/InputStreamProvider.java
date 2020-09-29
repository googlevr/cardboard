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

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;

/**
 * Provides an {@code InputStream} to read from a {@code File}.
 *
 * <p>This class is used to inject mock streams and test {@code CardboardParamsUtils}.
 */
public class InputStreamProvider {
  /** Interface to provide an {@code InputStream} from a file. */
  public interface Provider {
    /**
     * Returns an {@code InputStream} that wraps a file.
     *
     * @param[in] file A file to wrap with an {@code InputStream}.
     * @return An {@code InputStream}.
     * @throws FileNotFoundException When {@code file} cannot be openned.
     */
    InputStream get(File file) throws FileNotFoundException;
  }

  /**
   * Default {@code Provider} implementation based on a {@code BufferedInputStream}.
   */
  private static class BufferedProvider implements Provider {
    public BufferedProvider() {}

    @Override
    public InputStream get(File file) throws FileNotFoundException {
      return new BufferedInputStream(new FileInputStream(file));
    }
  }

  /**
   * Default {@code Provider} implementation that returns a {@code BufferedInputStream} from {@code
   * file}.
   */
  private static Provider provider = new BufferedProvider();

  private InputStreamProvider() {}

  /**
   * Setter of a custom {@code Provider} implementation.
   *
   * @param[in] provider A custom {@code Provider} implementation.
   */
  public static void setProvider(Provider provider) {
    InputStreamProvider.provider = provider;
  }

  /**
   * Getter of a default {@code Provider} that uses a {@code BufferedInputStream}.
   *
   * @return A {@code Provider}.
   */
  public static Provider getDefaultProvider() {
    return new BufferedProvider();
  }

  /**
   * Gets an {@code InputStream} wrapping a file.
   *
   * @param[in] file A file to wrap with an {@code InputStream}.
   * @return An {@code InputStream}.
   * @throws FileNotFoundException When {@code file} cannot be openned.
   */
  public static InputStream get(File file) throws FileNotFoundException {
    return provider.get(file);
  }
}
