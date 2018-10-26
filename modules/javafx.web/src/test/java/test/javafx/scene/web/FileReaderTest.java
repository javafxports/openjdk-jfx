/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

package test.javafx.scene.web;

import com.sun.javafx.webkit.FileReaderShim;
import com.sun.webkit.WebPage;
import com.sun.webkit.WebPageShim;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import javafx.concurrent.Worker.State;
import javafx.scene.web.WebEngineShim;

import static javafx.concurrent.Worker.State.SUCCEEDED;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import netscape.javascript.JSObject;
import org.junit.Before;
import org.junit.Test;

public class FileReaderTest extends TestBase {
    private final WebPage page = WebEngineShim.getPage(getEngine());
    private String[] fileList = {new File("src/test/resources/test/html/HelloWorld.txt").getAbsolutePath()};
    CountDownLatch latch;
    private State getLoadState() {
        return submit(() -> getEngine().getLoadWorker().getState());
    }

    private String getScriptString(String readAPI, String func, boolean abort) {
        String scriptContent = String.format("<script type=\"text/javascript\">" +
                                    "var result = null;" +
                                    "window.addEventListener('click', (e) => {" +
                                        "document.getElementById('file').click();" +
                                    "});" +
                                    "function ReadFile()" +
                                    "{" +
                                        "file = event.target.files[0];" +
                                        "var reader = new FileReader();" +
                                        "reader.onloadstart = function ()" +
                                        "{ %s" +
                                        "};" +
                                        "reader.onload = function ()" +
                                        "{" +
                                            "result = reader.result;" +
                                            "latch.countDown();" +
                                        "};" +
                                        "reader.onerror = function ()" +
                                        "{" +
                                            "result = reader.result;" +
                                            "latch.countDown();" +
                                        "};" +
                                        "reader." + readAPI + "(file" + func + ");" +
                                    "}" +
                               "</script>" +
                               "<body> <input type='file' id='file' onchange='ReadFile()'/></body>", (abort ? "reader.abort();" : ""));
        return scriptContent;
    }

    @Before
    public void before() {
        latch = new CountDownLatch(1);
        FileReaderShim.test_setChooseFiles(fileList);
    }

    private void testLatch(CountDownLatch latch) {
        assertTrue("Load task completed successfully", getLoadState() == SUCCEEDED);
        assertNotNull("Document should not be null", getEngine().getDocument());
        submit(() -> {
            final JSObject window = (JSObject) getEngine().executeScript("window");
            assertNotNull(window);
            window.setMember("latch", latch);
            WebPageShim.click(page, 0, 0);
        });

        try {
            latch.await();
        } catch (InterruptedException e) {
            throw new AssertionError(e);
        }
    }

    @Test public void testreadAsText() {
        loadContent(getScriptString("readAsText", "", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "Hello World", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_no_parameters() {
        loadContent(getScriptString("readAsText", ".slice()", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "Hello World", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_valid_parameters_1() {
        loadContent(getScriptString("readAsText", ".slice(3, 7)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "lo W", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_valid_parameters_2() {
        loadContent(getScriptString("readAsText", ".slice(3, file.length)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "lo World", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_valid_parameters_3() {
        loadContent(getScriptString("readAsText", ".slice(file.length)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "Hello World", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_valid_parameters_4() {
        loadContent(getScriptString("readAsText", ".slice(-7, file.length)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "o World", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_valid_parameters_5() {
        loadContent(getScriptString("readAsText", ".slice(file.length, 3)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "Hel", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_valid_parameters_6() {
        loadContent(getScriptString("readAsText", ".slice(file.length, -3)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "Hello Wo", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_valid_parameters_7() {
        loadContent(getScriptString("readAsText", ".slice(file.length, -100)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_single_parameter() {
        loadContent(getScriptString("readAsText", ".slice(6)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "World", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_negetive_parameter_1() {
        loadContent(getScriptString("readAsText", ".slice(-3)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "rld", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsText_slice_negetive_parameter_2() {
        loadContent(getScriptString("readAsText", ".slice(-3, -7)", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "", getEngine().executeScript("window.result"));
        });
    }

    @Test public void testreadAsBinaryString() throws FileNotFoundException, IOException {
        String binaryFile[] = {new File("src/test/resources/test/html/BinaryFile.dat").getAbsolutePath()};
        FileReaderShim.test_setChooseFiles(binaryFile);
        loadContent(getScriptString("readAsBinaryString", "", false));
        testLatch(latch);
        FileInputStream in = new FileInputStream(binaryFile[0]);
        final byte[] expectedBinaryData = in.readAllBytes();
        submit(() -> {
            final String obj = (String) getEngine().executeScript("window.result");
            final byte[] binBytes = obj.getBytes();
            for (int i = 0; i < expectedBinaryData.length; i++) {
                assertEquals("File Content matches", expectedBinaryData[i], binBytes[i]);
            }
        });
    }

    @Test public void testreadAsArrayBuffer() throws FileNotFoundException, IOException {
        loadContent(getScriptString("readAsArrayBuffer", "", false));
        testLatch(latch);
        FileInputStream in = new FileInputStream(fileList[0]);
        final byte[] expectedArrayBuffer = in.readAllBytes();
        submit(() -> {
            final JSObject obj = (JSObject) getEngine().executeScript("new Uint8Array(window.result)");
            assertEquals("Expected File Content matches", expectedArrayBuffer.length, 11);
            assertEquals("File Content matches", obj.getMember("length"), 11);
            for (int i = 0; i < expectedArrayBuffer.length; i++) {
                assertEquals("File Content matches", expectedArrayBuffer[i], ((Number)(obj.getSlot(i))).byteValue());
            }
        });
    }

    @Test public void testreadAsDataURL() {
        loadContent(getScriptString("readAsDataURL", "", false));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", "data:text/plain;base64,SGVsbG8gV29ybGQ=",
                          getEngine().executeScript("window.result"));
        });
    }

    @Test public void testAbort() {
        loadContent(getScriptString("readAsText", "", true));
        testLatch(latch);
        submit(() -> {
            assertEquals("File Content matches", null,
                          getEngine().executeScript("window.result"));
        });
    }
}
