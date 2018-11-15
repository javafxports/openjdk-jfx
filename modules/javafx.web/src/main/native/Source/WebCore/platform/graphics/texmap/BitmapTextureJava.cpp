/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "config.h"

#include "BitmapTextureJava.h"
#include "GraphicsLayer.h"
#include "NotImplemented.h"
#include "PlatformContextJava.h"
#include "TextureMapperJava.h"

namespace WebCore {

void BitmapTextureJava::updateContents(const void* data, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag)
{
#if PLATFORM(CAIRO)
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(static_cast<unsigned char*>(data()),
        CAIRO_FORMAT_ARGB32, targetRect.width(), targetRect.height(), bytesPerLine));
    m_image->context()->platformContext()->drawSurfaceToContext(surface.get(), targetRect,
        IntRect(sourceOffset, targetRect.size()), m_image->context());
#else
    UNUSED_PARAM(data);
    UNUSED_PARAM(targetRect);
    UNUSED_PARAM(sourceOffset);
    UNUSED_PARAM(bytesPerLine);
#endif
}

void BitmapTextureJava::updateContents(TextureMapper& mapper, GraphicsLayer* sourceLayer, const IntRect& targetRect, const IntPoint& sourceOffset, UpdateContentsFlag, float)
{
    GraphicsContext& context = m_image->context();
    // Share RenderThemeJava context
    context.platformContext()->setJRenderTheme(static_cast<TextureMapperJava&>(mapper).graphicsContext()->platformContext()->jRenderTheme());

    context.clearRect(targetRect);

    IntRect sourceRect(targetRect);
    sourceRect.setLocation(sourceOffset);
    context.save();
    context.clip(targetRect);
    context.translate(targetRect.x() - sourceOffset.x(), targetRect.y() - sourceOffset.y());
    sourceLayer->paintGraphicsLayerContents(context, sourceRect);
    context.restore();
}

void BitmapTextureJava::didReset()
{
    float devicePixelRatio = 1.0;
    m_image = ImageBuffer::create(contentSize(), Accelerated, devicePixelRatio);
}

void BitmapTextureJava::updateContents(Image* image, const IntRect& targetRect, const IntPoint& offset, UpdateContentsFlag)
{
    m_image->context().drawImage(*image, targetRect, IntRect(offset, targetRect.size()), CompositeCopy);
}

RefPtr<BitmapTexture> BitmapTextureJava::applyFilters(TextureMapper&, const FilterOperations&)
{
    notImplemented();
    return this;
}

} // namespace WebCore
