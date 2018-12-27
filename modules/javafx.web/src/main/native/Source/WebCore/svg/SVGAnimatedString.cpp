/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SVGAnimatedString.h"

#include "SVGAnimateElementBase.h"

namespace WebCore {

SVGAnimatedStringAnimator::SVGAnimatedStringAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedString, animationElement, contextElement)
{
}

std::unique_ptr<SVGAnimatedType> SVGAnimatedStringAnimator::constructFromString(const String& string)
{
    return SVGAnimatedType::create(SVGPropertyTraits<String>::fromString(string));
}

std::unique_ptr<SVGAnimatedType> SVGAnimatedStringAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return constructFromBaseValue<SVGAnimatedString>(animatedTypes);
}

void SVGAnimatedStringAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedString>(animatedTypes);
}

void SVGAnimatedStringAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType& type)
{
    resetFromBaseValue<SVGAnimatedString>(animatedTypes, type);
}

void SVGAnimatedStringAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedString>(animatedTypes);
}

void SVGAnimatedStringAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedString>(animatedTypes);
}

void SVGAnimatedStringAnimator::addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*)
{
    ASSERT_NOT_REACHED();
}

static String parseStringFromString(SVGAnimationElement*, const String& string)
{
    return string;
}

void SVGAnimatedStringAnimator::calculateAnimatedValue(float percentage, unsigned, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType*, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    auto fromString = from->as<String>();
    auto toString = to->as<String>();
    auto& animatedString = animated->as<String>();

    // Apply CSS inheritance rules.
    m_animationElement->adjustForInheritance<String>(parseStringFromString, m_animationElement->fromPropertyValueType(), fromString, m_contextElement);
    m_animationElement->adjustForInheritance<String>(parseStringFromString, m_animationElement->toPropertyValueType(), toString, m_contextElement);

    m_animationElement->animateDiscreteType<String>(percentage, fromString, toString, animatedString);
}

float SVGAnimatedStringAnimator::calculateDistance(const String&, const String&)
{
    // No paced animations for strings.
    return -1;
}

}
