/*
 * Copyright (c) 2013, 2016, Oracle and/or its affiliates. All rights reserved.
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

package javafx.scene;

import com.sun.javafx.scene.DirtyBits;
import com.sun.javafx.scene.PointLightHelper;
import com.sun.javafx.sg.prism.NGNode;
import com.sun.javafx.sg.prism.NGPointLight;
import javafx.beans.property.DoubleProperty;
import javafx.scene.paint.Color;

/**
 * Defines a point light source object. A light source that has a
 * fixed point in space and radiates light equally in all directions
 * away from itself.
 *
 * @since JavaFX 8.0
 */
public class PointLight extends LightBase {
    static {
        PointLightHelper.setPointLightAccessor(new PointLightHelper.PointLightAccessor() {
            @Override
            public NGNode doCreatePeer(Node node) {
                return ((PointLight) node).doCreatePeer();
            }

            @Override
            public void doUpdatePeer(Node node) {
                ((PointLight) node).doUpdatePeer();
            }
        });
    }

    {
        // To initialize the class helper at the begining each constructor of this class
        PointLightHelper.initHelper(this);
    }

    /**
     * Creates a new instance of {@code PointLight} class with a default Color.WHITE light source.
     */
    public PointLight() {
        super();
    }

    /**
     * Creates a new instance of {@code PointLight} class using the specified color.
     *
     * @param color the color of the light source
     */
    public PointLight(Color color) {
        super(color);
    }

    /**
     * The range of this {@code PointLight}. A pixel is affected by this light i.f.f. its
     * distance to the light source is less than or equal to the light's range.
     * Any negative value will treated as {@code 0}.
     * <p>
     * Lower {@code range} values can give better performance as pixels outside the range of the light
     * will not require complex calculation. For a realistic effect, {@code range} should be set to
     * a distance at which the attenuation is close to 0 as this will give a soft cutoff.
     * <p>
     * Nodes that are inside the light's range can still be excluded from the light's effect
     * by removing them from its {@link #getScope() scope}. If a node is known to always be
     * outside of the light's range, it is more performant to exclude it from its scope.
     *
     * @defaultValue {@code Double.POSITIVE_INFINITY}
     * @since 13
     */
    private DoubleProperty range;

    public final void setRange(double value) {
        rangeProperty().set(value);
    }

    private static final double DEFAULT_RANGE = NGPointLight.getDefaultRange();

    public final double getRange() {
        return range == null ? DEFAULT_RANGE : range.get();
    }

    public final DoubleProperty rangeProperty() {
        if (range == null) {
            range = getLightDoubleProperty("range", DEFAULT_RANGE);
        }
        return range;
    }

    /**
     * The constant attenuation coefficient. This is the term {@code c} in the attenuation formula:
     * <p>
     * <code>attn = 1 / (c + lc * dist + qc * dist^2)</code>
     *
     * @defaultValue {@code 1}
     * @since 13
     */
    private DoubleProperty constantAttenuation;

    public final void setConstantAttenuation(double value) {
        constantAttenuationProperty().set(value);
    }

    private static final double DEFAULT_CONSTANT_ATTENUATION = NGPointLight.getDefaultC();

    public final double getConstantAttenuation() {
        return constantAttenuation == null ? DEFAULT_CONSTANT_ATTENUATION : constantAttenuation.get();
    }

    public final DoubleProperty constantAttenuationProperty() {
        if (constantAttenuation == null) {
            constantAttenuation = getLightDoubleProperty("constantAttenuation", DEFAULT_CONSTANT_ATTENUATION);
        }
        return constantAttenuation;
    }


    private DoubleProperty linearAttenuation;

    public final void setLinearAttenuation(double value) {
        linearAttenuationProperty().set(value);
    }

    private static final double DEFAULT_LINEAR_CONSTANT = NGPointLight.getDefaultLc();

    public final double getLinearAttenuation() {
        return linearAttenuation == null ? DEFAULT_LINEAR_CONSTANT : linearAttenuation.get();
    }

    public final DoubleProperty linearAttenuationProperty() {
        if (linearAttenuation == null) {
            linearAttenuation = getLightDoubleProperty("linearAttenuation", DEFAULT_LINEAR_CONSTANT);
        }
        return linearAttenuation;
    }


    private DoubleProperty quadraticAttenuation;

    public final void setQuadraticAttenuation(double value) {
        quadraticAttenuationProperty().set(value);
    }

    private static final double DEFAULT_QUADRATIC_CONSTANT = NGPointLight.getDefaultQc();

    public final double getQuadraticAttenuation() {
        return quadraticAttenuation == null ? DEFAULT_QUADRATIC_CONSTANT : quadraticAttenuation.get();
    }

    public final DoubleProperty quadraticAttenuationProperty() {
        if (quadraticAttenuation == null) {
            quadraticAttenuation = getLightDoubleProperty("quadraticAttenuation", DEFAULT_QUADRATIC_CONSTANT);
        }
        return quadraticAttenuation;
    }

    /*
     * Note: This method MUST only be called via its accessor method.
     */
    private NGNode doCreatePeer() {
        return new NGPointLight();
    }

    private void doUpdatePeer() {
        if (isDirty(DirtyBits.NODE_LIGHT)) {
            NGPointLight peer = getPeer();
            peer.setC(getConstantAttenuation());
            peer.setLc(getLinearAttenuation());
            peer.setQc(getQuadraticAttenuation());
            peer.setRange(getRange());
        }
    }
}
