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

import com.sun.glass.ui.monocle.EPDSettingsShim;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

/**
 * Provides test cases for the {@code EPDSettings} class.
 */
public class EPDSettingsTest {

    private static final String BITS_PER_PIXEL = "monocle.epd.bitsPerPixel";
    private static final String ROTATE = "monocle.epd.rotate";
    private static final String Y8_INVERTED = "monocle.epd.y8inverted";
    private static final String NO_WAIT = "monocle.epd.noWait";
    private static final String WAVEFORM_MODE = "monocle.epd.waveformMode";
    private static final String FLAG_ENABLE_INVERSION = "monocle.epd.enableInversion";
    private static final String FLAG_FORCE_MONOCHROME = "monocle.epd.forceMonochrome";
    private static final String FLAG_USE_DITHERING_Y1 = "monocle.epd.useDitheringY1";
    private static final String FLAG_USE_DITHERING_Y4 = "monocle.epd.useDitheringY4";

    private EPDSettingsShim settings;

    /**
     * Removes all of the EPD system properties. This method runs before each of
     * the test cases.
     */
    @Before
    public void initialize() {
        System.clearProperty(BITS_PER_PIXEL);
        System.clearProperty(ROTATE);
        System.clearProperty(Y8_INVERTED);
        System.clearProperty(NO_WAIT);
        System.clearProperty(WAVEFORM_MODE);
        System.clearProperty(FLAG_ENABLE_INVERSION);
        System.clearProperty(FLAG_FORCE_MONOCHROME);
        System.clearProperty(FLAG_USE_DITHERING_Y1);
        System.clearProperty(FLAG_USE_DITHERING_Y4);
    }

    /**
     * Tests the EPD system property for the frame buffer color depth.
     */
    @Test
    public void testBitsPerPixel() {
        System.setProperty(BITS_PER_PIXEL, "8");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.bitsPerPixel, 8);

        System.setProperty(BITS_PER_PIXEL, "16");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.bitsPerPixel, 16);

        System.setProperty(BITS_PER_PIXEL, "32");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.bitsPerPixel, 32);

        System.setProperty(BITS_PER_PIXEL, "64");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.bitsPerPixel, 32);
    }

    /**
     * Tests the EPD system property for the frame buffer rotation.
     */
    @Test
    public void testRotate() {
        System.setProperty(ROTATE, "0");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.rotate, 0);

        System.setProperty(ROTATE, "1");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.rotate, 1);

        System.setProperty(ROTATE, "2");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.rotate, 2);

        System.setProperty(ROTATE, "3");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.rotate, 3);

        System.setProperty(ROTATE, "4");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.rotate, 0);
    }

    /**
     * Tests the EPD system property for whether the 8-bit pixels of the frame
     * buffer are inverted. This property is ignored if the frame buffer is not
     * in the Y8 pixel format.
     */
    @Test
    public void testY8Inverted() {
        System.setProperty(Y8_INVERTED, "false");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.grayscale, 0);

        System.setProperty(Y8_INVERTED, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.grayscale, 0);

        System.setProperty(BITS_PER_PIXEL, "8");

        System.setProperty(Y8_INVERTED, "false");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.grayscale, 0x1);

        System.setProperty(Y8_INVERTED, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.grayscale, 0x2);
    }

    /**
     * Tests the EPD system property for whether to wait for the previous update
     * to complete before sending the next update.
     */
    @Test
    public void testNoWait() {
        System.setProperty(NO_WAIT, "false");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.noWait, false);

        System.setProperty(NO_WAIT, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.noWait, true);
    }

    /**
     * Tests the EPD system property for the update waveform mode.
     */
    @Test
    public void testWaveformMode() {
        System.setProperty(WAVEFORM_MODE, "1");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.waveformMode, 1);

        System.setProperty(WAVEFORM_MODE, "2");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.waveformMode, 2);

        System.setProperty(WAVEFORM_MODE, "3");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.waveformMode, 3);

        System.setProperty(WAVEFORM_MODE, "4");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.waveformMode, 4);

        System.setProperty(WAVEFORM_MODE, "5");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.waveformMode, 257);

        System.setProperty(WAVEFORM_MODE, "257");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.waveformMode, 257);
    }

    /**
     * Tests the EPD system property for whether to invert the pixels in
     * updates.
     */
    @Test
    public void testFlagEnableInversion() {
        System.setProperty(FLAG_ENABLE_INVERSION, "false");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0);

        System.setProperty(FLAG_ENABLE_INVERSION, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0x01);
    }

    /**
     * Tests the EPD system property for whether to convert all pixels to
     * monochrome (black or white) in updates.
     */
    @Test
    public void testFlagForceMonochrome() {
        System.setProperty(FLAG_FORCE_MONOCHROME, "false");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0);

        System.setProperty(FLAG_FORCE_MONOCHROME, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0x02);
    }

    /**
     * Tests the EPD system property for whether to dither the 8-bit grayscale
     * contents of the frame buffer to 1-bit black and white.
     */
    @Test
    public void testFlagUseDitheringY1() {
        System.setProperty(FLAG_USE_DITHERING_Y1, "false");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0);

        System.setProperty(FLAG_USE_DITHERING_Y1, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0x2000);
    }

    /**
     * Tests the EPD system property for whether to dither the 8-bit grayscale
     * contents of the frame buffer to 4-bit grayscale.
     */
    @Test
    public void testFlagUseDitheringY4() {
        System.setProperty(FLAG_USE_DITHERING_Y4, "false");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0);

        System.setProperty(FLAG_USE_DITHERING_Y4, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0x4000);
    }

    /**
     * Tests the EPD system properties to enable all EPD update flags.
     */
    @Test
    public void testAllFlags() {
        System.setProperty(FLAG_ENABLE_INVERSION, "true");
        System.setProperty(FLAG_FORCE_MONOCHROME, "true");
        System.setProperty(FLAG_USE_DITHERING_Y1, "true");
        System.setProperty(FLAG_USE_DITHERING_Y4, "true");
        settings = EPDSettingsShim.newInstance();
        Assert.assertEquals(settings.flags, 0x01 | 0x02 | 0x2000 | 0x4000);
    }
}
