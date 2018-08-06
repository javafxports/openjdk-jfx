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

public class OpenTypeGlyphMapper extends CharToGlyphMapper {

    PrismFontFile font;
    CMap cmap;
    CMap cmap14;
    int offset_format[] = {0}; // offset of format14

    public OpenTypeGlyphMapper(PrismFontFile font) {
        this.font = font;
        offset_format[0] = 0;
        try {
            cmap = CMap.initialize(font, offset_format, -1);
        } catch (Exception e) {
            cmap = null;
        }
        if (cmap == null) {
            handleBadCMAP();
        }
        missingGlyph = 0; /* standard for TrueType fonts */
    }

    public CMap createCMap14() {
        if (cmap14 == null && offset_format[0] != 0) {
            try {
                cmap14 = CMap.initialize(font, offset_format, 14);
                cmap14.setDefCMap(this.cmap);
            } catch (Exception e) {
                cmap14 = CMap.theNullCmap;
            }
            offset_format[0] = 0;
        }
        return cmap14;
    }

    public int getGlyphCode(int charCode, int vs) {
        if (vs == 0) {
            try {
                return cmap.getGlyph(charCode);
            } catch(Exception e) {
                handleBadCMAP();
                return missingGlyph;
            }
        } else if (createCMap14() != null) {
            try {
                return cmap14.getGlyph(charCode, vs);
            } catch(Exception e) {
                return missingGlyph;
            }
        }
        return missingGlyph;
    }

    private void handleBadCMAP() {
        // REMIND: Need to deregister ?
        cmap = CMap.theNullCmap;
    }

    /* A pretty good heuristic is that the cmap we are using
     * supports 32 bit character codes.
     */
    boolean hasSupplementaryChars() {
        return
            cmap instanceof CMap.CMapFormat8 ||
            cmap instanceof CMap.CMapFormat10 ||
            cmap instanceof CMap.CMapFormat12;
    }
}
