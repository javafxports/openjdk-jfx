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
package test.robot.javafx.scene;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Orientation;
import javafx.scene.AccessibleAttribute;
import javafx.scene.control.Label;
import javafx.scene.control.ToolBar;
import javafx.scene.layout.Pane;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import javafx.scene.robot.Robot;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import javafx.stage.WindowEvent;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.*;

import test.util.Util;

public class ToolBarTest {

    private static final double TOOLBAR_SIZE = 300.0;
    private static final double CHILDREN_SIZE_TOO_BIG = 250.0;
    private static final double ORIGINAL_CHILDREN_SIZE = 100.0;

    private static final String ORIGINAL_STYLE = "-fx-border-width: 1; -fx-border-color: deepskyblue";
    private static final String MODIFIED_STYLE = "-fx-border-width: 1; -fx-border-color: red";

    static CountDownLatch startupLatch = new CountDownLatch(1);
    static Robot robot;

    static volatile Stage stage;
    static VBox root;
    static volatile Scene scene;
    private static final double SCENE_WIDTH = 300.0;
    private static final double SCENE_HEIGHT = SCENE_WIDTH;

    private ToolBar toolBar;
    private Label labelA;
    private Label labelB;

    @Test public void overflowShownInHorizontalAfterChildrenReziseTest() {
        Util.runAndWait(() -> toolBar.setOrientation(Orientation.HORIZONTAL));
        testOverflow();
    }

    @Test public void overflowShownInVerticalAfterChildrenReziseTest() {
        Util.runAndWait(() -> toolBar.setOrientation(Orientation.VERTICAL));
        testOverflow();
    }

    private void testOverflow() {
        assertOverflowNotShown();

        Util.runAndWait(() -> {
            setFixSize(labelA, CHILDREN_SIZE_TOO_BIG);
            labelA.setStyle(MODIFIED_STYLE);
        });

        assertOverflowShown();

        Util.runAndWait(() -> {
            setFixSize(labelA, ORIGINAL_CHILDREN_SIZE);
            labelA.setStyle(ORIGINAL_STYLE);
        });

        assertOverflowNotShown();
    }

    private void assertOverflowShown() {
        Pane pane = (Pane) toolBar.queryAccessibleAttribute(AccessibleAttribute.OVERFLOW_BUTTON);
        assertNotNull(pane);
        boolean visibleOverflow = pane.isVisible();
        assertTrue(visibleOverflow);
    }

    private void assertOverflowNotShown() {
        Pane pane = (Pane) toolBar.queryAccessibleAttribute(AccessibleAttribute.OVERFLOW_BUTTON);
        if(pane!=null) {
            boolean visibleOverflow = pane.isVisible();
            assertFalse(visibleOverflow);
        }
    }

    private Label createLabel() {
        Label label = new Label();
        setFixSize(label, ORIGINAL_CHILDREN_SIZE);
        label.setStyle(ORIGINAL_STYLE);
        return label;
    }

    private void setFixSize(Region region, double size) {
        region.setMinSize(Pane.USE_PREF_SIZE, Pane.USE_PREF_SIZE);
        region.setMaxSize(Pane.USE_PREF_SIZE, Pane.USE_PREF_SIZE);
        region.setPrefSize(size, size);
    }

    @After
    public void resetUI() {
        Util.runAndWait(() -> root.getChildren().clear());
    }

    @Before
    public void setupUI() {
        Util.runAndWait(() -> {
            toolBar = new ToolBar();

            labelA = createLabel();
            labelB = createLabel();
            toolBar.getItems().addAll(labelA, labelB);
            setFixSize(toolBar, TOOLBAR_SIZE);

            root.getChildren().add(toolBar);
        });
    }

    @BeforeClass
    public static void initFX() throws InterruptedException {
        new Thread(() -> Application.launch(ToolBarTest.TestApp.class, (String[])null)).start();
        waitForLatch(startupLatch, 10, "FX runtime failed to start.");
    }

    @AfterClass
    public static void exit() {
        Util.runAndWait(() -> stage.hide());
        Platform.exit();
    }

    public static class TestApp extends Application {
        @Override
        public void start(Stage primaryStage) {
            stage = primaryStage;
            root = new VBox();
            Scene scene = new Scene(root, SCENE_WIDTH, SCENE_HEIGHT);
            stage.setScene(scene);
            stage.initStyle(StageStyle.UNDECORATED);
            stage.addEventHandler(WindowEvent.WINDOW_SHOWN, e ->
                    Platform.runLater(startupLatch::countDown));
            stage.setAlwaysOnTop(true);
            stage.show();
        }
    }

    public static void waitForLatch(CountDownLatch latch, int seconds, String msg) throws InterruptedException {
        Assert.assertTrue("Timeout: " + msg, latch.await(seconds, TimeUnit.SECONDS));
    }
}
