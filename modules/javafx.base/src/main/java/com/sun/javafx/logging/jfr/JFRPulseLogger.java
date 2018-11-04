/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
package com.sun.javafx.logging.jfr;

import com.sun.javafx.logging.Logger;

import jdk.jfr.FlightRecorder;

public final class JFRPulseLogger extends Logger {
    private final ThreadLocal<JFRPulseEvent> currentPulseEvent;
    private final ThreadLocal<JFRInputEvent> currentInputEvent;

    private int pulseNumber;
    private int fxPulseNumber;
    private int renderPulseNumber;
    private Thread fxThread;

    public static Logger createInstance() {
        // Since events are off by default, and overhead is low, we can always
        // have this pulse logger available.
        return new JFRPulseLogger();
    }

    private JFRPulseLogger() {
        FlightRecorder.register(JFRInputEvent.class);
        FlightRecorder.register(JFRPulseEvent.class);
        currentPulseEvent = new ThreadLocal<JFRPulseEvent>() {
            @Override
            public JFRPulseEvent initialValue() {
                return new JFRPulseEvent();
            }
        };
        currentInputEvent = new ThreadLocal<JFRInputEvent>() {
            @Override
            public JFRInputEvent initialValue() {
                return new JFRInputEvent();
            }
        };
    }

    @Override
    public void pulseStart() {
        ++pulseNumber;
        fxPulseNumber = pulseNumber;
        if (fxThread == null) {
            fxThread = Thread.currentThread();
        }
        newPhase("Pulse start");
    }

    @Override
    public void pulseEnd() {
        newPhase(null);
        fxPulseNumber = 0;
    }

    @Override
    public void renderStart() {
        renderPulseNumber = fxPulseNumber;
    }

    @Override
    public void renderEnd() {
        newPhase(null);
        renderPulseNumber = 0;
    }

    /**
     * Finishes the current phase and starts a new one if phaseName is not null.
     * 
     * @param phaseName The name for the new phase.
     */
    @Override
    public void newPhase(String phaseName) {
        JFRPulseEvent event = currentPulseEvent.get();

        /* Cleanup if no longer enabled */
        if (!event.isEnabled()) {
            event.setPhaseName(null);
            return;
        }

        /* If there is an ongoing event, commit it */
        if (event.getPhaseName() != null) {
            event.commit();
        }

        /* Done if the new phase name is null */
        if (phaseName == null) {
            event.setPhaseName(null);
            return;
        }

        event = new JFRPulseEvent();
        event.begin();
        event.setPhaseName(phaseName);
        event.setPulseId(Thread.currentThread() == fxThread ? fxPulseNumber : renderPulseNumber);
        currentPulseEvent.set(event);
    }

    @Override
    public void newInput(String input) {
        JFRInputEvent event = currentInputEvent.get();

        /* Cleanup if no longer enabled */
        if (!event.isEnabled()) {
            event.setInput(null);
            return;
        }

        /* If there is an ongoing event, commit it */
        if (event.getInput() != null) {
            event.commit();
        }

        /* Done if the new input is null */
        if (input == null) {
            event.setInput(null);
            return;
        }

        event = new JFRInputEvent();
        event.begin();
        event.setInput(input);
        currentInputEvent.set(event);
    }
}
