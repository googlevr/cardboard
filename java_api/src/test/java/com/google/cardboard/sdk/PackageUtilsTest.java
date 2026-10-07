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
package com.google.cardboard.sdk;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import android.content.Context;
import androidx.test.core.app.ApplicationProvider;
import com.google.android.gms.common.GoogleSignatureVerifier;
import java.util.Collections;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class PackageUtilsTest {
  private Context context;

  @Before
  public void setUp() {
    context = ApplicationProvider.getApplicationContext();
    GoogleSignatureVerifier.resetForTests();
  }

  @After
  public void tearDown() {
    GoogleSignatureVerifier.resetForTests();
  }

  @Test
  public void isGooglePackage_withContext_nullInputs() {
    assertFalse(PackageUtils.isGooglePackage(null, "com.google.test"));
    assertFalse(PackageUtils.isGooglePackage(context, null));
    assertFalse(PackageUtils.isGooglePackage(null, null));
  }

  @Test
  public void isGooglePackage_withContext_signatureVerified() {
    String googlePackage = "com.google.vr.vrcore";
    String attackerPackage = "com.google.attacker";
    String otherPackage = "com.example.app";

    GoogleSignatureVerifier.setGoogleSignedForTesting(Collections.singleton(googlePackage));

    assertTrue(PackageUtils.isGooglePackage(context, googlePackage));
    // Even though attackerPackage starts with "com.google.", it is not Google-signed:
    assertFalse(PackageUtils.isGooglePackage(context, attackerPackage));
    assertFalse(PackageUtils.isGooglePackage(context, otherPackage));
  }
}
