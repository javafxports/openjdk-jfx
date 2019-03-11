/*
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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

package com.sun.javafx.sg.prism;

/**
 * TODO: 3D - Need documentation
 */
public class NGPointLight extends NGLightBase {

    private static final double DEFAULT_C = 1;
    private static final double DEFAULT_LC = 0;
    private static final double DEFAULT_QC = 0;
    private static final double DEFAULT_RANGE = Double.POSITIVE_INFINITY;

    public static double getDefaultC() {
        return DEFAULT_C;
    }

    public static double getDefaultLc() {
        return DEFAULT_LC;
    }

    public static double getDefaultQc() {
        return DEFAULT_QC;
    }

    public static double getDefaultRange() {
        return DEFAULT_RANGE;
    }

    private double c = DEFAULT_C;

    public double getC() {
        return c;
    }

    public void setC(double c) {
        this.c = c;
        visualsChanged();
    }


    private double lc = DEFAULT_LC;

    public double getLc() {
        return lc;
    }

    public void setLc(double lc) {
        this.lc = lc;
        visualsChanged();
    }


    private double qc = DEFAULT_QC;

    public double getQc() {
        return qc;
    }

    public void setQc(double qc) {
        this.qc = qc;
        visualsChanged();
    }


    private double range = DEFAULT_RANGE;

    public double getRange() {
        return range;
    }

    public void setRange(double range) {
        this.range = range < 0 ? 0 : range;
        visualsChanged();
    }

    public NGPointLight() {
    }

}
