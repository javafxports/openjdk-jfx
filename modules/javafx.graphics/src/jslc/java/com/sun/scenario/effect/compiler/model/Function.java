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

import java.util.Collections;
import java.util.List;
import java.util.Objects;

/**
 */
public class Function {

    private final String name;
    private final Type returnType;
    private final List<Param> params;

    /**
     * Whether or not this function was defined by the user's program (as opposed
     * to an intrinsic, builtin function). Defaults to {@literal false}.
     */
    private final boolean userFunc;

    Function(String name, Type returnType, List<Param> params) {
        this(name, returnType, params, false);
    }

    Function(String name, Type returnType, List<Param> params, boolean userFunc) {
        this.name = name;
        this.returnType = returnType;
        if (params != null) {
            this.params = params;
        } else {
            this.params = Collections.emptyList();
        }
        this.userFunc = userFunc;
    }

    public String getName() {
        return name;
    }

    public Type getReturnType() {
        return returnType;
    }

    public List<Param> getParams() {
        return params;
    }

    public boolean isUserFunc() {
        return userFunc;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Function other = (Function) obj;
        // Equality should not care whether or not this is a builtin or user function
        // so that we can check if a function has been declared via:
        // funcSet.contains(new Function(name, returnType, params))
        return Objects.equals(name, other.name) &&
                returnType == other.returnType &&
                Objects.equals(params, other.params);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name, returnType, params);
    }

    @Override
    public String toString() {
        return String.format("Function [name = %s, returnType = %s, params = %s]", name, returnType, params);
    }
}
