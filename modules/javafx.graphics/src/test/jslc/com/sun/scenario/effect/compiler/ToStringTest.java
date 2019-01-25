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

package com.sun.scenario.effect.compiler;

import java.io.File;
import java.util.Map;
import java.util.stream.Collectors;

import com.sun.scenario.effect.compiler.JSLC.JSLCInfo;
import com.sun.scenario.effect.compiler.tree.ProgramUnit;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ToStringTest {

    public ToStringTest() {
    }

    static JSLC.ParserInfo compile(String s) throws Exception {
        File tmpfile = File.createTempFile("foo", null);
        File tmpdir = tmpfile.getParentFile();
        JSLCInfo jslcinfo = new JSLCInfo();
        jslcinfo.outDir = tmpdir.getAbsolutePath();
        jslcinfo.shaderName = "Effect";
        jslcinfo.peerName = "Foo";
        jslcinfo.outTypes = JSLC.OUT_ALL;
        return JSLC.compile(jslcinfo, s, Long.MAX_VALUE);
    }

    @Test
    public void toStringTest() throws Exception {
        String s =
                "void main() {\n" +
                        "    float funcres = fma(1.5, 3.0, 5.0);\n" +
                        "}\n";
        JSLC.ParserInfo info = compile(s);
        assertSame(s, info.program);
    }

    private void assertSame(String program, ProgramUnit programUnit) {
        // We want to compare line-by-line but ignoring leading/trailing white space on each line.
        int[] num = {0};
        Map<Integer, String> expectedLines = program.lines().map(line -> new Line(num[0]++, line.strip()))
                .collect(Collectors.toMap(Line::getNumber, Line::getLine));
        num[0] = 0;
        Map<Integer, String> actualLines = programUnit.toString().lines().map(line -> new Line(num[0]++, line.strip()))
                .collect(Collectors.toMap(Line::getNumber, Line::getLine));
        assertEquals("the two programs to be identical (minus whitespace)", expectedLines, actualLines);
    }

    private static class Line {
        private final int number;
        private final String line;

        private Line(int number, String line) {
            this.number = number;
            this.line = line;
        }

        private int getNumber() {
            return number;
        }

        private String getLine() {
            return line;
        }
    }
}
