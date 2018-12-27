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

#include "config.h"
#include "CoordinatedBackingStore.h"

#if USE(COORDINATED_GRAPHICS)

#include "GraphicsLayer.h"
#include "NicosiaBuffer.h"
#include "TextureMapper.h"
#include "TextureMapperGL.h"

namespace WebCore {

void CoordinatedBackingStoreTile::swapBuffers(TextureMapper& textureMapper)
{
    if (!m_buffer)
        return;

    ASSERT(textureMapper.maxTextureSize().width() >= m_tileRect.size().width());
    ASSERT(textureMapper.maxTextureSize().height() >= m_tileRect.size().height());

    FloatRect unscaledTileRect(m_tileRect);
    unscaledTileRect.scale(1. / m_scale);

    if (!m_texture || unscaledTileRect != rect()) {
        setRect(unscaledTileRect);
        m_texture = textureMapper.acquireTextureFromPool(m_tileRect.size(), m_buffer->supportsAlpha() ? BitmapTexture::SupportsAlpha : BitmapTexture::NoFlag);
    } else if (m_buffer->supportsAlpha() == m_texture->isOpaque())
        m_texture->reset(m_tileRect.size(), m_buffer->supportsAlpha());

    m_buffer->waitUntilPaintingComplete();
    m_texture->updateContents(m_buffer->data(), m_sourceRect, m_bufferOffset, m_buffer->stride());
    m_buffer = nullptr;
}

void CoordinatedBackingStoreTile::setBackBuffer(const IntRect& tileRect, const IntRect& sourceRect, RefPtr<Nicosia::Buffer>&& buffer, const IntPoint& offset)
{
    m_sourceRect = sourceRect;
    m_tileRect = tileRect;
    m_bufferOffset = offset;
    m_buffer = WTFMove(buffer);
}

void CoordinatedBackingStore::createTile(uint32_t id, float scale)
{
    m_tiles.add(id, CoordinatedBackingStoreTile(scale));
    m_scale = scale;
}

void CoordinatedBackingStore::removeTile(uint32_t id)
{
    ASSERT(m_tiles.contains(id));
    m_tilesToRemove.add(id);
}

void CoordinatedBackingStore::removeAllTiles()
{
    for (auto& key : m_tiles.keys())
        m_tilesToRemove.add(key);
}

void CoordinatedBackingStore::updateTile(uint32_t id, const IntRect& sourceRect, const IntRect& tileRect, RefPtr<Nicosia::Buffer>&& buffer, const IntPoint& offset)
{
    CoordinatedBackingStoreTileMap::iterator it = m_tiles.find(id);
    ASSERT(it != m_tiles.end());
    it->value.setBackBuffer(tileRect, sourceRect, WTFMove(buffer), offset);
}

void CoordinatedBackingStore::setSize(const FloatSize& size)
{
    m_pendingSize = size;
}

void CoordinatedBackingStore::paintTilesToTextureMapper(Vector<TextureMapperTile*>& tiles, TextureMapper& textureMapper, const TransformationMatrix& transform, float opacity, const FloatRect& rect)
{
    for (auto& tile : tiles)
        tile->paint(textureMapper, transform, opacity, calculateExposedTileEdges(rect, tile->rect()));
}

TransformationMatrix CoordinatedBackingStore::adjustedTransformForRect(const FloatRect& targetRect)
{
    return TransformationMatrix::rectToRect(rect(), targetRect);
}

void CoordinatedBackingStore::paintToTextureMapper(TextureMapper& textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    if (m_tiles.isEmpty())
        return;
    ASSERT(!m_size.isZero());

    Vector<TextureMapperTile*> tilesToPaint;
    Vector<TextureMapperTile*> previousTilesToPaint;

    // We have to do this every time we paint, in case the opacity has changed.
    FloatRect coveredRect;
    for (auto& tile : m_tiles.values()) {
        if (!tile.texture())
            continue;

        if (tile.scale() == m_scale) {
            tilesToPaint.append(&tile);
            coveredRect.unite(tile.rect());
            continue;
        }

        // Only show the previous tile if the opacity is high, otherwise effect looks like a bug.
        // We show the previous-scale tile anyway if it doesn't intersect with any current-scale tile.
        if (opacity < 0.95 && coveredRect.intersects(tile.rect()))
            continue;

        previousTilesToPaint.append(&tile);
    }

    // targetRect is on the contents coordinate system, so we must compare two rects on the contents coordinate system.
    // See TiledBackingStore.
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);

    paintTilesToTextureMapper(previousTilesToPaint, textureMapper, adjustedTransform, opacity, rect());
    paintTilesToTextureMapper(tilesToPaint, textureMapper, adjustedTransform, opacity, rect());
}

void CoordinatedBackingStore::drawBorder(TextureMapper& textureMapper, const Color& borderColor, float borderWidth, const FloatRect& targetRect, const TransformationMatrix& transform)
{
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);
    for (auto& tile : m_tiles.values())
        textureMapper.drawBorder(borderColor, borderWidth, tile.rect(), adjustedTransform);
}

void CoordinatedBackingStore::drawRepaintCounter(TextureMapper& textureMapper, int repaintCount, const Color& borderColor, const FloatRect& targetRect, const TransformationMatrix& transform)
{
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);
    for (auto& tile : m_tiles.values())
        textureMapper.drawNumber(repaintCount, borderColor, tile.rect().location(), adjustedTransform);
}

void CoordinatedBackingStore::commitTileOperations(TextureMapper& textureMapper)
{
    if (!m_pendingSize.isZero()) {
        m_size = m_pendingSize;
        m_pendingSize = FloatSize();
    }

    for (auto& tileToRemove : m_tilesToRemove)
        m_tiles.remove(tileToRemove);
    m_tilesToRemove.clear();

    for (auto& tile : m_tiles.values())
        tile.swapBuffers(textureMapper);
}

} // namespace WebCore
#endif // USE(COORDINATED_GRAPHICS)
