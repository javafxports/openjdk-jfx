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

/**
 */
public enum BinaryOpType {

    ADD  ("+",  Op.MATH),
    SUB  ("-",  Op.MATH),
    MUL  ("*",  Op.MATH),
    DIV  ("/",  Op.MATH),
    MOD  ("%",  Op.MATH),
    EQ   ("=",  Op.ASSIGN),
    ADDEQ("+=", Op.ASSIGN),
    SUBEQ("-=", Op.ASSIGN),
    MULEQ("*=", Op.ASSIGN),
    DIVEQ("/=", Op.ASSIGN),
    MODEQ("%=", Op.ASSIGN),
    OR   ("||", Op.REL),
    XOR  ("^^", Op.REL),
    AND  ("&&", Op.REL),
    EQEQ ("==", Op.REL),
    NEQ  ("!=", Op.REL),
    LTEQ ("<=", Op.REL),
    GTEQ (">=", Op.REL),
    LT   ("<",  Op.REL),
    GT   (">",  Op.REL),
    NOT  ("~",  Op.BIT),
    LSHFT("<<", Op.BIT),
    RSHFT(">>", Op.BIT),
    LAND ("&",  Op.BIT),
    LOR  ("|", Op.BIT),
    LXOR ("^", Op.BIT),
    LSEQ ("<<=", Op.BIT),
    RSEQ (">>=", Op.BIT),
    ANDEQ("&=", Op.BIT),
    OREQ ("|=", Op.BIT),
    XOREQ("^=", Op.BIT);

    // FIXME: How to represent BITwise operators are only enabled in shader model 4.0+ ?? Maybe we dont have to
    // at this level.
    // Bitwise operators are defined to operate only on int and uint data types
    // Unlike short-circuit evaluation of &&, ||, and ?: in C, HLSL expressions never short-circuit
    // an evaluation because they are vector operations. All sides of the expression are always evaluated.
    private enum Op { MATH, ASSIGN, REL, BIT }
    private String symbol;
    private Op op;

    BinaryOpType(String symbol, Op op) {
        this.symbol = symbol;
        this.op = op;
    }

    public static BinaryOpType forSymbol(String symbol) {
        for (BinaryOpType ot : BinaryOpType.values()) {
            if (ot.getSymbol().equals(symbol)) {
                return ot;
            }
        }
        return null;
    }

    public String getSymbol() {
        return symbol;
    }

    public boolean isRelational() {
        return op == Op.REL;
    }

    public boolean isAssignment() {
        return op == Op.ASSIGN;
    }

    @Override
    public String toString() {
        return symbol;
    }
}
