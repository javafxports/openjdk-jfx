/*
 * Copyright (c) 2011, 2013, Oracle and/or its affiliates. All rights reserved.
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

package com.sun.javafx.font;

import com.sun.javafx.text.GlyphLayout;

/*
 * NB the versions that take a char as an int are used by the opentype
 * layout engine. If that remains in native these methods may not be
 * needed in the Java class.
 */
public abstract class CharToGlyphMapper {

    public static final int MISSING_GLYPH = 0;
    public static final int INVISIBLE_GLYPH_ID = 0xffff;

    public static final int SVS_START = 0xFE00;  // VS1
    public static final int SVS_END   = 0xFE0F;  // VS16
    public static final int IVS_START = 0xE0100; // VS17
    public static final int IVS_END   = 0xE01EF; // VS256
    public static final int FVS_START = 0x180B;  // FVS1
    public static final int FVS_END   = 0x180D;  // FVS3

    protected int missingGlyph = MISSING_GLYPH;

    public static boolean isVS(int code) {
        return (isIVS(code) || isSVS(code));
    }

    public static boolean isSVS(int code) {
        return (code >= SVS_START && code <= SVS_END);
    }

    public static boolean isIVS(int code) {
        return (code >= IVS_START && code <= IVS_END);
    }

    public boolean canDisplay(char cp) {
        int glyph = charToGlyph(cp);
        return glyph != missingGlyph;
    }

    public int getMissingGlyphCode() {
        return missingGlyph;
    }

    public abstract int getGlyphCode(int charCode, int vs);

    public final int charToGlyph(char unicode) {
        return getGlyphCode(unicode, (char)0);
    }

    public final int charToGlyph(int unicode) {
        return getGlyphCode(unicode, 0);
    }

    public int charToGlyph(char unicode, char vs) {
        return getGlyphCode(unicode, vs);
    }

    public int charToGlyph(int unicode, int vs) {
        return getGlyphCode(unicode, vs);
    }

    public void charsToGlyphs(int start, int count, char[] unicodes,
                              int[] glyphs, int glyphStart) {

        /* implement following patterns
         * (A) Normal char  (All chars except SurrogatePair, IVS, SVS)
         * (B) Surrogate_high + Surrogate_low
         *
         * (C) CJK + IVS_high + IVS_low
         * (D) IVS_high + IVS_low  (IVS only, not CJK + IVS)
         * (E) Surrogate_high + Surrogate_low + IVS_high + IVS_low
         *
         * (F) CJK + SVS
         * (G) SVS  (SVS only, not CJK + SVS)
         * (H) Surrogate_high + Surrogate_low + SVS
         */
        int prevSurrogate = 0; // store surrogate pair to handle (E)(H)
        for (int i=0; i<count; i++) {
            int st = start + i;
            int code = unicodes[st]; // char is unsigned.
            boolean isSURROGATE = false;

            if (Character.isHighSurrogate(unicodes[st]) &&
                i + 1 < count && Character.isLowSurrogate(unicodes[st + 1])) {
                code = Character.toCodePoint(unicodes[st], unicodes[st + 1]);
                isSURROGATE = true;
            }

            if (isSURROGATE == false && isSVS(code) == false) {
                glyphs[glyphStart + i] = getGlyphCode(code, 0); // (A) ASCII etc
                prevSurrogate = 0;
            } else if (isSURROGATE && isIVS(code) == false) {
                glyphs[glyphStart + i] = getGlyphCode(code, 0); // (B) Surrogate
                prevSurrogate = code; // store surrogate pair
            } else { // == else if (isIVS || isSVS)
                int glSt;
                glSt = glyphStart + i;
                if (prevSurrogate == 0) {
                    if (i > 0 && GlyphLayout.isIdeographic(unicodes[st - 1])) {
                        glyphs[glSt - 1] =
                          getGlyphCode(unicodes[st - 1], code); // (C) (F) VS
                        glyphs[glSt] = INVISIBLE_GLYPH_ID;
                    } else {
                        glyphs[glSt] = getGlyphCode(code, 0); // (D) (G) VS only
                    }
                } else { // Surrogate + VS
                    glyphs[glSt - 2] =
                      getGlyphCode(prevSurrogate, code); // (E) (H)
                    glyphs[glSt - 1] = INVISIBLE_GLYPH_ID;
                    glyphs[glSt] = INVISIBLE_GLYPH_ID;
                    prevSurrogate = 0;
                }
            }

            if (isSURROGATE) {
                i += 1; // Empty glyph slot after surrogate
                glyphs[glyphStart + i] = INVISIBLE_GLYPH_ID;
                continue;
            }
        }
    }

    public void charsToGlyphs(int start, int count, char[] unicodes, int[] glyphs) {
        charsToGlyphs(start, count, unicodes, glyphs, 0);
    }

    public void charsToGlyphs(int count, char[] unicodes, int[] glyphs) {
        charsToGlyphs(0, count, unicodes, glyphs, 0);
    }

}
