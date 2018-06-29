/*
 * Copyright (C) 2005 Oliver Hunt <ojh16@student.canterbury.ac.nz>
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
#include "SVGFEDiffuseLightingElement.h"

#include "FEDiffuseLighting.h"
#include "FilterEffect.h"
#include "RenderStyle.h"
#include "SVGFELightElement.h"
#include "SVGFilterBuilder.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGFEDiffuseLightingElement, SVGNames::inAttr, In1, in1)
DEFINE_ANIMATED_NUMBER(SVGFEDiffuseLightingElement, SVGNames::diffuseConstantAttr, DiffuseConstant, diffuseConstant)
DEFINE_ANIMATED_NUMBER(SVGFEDiffuseLightingElement, SVGNames::surfaceScaleAttr, SurfaceScale, surfaceScale)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEDiffuseLightingElement, SVGNames::kernelUnitLengthAttr, kernelUnitLengthXIdentifier(), KernelUnitLengthX, kernelUnitLengthX)
DEFINE_ANIMATED_NUMBER_MULTIPLE_WRAPPERS(SVGFEDiffuseLightingElement, SVGNames::kernelUnitLengthAttr, kernelUnitLengthYIdentifier(), KernelUnitLengthY, kernelUnitLengthY)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFEDiffuseLightingElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(in1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(diffuseConstant)
    REGISTER_LOCAL_ANIMATED_PROPERTY(surfaceScale)
    REGISTER_LOCAL_ANIMATED_PROPERTY(kernelUnitLengthX)
    REGISTER_LOCAL_ANIMATED_PROPERTY(kernelUnitLengthY)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGFilterPrimitiveStandardAttributes)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFEDiffuseLightingElement::SVGFEDiffuseLightingElement(const QualifiedName& tagName, Document& document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
    , m_diffuseConstant(1)
    , m_surfaceScale(1)
{
    ASSERT(hasTagName(SVGNames::feDiffuseLightingTag));
    registerAnimatedPropertiesForSVGFEDiffuseLightingElement();
}

Ref<SVGFEDiffuseLightingElement> SVGFEDiffuseLightingElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGFEDiffuseLightingElement(tagName, document));
}

const AtomicString& SVGFEDiffuseLightingElement::kernelUnitLengthXIdentifier()
{
    static NeverDestroyed<AtomicString> s_identifier("SVGKernelUnitLengthX", AtomicString::ConstructFromLiteral);
    return s_identifier;
}

const AtomicString& SVGFEDiffuseLightingElement::kernelUnitLengthYIdentifier()
{
    static NeverDestroyed<AtomicString> s_identifier("SVGKernelUnitLengthY", AtomicString::ConstructFromLiteral);
    return s_identifier;
}

void SVGFEDiffuseLightingElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == SVGNames::inAttr) {
        setIn1BaseValue(value);
        return;
    }

    if (name == SVGNames::surfaceScaleAttr) {
        setSurfaceScaleBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::diffuseConstantAttr) {
        setDiffuseConstantBaseValue(value.toFloat());
        return;
    }

    if (name == SVGNames::kernelUnitLengthAttr) {
        float x, y;
        if (parseNumberOptionalNumber(value, x, y)) {
            setKernelUnitLengthXBaseValue(x);
            setKernelUnitLengthYBaseValue(y);
        }
        return;
    }

    SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
}

bool SVGFEDiffuseLightingElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEDiffuseLighting* diffuseLighting = static_cast<FEDiffuseLighting*>(effect);

    if (attrName == SVGNames::lighting_colorAttr) {
        RenderObject* renderer = this->renderer();
        ASSERT(renderer);
        return diffuseLighting->setLightingColor(renderer->style().svgStyle().lightingColor());
    }
    if (attrName == SVGNames::surfaceScaleAttr)
        return diffuseLighting->setSurfaceScale(surfaceScale());
    if (attrName == SVGNames::diffuseConstantAttr)
        return diffuseLighting->setDiffuseConstant(diffuseConstant());

    auto& lightSource = const_cast<LightSource&>(diffuseLighting->lightSource());
    const SVGFELightElement* lightElement = SVGFELightElement::findLightElement(this);
    ASSERT(lightElement);

    if (attrName == SVGNames::azimuthAttr)
        return lightSource.setAzimuth(lightElement->azimuth());
    if (attrName == SVGNames::elevationAttr)
        return lightSource.setElevation(lightElement->elevation());
    if (attrName == SVGNames::xAttr)
        return lightSource.setX(lightElement->x());
    if (attrName == SVGNames::yAttr)
        return lightSource.setY(lightElement->y());
    if (attrName == SVGNames::zAttr)
        return lightSource.setZ(lightElement->z());
    if (attrName == SVGNames::pointsAtXAttr)
        return lightSource.setPointsAtX(lightElement->pointsAtX());
    if (attrName == SVGNames::pointsAtYAttr)
        return lightSource.setPointsAtY(lightElement->pointsAtY());
    if (attrName == SVGNames::pointsAtZAttr)
        return lightSource.setPointsAtZ(lightElement->pointsAtZ());
    if (attrName == SVGNames::specularExponentAttr)
        return lightSource.setSpecularExponent(lightElement->specularExponent());
    if (attrName == SVGNames::limitingConeAngleAttr)
        return lightSource.setLimitingConeAngle(lightElement->limitingConeAngle());

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEDiffuseLightingElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (attrName == SVGNames::surfaceScaleAttr || attrName == SVGNames::diffuseConstantAttr || attrName == SVGNames::kernelUnitLengthAttr || attrName == SVGNames::lighting_colorAttr) {
        InstanceInvalidationGuard guard(*this);
        primitiveAttributeChanged(attrName);
        return;
    }

    if (attrName == SVGNames::inAttr) {
        InstanceInvalidationGuard guard(*this);
        invalidate();
        return;
    }

    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
}

void SVGFEDiffuseLightingElement::lightElementAttributeChanged(const SVGFELightElement* lightElement, const QualifiedName& attrName)
{
    if (SVGFELightElement::findLightElement(this) != lightElement)
        return;

    // The light element has different attribute names.
    primitiveAttributeChanged(attrName);
}

RefPtr<FilterEffect> SVGFEDiffuseLightingElement::build(SVGFilterBuilder* filterBuilder, Filter& filter)
{
    auto input1 = filterBuilder->getEffectById(in1());

    if (!input1)
        return nullptr;

    auto lightElement = makeRefPtr(SVGFELightElement::findLightElement(this));
    if (!lightElement)
        return nullptr;

    auto lightSource = lightElement->lightSource(*filterBuilder);

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return nullptr;

    const Color& color = renderer->style().svgStyle().lightingColor();

    RefPtr<FilterEffect> effect = FEDiffuseLighting::create(filter, color, surfaceScale(), diffuseConstant(), kernelUnitLengthX(), kernelUnitLengthY(), WTFMove(lightSource));
    effect->inputEffects().append(input1);
    return effect;
}

}
