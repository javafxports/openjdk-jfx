/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "SVGAnimatedTransformList.h"
#include "SVGElement.h"
#include "SVGTests.h"
#include "SVGTransformable.h"

namespace WebCore {

class AffineTransform;
class Path;
class SVGRect;
class SVGMatrix;

class SVGGraphicsElement : public SVGElement, public SVGTransformable, public SVGTests {
public:
    virtual ~SVGGraphicsElement();

    Ref<SVGMatrix> getCTMForBindings();
    AffineTransform getCTM(StyleUpdateStrategy = AllowStyleUpdate) override;

    Ref<SVGMatrix> getScreenCTMForBindings();
    AffineTransform getScreenCTM(StyleUpdateStrategy = AllowStyleUpdate) override;

    SVGElement* nearestViewportElement() const override;
    SVGElement* farthestViewportElement() const override;

    AffineTransform localCoordinateSpaceTransform(SVGLocatable::CTMScope mode) const override { return SVGTransformable::localCoordinateSpaceTransform(mode); }
    AffineTransform animatedLocalTransform() const override;
    AffineTransform* supplementalTransform() override;

    Ref<SVGRect> getBBoxForBindings();
    FloatRect getBBox(StyleUpdateStrategy = AllowStyleUpdate) override;

    bool shouldIsolateBlending() const { return m_shouldIsolateBlending; }
    void setShouldIsolateBlending(bool isolate) { m_shouldIsolateBlending = isolate; }

    // "base class" methods for all the elements which render as paths
    virtual Path toClipPath();
    RenderPtr<RenderElement> createElementRenderer(RenderStyle&&, const RenderTreePosition&) override;

    size_t approximateMemoryCost() const override { return sizeof(*this); }

    // SVGTests
    Ref<SVGStringList> requiredFeatures();
    Ref<SVGStringList> requiredExtensions();
    Ref<SVGStringList> systemLanguage();

protected:
    SVGGraphicsElement(const QualifiedName&, Document&);

    bool supportsFocus() const override { return Element::supportsFocus() || hasFocusEventListeners(); }

    void parseAttribute(const QualifiedName&, const AtomicString&) override;
    void svgAttributeChanged(const QualifiedName&) override;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGGraphicsElement)
        DECLARE_ANIMATED_TRANSFORM_LIST(Transform, transform)
    END_DECLARE_ANIMATED_PROPERTIES

private:
    bool isSVGGraphicsElement() const override { return true; }

    static bool isSupportedAttribute(const QualifiedName&);

    // SVGTests
    void synchronizeRequiredFeatures() final { SVGTests::synchronizeRequiredFeatures(*this); }
    void synchronizeRequiredExtensions() final { SVGTests::synchronizeRequiredExtensions(*this); }
    void synchronizeSystemLanguage() final { SVGTests::synchronizeSystemLanguage(*this); }

    // Used by <animateMotion>
    std::unique_ptr<AffineTransform> m_supplementalTransform;

    // Used to isolate blend operations caused by masking.
    bool m_shouldIsolateBlending;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::SVGGraphicsElement)
    static bool isType(const WebCore::SVGElement& element) { return element.isSVGGraphicsElement(); }
    static bool isType(const WebCore::Node& node) { return is<WebCore::SVGElement>(node) && isType(downcast<WebCore::SVGElement>(node)); }
SPECIALIZE_TYPE_TRAITS_END()
