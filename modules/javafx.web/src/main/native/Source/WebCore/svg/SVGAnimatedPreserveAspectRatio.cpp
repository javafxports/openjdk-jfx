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
#include "SVGAnimatedPreserveAspectRatio.h"

#include "SVGAnimateElementBase.h"

namespace WebCore {

SVGAnimatedPreserveAspectRatioAnimator::SVGAnimatedPreserveAspectRatioAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedPreserveAspectRatio, animationElement, contextElement)
{
}

std::unique_ptr<SVGAnimatedType> SVGAnimatedPreserveAspectRatioAnimator::constructFromString(const String& string)
{
    return SVGAnimatedType::create(SVGPropertyTraits<SVGPreserveAspectRatioValue>::fromString(string));
}

std::unique_ptr<SVGAnimatedType> SVGAnimatedPreserveAspectRatioAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return constructFromBaseValue<SVGAnimatedPreserveAspectRatio>(animatedTypes);
}

void SVGAnimatedPreserveAspectRatioAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedPreserveAspectRatio>(animatedTypes);
}

void SVGAnimatedPreserveAspectRatioAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType& type)
{
    resetFromBaseValue<SVGAnimatedPreserveAspectRatio>(animatedTypes, type);
}

void SVGAnimatedPreserveAspectRatioAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedPreserveAspectRatio>(animatedTypes);
}

void SVGAnimatedPreserveAspectRatioAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedPreserveAspectRatio>(animatedTypes);
}

void SVGAnimatedPreserveAspectRatioAnimator::addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*)
{
    ASSERT_NOT_REACHED();
}

void SVGAnimatedPreserveAspectRatioAnimator::calculateAnimatedValue(float percentage, unsigned, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType*, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    const auto& fromPreserveAspectRatio = (m_animationElement->animationMode() == ToAnimation ? animated : from)->as<SVGPreserveAspectRatioValue>();
    const auto& toPreserveAspectRatio = to->as<SVGPreserveAspectRatioValue>();
    auto& animatedPreserveAspectRatio = animated->as<SVGPreserveAspectRatioValue>();

    m_animationElement->animateDiscreteType<SVGPreserveAspectRatioValue>(percentage, fromPreserveAspectRatio, toPreserveAspectRatio, animatedPreserveAspectRatio);
}

float SVGAnimatedPreserveAspectRatioAnimator::calculateDistance(const String&, const String&)
{
    // No paced animations for SVGPreserveAspectRatio.
    return -1;
}

}
