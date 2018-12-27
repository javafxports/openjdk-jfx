/*
 * Copyright (C) 2011 Apple Inc.
 * Copyright (C) 2010 Sencha, Inc.
 * Copyright (C) 2010 Igalia S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ShadowBlur_h
#define ShadowBlur_h

#include "Color.h"
#include "FloatRect.h"
#include "FloatRoundedRect.h"
#include <wtf/Function.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class AffineTransform;
class GraphicsContext;
struct GraphicsContextState;
class ImageBuffer;

class ShadowBlur {
    WTF_MAKE_NONCOPYABLE(ShadowBlur);
public:
    enum ShadowType {
        NoShadow,
        SolidShadow,
        BlurShadow
    };

    ShadowBlur();
    ShadowBlur(const FloatSize& radius, const FloatSize& offset, const Color&, bool shadowsIgnoreTransforms = false);
    ShadowBlur(const GraphicsContextState&);

    void setShadowValues(const FloatSize&, const FloatSize& , const Color&, bool ignoreTransforms = false);

    void setShadowsIgnoreTransforms(bool ignoreTransforms) { m_shadowsIgnoreTransforms = ignoreTransforms; }
    bool shadowsIgnoreTransforms() const { return m_shadowsIgnoreTransforms; }

    void drawRectShadow(GraphicsContext&, const FloatRoundedRect&);
    void drawInsetShadow(GraphicsContext&, const FloatRect&, const FloatRoundedRect& holeRect);

    using DrawBufferCallback = WTF::Function<void(ImageBuffer&, const FloatPoint&, const FloatSize&, const FloatRect&)>;
    void drawRectShadow(const AffineTransform&, const IntRect&, const FloatRoundedRect&, const DrawBufferCallback&);
    void drawInsetShadow(const AffineTransform&, const IntRect&, const FloatRect&, const FloatRoundedRect&, const DrawBufferCallback&);

    using DrawShadowCallback = WTF::Function<void(GraphicsContext&)>;
    void drawShadowLayer(const AffineTransform&, const IntRect&, const FloatRect&, const DrawShadowCallback&, const DrawBufferCallback&);

    void blurLayerImage(unsigned char*, const IntSize&, int stride);

    void clear();

    ShadowType type() const { return m_type; }

private:
    void updateShadowBlurValues();

    void drawShadowBuffer(GraphicsContext&);

    void adjustBlurRadius(const AffineTransform&);

    enum ShadowDirection {
        OuterShadow,
        InnerShadow
    };

    IntSize calculateLayerBoundingRect(const AffineTransform&, const FloatRect& layerArea, const IntRect& clipRect);
    IntSize templateSize(const IntSize& blurredEdgeSize, const FloatRoundedRect::Radii&) const;

    void drawRectShadowWithoutTiling(GraphicsContext&, const FloatRoundedRect&, const IntSize& layerSize);
    void drawRectShadowWithTiling(GraphicsContext&, const FloatRoundedRect&, const IntSize& shadowTemplateSize, const IntSize& blurredEdgeSize);

    void drawInsetShadowWithoutTiling(GraphicsContext&, const FloatRect&, const FloatRoundedRect& holeRect, const IntSize& layerSize);
    void drawInsetShadowWithTiling(GraphicsContext&, const FloatRect&, const FloatRoundedRect& holeRect, const IntSize& shadowTemplateSize, const IntSize& blurredEdgeSize);

    void drawLayerPieces(GraphicsContext&, const FloatRect& shadowBounds, const FloatRoundedRect::Radii&, const IntSize& roundedRadius, const IntSize& templateSize, ShadowDirection);

    void blurShadowBuffer(const IntSize& templateSize);
    void blurAndColorShadowBuffer(const IntSize& templateSize);

    IntSize blurredEdgeSize() const;


    ShadowType m_type { NoShadow };

    Color m_color;
    FloatSize m_blurRadius;
    FloatSize m_offset;

    ImageBuffer* m_layerImage { nullptr }; // Buffer to where the temporary shadow will be drawn to.

    FloatRect m_sourceRect; // Sub-rect of m_layerImage that contains the shadow pixels.
    FloatPoint m_layerOrigin; // Top-left corner of the (possibly clipped) bounding rect to draw the shadow to.
    FloatSize m_layerSize; // Size of m_layerImage pixels that need blurring.
    FloatSize m_layerContextTranslation; // Translation to apply to m_layerContext for the shadow to be correctly clipped.

    bool m_shadowsIgnoreTransforms { false };
};

} // namespace WebCore

#endif // ShadowBlur_h
