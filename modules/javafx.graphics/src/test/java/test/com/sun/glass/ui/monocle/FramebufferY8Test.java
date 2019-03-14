/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
package test.com.sun.glass.ui.monocle;

import com.sun.glass.ui.monocle.FramebufferY8Shim;
import com.sun.glass.ui.monocle.FramebufferY8SuperShim;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.nio.channels.Channels;
import java.text.MessageFormat;
import java.util.stream.IntStream;
import javax.imageio.ImageIO;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 * Provides test cases for the {@code FramebufferY8} class.
 */
public class FramebufferY8Test {

    /**
     * When {@code true}, saves a copy of the ARGB32 test image as
     * <i>{@value #IMAGE_PATH}</i> and the converted Y8 grayscale image as
     * <i>{@value #IMAGE_PATH_Y8}</i> in the current working directory. When
     * running under Gradle, the current working directory is
     * <i>modules/javafx.graphics</i>.
     */
    private static final boolean IMAGE_DEBUG = false;

    private static final String IMAGE_FORMAT = "png";
    private static final String IMAGE_NAME = "allrgb";
    private static final String IMAGE_PATH = IMAGE_NAME + "." + IMAGE_FORMAT;
    private static final String IMAGE_PATH_Y8 = IMAGE_NAME + "Y8." + IMAGE_FORMAT;

    /**
     * The number of iterations for the performance tests. A value of 100 can
     * provide more confidence in the results, but a low value of 10 allows the
     * automated tests to run with less delay.
     */
    private static final int ITERATIONS = 10;

    private static final int VALUES_4_BIT = 16;
    private static final int VALUES_12_BIT = VALUES_4_BIT * VALUES_4_BIT * VALUES_4_BIT;
    private static final int BITS_TO_BYTES = 3;

    private static final int WIDTH = VALUES_12_BIT;
    private static final int HEIGHT = VALUES_12_BIT;

    private static ByteBuffer bb;

    /**
     * Saves the pixel buffer to an image file in PNG format.
     *
     * @param pixels the pixels in ARGB32 format without alpha
     */
    private static void saveImage(IntBuffer pixels) {
        var image = new BufferedImage(WIDTH, HEIGHT, BufferedImage.TYPE_INT_RGB);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                image.setRGB(x, y, pixels.get());
            }
        }
        try {
            ImageIO.write(image, IMAGE_FORMAT, new File(IMAGE_PATH));
        } catch (IOException e) {
            System.err.println(MessageFormat.format("Error saving {0} ({1})", IMAGE_PATH, e));
        }
    }

    /**
     * Generates the test image in the composition buffer provided to the
     * {@code Framebuffer} and {@code FramebufferY8} constructors through their
     * shim subclasses. This method runs only once before all of the test cases.
     */
    @BeforeClass
    public static void onlyOnce() {
        bb = ByteBuffer.allocate(WIDTH * HEIGHT * Integer.BYTES);
        bb.order(ByteOrder.nativeOrder());
        IntBuffer pixels = bb.asIntBuffer();
        IntStream.range(0, WIDTH * HEIGHT).forEachOrdered(pixels::put);
        pixels.flip();
        if (IMAGE_DEBUG) {
            saveImage(pixels);
        }
    }

    /**
     * Prints the duration of a performance test.
     *
     * @param source the object containing the method tested
     * @param method the name of the method whose performance was tested
     * @param duration the duration of the performance test
     */
    private void printTime(Object source, String method, long duration) {
        float msPerFrame = (float) duration / ITERATIONS;
        System.out.println(String.format(
                "Converted %,d frames of %,d × %,d px to RGB565 in %,d ms (%,.0f ms/frame): %s.%s",
                ITERATIONS, WIDTH, HEIGHT, duration, msPerFrame,
                source.getClass().getSuperclass().getSimpleName(), method));
    }

    /**
     * Copies the image into a byte buffer using the original method of the
     * older {@code FramebufferY8} superclass, {@code Framebuffer}.
     *
     * @param bitsPerPixel the number of bits per pixel in the output buffer
     * @return a byte buffer containing the copied pixels
     */
    private ByteBuffer copyOld(int bitsPerPixel) {
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8SuperShim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        var target = ByteBuffer.allocate(WIDTH * HEIGHT * bytesPerPixel);
        source.copyToBuffer(target);
        target.flip();
        return target;
    }

    /**
     * Copies the image into a byte buffer using the updated method of the newer
     * {@code FramebufferY8} class.
     *
     * @param bitsPerPixel the number of bits per pixel in the output buffer
     * @return a byte buffer containing the copied pixels
     */
    private ByteBuffer copyNew(int bitsPerPixel) {
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8Shim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        var target = ByteBuffer.allocate(WIDTH * HEIGHT * bytesPerPixel);
        source.copyToBuffer(target);
        target.flip();
        return target;
    }

    /**
     * Tests the {@code FramebufferY8.copyToBuffer} method by comparing its
     * output to that of the original implementation in its superclass.
     *
     * @param bitsPerPixel the number of bits per pixel in the output buffer
     */
    private void copyTest(int bitsPerPixel) {
        ByteBuffer oldBuffer = copyOld(bitsPerPixel);
        ByteBuffer newBuffer = copyNew(bitsPerPixel);
        if (oldBuffer.hasArray() && newBuffer.hasArray()) {
            Assert.assertArrayEquals(oldBuffer.array(), newBuffer.array());
        } else {
            Assert.assertEquals(oldBuffer, newBuffer);
        }
    }

    /**
     * Writes the image into an output stream using the original method of the
     * older {@code FramebufferY8} superclass, {@code Framebuffer}.
     *
     * @param bitsPerPixel the number of bits per pixel in the output stream
     * @return an output stream containing the copied pixels
     * @throws IOException if an error occurs writing to the output stream
     */
    private ByteArrayOutputStream writeOld(int bitsPerPixel) throws IOException {
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8SuperShim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        try (var target = new ByteArrayOutputStream(WIDTH * HEIGHT * bytesPerPixel);
                var channel = Channels.newChannel(target)) {
            source.write(channel);
            return target;
        }
    }

    /**
     * Writes the image into an output stream using the updated method of the
     * newer {@code FramebufferY8} class.
     *
     * @param bitsPerPixel the number of bits per pixel in the output stream
     * @return an output stream containing the copied pixels
     * @throws IOException if an error occurs writing to the output stream
     */
    private ByteArrayOutputStream writeNew(int bitsPerPixel) throws IOException {
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8Shim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        try (var target = new ByteArrayOutputStream(WIDTH * HEIGHT * bytesPerPixel);
                var channel = Channels.newChannel(target)) {
            source.write(channel);
            return target;
        }
    }

    /**
     * Tests the {@code FramebufferY8.write} method by comparing its output to
     * that of the original implementation in its superclass.
     *
     * @param bitsPerPixel the number of bits per pixel in the output stream
     * @throws IOException if an error occurs writing to the output stream
     */
    private void writeTest(int bitsPerPixel) throws IOException {
        ByteArrayOutputStream oldStream = writeOld(bitsPerPixel);
        ByteArrayOutputStream newStream = writeNew(bitsPerPixel);
        Assert.assertArrayEquals(oldStream.toByteArray(), newStream.toByteArray());
    }

    /**
     * Tests copying the pixels to a 16-bit buffer in RGB565 format.
     */
    @Test
    public void copyTo16() {
        copyTest(Short.SIZE);
    }

    /**
     * Tests copying the pixels to a 32-bit buffer in ARGB32 format.
     */
    @Test
    public void copyTo32() {
        copyTest(Integer.SIZE);
    }

    /**
     * Tests writing the pixels to a 16-bit output stream in RGB565 format.
     *
     * @throws IOException if an error occurs writing to the output stream
     */
    @Test
    public void writeTo16() throws IOException {
        writeTest(Short.SIZE);
    }

    /**
     * Tests writing the pixels to a 32-bit output stream in ARGB32 format.
     *
     * @throws IOException if an error occurs writing to the output stream
     */
    @Test
    public void writeTo32() throws IOException {
        writeTest(Integer.SIZE);
    }

    /**
     * Measures the time for the original implementation to copy the test image
     * to a 16-bit buffer in RGB565 format {@value #ITERATIONS} times.
     */
    @Test
    public void timeOldCopyTo16() {
        int bitsPerPixel = Short.SIZE;
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8SuperShim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        var target = ByteBuffer.allocate(WIDTH * HEIGHT * bytesPerPixel);
        long begin = System.currentTimeMillis();
        for (int i = 0; i < ITERATIONS; i++) {
            source.copyToBuffer(target);
            target.flip();
        }
        long end = System.currentTimeMillis();
        printTime(source, "copyToBuffer", end - begin);
    }

    /**
     * Measures the time for the updated implementation to copy the test image
     * to a 16-bit buffer in RGB565 format {@value #ITERATIONS} times.
     */
    @Test
    public void timeNewCopyTo16() {
        int bitsPerPixel = Short.SIZE;
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8Shim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        var target = ByteBuffer.allocate(WIDTH * HEIGHT * bytesPerPixel);
        long begin = System.currentTimeMillis();
        for (int i = 0; i < ITERATIONS; i++) {
            source.copyToBuffer(target);
            target.flip();
        }
        long end = System.currentTimeMillis();
        printTime(source, "copyToBuffer", end - begin);
    }

    /**
     * Measures the time for the original implementation to write the test image
     * to a 16-bit output stream in RGB565 format {@value #ITERATIONS} times.
     *
     * @throws IOException if an error occurs writing to the output stream
     */
    @Test
    public void timeOldWriteTo16() throws IOException {
        int bitsPerPixel = Short.SIZE;
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8SuperShim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        try (var target = new ByteArrayOutputStream(WIDTH * HEIGHT * bytesPerPixel);
                var channel = Channels.newChannel(target)) {
            long begin = System.currentTimeMillis();
            for (int i = 0; i < ITERATIONS; i++) {
                source.write(channel);
                target.reset();
            }
            long end = System.currentTimeMillis();
            printTime(source, "write", end - begin);
        }
    }

    /**
     * Measures the time for the updated implementation to write the test image
     * to a 16-bit output stream in RGB565 format {@value #ITERATIONS} times.
     *
     * @throws IOException if an error occurs writing to the output stream
     */
    @Test
    public void timeNewWriteTo16() throws IOException {
        int bitsPerPixel = Short.SIZE;
        int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
        var source = new FramebufferY8Shim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
        try (var target = new ByteArrayOutputStream(WIDTH * HEIGHT * bytesPerPixel);
                var channel = Channels.newChannel(target)) {
            long begin = System.currentTimeMillis();
            for (int i = 0; i < ITERATIONS; i++) {
                source.write(channel);
                target.reset();
            }
            long end = System.currentTimeMillis();
            printTime(source, "write", end - begin);
        }
    }

    /**
     * Copies the test image to an 8-bit buffer in Y8 grayscale format and saves
     * the result as a PNG file when {@link #IMAGE_DEBUG} is {@code true};
     * otherwise, does nothing.
     */
    @Test
    public void saveImageY8() {
        if (IMAGE_DEBUG) {
            int bitsPerPixel = Byte.SIZE;
            int bytesPerPixel = bitsPerPixel >>> BITS_TO_BYTES;
            var source = new FramebufferY8Shim(bb, WIDTH, HEIGHT, bitsPerPixel, true);
            var target = ByteBuffer.allocate(WIDTH * HEIGHT * bytesPerPixel);
            source.copyToBuffer(target);
            target.flip();
            var image = new BufferedImage(WIDTH, HEIGHT, BufferedImage.TYPE_BYTE_GRAY);
            byte[] data = ((DataBufferByte) image.getRaster().getDataBuffer()).getData();
            System.arraycopy(target.array(), 0, data, 0, WIDTH * HEIGHT);
            try {
                ImageIO.write(image, IMAGE_FORMAT, new File(IMAGE_PATH_Y8));
            } catch (IOException e) {
                System.err.println(MessageFormat.format("Error saving {0} ({1})", IMAGE_PATH_Y8, e));
            }
        }
    }
}
