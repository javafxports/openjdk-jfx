/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "SVGAltGlyphElement.h"

#if ENABLE(SVG_FONTS)

#include "RenderInline.h"
#include "RenderSVGTSpan.h"
#include "SVGAltGlyphDefElement.h"
#include "SVGGlyphElement.h"
#include "SVGNames.h"
#include "XLinkNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGAltGlyphElement, XLinkNames::hrefAttr, Href, href)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGAltGlyphElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTextPositioningElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGAltGlyphElement::SVGAltGlyphElement(const QualifiedName& tagName, Document& document)
    : SVGTextPositioningElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::altGlyphTag));
    registerAnimatedPropertiesForSVGAltGlyphElement();
}

Ref<SVGAltGlyphElement> SVGAltGlyphElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGAltGlyphElement(tagName, document));
}

ExceptionOr<void> SVGAltGlyphElement::setGlyphRef(const AtomicString&)
{
    return Exception { NoModificationAllowedError };
}

const AtomicString& SVGAltGlyphElement::glyphRef() const
{
    return attributeWithoutSynchronization(SVGNames::glyphRefAttr);
}

ExceptionOr<void> SVGAltGlyphElement::setFormat(const AtomicString&)
{
    return Exception { NoModificationAllowedError };
}

const AtomicString& SVGAltGlyphElement::format() const
{
    return attributeWithoutSynchronization(SVGNames::formatAttr);
}

bool SVGAltGlyphElement::childShouldCreateRenderer(const Node& child) const
{
    return child.isTextNode();
}

RenderPtr<RenderElement> SVGAltGlyphElement::createElementRenderer(RenderStyle&& style, const RenderTreePosition&)
{
    return createRenderer<RenderSVGTSpan>(*this, WTFMove(style));
}

bool SVGAltGlyphElement::hasValidGlyphElements(Vector<String>& glyphNames) const
{
    String target;
    auto element = makeRefPtr(targetElementFromIRIString(getAttribute(XLinkNames::hrefAttr), document(), &target));

    if (is<SVGGlyphElement>(element)) {
        glyphNames.append(target);
        return true;
    }

    if (is<SVGAltGlyphDefElement>(element) && downcast<SVGAltGlyphDefElement>(*element).hasValidGlyphElements(glyphNames))
        return true;

    return false;
}

}

#endif
