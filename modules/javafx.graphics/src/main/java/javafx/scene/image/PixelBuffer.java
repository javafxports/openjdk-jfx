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

import java.lang.ref.WeakReference;
import java.nio.Buffer;
import java.util.ArrayList;

import com.sun.javafx.geom.Rectangle;
import com.sun.javafx.tk.Toolkit;
import javafx.geometry.Rectangle2D;
import javafx.util.Callback;

/**
 * A class to store pixel data using NIO Buffer. The format of the pixels in the
 * buffer is defined by the PixelFormat.
 * A PixelBuffer can be shared by multiple {@code WritableImages}. The buffer
 * can be updated by different applications on Java or native side.
 * A PixelBuffer can only be used with a WritableImage.
 * Pseudo code to create PixelBuffer:
 * <pre>{@code
 * int width = 100;
 * int height = 100;
 * ByteBuffer bf = ByteBuffer.allocateDirect(width * height * 4);
 * PixelFormat<ByteBuffer> pf = PixelFormat.getByteBgraPreInstance();
 * PixelBuffer pixelBuffer = new PixelBuffer(w, h, byteBuffer, pf);
 * Image img = new WritableImage(pixelBuffer);
 * }</pre>
 *
 *  TBD - more
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
     * Constructs a PixelBuffer from provided buffer.
     * The constructor does not allocate memory to store pixels. User must
     * allocate memory as required for the dimensions and {@code PixelFormat}.
     *
     * @param width width dimension of the PixelBuffer
     * @param height height dimension of the PixelBuffer
     * @param buffer NIO Buffer which stores the pixel data.
     * @param pixelFormat the PixelFormat object defining the format of the pixels in the buffer
     */
    public PixelBuffer(int width, int height, T buffer, PixelFormat pixelFormat) {
        this.buffer = buffer;
        this.width = width;
        this.height = height;
        this.pixelFormat = pixelFormat;
    }

    /**
     * Returns the actual {@code Buffer} which stores the pixel data.
     *
     * @return the {@code Buffer} of this {@code PixelBuffer}.
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
     * Returns the {@code PixelFormat} in which the buffer stores its pixels.
     *
     * @return the {@code PixelFormat} that describes the underlying pixels.
     */
    public PixelFormat getPixelFormat() {
        return pixelFormat;
    }

    /**
     * Updates the PixelBuffer using provided callback method.
     * <p>
     * This method must be called on the JavaFX Application thread. This method
     * must be provided with a Callback of type {@code Callback<PixelBuffer, Rectangle2D>}.
     * The callback method should update the Buffer and return the {@code Rectangle2D}
     * which encloses the modified part of image.
     * </p>
     * Pseudo-code how to use this method:
     * <pre>{@code
     * pixelBuffer.updateBuffer(pixelBuffer -> {
     *     ByteBuffer buffer = ((PixelBuffer<ByteBuffer>) pixelBuffer).getBuffer();
     *     // Update the buffer.
     *     return new Rectangle2D(x, y, width, height);
     * });
     * }</pre>
     *
     * @param callback the callback method which updates the buffer.
     *
     * @throws IllegalStateException If this method is called on a thread
     *         other than the JavaFX Application Thread.
     * @throws NullPointerException If {@code callback} is null or if
     *         {@code callback} returns null.
     *
     **/
    public void updateBuffer(Callback<PixelBuffer, Rectangle2D> callback) {
        if (callback == null) {
            throw new NullPointerException("callback must be specified");
        }
        Toolkit.getToolkit().checkFxUserThread();
        Rectangle2D rect2D = callback.call(this);
        if (rect2D == null) {
            throw new NullPointerException("callback must return a valid Rectangle2D");
        }
        Rectangle rect = new Rectangle((int) rect2D.getMinX(), (int) rect2D.getMinY(),
                (int) rect2D.getWidth(), (int) rect2D.getHeight());
        boolean clean = false;
        for (WeakReference<WritableImage> imageRef : imageRefs) {
            if (imageRef.get() != null) {
                imageRef.get().bufferDirty(rect);
            } else {
                clean = true;
            }
        }
        if (clean) {
            cleanImageRefs();
        }
    }

    void addImage(WritableImage image) {
        imageRefs.add(new WeakReference<>(image));
        cleanImageRefs();
    }

    private void cleanImageRefs() {
        WeakReference<WritableImage>[] refsToRemove = new WeakReference[imageRefs.size()];
        int count = 0;
        for (WeakReference<WritableImage> imageRef : imageRefs) {
            if (imageRef.get() == null) {
                refsToRemove[count++] = imageRef;
            }
        }
        for (int i = 0; i < count; i++) {
            imageRefs.remove(refsToRemove[i]);
        }
    }

    /**
     * TBD:
     * 1. The current updateBuffer method is synchronous, consider an asynchronous flavor of the method.
     * 2. Support for all Image formats.
     * 3. Image can be updated using PixelWriter on any thread, The behavior should be discussed.
     */

}
