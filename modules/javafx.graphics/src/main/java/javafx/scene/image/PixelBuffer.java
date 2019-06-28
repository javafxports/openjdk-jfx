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
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Objects;

/**
 * A class to provide a shared pixel data using {@link java.nio.Buffer} for {@code WritableImage}s.
 * {@code PixelBuffer} supports only {@code PixelFormat.INT_ARGB_PRE}
 * and {@code PixelFormat.BYTE_BGRA_PRE} pixel formats.
 * A {@code PixelBuffer} can only be drawn using a {@code WritableImage}.
 * Multiple {@code WritableImage}s and applications can share a {@code PixelBuffer}.
 * <p>
 * Pseudo code to create {@code PixelBuffer}:
 * <pre>{@code
 *
 * // Creating a PixelBuffer using PixelFormat.BYTE_BGRA_PRE
 * ByteBuffer byteBuffer = ByteBuffer.allocateDirect(width * height * 4);
 * PixelFormat<ByteBuffer> pixelFormat = PixelFormat.getByteBgraPreInstance();
 * PixelBuffer pixelBuffer = new PixelBuffer(width, height, byteBuffer, pixelFormat);
 * Image img = new WritableImage(pixelBuffer);
 *
 * // Creating a PixelBuffer using PixelFormat.INT_ARGB_PRE
 * IntBuffer intBuffer = IntBuffer.allocate(width * height);
 * PixelFormat<IntBuffer> pixelFormat = PixelFormat.getIntArgbPreInstance();
 * PixelBuffer pixelBuffer = new PixelBuffer(width, height, intBuffer, pixelFormat);
 * Image img = new WritableImage(pixelBuffer);
 * }</pre>
 *
 * @since 13
 */
public class PixelBuffer<T extends Buffer> {

    private final T buffer;
    private final int width;
    private final int height;
    private final PixelFormat pixelFormat;
    private ArrayList<WeakReference<WritableImage>> imageRefs = new ArrayList<>();

    /**
     * Construct a {@code PixelBuffer} from the provided {@link java.nio.Buffer} and {@link PixelFormat}.
     * The constructor does not allocate memory to store the pixel data, application
     * must allocate sufficient memory required for the dimensions(x, y) and {@code PixelFormat}.
     * {@code PixelFormat} must be either {@code PixelFormat.INT_ARGB_PRE} or
     * {@code PixelFormat.BYTE_BGRA_PRE}, otherwise an exception will be thrown.
     *
     * @param width       width dimension of the {@code PixelBuffer}.
     * @param height      height dimension of the {@code PixelBuffer}.
     * @param buffer      {@code java.nio.Buffer} which stores the pixel data.
     * @param pixelFormat the {@code PixelFormat} of pixels in the provided {@code buffer}.
     * @throws IllegalArgumentException if either {@code pixelBuffer} dimension
     *                                  is negative or zero or if {@code pixelFormat}
     *                                  is unsupported or if the  {@code buffer} does
     *                                  not have sufficient memory.
     * @throws NullPointerException     if {@code buffer} is {@code null}.
     */
    public PixelBuffer(int width, int height, T buffer, PixelFormat pixelFormat) {
        Objects.requireNonNull(buffer, "A valid buffer must be provided.");
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("PixelBuffer dimensions must be positive (w,h > 0)");
        }
        if (pixelFormat.getType() != PixelFormat.getByteBgraPreInstance().getType() &&
                pixelFormat.getType() != PixelFormat.getIntArgbPreInstance().getType()) {
            throw new IllegalArgumentException("Unsupported PixelFormat: " + pixelFormat.getType());
        }
        if ((pixelFormat.getType() == PixelFormat.getByteBgraPreInstance().getType()
                && buffer.capacity() < width * height * 4) ||
                (pixelFormat.getType() == PixelFormat.getIntArgbPreInstance().getType()
                && buffer.capacity() < width * height)) {
            throw new IllegalArgumentException("Insufficient memory allocated for buffer.");
        }
        this.buffer = buffer;
        this.width = width;
        this.height = height;
        this.pixelFormat = pixelFormat;
    }

    /**
     * Returns the {@code java.nio.Buffer} of this {@code PixelBuffer}.
     *
     * @return the {@code java.nio.Buffer} of this {@code PixelBuffer}.
     */
    public T getBuffer() {
        return buffer;
    }

    /**
     * Returns the width dimension of {@code PixelBuffer}.
     *
     * @return the width dimension of this {@code PixelBuffer}.
     */
    public int getWidth() {
        return width;
    }

    /**
     * Returns the height dimension of {@code PixelBuffer}.
     *
     * @return the height dimension of this {@code PixelBuffer}.
     */
    public int getHeight() {
        return height;
    }

    /**
     * Returns the {@code PixelFormat} of the pixels in the {@code java.nio.Buffer}.
     *
     * @return the {@code PixelFormat} of the pixels in the {@code java.nio.Buffer}.
     */
    public PixelFormat getPixelFormat() {
        return pixelFormat;
    }

    /**
     * Updates the pixel data in {@code java.nio.Buffer} using provided {@link javafx.util.Callback}.
     * This method must be called on the JavaFX Application thread.
     * The {@code callback} method should update the {@code java.nio.Buffer} and
     * return a {@code Rectangle2D} which encloses the modified region, or
     * {@code null} to indicate that entire buffer is dirty.
     * Pseudo-code how to use this method:
     * <pre>{@code
     * Callback<PixelBuffer, Rectangle2D> callback = pixelBuffer -> {
     *     ByteBuffer buf = ((PixelBuffer<ByteBuffer>) pixelBuffer).getBuffer();
     *     // Update the buffer.
     *     return new Rectangle2D(x, y, width, height);
     * };
     * pixelBuffer.updateBuffer(callback);
     * }</pre>
     *
     * @param callback the {@link javafx.util.Callback} method which updates the {@code java.nio.Buffer}.
     * @throws IllegalStateException if this method is not being called on the JavaFX Application Thread.
     * @throws NullPointerException  if {@code callback} is {@code null}.
     **/
    public void updateBuffer(Callback<PixelBuffer, Rectangle2D> callback) {
        Objects.requireNonNull(callback, "callback must be specified.");
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
            WeakReference<WritableImage> imageRef = iter.next();
            if (imageRef.get() != null) {
                imageRef.get().bufferDirty(rect);
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
