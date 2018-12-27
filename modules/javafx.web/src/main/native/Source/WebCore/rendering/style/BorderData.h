/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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
 *
 */

#pragma once

#include "BorderValue.h"
#include "LengthSize.h"
#include "NinePieceImage.h"

namespace WebCore {

class BorderData {
friend class RenderStyle;
public:
    BorderData()
        : m_topLeft { { 0, Fixed }, { 0, Fixed } }
        , m_topRight { { 0, Fixed }, { 0, Fixed } }
        , m_bottomLeft { { 0, Fixed }, { 0, Fixed } }
        , m_bottomRight { { 0, Fixed }, { 0, Fixed } }
    {
    }
    bool hasBorder() const
    {
        bool haveImage = m_image.hasImage();
        return m_left.nonZero(!haveImage) || m_right.nonZero(!haveImage) || m_top.nonZero(!haveImage) || m_bottom.nonZero(!haveImage);
    }

    bool hasVisibleBorder() const
    {
        bool haveImage = m_image.hasImage();
        return m_left.isVisible(!haveImage) || m_right.isVisible(!haveImage) || m_top.isVisible(!haveImage) || m_bottom.isVisible(!haveImage);
    }

    bool hasFill() const
    {
        return m_image.hasImage() && m_image.fill();
    }

    bool hasBorderRadius() const
    {
        return !m_topLeft.width.isZero()
            || !m_topRight.width.isZero()
            || !m_bottomLeft.width.isZero()
            || !m_bottomRight.width.isZero();
    }

    float borderLeftWidth() const
    {
        if (!m_image.hasImage() && (m_left.style() == BorderStyle::None || m_left.style() == BorderStyle::Hidden))
            return 0;
        return m_left.width();
    }

    float borderRightWidth() const
    {
        if (!m_image.hasImage() && (m_right.style() == BorderStyle::None || m_right.style() == BorderStyle::Hidden))
            return 0;
        return m_right.width();
    }

    float borderTopWidth() const
    {
        if (!m_image.hasImage() && (m_top.style() == BorderStyle::None || m_top.style() == BorderStyle::Hidden))
            return 0;
        return m_top.width();
    }

    float borderBottomWidth() const
    {
        if (!m_image.hasImage() && (m_bottom.style() == BorderStyle::None || m_bottom.style() == BorderStyle::Hidden))
            return 0;
        return m_bottom.width();
    }

    FloatBoxExtent borderWidth() const
    {
        return FloatBoxExtent(borderTopWidth(), borderRightWidth(), borderBottomWidth(), borderLeftWidth());
    }

    bool operator==(const BorderData& o) const
    {
        return m_left == o.m_left && m_right == o.m_right && m_top == o.m_top && m_bottom == o.m_bottom && m_image == o.m_image
               && m_topLeft == o.m_topLeft && m_topRight == o.m_topRight && m_bottomLeft == o.m_bottomLeft && m_bottomRight == o.m_bottomRight;
    }

    bool operator!=(const BorderData& o) const
    {
        return !(*this == o);
    }

    const BorderValue& left() const { return m_left; }
    const BorderValue& right() const { return m_right; }
    const BorderValue& top() const { return m_top; }
    const BorderValue& bottom() const { return m_bottom; }

    const NinePieceImage& image() const { return m_image; }

    const LengthSize& topLeft() const { return m_topLeft; }
    const LengthSize& topRight() const { return m_topRight; }
    const LengthSize& bottomLeft() const { return m_bottomLeft; }
    const LengthSize& bottomRight() const { return m_bottomRight; }

private:
    BorderValue m_left;
    BorderValue m_right;
    BorderValue m_top;
    BorderValue m_bottom;

    NinePieceImage m_image;

    LengthSize m_topLeft;
    LengthSize m_topRight;
    LengthSize m_bottomLeft;
    LengthSize m_bottomRight;
};

} // namespace WebCore
