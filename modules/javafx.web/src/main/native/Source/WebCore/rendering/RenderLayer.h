/*
 * Copyright (C) 2003, 2009, 2012, 2015 Apple Inc. All rights reserved.
 *
 * Portions are Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Robert O'Callahan <roc+@cs.cmu.edu>
 *   David Baron <dbaron@fas.harvard.edu>
 *   Christian Biesinger <cbiesinger@web.de>
 *   Randall Jesup <rjesup@wgate.com>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *   Josh Soref <timeless@mac.com>
 *   Boris Zbarsky <bzbarsky@mit.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#pragma once

#include "ClipRect.h"
#include "GraphicsLayer.h"
#include "LayerFragment.h"
#include "PaintInfo.h"
#include "RenderBox.h"
#include "RenderPtr.h"
#include "ScrollableArea.h"
#include <memory>

namespace WTF {
class TextStream;
}

namespace WebCore {

class ClipRects;
class ClipRectsCache;
class FilterEffectRenderer;
class FilterEffectRendererHelper;
class FilterOperations;
class HitTestRequest;
class HitTestResult;
class HitTestingTransformState;
class PaintFrequencyInfo;
class RenderFragmentedFlow;
class RenderGeometryMap;
class RenderLayerBacking;
class RenderLayerCompositor;
class RenderMarquee;
class RenderReplica;
class RenderScrollbarPart;
class RenderStyle;
class RenderView;
class Scrollbar;
class TransformationMatrix;

enum BorderRadiusClippingRule { IncludeSelfForBorderRadius, DoNotIncludeSelfForBorderRadius };
enum IncludeSelfOrNot { IncludeSelf, ExcludeSelf };

enum RepaintStatus {
    NeedsNormalRepaint,
    NeedsFullRepaint,
    NeedsFullRepaintForPositionedMovementLayout
};

enum ClipRectsType {
    PaintingClipRects, // Relative to painting ancestor. Used for painting.
    RootRelativeClipRects, // Relative to the ancestor treated as the root (e.g. transformed layer). Used for hit testing.
    AbsoluteClipRects, // Relative to the RenderView's layer. Used for compositing overlap testing.
    NumCachedClipRectsTypes,
    AllClipRectTypes,
    TemporaryClipRects
};

enum ShouldRespectOverflowClip {
    IgnoreOverflowClip,
    RespectOverflowClip
};

enum ShouldApplyRootOffsetToFragments {
    ApplyRootOffsetToFragments,
    IgnoreRootOffsetForFragments
};

enum LayerScrollCoordinationRole {
    ViewportConstrained = 1 << 0,
    Scrolling           = 1 << 1
};
typedef unsigned LayerScrollCoordinationRoles;

enum class RequestState {
    Unknown,
    DontCare,
    False,
    True,
    Undetermined
};

class RenderLayer final : public ScrollableArea {
    WTF_MAKE_FAST_ALLOCATED;
public:
    friend class RenderReplica;

    explicit RenderLayer(RenderLayerModelObject&);
    virtual ~RenderLayer();

#if PLATFORM(IOS)
    // Called before the renderer's widget (if any) has been nulled out.
    void willBeDestroyed();
#endif
    String name() const;

    RenderLayerModelObject& renderer() const { return m_renderer; }
    RenderBox* renderBox() const { return is<RenderBox>(renderer()) ? &downcast<RenderBox>(renderer()) : nullptr; }
    RenderLayer* parent() const { return m_parent; }
    RenderLayer* previousSibling() const { return m_previous; }
    RenderLayer* nextSibling() const { return m_next; }
    RenderLayer* firstChild() const { return m_first; }
    RenderLayer* lastChild() const { return m_last; }

    void addChild(RenderLayer* newChild, RenderLayer* beforeChild = nullptr);
    RenderLayer* removeChild(RenderLayer*);

    Page& page() const { return renderer().page(); }

    void removeOnlyThisLayer();
    void insertOnlyThisLayer();

    void repaintIncludingDescendants();

    // Indicate that the layer contents need to be repainted. Only has an effect
    // if layer compositing is being used.
    void setBackingNeedsRepaint(GraphicsLayer::ShouldClipToLayer = GraphicsLayer::ClipToLayer);

    // The rect is in the coordinate space of the layer's render object.
    void setBackingNeedsRepaintInRect(const LayoutRect&, GraphicsLayer::ShouldClipToLayer = GraphicsLayer::ClipToLayer);
    void repaintIncludingNonCompositingDescendants(RenderLayerModelObject* repaintContainer);

    void styleChanged(StyleDifference, const RenderStyle* oldStyle);

    RenderMarquee* marquee() const { return m_marquee.get(); }

    bool isNormalFlowOnly() const { return m_isNormalFlowOnly; }
    bool isSelfPaintingLayer() const { return m_isSelfPaintingLayer; }

    bool cannotBlitToWindow() const;

    bool isTransparent() const { return renderer().isTransparent() || renderer().hasMask(); }

    bool hasReflection() const { return renderer().hasReflection(); }
    bool isReflection() const { return renderer().isReplica(); }
    RenderReplica* reflection() const { return m_reflection.get(); }
    RenderLayer* reflectionLayer() const;

    const RenderLayer* root() const
    {
        const RenderLayer* curr = this;
        while (curr->parent())
            curr = curr->parent();
        return curr;
    }

    const LayoutPoint& location() const { return m_topLeft; }
    void setLocation(const LayoutPoint& p) { m_topLeft = p; }

    const IntSize& size() const { return m_layerSize; }
    void setSize(const IntSize& size) { m_layerSize = size; }

    LayoutRect rect() const { return LayoutRect(location(), size()); }

    int scrollWidth() const;
    int scrollHeight() const;

    void panScrollFromPoint(const IntPoint&);

    // Scrolling methods for layers that can scroll their overflow.
    void scrollByRecursively(const IntSize& delta, ScrollableArea** scrolledArea = nullptr);

    WEBCORE_EXPORT void scrollToOffset(const ScrollOffset&, ScrollClamping = ScrollClamping::Clamped);
    void scrollToXOffset(int x, ScrollClamping clamping = ScrollClamping::Clamped) { scrollToOffset(ScrollOffset(x, scrollOffset().y()), clamping); }
    void scrollToYOffset(int y, ScrollClamping clamping = ScrollClamping::Clamped) { scrollToOffset(ScrollOffset(scrollOffset().x(), y), clamping); }

    void scrollToXPosition(int x, ScrollClamping = ScrollClamping::Clamped);
    void scrollToYPosition(int y, ScrollClamping = ScrollClamping::Clamped);

    void setPostLayoutScrollPosition(std::optional<ScrollPosition>);
    void applyPostLayoutScrollPositionIfNeeded();

    ScrollOffset scrollOffset() const { return scrollOffsetFromPosition(m_scrollPosition); }
    IntSize scrollableContentsSize() const;

    void availableContentSizeChanged(AvailableSizeChangeReason) override;

    // "absoluteRect" is in scaled document coordinates.
    void scrollRectToVisible(SelectionRevealMode, const LayoutRect& absoluteRect, bool insideFixed, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    bool scrollsOverflow() const;
    bool hasScrollbars() const { return m_hBar || m_vBar; }
    void setHasHorizontalScrollbar(bool);
    void setHasVerticalScrollbar(bool);

    Ref<Scrollbar> createScrollbar(ScrollbarOrientation);
    void destroyScrollbar(ScrollbarOrientation);

    bool hasHorizontalScrollbar() const { return horizontalScrollbar(); }
    bool hasVerticalScrollbar() const { return verticalScrollbar(); }

    // ScrollableArea overrides
    ScrollPosition scrollPosition() const override { return m_scrollPosition; }

    Scrollbar* horizontalScrollbar() const override { return m_hBar.get(); }
    Scrollbar* verticalScrollbar() const override { return m_vBar.get(); }
    ScrollableArea* enclosingScrollableArea() const override;
    bool isScrollableOrRubberbandable() override;
    bool hasScrollableOrRubberbandableAncestor() override;
#if ENABLE(CSS_SCROLL_SNAP)
    void updateSnapOffsets() override;
#endif

#if PLATFORM(IOS)
#if ENABLE(TOUCH_EVENTS)
    bool handleTouchEvent(const PlatformTouchEvent&) override;
#endif

    void didStartScroll() override;
    void didEndScroll() override;
    void didUpdateScroll() override;
    void setIsUserScroll(bool isUserScroll) override { m_inUserScroll = isUserScroll; }

    bool isInUserScroll() const { return m_inUserScroll; }

    bool requiresScrollBoundsOriginUpdate() const { return m_requiresScrollBoundsOriginUpdate; }
    void setRequiresScrollBoundsOriginUpdate(bool requiresUpdate = true) { m_requiresScrollBoundsOriginUpdate = requiresUpdate; }

    // Returns true when the layer could do touch scrolling, but doesn't look at whether there is actually scrollable overflow.
    bool hasAcceleratedTouchScrolling() const;
    // Returns true when there is actually scrollable overflow (requires layout to be up-to-date).
    bool hasTouchScrollableOverflow() const;
#else
    bool hasAcceleratedTouchScrolling() const { return false; }
    bool hasTouchScrollableOverflow() const { return false; }
#endif
    bool usesAcceleratedScrolling() const;

    int verticalScrollbarWidth(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;
    int horizontalScrollbarHeight(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;

    bool hasOverflowControls() const;
    bool isPointInResizeControl(const IntPoint& absolutePoint) const;
    bool hitTestOverflowControls(HitTestResult&, const IntPoint& localPoint);
    IntSize offsetFromResizeCorner(const IntPoint& absolutePoint) const;

    void paintOverflowControls(GraphicsContext&, const IntPoint&, const IntRect& damageRect, bool paintingOverlayControls = false);
    void paintScrollCorner(GraphicsContext&, const IntPoint&, const IntRect& damageRect);
    void paintResizer(GraphicsContext&, const LayoutPoint&, const LayoutRect& damageRect);

    void updateScrollInfoAfterLayout();

    bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1);
    void autoscroll(const IntPoint&);

    bool canResize() const;
    void resize(const PlatformMouseEvent&, const LayoutSize&);
    bool inResizeMode() const { return m_inResizeMode; }
    void setInResizeMode(bool b) { m_inResizeMode = b; }

    bool isRenderViewLayer() const { return m_isRenderViewLayer; }

    RenderLayerCompositor& compositor() const;

    // Notification from the renderer that its content changed (e.g. current frame of image changed).
    // Allows updates of layer content without repainting.
    void contentChanged(ContentChangeType);

    bool canRender3DTransforms() const;

    enum UpdateLayerPositionsFlag {
        CheckForRepaint = 1 << 0,
        NeedsFullRepaintInBacking = 1 << 1,
        IsCompositingUpdateRoot = 1 << 2,
        UpdateCompositingLayers = 1 << 3,
        UpdatePagination = 1 << 4,
        SeenTransformedLayer = 1 << 5,
        Seen3DTransformedLayer = 1 << 6
    };
    typedef unsigned UpdateLayerPositionsFlags;
    static const UpdateLayerPositionsFlags defaultFlags = CheckForRepaint | IsCompositingUpdateRoot | UpdateCompositingLayers;

    void updateLayerPositionsAfterLayout(const RenderLayer* rootLayer, UpdateLayerPositionsFlags);

    void updateLayerPositionsAfterOverflowScroll();
    void updateLayerPositionsAfterDocumentScroll();

    void positionNewlyCreatedOverflowControls();

    bool hasCompositedLayerInEnclosingPaginationChain() const;
    enum PaginationInclusionMode { ExcludeCompositedPaginatedLayers, IncludeCompositedPaginatedLayers };
    RenderLayer* enclosingPaginationLayer(PaginationInclusionMode mode) const
    {
        if (mode == ExcludeCompositedPaginatedLayers && hasCompositedLayerInEnclosingPaginationChain())
            return nullptr;
        return m_enclosingPaginationLayer;
    }

    void updateTransform();

#if ENABLE(CSS_COMPOSITING)
    void updateBlendMode();
#endif

    const LayoutSize& offsetForInFlowPosition() const { return m_offsetForInFlowPosition; }

    void clearClipRectsIncludingDescendants(ClipRectsType typeToClear = AllClipRectTypes);
    void clearClipRects(ClipRectsType typeToClear = AllClipRectTypes);

    void addBlockSelectionGapsBounds(const LayoutRect&);
    void clearBlockSelectionGapsBounds();
    void repaintBlockSelectionGaps();

    // A stacking context is a layer that has a non-auto z-index.
    bool isStackingContext() const { return isStackingContext(&renderer().style()); }

    // A stacking container can have z-order lists. All stacking contexts are
    // stacking containers, but the converse is not true. Layers that use
    // composited scrolling are stacking containers, but they may not
    // necessarily be stacking contexts.
    bool isStackingContainer() const { return isStackingContext() || needsCompositedScrolling(); }

    // Gets the enclosing stacking container for this layer, excluding this
    // layer itself.
    RenderLayer* stackingContainer() const;

    // Gets the enclosing stacking container for this layer, possibly the layer
    // itself, if it is a stacking container.
    RenderLayer* enclosingStackingContainer() { return isStackingContainer() ? this : stackingContainer(); }

    void dirtyZOrderLists();
    void dirtyStackingContainerZOrderLists();

    Vector<RenderLayer*>* posZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContainer() || !m_posZOrderList);
        return m_posZOrderList.get();
    }

    bool hasNegativeZOrderList() const { return negZOrderList() && negZOrderList()->size(); }

    Vector<RenderLayer*>* negZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContainer() || !m_negZOrderList);
        return m_negZOrderList.get();
    }

    void dirtyNormalFlowList();
    Vector<RenderLayer*>* normalFlowList() const { ASSERT(!m_normalFlowListDirty); return m_normalFlowList.get(); }

    // Update our normal and z-index lists.
    void updateLayerListsIfNeeded();

    // Update the normal and z-index lists of our descendants.
    void updateDescendantsLayerListsIfNeeded(bool recursive);

    // FIXME: We should ASSERT(!m_visibleContentStatusDirty) here, but see https://bugs.webkit.org/show_bug.cgi?id=71044
    // ditto for hasVisibleDescendant(), see https://bugs.webkit.org/show_bug.cgi?id=71277
    bool hasVisibleContent() const { return m_hasVisibleContent; }
    bool hasVisibleDescendant() const { return m_hasVisibleDescendant; }

    void setHasVisibleContent();
    void dirtyVisibleContentStatus();

    bool hasVisibleBoxDecorationsOrBackground() const;
    bool hasVisibleBoxDecorations() const;

    struct PaintedContentRequest {
        void makeStatesUndetermined()
        {
            if (hasPaintedContent == RequestState::Unknown)
                hasPaintedContent = RequestState::Undetermined;

            if (hasSubpixelAntialiasedText == RequestState::Unknown)
                hasSubpixelAntialiasedText = RequestState::Undetermined;
        }

        void setHasPaintedContent() { hasPaintedContent = RequestState::True; }
        void setHasSubpixelAntialiasedText() { hasSubpixelAntialiasedText = RequestState::True; }

        bool needToDeterminePaintedContentState() const { return hasPaintedContent == RequestState::Unknown; }
        bool needToDetermineSubpixelAntialiasedTextState() const { return hasSubpixelAntialiasedText == RequestState::Unknown; }

        bool probablyHasPaintedContent() const { return hasPaintedContent == RequestState::True || hasPaintedContent == RequestState::Undetermined; }
        bool probablyHasSubpixelAntialiasedText() const { return hasSubpixelAntialiasedText == RequestState::True || hasSubpixelAntialiasedText == RequestState::Undetermined; }

        bool isSatisfied() const { return hasPaintedContent != RequestState::Unknown && hasSubpixelAntialiasedText != RequestState::Unknown; }

        RequestState hasPaintedContent { RequestState::Unknown };
        RequestState hasSubpixelAntialiasedText { RequestState::DontCare };
    };

    // Returns true if this layer has visible content (ignoring any child layers).
    bool isVisuallyNonEmpty(PaintedContentRequest* = nullptr) const;
    // True if this layer container renderers that paint.
    bool hasNonEmptyChildRenderers(PaintedContentRequest&) const;

    // FIXME: We should ASSERT(!m_hasSelfPaintingLayerDescendantDirty); here but we hit the same bugs as visible content above.
    // Part of the issue is with subtree relayout: we don't check if our ancestors have some descendant flags dirty, missing some updates.
    bool hasSelfPaintingLayerDescendant() const { return m_hasSelfPaintingLayerDescendant; }

    // This returns true if we have an out of flow positioned descendant whose
    // containing block is not a descendant of ours. If this is true, we cannot
    // automatically opt into composited scrolling since this out of flow
    // positioned descendant would become clipped by us, possibly altering the
    // rendering of the page.
    // FIXME: We should ASSERT(!m_hasOutOfFlowPositionedDescendantDirty); here but we may hit the same bugs as visible content above.
    bool hasOutOfFlowPositionedDescendant() const { return m_hasOutOfFlowPositionedDescendant; }

    // Gets the nearest enclosing positioned ancestor layer (also includes
    // the <html> layer and the root layer).
    RenderLayer* enclosingAncestorForPosition(EPosition) const;

    // Returns the nearest enclosing layer that is scrollable.
    RenderLayer* enclosingScrollableLayer() const;

    // The layer relative to which clipping rects for this layer are computed.
    RenderLayer* clippingRootForPainting() const;

    RenderLayer* enclosingOverflowClipLayer(IncludeSelfOrNot) const;

    // Enclosing compositing layer; if includeSelf is true, may return this.
    RenderLayer* enclosingCompositingLayer(IncludeSelfOrNot = IncludeSelf) const;
    RenderLayer* enclosingCompositingLayerForRepaint(IncludeSelfOrNot = IncludeSelf) const;
    // Ancestor compositing layer, excluding this.
    RenderLayer* ancestorCompositingLayer() const { return enclosingCompositingLayer(ExcludeSelf); }

    RenderLayer* enclosingFilterLayer(IncludeSelfOrNot = IncludeSelf) const;
    RenderLayer* enclosingFilterRepaintLayer() const;
    void setFilterBackendNeedsRepaintingInRect(const LayoutRect&);
    bool hasAncestorWithFilterOutsets() const;

    bool canUseConvertToLayerCoords() const
    {
        // These RenderObject have an impact on their layers' without them knowing about it.
        return !renderer().hasTransform() && !renderer().isSVGRoot();
    }

    // FIXME: adjustForColumns allows us to position compositing layers in columns correctly, but eventually they need to be split across columns too.
    enum ColumnOffsetAdjustment { DontAdjustForColumns, AdjustForColumns };
    void convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntPoint& location, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;
    LayoutPoint convertToLayerCoords(const RenderLayer* ancestorLayer, const LayoutPoint&, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;
    LayoutSize offsetFromAncestor(const RenderLayer*, ColumnOffsetAdjustment = DontAdjustForColumns) const;

    int zIndex() const { return renderer().style().zIndex(); }

    enum PaintLayerFlag {
        PaintLayerHaveTransparency                      = 1 << 0,
        PaintLayerAppliedTransform                      = 1 << 1,
        PaintLayerTemporaryClipRects                    = 1 << 2,
        PaintLayerPaintingReflection                    = 1 << 3,
        PaintLayerPaintingOverlayScrollbars             = 1 << 4,
        PaintLayerPaintingCompositingBackgroundPhase    = 1 << 5,
        PaintLayerPaintingCompositingForegroundPhase    = 1 << 6,
        PaintLayerPaintingCompositingMaskPhase          = 1 << 7,
        PaintLayerPaintingCompositingClipPathPhase      = 1 << 8,
        PaintLayerPaintingCompositingScrollingPhase     = 1 << 9,
        PaintLayerPaintingOverflowContents              = 1 << 10,
        PaintLayerPaintingRootBackgroundOnly            = 1 << 11,
        PaintLayerPaintingSkipRootBackground            = 1 << 12,
        PaintLayerPaintingChildClippingMaskPhase        = 1 << 13,
        PaintLayerPaintingCompositingAllPhases          = PaintLayerPaintingCompositingBackgroundPhase | PaintLayerPaintingCompositingForegroundPhase
    };

    typedef unsigned PaintLayerFlags;

    enum class SecurityOriginPaintPolicy { AnyOrigin, AccessibleOriginOnly };

    // The two main functions that use the layer system.  The paint method
    // paints the layers that intersect the damage rect from back to
    // front.  The hitTest method looks for mouse events by walking
    // layers that intersect the point from front to back.
    void paint(GraphicsContext&, const LayoutRect& damageRect, const LayoutSize& subpixelOffset = LayoutSize(), PaintBehavior = PaintBehaviorNormal,
        RenderObject* subtreePaintRoot = nullptr, PaintLayerFlags = 0, SecurityOriginPaintPolicy = SecurityOriginPaintPolicy::AnyOrigin);
    bool hitTest(const HitTestRequest&, HitTestResult&);
    bool hitTest(const HitTestRequest&, const HitTestLocation&, HitTestResult&);
    void paintOverlayScrollbars(GraphicsContext&, const LayoutRect& damageRect, PaintBehavior, RenderObject* subtreePaintRoot = nullptr);

    struct ClipRectsContext {
        ClipRectsContext(const RenderLayer* inRootLayer, ClipRectsType inClipRectsType, OverlayScrollbarSizeRelevancy inOverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip inRespectOverflowClip = RespectOverflowClip)
            : rootLayer(inRootLayer)
            , clipRectsType(inClipRectsType)
            , overlayScrollbarSizeRelevancy(inOverlayScrollbarSizeRelevancy)
            , respectOverflowClip(inRespectOverflowClip)
        { }
        const RenderLayer* rootLayer;
        ClipRectsType clipRectsType;
        OverlayScrollbarSizeRelevancy overlayScrollbarSizeRelevancy;
        ShouldRespectOverflowClip respectOverflowClip;
    };

    // This method figures out our layerBounds in coordinates relative to
    // |rootLayer}.  It also computes our background and foreground clip rects
    // for painting/event handling.
    // Pass offsetFromRoot if known.
    void calculateRects(const ClipRectsContext&, const LayoutRect& paintDirtyRect, LayoutRect& layerBounds,
        ClipRect& backgroundRect, ClipRect& foregroundRect, const LayoutSize& offsetFromRoot) const;

    // Public just for RenderTreeAsText.
    void collectFragments(LayerFragments&, const RenderLayer* rootLayer, const LayoutRect& dirtyRect,
        PaginationInclusionMode,
        ClipRectsType, OverlayScrollbarSizeRelevancy inOverlayScrollbarSizeRelevancy, ShouldRespectOverflowClip, const LayoutSize& offsetFromRoot,
        const LayoutRect* layerBoundingBox = nullptr, ShouldApplyRootOffsetToFragments = IgnoreRootOffsetForFragments);

    LayoutRect childrenClipRect() const; // Returns the foreground clip rect of the layer in the document's coordinate space.
    LayoutRect selfClipRect() const; // Returns the background clip rect of the layer in the document's coordinate space.
    LayoutRect localClipRect(bool& clipExceedsBounds) const; // Returns the background clip rect of the layer in the local coordinate space.

    // Pass offsetFromRoot if known.
    bool intersectsDamageRect(const LayoutRect& layerBounds, const LayoutRect& damageRect, const RenderLayer* rootLayer, const LayoutSize& offsetFromRoot, const LayoutRect* cachedBoundingBox = nullptr) const;

    enum CalculateLayerBoundsFlag {
        IncludeSelfTransform                    = 1 << 0,
        UseLocalClipRectIfPossible              = 1 << 1,
        IncludeLayerFilterOutsets               = 1 << 2,
        ExcludeHiddenDescendants                = 1 << 3,
        DontConstrainForMask                    = 1 << 4,
        IncludeCompositedDescendants            = 1 << 5,
        UseFragmentBoxesExcludingCompositing    = 1 << 6,
        UseFragmentBoxesIncludingCompositing    = 1 << 7,
        DefaultCalculateLayerBoundsFlags        = IncludeSelfTransform | UseLocalClipRectIfPossible | IncludeLayerFilterOutsets | UseFragmentBoxesExcludingCompositing
    };
    typedef unsigned CalculateLayerBoundsFlags;

    // Bounding box relative to some ancestor layer. Pass offsetFromRoot if known.
    LayoutRect boundingBox(const RenderLayer* rootLayer, const LayoutSize& offsetFromRoot = LayoutSize(), CalculateLayerBoundsFlags = 0) const;
    // Bounding box in the coordinates of this layer.
    LayoutRect localBoundingBox(CalculateLayerBoundsFlags = 0) const;
    // Deprecated: Pixel snapped bounding box relative to the root.
    WEBCORE_EXPORT IntRect absoluteBoundingBox() const;
    // Device pixel snapped bounding box relative to the root. absoluteBoundingBox() callers will be directed to this.
    FloatRect absoluteBoundingBoxForPainting() const;

    // Bounds used for layer overlap testing in RenderLayerCompositor.
    LayoutRect overlapBounds() const { return overlapBoundsIncludeChildren() ? calculateLayerBounds(this, LayoutSize()) : localBoundingBox(); }

    // Takes transform animations into account, returning true if they could be cheaply computed.
    // Unlike overlapBounds, these bounds include descendant layers.
    bool getOverlapBoundsIncludingChildrenAccountingForTransformAnimations(LayoutRect&, CalculateLayerBoundsFlags additionalFlags = 0) const;

    // If true, this layer's children are included in its bounds for overlap testing.
    // We can't rely on the children's positions if this layer has a filter that could have moved the children's pixels around.
    bool overlapBoundsIncludeChildren() const { return hasFilter() && renderer().style().filter().hasFilterThatMovesPixels(); }

    // Can pass offsetFromRoot if known.
    LayoutRect calculateLayerBounds(const RenderLayer* ancestorLayer, const LayoutSize& offsetFromRoot, CalculateLayerBoundsFlags = DefaultCalculateLayerBoundsFlags) const;

    // Return a cached repaint rect, computed relative to the layer renderer's containerForRepaint.
    bool hasComputedRepaintRects() const { return renderer().hasRepaintLayoutRects(); }
    LayoutRect repaintRectIncludingNonCompositingDescendants() const;

    void setRepaintStatus(RepaintStatus status) { m_repaintStatus = status; }
    RepaintStatus repaintStatus() const { return static_cast<RepaintStatus>(m_repaintStatus); }

    LayoutUnit staticInlinePosition() const { return m_staticInlinePosition; }
    LayoutUnit staticBlockPosition() const { return m_staticBlockPosition; }

    void setStaticInlinePosition(LayoutUnit position) { m_staticInlinePosition = position; }
    void setStaticBlockPosition(LayoutUnit position) { m_staticBlockPosition = position; }

#if PLATFORM(IOS)
    bool adjustForIOSCaretWhenScrolling() const { return m_adjustForIOSCaretWhenScrolling; }
    void setAdjustForIOSCaretWhenScrolling(bool adjustForIOSCaretWhenScrolling) { m_adjustForIOSCaretWhenScrolling = adjustForIOSCaretWhenScrolling; }
#endif

    bool hasTransform() const { return renderer().hasTransform(); }
    // Note that this transform has the transform-origin baked in.
    TransformationMatrix* transform() const { return m_transform.get(); }
    // currentTransform computes a transform which takes accelerated animations into account. The
    // resulting transform has transform-origin baked in. If the layer does not have a transform,
    // returns the identity matrix.
    TransformationMatrix currentTransform(RenderStyle::ApplyTransformOrigin = RenderStyle::IncludeTransformOrigin) const;
    TransformationMatrix renderableTransform(PaintBehavior) const;

    // Get the perspective transform, which is applied to transformed sublayers.
    // Returns true if the layer has a -webkit-perspective.
    // Note that this transform has the perspective-origin baked in.
    TransformationMatrix perspectiveTransform() const;
    FloatPoint perspectiveOrigin() const;
    bool preserves3D() const { return renderer().style().transformStyle3D() == TransformStyle3DPreserve3D; }
    bool has3DTransform() const { return m_transform && !m_transform->isAffine(); }

    void filterNeedsRepaint();
    bool hasFilter() const { return renderer().hasFilter(); }
    bool hasBackdropFilter() const
    {
#if ENABLE(FILTERS_LEVEL_2)
        return renderer().hasBackdropFilter();
#else
        return false;
#endif
    }

#if ENABLE(CSS_COMPOSITING)
    bool hasBlendMode() const { return renderer().hasBlendMode(); }
    BlendMode blendMode() const { return static_cast<BlendMode>(m_blendMode); }

    bool isolatesCompositedBlending() const { return m_hasNotIsolatedCompositedBlendingDescendants && isStackingContext(); }
    bool hasNotIsolatedCompositedBlendingDescendants() const { return m_hasNotIsolatedCompositedBlendingDescendants; }
    void setHasNotIsolatedCompositedBlendingDescendants(bool hasNotIsolatedCompositedBlendingDescendants)
    {
        m_hasNotIsolatedCompositedBlendingDescendants = hasNotIsolatedCompositedBlendingDescendants;
    }

    bool isolatesBlending() const { return hasNotIsolatedBlendingDescendants() && isStackingContext(); }

    // FIXME: We should ASSERT(!m_hasNotIsolatedBlendingDescendantsStatusDirty); here but we hit the same bugs as visible content above.
    bool hasNotIsolatedBlendingDescendants() const { return m_hasNotIsolatedBlendingDescendants; }
    bool hasNotIsolatedBlendingDescendantsStatusDirty() const { return m_hasNotIsolatedBlendingDescendantsStatusDirty; }
#else
    bool hasBlendMode() const { return false; }
    bool isolatesCompositedBlending() const { return false; }
    bool isolatesBlending() const { return false; }
    bool hasNotIsolatedBlendingDescendantsStatusDirty() const { return false; }
#endif

    bool isComposited() const { return m_backing != 0; }
    bool hasCompositingDescendant() const { return m_hasCompositingDescendant; }
    bool hasCompositedMask() const;
    RenderLayerBacking* backing() const { return m_backing.get(); }
    RenderLayerBacking* ensureBacking();
    void clearBacking(bool layerBeingDestroyed = false);
    GraphicsLayer* layerForScrolling() const override;
    GraphicsLayer* layerForHorizontalScrollbar() const override;
    GraphicsLayer* layerForVerticalScrollbar() const override;
    GraphicsLayer* layerForScrollCorner() const override;
    bool usesCompositedScrolling() const override;
    bool usesAsyncScrolling() const override;
    WEBCORE_EXPORT bool needsCompositedScrolling() const;

    bool paintsWithTransparency(PaintBehavior paintBehavior) const
    {
        return (isTransparent() || hasBlendMode() || (isolatesBlending() && !renderer().isDocumentElementRenderer())) && ((paintBehavior & PaintBehaviorFlattenCompositingLayers) || !isComposited());
    }

    bool paintsWithTransform(PaintBehavior) const;
    bool shouldPaintMask(PaintBehavior, PaintLayerFlags) const;
    bool shouldApplyClipPath(PaintBehavior, PaintLayerFlags) const;

    // Returns true if background phase is painted opaque in the given rect.
    // The query rect is given in local coordinates.
    bool backgroundIsKnownToBeOpaqueInRect(const LayoutRect&) const;

    bool scrollingMayRevealBackground() const;

    bool containsDirtyOverlayScrollbars() const { return m_containsDirtyOverlayScrollbars; }
    void setContainsDirtyOverlayScrollbars(bool dirtyScrollbars) { m_containsDirtyOverlayScrollbars = dirtyScrollbars; }

    bool paintsWithFilters() const;
    bool requiresFullLayerImageForFilters() const;
    FilterEffectRenderer* filterRenderer() const;

#if !ASSERT_DISABLED
    bool layerListMutationAllowed() const { return m_layerListMutationAllowed; }
    void setLayerListMutationAllowed(bool flag) { m_layerListMutationAllowed = flag; }
#endif

    Element* enclosingElement() const;

    enum ViewportConstrainedNotCompositedReason {
        NoNotCompositedReason,
        NotCompositedForBoundsOutOfView,
        NotCompositedForNonViewContainer,
        NotCompositedForNoVisibleContent,
    };

    void setViewportConstrainedNotCompositedReason(ViewportConstrainedNotCompositedReason reason) { m_viewportConstrainedNotCompositedReason = reason; }
    ViewportConstrainedNotCompositedReason viewportConstrainedNotCompositedReason() const { return static_cast<ViewportConstrainedNotCompositedReason>(m_viewportConstrainedNotCompositedReason); }

    bool isRenderFragmentedFlow() const { return renderer().isRenderFragmentedFlow(); }
    bool isOutOfFlowRenderFragmentedFlow() const { return renderer().isOutOfFlowRenderFragmentedFlow(); }
    bool isInsideFragmentedFlow() const { return renderer().fragmentedFlowState() != RenderObject::NotInsideFragmentedFlow; }
    bool isDirtyRenderFragmentedFlow() const
    {
        ASSERT(isRenderFragmentedFlow());
        return m_zOrderListsDirty || m_normalFlowListDirty;
    }

    RenderLayer* enclosingFragmentedFlowAncestor() const;

    bool shouldPlaceBlockDirectionScrollbarOnLeft() const final { return renderer().shouldPlaceBlockDirectionScrollbarOnLeft(); }

    WEBCORE_EXPORT void simulateFrequentPaint();
    WEBCORE_EXPORT bool paintingFrequently() const;
    void clearPaintFrequencyInfo();

private:
    enum CollectLayersBehavior { StopAtStackingContexts, StopAtStackingContainers };

    struct LayerPaintingInfo {
        LayerPaintingInfo(RenderLayer* inRootLayer, const LayoutRect& inDirtyRect, PaintBehavior inPaintBehavior, const LayoutSize& inSubpixelOffset, RenderObject* inSubtreePaintRoot = nullptr, OverlapTestRequestMap* inOverlapTestRequests = nullptr, bool inRequireSecurityOriginAccessForWidgets = false)
            : rootLayer(inRootLayer)
            , subtreePaintRoot(inSubtreePaintRoot)
            , paintDirtyRect(inDirtyRect)
            , subpixelOffset(inSubpixelOffset)
            , overlapTestRequests(inOverlapTestRequests)
            , paintBehavior(inPaintBehavior)
            , requireSecurityOriginAccessForWidgets(inRequireSecurityOriginAccessForWidgets)
        { }

        RenderLayer* rootLayer;
        RenderObject* subtreePaintRoot; // Only paint descendants of this object.
        LayoutRect paintDirtyRect; // Relative to rootLayer;
        LayoutSize subpixelOffset;
        OverlapTestRequestMap* overlapTestRequests; // May be null.
        PaintBehavior paintBehavior;
        bool requireSecurityOriginAccessForWidgets;
        bool clipToDirtyRect { true };
    };

    // Compute, cache and return clip rects computed with the given layer as the root.
    Ref<ClipRects> updateClipRects(const ClipRectsContext&);
    // Compute and return the clip rects. If useCached is true, will used previously computed clip rects on ancestors
    // (rather than computing them all from scratch up the parent chain).
    void calculateClipRects(const ClipRectsContext&, ClipRects&) const;
    ClipRects* clipRects(const ClipRectsContext&) const;

    void updateZOrderLists();
    void rebuildZOrderLists();
    void rebuildZOrderLists(CollectLayersBehavior, std::unique_ptr<Vector<RenderLayer*>>&, std::unique_ptr<Vector<RenderLayer*>>&);
    void clearZOrderLists();

    void updateNormalFlowList();

    // Non-auto z-index always implies stacking context here, because StyleResolver::adjustRenderStyle already adjusts z-index
    // based on positioning and other criteria.
    bool isStackingContext(const RenderStyle* style) const { return !style->hasAutoZIndex() || isRenderViewLayer() || m_forcedStackingContext; }

    bool isDirtyStackingContainer() const { return m_zOrderListsDirty && isStackingContainer(); }

    void setAncestorChainHasSelfPaintingLayerDescendant();
    void dirtyAncestorChainHasSelfPaintingLayerDescendantStatus();

    bool acceleratedCompositingForOverflowScrollEnabled() const;
    void updateDescendantsAreContiguousInStackingOrder();
    void updateDescendantsAreContiguousInStackingOrderRecursive(const HashMap<const RenderLayer*, int>&, int& minIndex, int& maxIndex, int& count, bool firstIteration);

    void computeRepaintRects(const RenderLayerModelObject* repaintContainer, const RenderGeometryMap* = nullptr);
    void computeRepaintRectsIncludingDescendants();
    void clearRepaintRects();

    LayoutRect clipRectRelativeToAncestor(RenderLayer* ancestor, LayoutSize offsetFromAncestor, const LayoutRect& constrainingRect) const;

    void clipToRect(GraphicsContext&, const LayerPaintingInfo&, const ClipRect&, BorderRadiusClippingRule = IncludeSelfForBorderRadius);
    void restoreClip(GraphicsContext&, const LayerPaintingInfo&, const ClipRect&);

    bool shouldRepaintAfterLayout() const;

    void updateSelfPaintingLayer();
    void updateStackingContextsAfterStyleChange(const RenderStyle* oldStyle);

    void updateScrollbarsAfterStyleChange(const RenderStyle* oldStyle);
    void updateScrollbarsAfterLayout();

    void setAncestorChainHasOutOfFlowPositionedDescendant(RenderBlock* containingBlock);
    void dirtyAncestorChainHasOutOfFlowPositionedDescendantStatus();
    void updateOutOfFlowPositioned(const RenderStyle* oldStyle);

    void updateNeedsCompositedScrolling();

    // Returns true if the position changed.
    bool updateLayerPosition();

    void updateLayerPositions(RenderGeometryMap* = nullptr, UpdateLayerPositionsFlags = defaultFlags);

    enum UpdateLayerPositionsAfterScrollFlag {
        NoFlag = 0,
        IsOverflowScroll = 1 << 0,
        HasSeenViewportConstrainedAncestor = 1 << 1,
        HasSeenAncestorWithOverflowClip = 1 << 2,
        HasChangedAncestor = 1 << 3
    };
    typedef unsigned UpdateLayerPositionsAfterScrollFlags;
    void updateLayerPositionsAfterScroll(RenderGeometryMap*, UpdateLayerPositionsAfterScrollFlags = NoFlag);

    ScrollOffset clampScrollOffset(const ScrollOffset&) const;

    RenderLayer* enclosingPaginationLayerInSubtree(const RenderLayer* rootLayer, PaginationInclusionMode) const;

    void setNextSibling(RenderLayer* next) { m_next = next; }
    void setPreviousSibling(RenderLayer* prev) { m_previous = prev; }
    void setParent(RenderLayer* parent);
    void setFirstChild(RenderLayer* first) { m_first = first; }
    void setLastChild(RenderLayer* last) { m_last = last; }

    LayoutPoint renderBoxLocation() const { return is<RenderBox>(renderer()) ? downcast<RenderBox>(renderer()).location() : LayoutPoint(); }

    void collectLayers(bool includeHiddenLayers, CollectLayersBehavior, std::unique_ptr<Vector<RenderLayer*>>&, std::unique_ptr<Vector<RenderLayer*>>&);

    void updateCompositingAndLayerListsIfNeeded();

    bool setupFontSubpixelQuantization(GraphicsContext&, bool& didQuantizeFonts);

    Path computeClipPath(const LayoutSize& offsetFromRoot, LayoutRect& rootRelativeBounds, WindRule&) const;

    bool setupClipPath(GraphicsContext&, const LayerPaintingInfo&, const LayoutSize& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed);

    class FilterInfo;
    std::pair<FilterInfo*, std::unique_ptr<FilterEffectRendererHelper>> filterPainter(GraphicsContext&, PaintLayerFlags) const;
    bool hasFilterThatIsPainting(GraphicsContext&, PaintLayerFlags) const;
    std::unique_ptr<FilterEffectRendererHelper> setupFilters(GraphicsContext&, LayerPaintingInfo&, PaintLayerFlags, const LayoutSize& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed);
    void applyFilters(FilterEffectRendererHelper*, GraphicsContext& originalContext, const LayerPaintingInfo&, const LayerFragments&);

    void paintLayer(GraphicsContext&, const LayerPaintingInfo&, PaintLayerFlags);
    void paintLayerContentsAndReflection(GraphicsContext&, const LayerPaintingInfo&, PaintLayerFlags);
    void paintLayerByApplyingTransform(GraphicsContext&, const LayerPaintingInfo&, PaintLayerFlags, const LayoutSize& translationOffset = LayoutSize());
    void paintLayerContents(GraphicsContext&, const LayerPaintingInfo&, PaintLayerFlags);
    void paintList(Vector<RenderLayer*>*, GraphicsContext&, const LayerPaintingInfo&, PaintLayerFlags);

    void updatePaintingInfoForFragments(LayerFragments&, const LayerPaintingInfo&, PaintLayerFlags, bool shouldPaintContent, const LayoutSize& offsetFromRoot);
    void paintBackgroundForFragments(const LayerFragments&, GraphicsContext&, GraphicsContext& transparencyLayerContext,
        const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintForegroundForFragments(const LayerFragments&, GraphicsContext&, GraphicsContext& transparencyLayerContext,
        const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintForegroundForFragmentsWithPhase(PaintPhase, const LayerFragments&, GraphicsContext&, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintOutlineForFragments(const LayerFragments&, GraphicsContext&, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintOverflowControlsForFragments(const LayerFragments&, GraphicsContext&, const LayerPaintingInfo&);
    void paintMaskForFragments(const LayerFragments&, GraphicsContext&, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintChildClippingMaskForFragments(const LayerFragments&, GraphicsContext&, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintTransformedLayerIntoFragments(GraphicsContext&, const LayerPaintingInfo&, PaintLayerFlags);

    RenderLayer* transparentPaintingAncestor();
    void beginTransparencyLayers(GraphicsContext&, const LayerPaintingInfo&, const LayoutRect& dirtyRect);

    RenderLayer* hitTestLayer(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
        const LayoutRect& hitTestRect, const HitTestLocation&, bool appliedTransform,
        const HitTestingTransformState* = nullptr, double* zOffset = nullptr);
    RenderLayer* hitTestLayerByApplyingTransform(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest&, HitTestResult&,
        const LayoutRect& hitTestRect, const HitTestLocation&, const HitTestingTransformState* = nullptr, double* zOffset = nullptr,
        const LayoutSize& translationOffset = LayoutSize());
    RenderLayer* hitTestList(Vector<RenderLayer*>*, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
        const LayoutRect& hitTestRect, const HitTestLocation&,
        const HitTestingTransformState*, double* zOffsetForDescendants, double* zOffset,
        const HitTestingTransformState* unflattenedTransformState, bool depthSortDescendants);

    Ref<HitTestingTransformState> createLocalTransformState(RenderLayer* rootLayer, RenderLayer* containerLayer,
        const LayoutRect& hitTestRect, const HitTestLocation&,
        const HitTestingTransformState* containerTransformState,
        const LayoutSize& translationOffset = LayoutSize()) const;

    bool hitTestContents(const HitTestRequest&, HitTestResult&, const LayoutRect& layerBounds, const HitTestLocation&, HitTestFilter) const;
    bool hitTestContentsForFragments(const LayerFragments&, const HitTestRequest&, HitTestResult&, const HitTestLocation&, HitTestFilter, bool& insideClipRect) const;
    bool hitTestResizerInFragments(const LayerFragments&, const HitTestLocation&) const;
    RenderLayer* hitTestTransformedLayerInFragments(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest&, HitTestResult&,
        const LayoutRect& hitTestRect, const HitTestLocation&, const HitTestingTransformState* = nullptr, double* zOffset = nullptr);

    bool listBackgroundIsKnownToBeOpaqueInRect(const Vector<RenderLayer*>*, const LayoutRect&) const;

    void computeScrollDimensions();
    bool hasHorizontalOverflow() const;
    bool hasVerticalOverflow() const;
    bool hasScrollableHorizontalOverflow() const;
    bool hasScrollableVerticalOverflow() const;

    bool showsOverflowControls() const;

    bool shouldBeNormalFlowOnly() const;
    bool shouldBeSelfPaintingLayer() const;

    int scrollOffset(ScrollbarOrientation) const override;

    // ScrollableArea interface
    void invalidateScrollbarRect(Scrollbar&, const IntRect&) override;
    void invalidateScrollCornerRect(const IntRect&) override;
    bool isActive() const override;
    bool isScrollCornerVisible() const override;
    IntRect scrollCornerRect() const override;
    IntRect convertFromScrollbarToContainingView(const Scrollbar&, const IntRect&) const override;
    IntRect convertFromContainingViewToScrollbar(const Scrollbar&, const IntRect&) const override;
    IntPoint convertFromScrollbarToContainingView(const Scrollbar&, const IntPoint&) const override;
    IntPoint convertFromContainingViewToScrollbar(const Scrollbar&, const IntPoint&) const override;
    int scrollSize(ScrollbarOrientation) const override;
    void setScrollOffset(const ScrollOffset&) override;

    IntRect visibleContentRectInternal(VisibleContentRectIncludesScrollbars, VisibleContentRectBehavior) const override;
    IntSize visibleSize() const override;
    IntSize contentsSize() const override;
    IntSize overhangAmount() const override;
    IntPoint lastKnownMousePosition() const override;
    bool isHandlingWheelEvent() const override;
    bool shouldSuspendScrollAnimations() const override;
    IntRect scrollableAreaBoundingBox(bool* isInsideFixed = nullptr) const override;
    bool isRubberBandInProgress() const override;
    bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting() const override;
#if ENABLE(CSS_SCROLL_SNAP)
    bool isScrollSnapInProgress() const override;
#endif
    bool usesMockScrollAnimator() const override;
    void logMockScrollAnimatorMessage(const String&) const override;

#if PLATFORM(IOS)
    void registerAsTouchEventListenerForScrolling();
    void unregisterAsTouchEventListenerForScrolling();
#endif

    // Rectangle encompassing the scroll corner and resizer rect.
    LayoutRect scrollCornerAndResizerRect() const;

    // NOTE: This should only be called by the overriden setScrollOffset from ScrollableArea.
    void scrollTo(const ScrollPosition&);
    void updateCompositingLayersAfterScroll();

    IntSize scrollbarOffset(const Scrollbar&) const;

    void updateScrollableAreaSet(bool hasOverflow);

    bool allowsCurrentScroll() const;

    void dirtyAncestorChainVisibleDescendantStatus();
    void setAncestorChainHasVisibleDescendant();

    void updateDescendantDependentFlags(HashSet<const RenderObject*>* outOfFlowDescendantContainingBlocks = nullptr);

    bool has3DTransformedDescendant() const { return m_has3DTransformedDescendant; }

    bool hasTransformedAncestor() const { return m_hasTransformedAncestor; }
    bool has3DTransformedAncestor() const { return m_has3DTransformedAncestor; }

    void dirty3DTransformedDescendantStatus();
    // Both updates the status, and returns true if descendants of this have 3d.
    bool update3DTransformedDescendantStatus();

    void createReflection();
    void removeReflection();

    RenderStyle createReflectionStyle();
    bool paintingInsideReflection() const { return m_paintingInsideReflection; }
    void setPaintingInsideReflection(bool b) { m_paintingInsideReflection = b; }

    void updateOrRemoveFilterClients();
    void updateOrRemoveFilterEffectRenderer();

#if ENABLE(CSS_COMPOSITING)
    void updateAncestorChainHasBlendingDescendants();
    void dirtyAncestorChainHasBlendingDescendants();
#endif

    Ref<ClipRects> parentClipRects(const ClipRectsContext&) const;
    ClipRect backgroundClipRect(const ClipRectsContext&) const;

    RenderLayer* enclosingTransformedAncestor() const;

    LayoutRect getRectToExpose(const LayoutRect& visibleRect, const LayoutRect& exposeRect, bool insideFixed, const ScrollAlignment& alignX, const ScrollAlignment& alignY) const;

    // Convert a point in absolute coords into layer coords, taking transforms into account
    LayoutPoint absoluteToContents(const LayoutPoint&) const;

    void positionOverflowControls(const IntSize&);
    void updateScrollCornerStyle();
    void clearScrollCorner();
    void updateResizerStyle();
    void clearResizer();

    void drawPlatformResizerImage(GraphicsContext&, const LayoutRect& resizerCornerRect);

    void updatePagination();

    void setHasCompositingDescendant(bool b)  { m_hasCompositingDescendant = b; }

    enum class IndirectCompositingReason {
        None,
        Stacking,
        Overlap,
        BackgroundLayer,
        GraphicalEffect, // opacity, mask, filter, transform etc.
        Perspective,
        Preserve3D
    };

    void setIndirectCompositingReason(IndirectCompositingReason reason) { m_indirectCompositingReason = static_cast<unsigned>(reason); }
    IndirectCompositingReason indirectCompositingReason() const { return static_cast<IndirectCompositingReason>(m_indirectCompositingReason); }
    bool mustCompositeForIndirectReasons() const { return m_indirectCompositingReason; }

    // Returns true if z ordering would not change if this layer were a stacking container.
    bool canBeStackingContainer() const;

    friend class RenderLayerBacking;
    friend class RenderLayerCompositor;
    friend class RenderLayerModelObject;

    LayoutUnit overflowTop() const;
    LayoutUnit overflowBottom() const;
    LayoutUnit overflowLeft() const;
    LayoutUnit overflowRight() const;

    IntRect rectForHorizontalScrollbar(const IntRect& borderBoxRect) const;
    IntRect rectForVerticalScrollbar(const IntRect& borderBoxRect) const;

    LayoutUnit verticalScrollbarStart(int minX, int maxX) const;
    LayoutUnit horizontalScrollbarStart(int minX) const;

    bool overflowControlsIntersectRect(const IntRect& localRect) const;

    // The bitfields are up here so they will fall into the padding from ScrollableArea on 64-bit.

    const bool m_isRenderViewLayer : 1;
    const bool m_forcedStackingContext : 1;

    // Keeps track of whether the layer is currently resizing, so events can cause resizing to start and stop.
    bool m_inResizeMode : 1;

    bool m_scrollDimensionsDirty : 1;
    bool m_zOrderListsDirty : 1;
    bool m_normalFlowListDirty: 1;
    bool m_isNormalFlowOnly : 1;

    bool m_isSelfPaintingLayer : 1;

    // If have no self-painting descendants, we don't have to walk our children during painting. This can lead to
    // significant savings, especially if the tree has lots of non-self-painting layers grouped together (e.g. table cells).
    bool m_hasSelfPaintingLayerDescendant : 1;
    bool m_hasSelfPaintingLayerDescendantDirty : 1;

    // If we have no out of flow positioned descendants and no non-descendant
    // appears between our descendants in stacking order, then we may become a
    // stacking context.
    bool m_hasOutOfFlowPositionedDescendant : 1;
    bool m_hasOutOfFlowPositionedDescendantDirty : 1;

    bool m_needsCompositedScrolling : 1;

    // If this is true, then no non-descendant appears between any of our
    // descendants in stacking order. This is one of the requirements of being
    // able to safely become a stacking context.
    bool m_descendantsAreContiguousInStackingOrder : 1;

    bool m_usedTransparency : 1; // Tracks whether we need to close a transparent layer, i.e., whether
                                 // we ended up painting this layer or any descendants (and therefore need to
                                 // blend).
    bool m_paintingInsideReflection : 1;  // A state bit tracking if we are painting inside a replica.
    bool m_inOverflowRelayout : 1;
    unsigned m_repaintStatus : 2; // RepaintStatus

    bool m_visibleContentStatusDirty : 1;
    bool m_hasVisibleContent : 1;
    bool m_visibleDescendantStatusDirty : 1;
    bool m_hasVisibleDescendant : 1;
    bool m_registeredScrollableArea : 1;

    bool m_3DTransformedDescendantStatusDirty : 1;
    bool m_has3DTransformedDescendant : 1;  // Set on a stacking context layer that has 3D descendants anywhere
                                            // in a preserves3D hierarchy. Hint to do 3D-aware hit testing.
    bool m_hasCompositingDescendant : 1; // In the z-order tree.

    bool m_hasTransformedAncestor : 1;
    bool m_has3DTransformedAncestor : 1;

    unsigned m_indirectCompositingReason : 3;
    unsigned m_viewportConstrainedNotCompositedReason : 2;

#if PLATFORM(IOS)
    bool m_adjustForIOSCaretWhenScrolling : 1;
#if ENABLE(IOS_TOUCH_EVENTS)
    bool m_registeredAsTouchEventListenerForScrolling : 1;
#endif
    bool m_inUserScroll : 1;
    bool m_requiresScrollBoundsOriginUpdate : 1;
#endif

    bool m_containsDirtyOverlayScrollbars : 1;
    bool m_updatingMarqueePosition : 1;

#if !ASSERT_DISABLED
    bool m_layerListMutationAllowed : 1;
#endif

    bool m_hasFilterInfo : 1;

#if ENABLE(CSS_COMPOSITING)
    unsigned m_blendMode : 5;
    bool m_hasNotIsolatedCompositedBlendingDescendants : 1;
    bool m_hasNotIsolatedBlendingDescendants : 1;
    bool m_hasNotIsolatedBlendingDescendantsStatusDirty : 1;
#endif

    RenderLayerModelObject& m_renderer;

    RenderLayer* m_parent;
    RenderLayer* m_previous;
    RenderLayer* m_next;
    RenderLayer* m_first;
    RenderLayer* m_last;

    // Our current relative position offset.
    LayoutSize m_offsetForInFlowPosition;

    // Our (x,y) coordinates are in our parent layer's coordinate space.
    LayoutPoint m_topLeft;

    // The layer's width/height
    IntSize m_layerSize;

    ScrollPosition m_scrollPosition;
    std::optional<ScrollPosition> m_postLayoutScrollPosition;

    // The width/height of our scrolled area.
    IntSize m_scrollSize;

    // For layers with overflow, we have a pair of scrollbars.
    RefPtr<Scrollbar> m_hBar;
    RefPtr<Scrollbar> m_vBar;

    // For layers that establish stacking contexts, m_posZOrderList holds a sorted list of all the
    // descendant layers within the stacking context that have z-indices of 0 or greater
    // (auto will count as 0).  m_negZOrderList holds descendants within our stacking context with negative
    // z-indices.
    std::unique_ptr<Vector<RenderLayer*>> m_posZOrderList;
    std::unique_ptr<Vector<RenderLayer*>> m_negZOrderList;

    // This list contains child layers that cannot create stacking contexts.  For now it is just
    // overflow layers, but that may change in the future.
    std::unique_ptr<Vector<RenderLayer*>> m_normalFlowList;

    std::unique_ptr<ClipRectsCache> m_clipRectsCache;

    IntPoint m_cachedOverlayScrollbarOffset;

    std::unique_ptr<RenderMarquee> m_marquee; // Used for <marquee>.

    // Cached normal flow values for absolute positioned elements with static left/top values.
    LayoutUnit m_staticInlinePosition;
    LayoutUnit m_staticBlockPosition;

    std::unique_ptr<TransformationMatrix> m_transform;

    // May ultimately be extended to many replicas (with their own paint order).
    RenderPtr<RenderReplica> m_reflection;

    // Renderers to hold our custom scroll corner and resizer.
    RenderPtr<RenderScrollbarPart> m_scrollCorner;
    RenderPtr<RenderScrollbarPart> m_resizer;

    // Pointer to the enclosing RenderLayer that caused us to be paginated. It is 0 if we are not paginated.
    RenderLayer* m_enclosingPaginationLayer;

    IntRect m_blockSelectionGapsBounds;

    std::unique_ptr<RenderLayerBacking> m_backing;

    std::unique_ptr<PaintFrequencyInfo> m_paintFrequencyInfo;
};

inline void RenderLayer::clearZOrderLists()
{
    ASSERT(!isStackingContainer());
    ASSERT(m_layerListMutationAllowed);

    m_posZOrderList = nullptr;
    m_negZOrderList = nullptr;
}

inline void RenderLayer::updateZOrderLists()
{
    if (!m_zOrderListsDirty)
        return;

    if (!isStackingContainer()) {
        clearZOrderLists();
        m_zOrderListsDirty = false;
        return;
    }

    rebuildZOrderLists();
}

#if !ASSERT_DISABLED
class LayerListMutationDetector {
public:
    LayerListMutationDetector(RenderLayer* layer)
        : m_layer(layer)
        , m_previousMutationAllowedState(layer->layerListMutationAllowed())
    {
        m_layer->setLayerListMutationAllowed(false);
    }

    ~LayerListMutationDetector()
    {
        m_layer->setLayerListMutationAllowed(m_previousMutationAllowedState);
    }

private:
    RenderLayer* m_layer;
    bool m_previousMutationAllowedState;
};
#endif

void makeMatrixRenderable(TransformationMatrix&, bool has3DRendering);

bool compositedWithOwnBackingStore(const RenderLayer&);

WTF::TextStream& operator<<(WTF::TextStream&, const RenderLayer&);

} // namespace WebCore

#if ENABLE(TREE_DEBUGGING)
// Outside the WebCore namespace for ease of invocation from lldb.
void showLayerTree(const WebCore::RenderLayer*);
void showLayerTree(const WebCore::RenderObject*);
#endif
