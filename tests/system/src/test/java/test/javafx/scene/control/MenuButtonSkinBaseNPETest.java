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

package test.javafx.scene.control;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.control.Menu;
import javafx.scene.control.MenuBar;
import javafx.scene.control.MenuItem;
import javafx.stage.Stage;
import javafx.stage.WindowEvent;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import test.util.Util;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class MenuButtonSkinBaseNPETest {
    static CountDownLatch startupLatch;
    static Menu menu;
    static MenuBar menuBar;

    public static void main(String[] args) throws Exception {
        initFX();
        try {
            var test = new MenuButtonSkinBaseNPETest();
            test.testMenuButtonNPE();
        } catch (Throwable e) {
            e.printStackTrace();
        } finally {
            tearDown();
        }
    }

    @Test
    public void testMenuButtonNPE() throws Exception {
        PrintStream defaultErrorStream = System.err;
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        System.setErr(new PrintStream(out,true));
        Thread.sleep(1000);
        Util.runAndWait(()-> {
            menu.hide();
            menuBar.getMenus().clear();
        });
        Thread.sleep(100);
        System.setErr(defaultErrorStream);
        Assert.assertEquals("No error should be thrown", "", out.toString());
    }

    public static class TestApp extends Application {

        @Override
        public void start(Stage primaryStage) throws Exception {
            menu = new Menu("Menu", null, new MenuItem("Press '_a'"));
            menuBar = new MenuBar(menu);
            Scene scene = new Scene(menuBar);
            primaryStage.addEventHandler(WindowEvent.WINDOW_SHOWN, event -> startupLatch.countDown());
            primaryStage.setScene(scene);
            primaryStage.show();
            menu.show();
        }
    }

    @BeforeClass
    public static void initFX() throws Exception {
        startupLatch = new CountDownLatch(1);
        new Thread(() -> Application.launch(TestApp.class, (String[])null)).start();
        Assert.assertTrue("Timeout waiting for FX runtime to start",
                startupLatch.await(15, TimeUnit.SECONDS));
    }

    @AfterClass
    public static void tearDown() {
        Platform.exit();
    }
}
