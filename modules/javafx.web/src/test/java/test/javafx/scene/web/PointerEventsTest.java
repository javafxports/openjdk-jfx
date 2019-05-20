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
package test.javafx.scene.web;

import java.io.File;
import static java.util.Arrays.asList;
import java.util.concurrent.CountDownLatch;
import javafx.event.EventType;
import javafx.scene.input.MouseButton;
import javafx.scene.input.MouseEvent;
import javafx.scene.web.WebView;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.application.Platform;
import org.junit.BeforeClass;
import com.sun.javafx.application.PlatformImpl;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.TimeUnit;
import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.concurrent.Worker.State;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class PointerEventsTest {

    @BeforeClass
    public static void setupOnce() {
        final CountDownLatch startupLatch = new CountDownLatch(1);

        PlatformImpl.startup(() -> {
            startupLatch.countDown();
        });

        try {
            startupLatch.await();
        } catch (InterruptedException ex) {
        }
    }

    /**
     * This test loads a website with SVG shapes "polyline", "path", "rect",
     * "circle", "ellipse", "polygon". By setting the CSS property
     * "pointer-events" to "stroke", the rendering engine is configured to only
     * react to clicks on the stroke of the shapes.
     *
     * <p>With a javascript event click handler, mouse events on the shapes
     * are recorded.</p>
     *
     * <p>This tests simulates clicks on the stroke, so the shapes should have
     * received clicks and report being activated.</p>
     */
    @Test
    public void testClickOnStrokePointerEventsStroke() throws Exception {
        WebView[] viewHolder = new WebView[1];
        CountDownLatch waitTestDocumentLoaded = new CountDownLatch(1);

        Platform.runLater(() -> {
            final Stage primaryStage = new Stage();
            final WebView view = new WebView();
            viewHolder[0] = view;
            primaryStage.setScene(new Scene(view, 800, 600));
            primaryStage.show();

            view.getEngine().getLoadWorker().stateProperty().addListener(new ChangeListener<State>() {
                @Override
                public void changed(ObservableValue<? extends State> ov, State t, State t1) {
                    if(t1 == State.SUCCEEDED) {
                        waitTestDocumentLoaded.countDown();
                    }
                }
            });

            String url = new File("src/test/resources/test/html/pointerevents-stroke.html").toURI().toASCIIString();
            view.getEngine().load(url);
        });

        waitTestDocumentLoaded.await();

        CountDownLatch resultMapDone = new CountDownLatch(1);
        final Map<String,Boolean> resultMap = new HashMap<>();

        Platform.runLater(() -> {
            WebView view = viewHolder[0];

            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 130, 80));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 130, 80));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 330, 80));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 330, 80));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 530, 80));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 530, 80));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 130, 280));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 130, 280));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 330, 280));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 330, 280));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 530, 280));
            view.fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 530, 280));

            for (String s : asList("polyline", "path", "rect", "circle", "ellipse", "polygon")) {
                resultMap.put(s, (boolean) view.getEngine().executeScript("isActivated(\"" + s + "\")"));
            }

            resultMapDone.countDown();
        });

        resultMapDone.await(20, TimeUnit.SECONDS);
        for(Entry<String,Boolean> e: resultMap.entrySet()) {
            assertTrue("Expected element '" + e.getKey() + "' to be activated", e.getValue());
        }
    }

    /**
     * This test loads a website with SVG shapes "polyline", "path", "rect",
     * "circle", "ellipse", "polygon". By setting the CSS property
     * "pointer-events" to "stroke", the rendering engine is configured to only
     * react to clicks on the stroke of the shapes.
     *
     * <p>With a javascript event click handler, mouse events on the shapes
     * are recorded.</p>
     *
     * <p>This tests simulates clicks on the fill, so the shapes should not
     * receive clicks.</p>
     */
    @Test
    public void testClickOnFillPointerEventsStroke() throws Exception {
        WebView[] view = new WebView[1];
        CountDownLatch waitTestDocumentLoaded = new CountDownLatch(1);

        Platform.runLater(() -> {
            final Stage primaryStage = new Stage();
            view[0] = new WebView();
            primaryStage.setScene(new Scene(view[0], 800, 600));
            primaryStage.show();

            view[0].getEngine().getLoadWorker().stateProperty().addListener(new ChangeListener<State>() {
                @Override
                public void changed(ObservableValue<? extends State> ov, State t, State t1) {
                    if(t1 == State.SUCCEEDED) {
                        waitTestDocumentLoaded.countDown();
                    }
                }
            });

            String url = new File("src/test/resources/test/html/pointerevents-stroke.html").toURI().toASCIIString();
            view[0].getEngine().load(url);
        });

        waitTestDocumentLoaded.await();

        CountDownLatch resultMapDone = new CountDownLatch(1);
        final Map<String,Boolean> resultMap = new HashMap<>();

        Platform.runLater(() -> {
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 80, 80));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 80, 80));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 280, 80));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 280, 80));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 480, 80));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 480, 80));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 80, 280));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 80, 280));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 280, 280));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 280, 280));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_PRESSED, 480, 280));
            view[0].fireEvent( generateMouseEvent(MouseEvent.MOUSE_RELEASED, 480, 280));

            for (String s : asList("polyline", "path", "rect", "circle", "ellipse", "polygon")) {
                resultMap.put(s, (boolean) view[0].getEngine().executeScript("isActivated(\"" + s + "\")"));
            }

            resultMapDone.countDown();
        });

        resultMapDone.await(20, TimeUnit.SECONDS);
        for(Entry<String,Boolean> e: resultMap.entrySet()) {
            assertFalse("Expected element '" + e.getKey() + "' not to be activated", e.getValue());
        }
    }

    public static MouseEvent generateMouseEvent(EventType<MouseEvent> type,
        double x, double y) {

        MouseButton button = MouseButton.NONE;
        if (type == MouseEvent.MOUSE_PRESSED
            || type == MouseEvent.MOUSE_RELEASED
            || type == MouseEvent.MOUSE_DRAGGED) {
            button = MouseButton.PRIMARY;
        }

        boolean primaryButtonDown = false;

        if (type == MouseEvent.MOUSE_PRESSED
            || type == MouseEvent.MOUSE_DRAGGED) {
            primaryButtonDown = true;
        }

        if (type == MouseEvent.MOUSE_RELEASED) {
            primaryButtonDown = false;
        }

        MouseEvent event = new MouseEvent(type, x, y, x, y, button,
            1, false, false, false, false, primaryButtonDown,
            false, false, false, false, false, null);

        return event;
    }
}
