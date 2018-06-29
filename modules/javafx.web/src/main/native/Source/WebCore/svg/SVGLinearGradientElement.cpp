/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#include "config.h"
#include "SVGLinearGradientElement.h"

#include "Document.h"
#include "FloatPoint.h"
#include "LinearGradientAttributes.h"
#include "RenderSVGResourceLinearGradient.h"
#include "SVGLengthValue.h"
#include "SVGNames.h"
#include "SVGUnitTypes.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::x1Attr, X1, x1)
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::y1Attr, Y1, y1)
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::x2Attr, X2, x2)
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::y2Attr, Y2, y2)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGLinearGradientElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x2)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y2)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGradientElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGLinearGradientElement::SVGLinearGradientElement(const QualifiedName& tagName, Document& document)
    : SVGGradientElement(tagName, document)
    , m_x1(LengthModeWidth)
    , m_y1(LengthModeHeight)
    , m_x2(LengthModeWidth, "100%")
    , m_y2(LengthModeHeight)
{
    // Spec: If the x2 attribute is not specified, the effect is as if a value of "100%" were specified.
    ASSERT(hasTagName(SVGNames::linearGradientTag));
    registerAnimatedPropertiesForSVGLinearGradientElement();
}

Ref<SVGLinearGradientElement> SVGLinearGradientElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGLinearGradientElement(tagName, document));
}

bool SVGLinearGradientElement::isSupportedAttribute(const QualifiedName& attrName)
{
    static const auto supportedAttributes = makeNeverDestroyed(HashSet<QualifiedName> {
        SVGNames::x1Attr, SVGNames::x2Attr, SVGNames::y1Attr, SVGNames::y2Attr
    });
    return supportedAttributes.get().contains<SVGAttributeHashTranslator>(attrName);
}

void SVGLinearGradientElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (name == SVGNames::x1Attr)
        setX1BaseValue(SVGLengthValue::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::y1Attr)
        setY1BaseValue(SVGLengthValue::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::x2Attr)
        setX2BaseValue(SVGLengthValue::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::y2Attr)
        setY2BaseValue(SVGLengthValue::construct(LengthModeHeight, value, parseError));

    reportAttributeParsingError(parseError, name, value);

    SVGGradientElement::parseAttribute(name, value);
}

void SVGLinearGradientElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGradientElement::svgAttributeChanged(attrName);
        return;
    }

    InstanceInvalidationGuard guard(*this);

    updateRelativeLengthsInformation();

    if (RenderObject* object = renderer())
        object->setNeedsLayout();
}

RenderPtr<RenderElement> SVGLinearGradientElement::createElementRenderer(RenderStyle&& style, const RenderTreePosition&)
{
    return createRenderer<RenderSVGResourceLinearGradient>(*this, WTFMove(style));
}

static void setGradientAttributes(SVGGradientElement& element, LinearGradientAttributes& attributes, bool isLinear = true)
{
    if (!attributes.hasSpreadMethod() && element.hasAttribute(SVGNames::spreadMethodAttr))
        attributes.setSpreadMethod(element.spreadMethod());

    if (!attributes.hasGradientUnits() && element.hasAttribute(SVGNames::gradientUnitsAttr))
        attributes.setGradientUnits(element.gradientUnits());

    if (!attributes.hasGradientTransform() && element.hasAttribute(SVGNames::gradientTransformAttr)) {
        AffineTransform transform;
        element.gradientTransform().concatenate(transform);
        attributes.setGradientTransform(transform);
    }

    if (!attributes.hasStops()) {
        const Vector<Gradient::ColorStop>& stops(element.buildStops());
        if (!stops.isEmpty())
            attributes.setStops(stops);
    }

    if (isLinear) {
        SVGLinearGradientElement& linear = downcast<SVGLinearGradientElement>(element);

        if (!attributes.hasX1() && element.hasAttribute(SVGNames::x1Attr))
            attributes.setX1(linear.x1());

        if (!attributes.hasY1() && element.hasAttribute(SVGNames::y1Attr))
            attributes.setY1(linear.y1());

        if (!attributes.hasX2() && element.hasAttribute(SVGNames::x2Attr))
            attributes.setX2(linear.x2());

        if (!attributes.hasY2() && element.hasAttribute(SVGNames::y2Attr))
            attributes.setY2(linear.y2());
    }
}

bool SVGLinearGradientElement::collectGradientAttributes(LinearGradientAttributes& attributes)
{
    if (!renderer())
        return false;

    HashSet<SVGGradientElement*> processedGradients;
    SVGGradientElement* current = this;

    setGradientAttributes(*current, attributes);
    processedGradients.add(current);

    while (true) {
        // Respect xlink:href, take attributes from referenced element
        auto refNode = makeRefPtr(SVGURIReference::targetElementFromIRIString(current->href(), document()));
        if (is<SVGGradientElement>(refNode)) {
            current = downcast<SVGGradientElement>(refNode.get());

            // Cycle detection
            if (processedGradients.contains(current))
                return true;

            if (!current->renderer())
                return false;

            setGradientAttributes(*current, attributes, current->hasTagName(SVGNames::linearGradientTag));
            processedGradients.add(current);
        } else
            return true;
    }

    ASSERT_NOT_REACHED();
    return false;
}

bool SVGLinearGradientElement::selfHasRelativeLengths() const
{
    return x1().isRelative()
        || y1().isRelative()
        || x2().isRelative()
        || y2().isRelative();
}

}
