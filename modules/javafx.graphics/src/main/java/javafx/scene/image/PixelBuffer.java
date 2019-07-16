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

package javafx.scene.image;

import com.sun.javafx.geom.Rectangle;
import com.sun.javafx.tk.Toolkit;
import javafx.geometry.Rectangle2D;
import javafx.util.Callback;

import java.lang.ref.WeakReference;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.LinkedList;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;

/**
 * The {@code PixelBuffer} class provides a way to share the provided {@link java.nio.Buffer}
 * as pixel data among multiple {@link WritableImage}s.
 * Pixel data should be stored either in an {@link java.nio.IntBuffer} using
 * {@code INT_ARGB_PRE} {@code PixelFormat} or in a {@link java.nio.ByteBuffer} using
 * {@code BYTE_BGRA_PRE} {@code PixelFormat}. When the {@link java.nio.Buffer} is updated
 * using the API {@code PixelBuffer.updateBuffer()}, all the {@code WritableImage}s that were
 * created using the this {@code PixelBuffer} get redrawn.
 *
 * <p>
 * Example code that shows how to create a {@code PixelBuffer}:
 * <pre>{@code
 *
 * // Creating a PixelBuffer using BYTE_BGRA_PRE pixel format.
 * ByteBuffer byteBuffer = ByteBuffer.allocateDirect(width * height * 4);
 * PixelFormat<ByteBuffer> pixelFormat = PixelFormat.getByteBgraPreInstance();
 * PixelBuffer<ByteBuffer> pixelBuffer = new PixelBuffer<>(width, height, byteBuffer, pixelFormat);
 * Image img = new WritableImage(pixelBuffer);
 *
 * // Creating a PixelBuffer using INT_ARGB_PRE pixel format.
 * IntBuffer intBuffer = IntBuffer.allocate(width * height);
 * PixelFormat<IntBuffer> pixelFormat = PixelFormat.getIntArgbPreInstance();
 * PixelBuffer<IntBuffer> pixelBuffer = new PixelBuffer<>(width, height, intBuffer, pixelFormat);
 * Image img = new WritableImage(pixelBuffer);
 * }</pre>
 *
 * @param <T> the type of {@link java.nio.Buffer} that should be either {@link java.nio.IntBuffer}
 *           or {@link java.nio.ByteBuffer}
 * @see WritableImage#WritableImage(PixelBuffer)
 * @since 13
 */
public class PixelBuffer<T extends Buffer> {

    private final T buffer;
    private final int width;
    private final int height;
    private final PixelFormat<T> pixelFormat;
    private final List<WeakReference<WritableImage>> imageRefs;

    /**
     * Constructs a {@code PixelBuffer} using the provided {@link java.nio.Buffer} and {@link PixelFormat}.
     * The type of specified {@code PixelFormat} must be either {@code PixelFormat.Type.INT_ARGB_PRE} or
     * {@code PixelFormat.Type.BYTE_BGRA_PRE}. Any other type of {@code PixelFormat} will cause an exception to be thrown.
     * The constructor does not allocate memory to store the pixel data. The application must allocate
     * sufficient memory required for the combination of dimensions(width, height) and {@code PixelFormat}.
     * {@code PixelFormat.Type.INT_ARGB_PRE} requires an {@link java.nio.IntBuffer} with minimum capacity of
     * (width * height) and  {@code PixelFormat.Type.BYTE_BGRA_PRE} requires a {@link java.nio.ByteBuffer}
     * with minimum capacity of (width * height * 4).
     *
     * @param width       width in pixels of this {@code PixelBuffer}
     * @param height      height in pixels of this {@code PixelBuffer}
     * @param buffer      {@link java.nio.Buffer} that stores the pixel data
     * @param pixelFormat {@code PixelFormat} of pixels in the {@code buffer}
     * @throws IllegalArgumentException if either {@code width} or {@code height}
     *                                  is negative or zero, or if {@code pixelFormat}
     *                                  is unsupported, or if the  {@code buffer} does
     *                                  not have sufficient memory, or if the type of {@code java.nio.Buffer}
     *                                  and {@code PixelFormat} do not match
     * @throws NullPointerException     if {@code buffer} or {@code pixelFormat} is {@code null}
     */
    public PixelBuffer(int width, int height, T buffer, PixelFormat<T> pixelFormat) {
        Objects.requireNonNull(buffer, "buffer must not be null.");
        Objects.requireNonNull(pixelFormat, "pixelFormat must not be null.");
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("PixelBuffer dimensions must be positive (w,h > 0)");
        }
        switch (pixelFormat.getType()) {
            case BYTE_BGRA_PRE:
                if (buffer.capacity() / width / 4 < height) {
                    throw new IllegalArgumentException("Insufficient memory allocated for ByteBuffer.");
                }
                if (!(buffer instanceof ByteBuffer)) {
                    throw new IllegalArgumentException("PixelFormat<ByteBuffer> requires a ByteBuffer.");
                }
                break;
            case INT_ARGB_PRE:
                if (buffer.capacity() / width < height) {
                    throw new IllegalArgumentException("Insufficient memory allocated for IntBuffer.");
                }
                if (!(buffer instanceof IntBuffer)) {
                    throw new IllegalArgumentException("PixelFormat<IntBuffer> requires an IntBuffer.");
                }
                break;
            default:
                throw new IllegalArgumentException("Unsupported PixelFormat: " + pixelFormat.getType());
        }
        this.buffer = buffer;
        this.width = width;
        this.height = height;
        this.pixelFormat = pixelFormat;
        this.imageRefs = new LinkedList<>();
    }

    /**
     * Returns the {@link java.nio.Buffer} of this {@code PixelBuffer}.
     *
     * @return the {@link java.nio.Buffer} of this {@code PixelBuffer}
     */
    public T getBuffer() {
        return buffer;
    }

    /**
     * Returns the width of this {@code PixelBuffer}.
     *
     * @return the width of this {@code PixelBuffer}
     */
    public int getWidth() {
        return width;
    }

    /**
     * Returns the height of this {@code PixelBuffer}.
     *
     * @return the height of this {@code PixelBuffer}
     */
    public int getHeight() {
        return height;
    }

    /**
     * Returns the {@code PixelFormat} of this {@code PixelBuffer}.
     *
     * @return the {@code PixelFormat} of this {@code PixelBuffer}
     */
    public PixelFormat getPixelFormat() {
        return pixelFormat;
    }

    /**
     * Updates the pixel data in the {@link java.nio.Buffer} using the provided {@link javafx.util.Callback}.
     * This method must be called on the JavaFX Application thread.
     * The {@code callback} method should update the {@link java.nio.Buffer} and
     * return a {@code Rectangle2D} which encloses the modified region, or
     * {@code null} to indicate that entire buffer is dirty.
     * <p>
     * Example code that shows how to use this method:
     * <pre>{@code
     * Callback<PixelBuffer<ByteBuffer>, Rectangle2D> callback = pixelBuffer -> {
     *     ByteBuffer buffer = pixelBuffer.getBuffer();
     *     // Update the buffer.
     *     return new Rectangle2D(x, y, width, height);
     * };
     * pixelBuffer.updateBuffer(callback);
     * }</pre>
     *
     * @param callback the {@link javafx.util.Callback} method that updates the {@link java.nio.Buffer}
     * @throws IllegalStateException if this method is not called on the JavaFX Application Thread
     * @throws NullPointerException  if {@code callback} is {@code null}
     **/
    public void updateBuffer(Callback<PixelBuffer<T>, Rectangle2D> callback) {
        Objects.requireNonNull(callback, "callback must not be null.");
        Toolkit.getToolkit().checkFxUserThread();
        Rectangle2D rect2D = callback.call(this);
        Rectangle rect = null;
        if (rect2D != null) {
            rect = new Rectangle((int) rect2D.getMinX(), (int) rect2D.getMinY(),
                    (int) rect2D.getWidth(), (int) rect2D.getHeight());
        }
        bufferDirty(rect);
    }

    private void bufferDirty(Rectangle rect) {
        Iterator<WeakReference<WritableImage>> iter = imageRefs.iterator();
        while (iter.hasNext()) {
            final WritableImage image = iter.next().get();
            if (image != null) {
                image.bufferDirty(rect);
            } else {
                iter.remove();
            }
        }
    }

    void addImage(WritableImage image) {
        imageRefs.add(new WeakReference<>(image));
        imageRefs.removeIf(imageRef -> (imageRef.get() == null));
    }
}
