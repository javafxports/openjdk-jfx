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

package com.sun.scenario.effect.compiler.model;

public class Types {

    public static final Type VOID = new Type(BaseType.VOID, "void", 1);
    public static final Type FLOAT = new Type(BaseType.FLOAT, "float", 1);
    public static final Type FLOAT2 = new Type(BaseType.FLOAT, "float2", 2);
    public static final Type FLOAT3 = new Type(BaseType.FLOAT, "float3", 3);
    public static final Type FLOAT4 = new Type(BaseType.FLOAT, "float4", 4);
    public static final Type INT = new Type(BaseType.INT, "int", 1);
    public static final Type INT2 = new Type(BaseType.INT, "int2", 2);
    public static final Type INT3 = new Type(BaseType.INT, "int3", 3);
    public static final Type INT4 = new Type(BaseType.INT, "int4", 4);
    public static final Type BOOL = new Type(BaseType.BOOL, "bool", 1);
    public static final Type BOOL2 = new Type(BaseType.BOOL, "bool2", 2);
    public static final Type BOOL3 = new Type(BaseType.BOOL, "bool3", 3);
    public static final Type BOOL4 = new Type(BaseType.BOOL, "bool4", 4);
    public static final Type SAMPLER = new Type(BaseType.SAMPLER, "sampler", 1);
    public static final Type LSAMPLER = new Type(BaseType.SAMPLER, "lsampler", 1);
    public static final Type FSAMPLER = new Type(BaseType.SAMPLER, "fsampler", 1);
}
