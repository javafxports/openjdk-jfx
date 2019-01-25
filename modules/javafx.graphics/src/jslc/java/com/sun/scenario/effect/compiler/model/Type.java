/*
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

import java.util.List;
import java.util.Locale;
import java.util.Objects;

/**
 */
public class Type {
    private final BaseType baseType;
    private final String name;
    private final int numFields;

    protected Type(BaseType baseType, String name, int numFields) {
        this.baseType = baseType;
        this.name = name;
        this.numFields = numFields;
    }

    public BaseType getBaseType() {
        return baseType;
    }

    public int getNumFields() {
        return numFields;
    }

    public boolean isVector() {
        return numFields > 1;
    }

    public boolean isMatrix() {
        return false;
    }

    public static List<Type> types() {
        return List.of(Types.VOID, Types.FLOAT, Types.FLOAT2, Types.FLOAT3, Types.FLOAT4, Types.INT, Types.INT2,
                Types.INT3, Types.INT4, Types.BOOL, Types.BOOL2, Types.BOOL3, Types.BOOL4, Types.SAMPLER,
                Types.LSAMPLER, Types.FSAMPLER);
    }

    /**
     * Returns a {@code Type} instance given a lowercase token string.
     * For example, given "float3", this method will return {@code Type.FLOAT3}.
     */
    public static Type fromToken(String s) {
        switch (s) {
            case "void":
                return Types.VOID;
            case "float":
                return Types.FLOAT;
            case "float2":
                return Types.FLOAT2;
            case "float3":
                return Types.FLOAT3;
            case "float4":
                return Types.FLOAT4;
            case "int":
                return Types.INT;
            case "int2":
                return Types.INT2;
            case "int3":
                return Types.INT3;
            case "int4":
                return Types.INT4;
            case "bool":
                return Types.BOOL;
            case "bool2":
                return Types.BOOL2;
            case "bool3":
                return Types.BOOL3;
            case "bool4":
                return Types.BOOL4;
            case "sampler":
                return Types.SAMPLER;
            case "lsampler":
                return Types.LSAMPLER;
            case "fsampler":
                return Types.FSAMPLER;
            default:
                throw new IllegalArgumentException("could not get type from token: " + s);
        }
    }

    @Override
    public String toString() {
        return name.toLowerCase(Locale.ENGLISH);
    }

    @Override
    public boolean equals(Object object) {
        if (object == null || getClass() != object.getClass()) {
            return false;
        }

        final Type other = (Type) object;

        return baseType == other.baseType &&
                name.equals(other.name) &&
                numFields == other.numFields;
    }

    @Override
    public int hashCode() {
        return Objects.hash(baseType, name, numFields);
    }
}
