/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#ifndef TextureMapperBackingStore_h
#define TextureMapperBackingStore_h

#include "FloatRect.h"
#include "TextureMapper.h"
#include "TextureMapperPlatformLayer.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class GraphicsLayer;

class TextureMapperBackingStore : public TextureMapperPlatformLayer, public RefCounted<TextureMapperBackingStore> {
public:
    virtual RefPtr<BitmapTexture> texture() const = 0;
    void paintToTextureMapper(TextureMapper&, const FloatRect&, const TransformationMatrix&, float) override = 0;
    virtual void drawRepaintCounter(TextureMapper&, int /* repaintCount */, const Color&, const FloatRect&, const TransformationMatrix&) { }
    virtual ~TextureMapperBackingStore() = default;

protected:
    WEBCORE_EXPORT static unsigned calculateExposedTileEdges(const FloatRect& totalRect, const FloatRect& tileRect);
};

}

#endif // TextureMapperBackingStore_h
