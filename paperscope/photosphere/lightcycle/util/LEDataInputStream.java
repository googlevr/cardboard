package com.google.vr.cardboard.paperscope.demo.photosphere.lightcycle.util;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * A little-endian DataInputStream.
 *
 * NOTE(dcoz): copied from
 * java/com/google/android/apps/gmm/streetview/internal/LEDataInputStream.java
 *
 * @author law@google.com (Kevin Law)
 */
class LEDataInputStream extends FilterInputStream implements DataInput {
  /** This underlying DIS implements the endian-agnostic methods. */
  DataInputStream dataInputStream;

  public LEDataInputStream(InputStream inputStream) {
    super(inputStream);
    dataInputStream = new DataInputStream(inputStream);
  }

  @Override
  public boolean readBoolean() throws IOException {
    return read() != 0;
  }

  @Override
  public byte readByte() throws IOException {
    return (byte) read();
  }

  @Override
  public char readChar() {
    throw new UnsupportedOperationException();
  }

  @Override
  public double readDouble() {
    throw new UnsupportedOperationException();
  }

  @Override
  public float readFloat() throws IOException {
    return Float.intBitsToFloat(readInt());
  }

  @Override
  public void readFully(byte[] buffer) throws IOException {
    dataInputStream.readFully(buffer);
  }

  @Override
  public void readFully(byte[] buffer, int offset, int length)
      throws IOException {
    dataInputStream.readFully(buffer, offset, length);
  }

  @Override
  public int readInt() throws IOException {
    return readUnsignedByte() | (readUnsignedByte() << 8) |
        (readUnsignedByte() << 16) | (readUnsignedByte() << 24);
  }

  @Override
  public String readLine() {
    throw new UnsupportedOperationException();
  }

  @Override
  public long readLong() {
    throw new UnsupportedOperationException();
  }

  @Override
  public short readShort() {
    throw new UnsupportedOperationException();
  }

  @Override
  public String readUTF() {
    throw new UnsupportedOperationException();
  }

  @Override
  public int readUnsignedByte() throws IOException {
    return read() & 0xff;
  }

  @Override
  public int readUnsignedShort() throws IOException {
    return readUnsignedByte() | (readUnsignedByte() << 8);
  }

  @Override
  public int skipBytes(int n) {
    throw new UnsupportedOperationException();
  }
}
