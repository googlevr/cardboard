/*
 * Copyright 2022 Google LLC
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

import android.os.Handler;
import android.os.Looper;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * AsyncTask is a helper class around Executor and Handler APIs. An asynchronous task runs on a
 * background thread and publishes its results on the UI thread.
 */
public abstract class AsyncTask<PARAM, RESULT> {
  private final ExecutorService executor;
  private final Handler handler;

  public AsyncTask() {
    executor = Executors.newSingleThreadExecutor();
    handler = new Handler(Looper.getMainLooper());
  }

  public void execute(PARAM param) {
    executor.execute(
        () -> {
          RESULT result = doInBackground(param);
          handler.post(() -> onPostExecute(result));
        });
    executor.shutdown();
  }

  protected abstract RESULT doInBackground(PARAM param);

  protected abstract void onPostExecute(RESULT result);
}
