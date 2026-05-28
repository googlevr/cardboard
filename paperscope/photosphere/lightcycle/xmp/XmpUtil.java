/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.xmp;

import static java.nio.charset.StandardCharsets.UTF_8;

import android.util.Log;
import com.adobe.xmp.XMPException;
import com.adobe.xmp.XMPMeta;
import com.adobe.xmp.XMPMetaFactory;
import com.adobe.xmp.options.SerializeOptions;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Formatter;
import java.util.List;
import java.util.Locale;
import org.jspecify.annotations.Nullable;

/**
 * Util class to read/write xmp from a jpeg image file. It only supports jpeg image format, and
 * doesn't support extended xmp now. To use it: XMPMeta xmpMeta =
 * XmpUtil.extractOrCreateXMPMeta(filename);
 * xmpMeta.setProperty(PanoConstants.GOOGLE_PANO_NAMESPACE, "property_name", "value");
 * XmpUtil.writeXMPMeta(filename, xmpMeta);
 *
 * <p>Or if you don't care the existing XMP meta data in image file: XMPMeta xmpMeta =
 * XmpUtil.createXMPMeta(); xmpMeta.setPropertyBoolean(PanoConstants.GOOGLE_PANO_NAMESPACE,
 * "bool_property_name", "true"); XmpUtil.writeXMPMeta(filename, xmpMeta);
 */
public class XmpUtil {
  private static final String TAG = "XmpUtil";
  private static final int XMP_HEADER_SIZE = 29;
  private static final String XMP_HEADER = "http://ns.adobe.com/xap/1.0/\0";
  private static final int MAX_XMP_BUFFER_SIZE = 65502;

  private static final String EXTENDED_XMP_HEADER_SIGNATURE =
      "http://ns.adobe.com/xmp/extension/\0";
  private static final String XMP_NOTE_NAMESPACE = "http://ns.adobe.com/xmp/note/";
  private static final String NOTE_PREFIX = "xmpNote";

  private static final int MAX_EXTENDED_XMP_BUFFER_SIZE = 65000;
  private static final int EXTEND_XMP_HEADER_SIZE = 75;

  private static final String GOOGLE_PANO_NAMESPACE = "http://ns.google.com/photos/1.0/panorama/";
  private static final String PANO_PREFIX = "GPano";

  private static final int M_SOI = 0xd8; // File start marker.
  private static final int M_APP1 = 0xe1; // Marker for Exif or XMP.
  private static final int M_SOS = 0xda; // Image data marker.

  // Jpeg file is composed of many sections and image data. This class is used
  // to hold the section data from image file.
  private static class Section {
    int marker;
    int length;
    byte[] data;
  }

  static {
    try {
      XMPMetaFactory.getSchemaRegistry().registerNamespace(GOOGLE_PANO_NAMESPACE, PANO_PREFIX);

      XMPMetaFactory.getSchemaRegistry().registerNamespace(XMP_NOTE_NAMESPACE, NOTE_PREFIX);
    } catch (XMPException e) {
      Log.e(TAG, "Error registering XMP namespace", e);
    }
  }

  /**
   * Extracts XMPMeta from JPEG image file.
   *
   * @param filename JPEG image file name.
   * @return Extracted XMPMeta or null.
   */
  public static @Nullable XMPMeta extractXmpMeta(String filename) {
    if (!isJpeg(filename)) {
      Log.d(TAG, "XMP parse: only jpeg file is supported");
      return null;
    }

    try {
      return extractXmpMeta(new FileInputStream(filename));
    } catch (FileNotFoundException e) {
      Log.e(TAG, "Could not read file: " + filename, e);
      return null;
    }
  }

  /**
   * Extracts XMPMeta from a JPEG image file stream.
   *
   * @param is the input stream containing the JPEG image file.
   * @return Extracted XMPMeta or null.
   */
  public static @Nullable XMPMeta extractXmpMeta(InputStream is) {
    List<Section> sections = parse(is, true);
    if (sections == null) {
      return null;
    }
    // Now we don't support extended xmp.
    for (Section section : sections) {
      if (hasStandardXmpHeader(section.data)) {
        int end = getXmpContentEnd(section.data);
        byte[] buffer = new byte[end - XMP_HEADER_SIZE];
        System.arraycopy(section.data, XMP_HEADER_SIZE, buffer, 0, buffer.length);
        try {
          return XMPMetaFactory.parseFromBuffer(buffer);
        } catch (XMPException e) {
          Log.d(TAG, "XMP parse error", e);
          return null;
        }
      }
    }
    return null;
  }

  /** Creates a new XMPMeta. */
  public static XMPMeta createXmpMeta() {
    return XMPMetaFactory.create();
  }

  /** Tries to extract XMP meta from image file first, if failed, create one. */
  public static XMPMeta extractOrCreateXmpMeta(String filename) {
    XMPMeta meta = extractXmpMeta(filename);
    return meta == null ? createXmpMeta() : meta;
  }

  /** Writes the XMPMeta to the jpeg image file. */
  public static boolean writeXmpMeta(String filename, XMPMeta meta) {
    if (!isJpeg(filename)) {
      Log.d(TAG, "XMP parse: only jpeg file is supported");
      return false;
    }
    List<Section> sections = null;
    try {
      sections = parse(new FileInputStream(filename), /* readMetaOnly= */ false);
      sections = insertXmpSection(sections, meta);
      if (sections == null) {
        return false;
      }
    } catch (FileNotFoundException e) {
      Log.e(TAG, "Could not read file: " + filename, e);
      return false;
    }
    try (FileOutputStream outputStream = new FileOutputStream(filename)) {
      // Overwrite the image file with the new meta data.
      writeJpegFile(outputStream, sections);
    } catch (IOException e) {
      Log.d(TAG, "Write file failed:" + filename, e);
      return false;
    }
    return true;
  }

  /** Updates a jpeg file from inputStream with XMPMeta to outputStream. */
  public static boolean writeXmpMeta(
      InputStream inputStream, OutputStream outputStream, XMPMeta meta) {
    List<Section> sections = parse(inputStream, false);
    sections = insertXmpSection(sections, meta);
    if (sections == null) {
      return false;
    }
    try {
      // Overwrite the image file with the new meta data.
      writeJpegFile(outputStream, sections);
    } catch (IOException e) {
      Log.d(TAG, "Write to stream failed", e);
      return false;
    } finally {
      if (outputStream != null) {
        try {
          outputStream.close();
        } catch (IOException e) {
          // Ignore.
        }
      }
    }
    return true;
  }

  /**
   * Updates a jpeg file from inputStream with XMPMeta to outputStream.
   *
   * @param inputStream Input image data stream
   * @param outputStream Output image data stream
   * @param standardMeta The main portion of the metadata tree must be serialized and written as the
   *     standard XMP packet
   * @param extendedMeta The extended portion must be serialized without a packet wrapper, and
   *     written as a series of APP1 marker segments
   * @return True if the XMP meta data is written successfully, false otherwise.
   */
  public static boolean writeXmpMeta(
      InputStream inputStream,
      OutputStream outputStream,
      XMPMeta standardMeta,
      XMPMeta extendedMeta) {
    byte[] buffer;
    try {
      SerializeOptions options = new SerializeOptions();
      options.setUseCompactFormat(true);
      // We have to omit packet wrapper here because
      // javax.xml.parsers.DocumentBuilder
      // fails to parse the packet end <?xpacket end="w"?> in android.
      options.setOmitPacketWrapper(true);
      buffer = XMPMetaFactory.serializeToBuffer(extendedMeta, options);
    } catch (XMPException e) {
      Log.d(TAG, "Serialize extended xmp failed", e);
      return false;
    }

    String guid = getGuid(buffer);
    try {
      standardMeta.setProperty(XMP_NOTE_NAMESPACE, "HasExtendedXMP", guid);
    } catch (XMPException exception) {
      Log.d(TAG, "set XMPMeta Property", exception);
      return false;
    }
    List<Section> sections = parse(inputStream, false);
    List<Section> xmpSections = new ArrayList<>();
    Section standardXmpSection = createStandardXmpSection(standardMeta);
    if (standardXmpSection == null) {
      Log.e(TAG, "create standard meta section error");
      return false;
    }
    xmpSections.add(standardXmpSection);

    List<Section> extendedSections = splitExtendXmpMeta(buffer, guid);
    xmpSections.addAll(extendedSections);
    sections = insertXmpSection(sections, xmpSections);
    if (sections == null) {
      Log.d(TAG, "Insert XMP fialed");
      return false;
    }
    try {
      // Overwrite the image file with the new meta data.
      writeJpegFile(outputStream, sections);
    } catch (IOException e) {
      Log.d(TAG, "Write to stream failed", e);
      return false;
    } finally {
      if (outputStream != null) {
        try {
          outputStream.close();
        } catch (IOException e) {
          // Ignore.
        }
      }
    }
    return true;
  }

  /** Write a list of sections to a Jpeg file. */
  private static void writeJpegFile(OutputStream os, List<Section> sections) throws IOException {
    // Writes the jpeg file header.
    os.write(0xff);
    os.write(M_SOI);
    for (Section section : sections) {
      os.write(0xff);
      os.write(section.marker);
      if (section.length > 0) {
        // It's not the image data.
        int lh = section.length >> 8;
        int ll = section.length & 0xff;
        os.write(lh);
        os.write(ll);
      }
      os.write(section.data);
    }
  }

  /**
   * Checks whether the byte array has the standard XMP header. The standard XMP section contains a
   * fixed length header XMP_HEADER.
   *
   * @param data Xmp metadata.
   * @return True if the byte array has the standard XMP header, false otherwise.
   */
  private static boolean hasStandardXmpHeader(byte[] data) {
    if (data.length < XMP_HEADER_SIZE) {
      return false;
    }
    byte[] header = Arrays.copyOf(data, XMP_HEADER_SIZE);
    if (new String(header, UTF_8).equals(XMP_HEADER)) {
      return true;
    }
    return false;
  }

  /**
   * Checks whether the byte array has the extended XMP header. Extended XMP sections start with
   * EXTENDED_XMP_HEADER_SIGNATURE.
   *
   * @param data Xmp metadata.
   * @return True if the byte array has the extended XMP header, false otherwise.
   */
  private static boolean hasExtendedXmpHeader(byte[] data) {
    if (data.length < EXTENDED_XMP_HEADER_SIGNATURE.length()) {
      return false;
    }
    byte[] header = Arrays.copyOf(data, EXTENDED_XMP_HEADER_SIGNATURE.length());
    if (new String(header, UTF_8).equals(EXTENDED_XMP_HEADER_SIGNATURE)) {
      return true;
    }
    return false;
  }

  /**
   * Checks if a section is an XMP section, either standard or extended.
   *
   * @param section The JPEG section to check.
   * @return True if the section contains XMP data, false otherwise.
   */
  private static boolean isXmpSection(Section section) {
    return section.marker == M_APP1
        && (hasStandardXmpHeader(section.data) || hasExtendedXmpHeader(section.data));
  }

  /**
   * Gets the end of the xmp meta content. If there is no packet wrapper, return data.length,
   * otherwise return 1 + the position of last '>' without '?' before it. Usually the packet wrapper
   * end is "<?xpacket end="w"?> but javax.xml.parsers.DocumentBuilder fails to parse it in android.
   *
   * @param data xmp metadata bytes.
   * @return The end of the xmp metadata content.
   */
  private static int getXmpContentEnd(byte[] data) {
    for (int i = data.length - 1; i >= 1; --i) {
      if (data[i] == '>') {
        if (data[i - 1] != '?') {
          return i + 1;
        }
      }
    }
    // It should not reach here for a valid xmp meta.
    return data.length;
  }

  /**
   * Parses the jpeg image file. If readMetaOnly is true, only keeps the Exif and XMP sections (with
   * marker M_APP1) and ignore others; otherwise, keep all sections. The last section with image
   * data will have -1 length.
   *
   * @param is Input image data stream.
   * @param readMetaOnly Whether only reads the metadata in jpg.
   * @return The parse result.
   */
  private static @Nullable List<Section> parse(InputStream is, boolean readMetaOnly) {
    try {
      if (is.read() != 0xff || is.read() != M_SOI) {
        return null;
      }
      List<Section> sections = new ArrayList<>();
      int c;
      while ((c = is.read()) != -1) {
        if (c != 0xff) {
          return null;
        }
        // Skip padding bytes.
        while ((c = is.read()) == 0xff) {}
        if (c == -1) {
          return null;
        }
        int marker = c;
        if (marker == M_SOS) {
          // M_SOS indicates the image data will follow and no metadata after
          // that, so read all data at one time.
          if (!readMetaOnly) {
            Section section = new Section();
            section.marker = marker;
            section.length = -1;
            ByteArrayOutputStream buffer = new ByteArrayOutputStream();
            int nRead;
            byte[] data = new byte[16384];
            while ((nRead = is.read(data, 0, data.length)) != -1) {
              buffer.write(data, 0, nRead);
            }
            buffer.flush();
            // For large panorama files, reading the entire entropy-coded data (ECD) into a
            // single byte array can lead to OutOfMemoryError. A more memory-efficient approach
            // would involve handling this section in a stream-based manner when writing,
            // avoiding the need to load the entire content into memory at once.
            section.data = buffer.toByteArray();
            sections.add(section);
          }
          return sections;
        }
        int lh = is.read();
        int ll = is.read();
        if (lh == -1 || ll == -1) {
          return null;
        }
        int length = lh << 8 | ll;
        if (!readMetaOnly || c == M_APP1) {
          Section section = new Section();
          section.marker = marker;
          section.length = length;
          section.data = new byte[length - 2];
          int numBytesRead = 0;
          int bytesToRead = length - 2;
          while (numBytesRead < bytesToRead) {
            int readResult = is.read(section.data, numBytesRead, bytesToRead - numBytesRead);
            if (readResult == -1) {
              break; // End of stream
            }
            numBytesRead += readResult;
          }
          if (numBytesRead != bytesToRead) {
            Log.d(TAG, "Could not read all bytes for section.");
            return null;
          }
          sections.add(section);
        } else {
          // Skip this section since all exif/xmp meta will be in M_APP1
          // section.
          long bytesToSkip = length - 2L;
          while (bytesToSkip > 0) {
            long skipped = is.skip(bytesToSkip);
            if (skipped <= 0) {
              // Should not happen if length is positive, but handle for safety.
              Log.d(TAG, "Could not skip all bytes for section.");
              return null;
            }
            bytesToSkip -= skipped;
          }
        }
      }
      return sections;
    } catch (IOException e) {
      Log.d(TAG, "Could not parse file.", e);
      return null;
    } finally {
      if (is != null) {
        try {
          is.close();
        } catch (IOException e) {
          // Ignore.
        }
      }
    }
  }

  private static @Nullable Section createStandardXmpSection(XMPMeta meta) {
    byte[] buffer;
    try {
      SerializeOptions options = new SerializeOptions();
      options.setUseCompactFormat(true);
      // We have to omit packet wrapper here because
      // javax.xml.parsers.DocumentBuilder
      // fails to parse the packet end <?xpacket end="w"?> in android.
      options.setOmitPacketWrapper(true);
      buffer = XMPMetaFactory.serializeToBuffer(meta, options);
    } catch (XMPException e) {
      Log.d(TAG, "Serialize xmp failed", e);
      return null;
    }
    if (buffer.length > MAX_XMP_BUFFER_SIZE) {
      Log.e(TAG, "exceed max size");
      return null;
    }
    // The XMP section starts with XMP_HEADER and then the real xmp data.
    byte[] xmpdata = new byte[buffer.length + XMP_HEADER_SIZE];
    System.arraycopy(XMP_HEADER.getBytes(), 0, xmpdata, 0, XMP_HEADER_SIZE);
    System.arraycopy(buffer, 0, xmpdata, XMP_HEADER_SIZE, buffer.length);
    Section xmpSection = new Section();
    xmpSection.marker = M_APP1;
    // Adds the length place (2 bytes) to the section length.
    xmpSection.length = xmpdata.length + 2;
    xmpSection.data = xmpdata;

    return xmpSection;
  }

  private static @Nullable Section createSection(byte[] portionOfExtendedMeta, byte[] headerBytes) {

    if (portionOfExtendedMeta.length > MAX_EXTENDED_XMP_BUFFER_SIZE) {
      // Do not support extended xmp now.
      Log.e(TAG, "createSection fail exceed max size");
      return null;
    }

    byte[] xmpdata = Arrays.copyOf(headerBytes, portionOfExtendedMeta.length + 75);
    System.arraycopy(headerBytes, 0, xmpdata, 0, headerBytes.length);

    System.arraycopy(
        portionOfExtendedMeta, 0, xmpdata, headerBytes.length, portionOfExtendedMeta.length);
    Section xmpSection = new Section();
    xmpSection.marker = M_APP1;
    // Adds the length place (2 bytes) to the section length.
    xmpSection.length = xmpdata.length + 2;
    xmpSection.data = xmpdata;
    ByteBuffer byteBuffer2 = ByteBuffer.wrap(xmpdata);
    Log.d(TAG, "fullLength=" + byteBuffer2.getInt(67) + " offset=" + byteBuffer2.getInt(71));
    return xmpSection;
  }

  /**
   * Split extendXMPMeta to multiple marker segments
   *
   * @param extendedXmpMetaBytes serialized extended XMP
   * @param guid Is a 128-bit MD5 digest of the full ExtendedXMP serialization, stored as a 32-byte
   *     ASCII hex string
   * @return split result
   */
  private static @Nullable List<Section> splitExtendXmpMeta(
      byte[] extendedXmpMetaBytes, String guid) {
    List<Section> sections = new ArrayList<>();
    /*
    The extended XMP JPEG marker segment content holds:
    - a signature string, "http://ns.adobe.com/xmp/extension/\0"
    - a 128 bit GUID stored as a 32 byte ASCII hex string
    - a UInt32 full length of the entire extended XMP
    - a UInt32 offset for this portion of the extended XMP
    - the UTF-8 text for this portion of the extended XMP
     */
    int splitNum = extendedXmpMetaBytes.length / MAX_EXTENDED_XMP_BUFFER_SIZE;
    byte[] portion = new byte[MAX_EXTENDED_XMP_BUFFER_SIZE];
    ByteBuffer byteBuffer = ByteBuffer.wrap(extendedXmpMetaBytes);
    Section extendedXmpSection = null;

    byte[] headerBytes = new byte[EXTEND_XMP_HEADER_SIZE];
    int index = 0;
    System.arraycopy(
        EXTENDED_XMP_HEADER_SIGNATURE.getBytes(),
        0,
        headerBytes,
        0,
        EXTENDED_XMP_HEADER_SIGNATURE.length());
    index += EXTENDED_XMP_HEADER_SIGNATURE.length();

    System.arraycopy(guid.getBytes(), 0, headerBytes, index, guid.length());
    index += guid.length();

    Log.d(TAG, "buffer.length=" + extendedXmpMetaBytes.length);
    byte[] fullLengthBytes = new byte[4];
    ByteBuffer intBuffer = ByteBuffer.wrap(fullLengthBytes);
    intBuffer.putInt(0, extendedXmpMetaBytes.length);
    System.arraycopy(fullLengthBytes, 0, headerBytes, index, 4);
    index += 4;

    byte[] offsetBytes = new byte[4];
    ByteBuffer offsetBuffer = ByteBuffer.wrap(offsetBytes);
    for (int i = 0; i < splitNum; ++i) {
      offsetBuffer.putInt(0, i * MAX_EXTENDED_XMP_BUFFER_SIZE);
      System.arraycopy(offsetBytes, 0, headerBytes, index, 4);

      byteBuffer.get(portion);
      extendedXmpSection = createSection(portion, headerBytes);
      sections.add(extendedXmpSection);
    }

    int remainSize = extendedXmpMetaBytes.length - splitNum * MAX_EXTENDED_XMP_BUFFER_SIZE;
    if (remainSize > 0) {
      offsetBuffer.putInt(0, splitNum * MAX_EXTENDED_XMP_BUFFER_SIZE);
      System.arraycopy(offsetBytes, 0, headerBytes, index, 4);

      byte[] remain = new byte[remainSize];
      byteBuffer.get(remain);
      extendedXmpSection = createSection(remain, headerBytes);
      sections.add(extendedXmpSection);
    }

    return sections;
  }

  /**
   * Inserts a list of XMP Sections into the overall list of JPEG sections. This method will replace
   * any existing XMP sections found in the input {@code sections}.
   *
   * @param sections The list of JPEG sections parsed from the file.
   * @param xmpSections The list of new XMP sections to insert.
   * @return A new list of sections with the old XMP sections replaced by {@code xmpSections}, or
   *     null if the input is invalid.
   */
  private static @Nullable List<Section> insertXmpSection(
      List<Section> sections, List<Section> xmpSections) {
    if (sections == null || sections.isEmpty()) {
      return null;
    }

    List<Section> resultSections = new ArrayList<>();
    int firstXmpIndex = -1;
    int lastXmpIndex = -1;

    // Find the range of existing XMP sections (consecutive M_APP1 with standard or extended XMP
    // headers).
    for (int i = 0; i < sections.size(); i++) {
      Section section = sections.get(i);
      if (isXmpSection(section)) {
        if (firstXmpIndex == -1) {
          firstXmpIndex = i;
        }
        lastXmpIndex = i;
      } else if (firstXmpIndex != -1) {
        // Stop if a non-XMP section is encountered after finding XMP.
        break;
      }
    }

    if (firstXmpIndex != -1) {
      // Existing XMP sections found. Replace them with the new xmpSections.
      resultSections.addAll(sections.subList(0, firstXmpIndex));
      resultSections.addAll(xmpSections);
      if (lastXmpIndex + 1 < sections.size()) {
        resultSections.addAll(sections.subList(lastXmpIndex + 1, sections.size()));
      }
    } else {
      // No existing XMP sections. Determine insertion point.
      // If the first section is M_APP1 (e.g., Exif), insert after it.
      // Otherwise, make xmp data the first section.
      int insertPosition = 0;
      if (!sections.isEmpty() && sections.get(0).marker == M_APP1) {
        insertPosition = 1;
      }
      resultSections.addAll(sections.subList(0, insertPosition));
      resultSections.addAll(xmpSections);
      resultSections.addAll(sections.subList(insertPosition, sections.size()));
    }
    return resultSections;
  }

  private static @Nullable List<Section> insertXmpSection(List<Section> sections, XMPMeta meta) {
    if (sections == null || sections.size() <= 1) {
      return null;
    }
    Section standardXmpSection = createStandardXmpSection(meta);
    if (standardXmpSection == null) {
      return null;
    }
    List<Section> xmpSections = new ArrayList<>();
    xmpSections.add(standardXmpSection);
    return insertXmpSection(sections, xmpSections);
  }

  private static boolean isJpeg(String filename) {
    return filename.toLowerCase(Locale.ROOT).endsWith(".jpg")
        || filename.toLowerCase(Locale.ROOT).endsWith(".jpeg");
  }

  private static @Nullable String getGuid(byte[] src) {
    StringBuilder builder = new StringBuilder();
    try {
      MessageDigest digester = MessageDigest.getInstance("MD5");
      digester.update(src);
      byte[] digest = digester.digest();

      Formatter formatter = new Formatter(builder);
      for (int i = 0; i < digest.length; ++i) {
        formatter.format("%02x", ((256 + digest[i]) % 256));
      }
    } catch (NoSuchAlgorithmException exception) {
      Log.d(TAG, "get md5 instance failure" + exception);
      return null;
    }

    return builder.toString().toUpperCase(Locale.ROOT);
  }

  private XmpUtil() {}
}
