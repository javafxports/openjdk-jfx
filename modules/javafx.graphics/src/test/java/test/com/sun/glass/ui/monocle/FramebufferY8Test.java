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

import com.sun.glass.ui.monocle.FramebufferY8SuperShim;
import com.sun.glass.ui.monocle.FramebufferY8Shim;
import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.nio.channels.Channels;
import javax.imageio.ImageIO;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 * Provides test cases for the {@code FramebufferY8} class.
 */
public class FramebufferY8Test {

    /**
     * An image with all 16,777,216 colors of the 24-bit RGB palette in a 4096 ×
     * 4096 bitmap. The image was created by Naruto_64 and published on
     * Wikimedia Commons under the Creative Commons "CC BY-SA 4.0" license.
     *
     * @see
     * <a href="https://commons.wikimedia.org/wiki/File:16777216colors_diffpatt.png">
     * File:16777216colors diffpatt.png - Wikimedia Commons</a>
     */
    private static final String ALL_24BIT_RGB = "16777216colors_diffpatt.png";

    private static ByteBuffer bb;
    private static int width;
    private static int height;

    /**
     * Loads the sample image into the composition buffer held by the
     * {@code FramebufferY8} class. This method runs only once before all of the
     * test cases. It loads the image into the buffer as 32-bit pixels in ARGB32
     * format.
     *
     * @throws IOException if an error occurs loading the image
     */
    @BeforeClass
    public static void onlyOnce() throws IOException {
        InputStream stream = FramebufferY8Test.class.getResourceAsStream(ALL_24BIT_RGB);
        BufferedImage image = ImageIO.read(stream);
        width = image.getWidth();
        height = image.getHeight();

        bb = ByteBuffer.allocate(width * height * Integer.BYTES);
        bb.order(ByteOrder.nativeOrder());
        IntBuffer pixels = bb.asIntBuffer();
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                pixels.put(image.getRGB(x, y));
            }
        }
    }

    /**
     * Tests the {@code FramebufferY8.copyToBuffer} method by comparing its
     * output to that of the original implementation in its superclass.
     *
     * @param bitsPerPixel the number of bits per pixel in the output buffer
     * @param bytesPerPixel the number of bytes per pixel in the output buffer
     */
    private void copyTest(int bitsPerPixel, int bytesPerPixel) {
        var oldSource = new FramebufferY8SuperShim(bb, width, height, bitsPerPixel, true);
        var oldTarget = ByteBuffer.allocate(width * height * bytesPerPixel);
        oldSource.copyToBuffer(oldTarget);
        oldTarget.flip();

        var newSource = new FramebufferY8Shim(bb, width, height, bitsPerPixel, true);
        var newTarget = ByteBuffer.allocate(width * height * bytesPerPixel);
        newSource.copyToBuffer(newTarget);
        newTarget.flip();

        if (oldTarget.hasArray() && newTarget.hasArray()) {
            Assert.assertArrayEquals(oldTarget.array(), newTarget.array());
        } else {
            Assert.assertEquals(oldTarget, newTarget);
        }
    }

    /**
     * Tests the {@code FramebufferY8.write} method by comparing its output to
     * that of the original implementation in its superclass.
     *
     * @param bitsPerPixel the number of bits per pixel in the output buffer
     * @param bytesPerPixel the number of bytes per pixel in the output buffer
     * @throws IOException if an error occurs writing to an output channel
     */
    private void writeTest(int bitsPerPixel, int bytesPerPixel) throws IOException {
        var oldSource = new FramebufferY8SuperShim(bb, width, height, bitsPerPixel, true);
        var oldTarget = new ByteArrayOutputStream(width * height * bytesPerPixel);
        try (var oldChannel = Channels.newChannel(oldTarget)) {
            oldSource.write(oldChannel);
        }

        var newSource = new FramebufferY8Shim(bb, width, height, bitsPerPixel, true);
        var newTarget = new ByteArrayOutputStream(width * height * bytesPerPixel);
        try (var newChannel = Channels.newChannel(newTarget)) {
            newSource.write(newChannel);
        }

        Assert.assertArrayEquals(oldTarget.toByteArray(), newTarget.toByteArray());
    }

    /**
     * Tests copying to a buffer with 16-bit pixels in RGB565 format.
     */
    @Test
    public void copyTo16() {
        copyTest(Short.SIZE, Short.BYTES);
    }

    /**
     * Tests copying to a buffer with 32-bit pixels in ARGB32 format.
     */
    @Test
    public void copyTo32() {
        copyTest(Integer.SIZE, Integer.BYTES);
    }

    /**
     * Tests writing to a channel with 16-bit pixels in RGB565 format.
     */
    @Test
    public void writeTo16() throws IOException {
        writeTest(Short.SIZE, Short.BYTES);
    }

    /**
     * Tests writing to a channel with 32-bit pixels in ARGB32 format.
     */
    @Test
    public void writeTo32() throws IOException {
        writeTest(Integer.SIZE, Integer.BYTES);
    }
}
