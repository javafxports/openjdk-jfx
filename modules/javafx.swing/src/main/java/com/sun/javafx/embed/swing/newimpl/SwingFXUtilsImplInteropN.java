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

package com.sun.javafx.embed.swing.newimpl;

import com.sun.javafx.application.PlatformImpl;
import com.sun.javafx.tk.Toolkit;
import java.awt.EventQueue;
import java.awt.SecondaryLoop;
import java.util.concurrent.atomic.AtomicBoolean;
import javafx.application.Platform;
import jdk.swing.interop.DispatcherWrapper;

public class SwingFXUtilsImplInteropN {

    private static class FwSecondaryLoop implements SecondaryLoop {

        private final AtomicBoolean isRunning = new AtomicBoolean(false);

        @Override
        public boolean enter() {
            if (isRunning.compareAndSet(false, true)) {
                PlatformImpl.runAndWait(() -> {
                    Toolkit.getToolkit().enterNestedEventLoop(FwSecondaryLoop.this);
                });
                return true;
            }
            return false;
        }

        @Override
        public boolean exit() {
            if (isRunning.compareAndSet(true, false)) {
                PlatformImpl.runAndWait(() -> {
                    Toolkit.getToolkit().exitNestedEventLoop(FwSecondaryLoop.this, null);
                });
                return true;
            }
            return false;
        }
    }

    private static class FXDispatcher extends DispatcherWrapper {

        @Override
        public boolean isDispatchThread() {
            return Platform.isFxApplicationThread();
        }

        @Override
        public void scheduleDispatch(Runnable runnable) {
            Platform.runLater(runnable);
        }

        @Override
        public SecondaryLoop createSecondaryLoop() {
            return new FwSecondaryLoop();
        }
    }

    public void setFwDispatcher(EventQueue eventQueue) {
        DispatcherWrapper.setFwDispatcher(eventQueue, new FXDispatcher());
    }
}

