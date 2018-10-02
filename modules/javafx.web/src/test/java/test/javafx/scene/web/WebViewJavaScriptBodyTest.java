/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import javafx.application.Platform;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import javafx.scene.web.WebView;
import net.java.html.boot.fx.FXBrowsers;
import net.java.html.js.JavaScriptBody;
import net.java.html.js.JavaScriptResource;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import org.netbeans.html.json.tck.JavaScriptTCK;
import org.netbeans.html.json.tck.KOTest;

/**
 * Apache HTML/Java API provides intrinsic, yet portable interface to JavaFX WebView.
 * It relies on many internals of the interaction between Java and JavaScript engines
 * including garbage collector behavior.
 *
 * This test ensures that the low level operations based on {@link JavaScriptBody}
 * and {@link JavaScriptResource} annotations are working as expected in the latest
 * release of JavaFX {@link WebView}. The test executes methods annotated with
 * {@link KOTest} annotation from classes provided by {@link JavaScriptTCK#testClasses}
 * method.
 */
@RunWith(Parameterized.class)
public final class WebViewJavaScriptBodyTest extends TestBase {
    @Before
    public void loadAndInitTheWebView() throws Exception {
        CountDownLatch await = new CountDownLatch(1);

        URL page = getClass().getResource("/test/html/WebViewJavaScriptBodyTest.html");
        assertNotNull(page);

        Platform.runLater(() -> {
            FXBrowsers.load(getView(), page, await::countDown);
        });

        await.await();
    }

    @Parameterized.Parameters
    public static List<Object[]> testMethods() {
        return WebKitTCK.testMethods();
    }

    private final Method method;
    private Object instance;

    public WebViewJavaScriptBodyTest(Method method) {
        this.method = method;
    }


    @Test public void invoke() throws Exception {
        if (instance == null) {
            instance = method.getDeclaringClass().newInstance();
        }
        CountDownLatch execute = new CountDownLatch(1);
        Throwable[] result = { null };
        FXBrowsers.runInBrowser(getView(), () -> {
            try {
                method.invoke(instance);
            } catch (Throwable error) {
                result[0] = error;
            } finally {
                execute.countDown();
            }
        });
        if (result[0] instanceof Exception) {
            throw (Exception)result[0];
        }
        if (result[0] instanceof Error) {
            throw (Error)result[0];
        }
    }

    public static final class WebKitTCK extends JavaScriptTCK {
        public static List<Object[]> testMethods() {
            List<Object[]> methods = new ArrayList<>();
            Class<? extends Annotation> test = KOTest.class;
            Class[] arr = testClasses();
            for (Class c : arr) {
                for (Method m : c.getMethods()) {
                    if (m.getAnnotation(test) != null) {
                        methods.add(new Object[] { m });
                    }
                }
            }
            return methods;
        }
    }
}
