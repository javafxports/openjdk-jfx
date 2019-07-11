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

package test.javafx.scene;


import java.nio.ByteBuffer;
import java.nio.IntBuffer;

import javafx.geometry.Rectangle2D;
import javafx.scene.image.Image;
import javafx.scene.image.PixelBuffer;
import javafx.scene.image.PixelFormat;
import javafx.scene.image.WritableImage;
import javafx.util.Callback;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.fail;
import org.junit.Test;

public final class PixelBufferTest {
    static final int WIDTH = 10;
    static final int HEIGHT = 10;
    static final PixelFormat<ByteBuffer> byteBGRAPrePf = PixelFormat.getByteBgraPreInstance();
    static final ByteBuffer byteBuffer = ByteBuffer.allocateDirect(WIDTH * HEIGHT * 4);

    static final PixelFormat<IntBuffer> intARGBPrePf = PixelFormat.getIntArgbPreInstance();
    static final IntBuffer intBuffer = IntBuffer.allocate(WIDTH * HEIGHT);

    @Test
    public void testCreatePixelBufferByteBGRAPre() {
        PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, byteBGRAPrePf);
        assertEquals(WIDTH, pb.getWidth());
        assertEquals(HEIGHT, pb.getHeight());
        assertSame(byteBuffer, pb.getBuffer());
        assertSame(byteBGRAPrePf, pb.getPixelFormat());
    }

    @Test
    public void testCreatePixelBufferLessCapacityBuffer() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT + 1, byteBuffer, byteBGRAPrePf);
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreatePB_Width0() {
        try {
            PixelBuffer pb = new PixelBuffer(0, HEIGHT, byteBuffer, byteBGRAPrePf);
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreatePB_Height0() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, 0, byteBuffer, byteBGRAPrePf);
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreatePB_BufferNull() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, null, byteBGRAPrePf);
            fail("Expected NullPointerException");
        } catch (NullPointerException e) {
        }
    }

    @Test
    public void testCreatePB_PixelFormatNull() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, null);
            fail("Expected NullPointerException");
        } catch (NullPointerException e) {
        }
    }

    @Test
    public void testUpdateBuffer_CallBackValid() {
        PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, byteBGRAPrePf);
        Callback<PixelBuffer, Rectangle2D> callback = pixBuf -> {
            // Assuming this Callback modifies the buffer.
            return new Rectangle2D(1, 1, WIDTH - 1, HEIGHT - 1);
        };
        pb.updateBuffer(callback);
    }

    @Test
    public void testUpdateBuffer_CallBackNull() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, byteBGRAPrePf);
            pb.updateBuffer(null);
            fail("Expected NullPointerException");
        } catch (NullPointerException e) {
        }
    }

    @Test
    public void testCreatePixelBufferIntArgbPre() {
        PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, intBuffer, intARGBPrePf);
        assertEquals(WIDTH, pb.getWidth());
        assertEquals(HEIGHT, pb.getHeight());
        assertSame(intBuffer, pb.getBuffer());
        assertSame(intARGBPrePf, pb.getPixelFormat());
    }

    @Test
    public void testCreatePixelBufferIntArgb() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, intBuffer, PixelFormat.getIntArgbInstance());
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreatePixelBufferByteBgra() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, PixelFormat.getByteBgraInstance());
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreatePixelBufferByteRgb() {
        try {
            PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, PixelFormat.getByteRgbInstance());
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreateWritableImageUsingPB() {
        PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, byteBGRAPrePf);
        WritableImage image = new WritableImage(pb);
    }

    @Test
    public void testWritableImagePBgetPixelWriter() {
        PixelBuffer pb = new PixelBuffer(WIDTH, HEIGHT, byteBuffer, byteBGRAPrePf);
        WritableImage image = new WritableImage(pb);
        assertSame(null, image.getPixelWriter());
    }
}
