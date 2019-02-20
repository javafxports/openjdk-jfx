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

package com.sun.prism.es2;

/**
 * TODO: 3D - Need documentation
 */
class ES2Light {

    float x, y, z = 0;
    float r, g, b, w = 1;
	float ca, la, qa = 1;
	float range = 1;

    ES2Light(float ix, float iy, float iz, float ir, float ig, float ib, float iw) {
		this(ix, iy, iz, ir, ig, ib, iw, 1, 1, 1, 1);
    }
	
	ES2Light(float ix, float iy, float iz, float ir, float ig, float ib, float iw, float irange, float ica, float ila, float iqa) {
        x = ix;
        y = iy;
        z = iz;
        r = ir;
        g = ig;
        b = ib;
        w = iw;
		range = irange;
		ca = ica;
		la = ila;
		qa = iqa;
    }
}
