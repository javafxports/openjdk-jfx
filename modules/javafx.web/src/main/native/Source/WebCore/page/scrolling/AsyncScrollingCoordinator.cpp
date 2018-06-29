/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(ASYNC_SCROLLING)
#include "AsyncScrollingCoordinator.h"

#include "DebugPageOverlays.h"
#include "Document.h"
#include "EditorClient.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsLayer.h"
#include "Logging.h"
#include "MainFrame.h"
#include "Page.h"
#include "PerformanceLoggingClient.h"
#include "ScrollAnimator.h"
#include "ScrollingConstraints.h"
#include "ScrollingStateFixedNode.h"
#include "ScrollingStateFrameScrollingNode.h"
#include "ScrollingStateOverflowScrollingNode.h"
#include "ScrollingStateStickyNode.h"
#include "ScrollingStateTree.h"
#include "Settings.h"
#include "WheelEventTestTrigger.h"
#include <wtf/text/TextStream.h>

namespace WebCore {

AsyncScrollingCoordinator::AsyncScrollingCoordinator(Page* page)
    : ScrollingCoordinator(page)
    , m_updateNodeScrollPositionTimer(*this, &AsyncScrollingCoordinator::updateScrollPositionAfterAsyncScrollTimerFired)
    , m_scrollingStateTree(std::make_unique<ScrollingStateTree>(this))
{
}

AsyncScrollingCoordinator::~AsyncScrollingCoordinator() = default;

void AsyncScrollingCoordinator::scrollingStateTreePropertiesChanged()
{
    scheduleTreeStateCommit();
}

static inline void setStateScrollingNodeSnapOffsetsAsFloat(ScrollingStateScrollingNode& node, ScrollEventAxis axis, const Vector<LayoutUnit>* snapOffsets, const Vector<ScrollOffsetRange<LayoutUnit>>* snapOffsetRanges, float deviceScaleFactor)
{
    // FIXME: Incorporate current page scale factor in snapping to device pixel. Perhaps we should just convert to float here and let UI process do the pixel snapping?
    Vector<float> snapOffsetsAsFloat;
    if (snapOffsets) {
        snapOffsetsAsFloat.reserveInitialCapacity(snapOffsets->size());
        for (auto& offset : *snapOffsets)
            snapOffsetsAsFloat.uncheckedAppend(roundToDevicePixel(offset, deviceScaleFactor, false));
    }

    Vector<ScrollOffsetRange<float>> snapOffsetRangesAsFloat;
    if (snapOffsetRanges) {
        snapOffsetRangesAsFloat.reserveInitialCapacity(snapOffsetRanges->size());
        for (auto& range : *snapOffsetRanges)
            snapOffsetRangesAsFloat.uncheckedAppend({ roundToDevicePixel(range.start, deviceScaleFactor, false), roundToDevicePixel(range.end, deviceScaleFactor, false) });
    }
    if (axis == ScrollEventAxis::Horizontal) {
        node.setHorizontalSnapOffsets(snapOffsetsAsFloat);
        node.setHorizontalSnapOffsetRanges(snapOffsetRangesAsFloat);
    } else {
        node.setVerticalSnapOffsets(snapOffsetsAsFloat);
        node.setVerticalSnapOffsetRanges(snapOffsetRangesAsFloat);
    }
}

void AsyncScrollingCoordinator::setEventTrackingRegionsDirty()
{
    m_eventTrackingRegionsDirty = true;
    // We have to schedule a commit, but the computed non-fast region may not have actually changed.
    scheduleTreeStateCommit();
}

void AsyncScrollingCoordinator::willCommitTree()
{
    updateEventTrackingRegions();
}

void AsyncScrollingCoordinator::updateEventTrackingRegions()
{
    if (!m_eventTrackingRegionsDirty)
        return;

    if (!m_scrollingStateTree->rootStateNode())
        return;

    m_scrollingStateTree->rootStateNode()->setEventTrackingRegions(absoluteEventTrackingRegions());
    m_eventTrackingRegionsDirty = false;
}

void AsyncScrollingCoordinator::frameViewLayoutUpdated(FrameView& frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    // If there isn't a root node yet, don't do anything. We'll be called again after creating one.
    if (!m_scrollingStateTree->rootStateNode())
        return;

    setEventTrackingRegionsDirty();
    if (!coordinatesScrollingForFrameView(frameView))
        return;

    auto* node = downcast<ScrollingStateFrameScrollingNode>(m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()));
    if (!node)
        return;

    auto* verticalScrollbar = frameView.verticalScrollbar();
    auto* horizontalScrollbar = frameView.horizontalScrollbar();
    node->setScrollerImpsFromScrollbars(verticalScrollbar, horizontalScrollbar);

    node->setFrameScaleFactor(frameView.frame().frameScaleFactor());
    node->setHeaderHeight(frameView.headerHeight());
    node->setFooterHeight(frameView.footerHeight());
    node->setTopContentInset(frameView.topContentInset());

    node->setVisualViewportEnabled(visualViewportEnabled());
    node->setLayoutViewport(frameView.layoutViewportRect());
    node->setMinLayoutViewportOrigin(frameView.minStableLayoutViewportOrigin());
    node->setMaxLayoutViewportOrigin(frameView.maxStableLayoutViewportOrigin());

    node->setScrollOrigin(frameView.scrollOrigin());
    node->setScrollableAreaSize(frameView.visibleContentRect().size());
    node->setTotalContentsSize(frameView.totalContentsSize());
    node->setReachableContentsSize(frameView.totalContentsSize());
    node->setFixedElementsLayoutRelativeToFrame(frameView.fixedElementsLayoutRelativeToFrame());
    node->setScrollBehaviorForFixedElements(frameView.scrollBehaviorForFixedElements());

#if ENABLE(CSS_SCROLL_SNAP)
    frameView.updateSnapOffsets();
    updateScrollSnapPropertiesWithFrameView(frameView);
#endif

#if PLATFORM(COCOA)
    auto* page = frameView.frame().page();
    if (page && page->expectsWheelEventTriggers()) {
        LOG(WheelEventTestTriggers, "    AsyncScrollingCoordinator::frameViewLayoutUpdated: Expects wheel event test trigger=%d", page->expectsWheelEventTriggers());
        node->setExpectsWheelEventTestTrigger(page->expectsWheelEventTriggers());
    }
#endif

    ScrollableAreaParameters scrollParameters;
    scrollParameters.horizontalScrollElasticity = frameView.horizontalScrollElasticity();
    scrollParameters.verticalScrollElasticity = frameView.verticalScrollElasticity();
    scrollParameters.hasEnabledHorizontalScrollbar = horizontalScrollbar && horizontalScrollbar->enabled();
    scrollParameters.hasEnabledVerticalScrollbar = verticalScrollbar && verticalScrollbar->enabled();
    scrollParameters.horizontalScrollbarMode = frameView.horizontalScrollbarMode();
    scrollParameters.verticalScrollbarMode = frameView.verticalScrollbarMode();

    node->setScrollableAreaParameters(scrollParameters);
}

void AsyncScrollingCoordinator::updateExpectsWheelEventTestTriggerWithFrameView(const FrameView& frameView)
{
    auto* page = frameView.frame().page();
    if (!page)
        return;

    auto* node = downcast<ScrollingStateFrameScrollingNode>(m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()));
    if (!node)
        return;

    node->setExpectsWheelEventTestTrigger(page->expectsWheelEventTriggers());
}

void AsyncScrollingCoordinator::frameViewEventTrackingRegionsChanged(FrameView& frameView)
{
    if (!m_scrollingStateTree->rootStateNode())
        return;

    setEventTrackingRegionsDirty();
    DebugPageOverlays::didChangeEventHandlers(frameView.frame());
}

void AsyncScrollingCoordinator::frameViewRootLayerDidChange(FrameView& frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (!coordinatesScrollingForFrameView(frameView))
        return;

    // FIXME: In some navigation scenarios, the FrameView has no RenderView or that RenderView has not been composited.
    // This needs cleaning up: https://bugs.webkit.org/show_bug.cgi?id=132724
    if (!frameView.scrollLayerID())
        return;

    // If the root layer does not have a ScrollingStateNode, then we should create one.
    ensureRootStateNodeForFrameView(frameView);
    ASSERT(m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()));

    ScrollingCoordinator::frameViewRootLayerDidChange(frameView);

    auto* node = downcast<ScrollingStateFrameScrollingNode>(m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()));
    node->setLayer(scrollLayerForFrameView(frameView));
    node->setScrolledContentsLayer(rootContentLayerForFrameView(frameView));
    node->setCounterScrollingLayer(counterScrollingLayerForFrameView(frameView));
    node->setInsetClipLayer(insetClipLayerForFrameView(frameView));
    node->setContentShadowLayer(contentShadowLayerForFrameView(frameView));
    node->setHeaderLayer(headerLayerForFrameView(frameView));
    node->setFooterLayer(footerLayerForFrameView(frameView));
    node->setScrollBehaviorForFixedElements(frameView.scrollBehaviorForFixedElements());
}

bool AsyncScrollingCoordinator::requestScrollPositionUpdate(FrameView& frameView, const IntPoint& scrollPosition)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (!coordinatesScrollingForFrameView(frameView))
        return false;

    bool isProgrammaticScroll = frameView.inProgrammaticScroll();
    if (isProgrammaticScroll || frameView.frame().document()->pageCacheState() != Document::NotInPageCache)
        updateScrollPositionAfterAsyncScroll(frameView.scrollLayerID(), scrollPosition, std::nullopt, isProgrammaticScroll, ScrollingLayerPositionAction::Set);

    // If this frame view's document is being put into the page cache, we don't want to update our
    // main frame scroll position. Just let the FrameView think that we did.
    if (frameView.frame().document()->pageCacheState() != Document::NotInPageCache)
        return true;

    auto* stateNode = downcast<ScrollingStateScrollingNode>(m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()));
    if (!stateNode)
        return false;

    stateNode->setRequestedScrollPosition(scrollPosition, isProgrammaticScroll);
    return true;
}

void AsyncScrollingCoordinator::scheduleUpdateScrollPositionAfterAsyncScroll(ScrollingNodeID nodeID, const FloatPoint& scrollPosition, const std::optional<FloatPoint>& layoutViewportOrigin, bool programmaticScroll, ScrollingLayerPositionAction scrollingLayerPositionAction)
{
    ScheduledScrollUpdate scrollUpdate(nodeID, scrollPosition, layoutViewportOrigin, programmaticScroll, scrollingLayerPositionAction);

    // For programmatic scrolls, requestScrollPositionUpdate() has already called updateScrollPositionAfterAsyncScroll().
    if (programmaticScroll)
        return;

    if (m_updateNodeScrollPositionTimer.isActive()) {
        if (m_scheduledScrollUpdate.matchesUpdateType(scrollUpdate)) {
            m_scheduledScrollUpdate.scrollPosition = scrollPosition;
            m_scheduledScrollUpdate.layoutViewportOrigin = layoutViewportOrigin;
            return;
        }

        // If the parameters don't match what was previously scheduled, dispatch immediately.
        m_updateNodeScrollPositionTimer.stop();
        updateScrollPositionAfterAsyncScroll(m_scheduledScrollUpdate.nodeID, m_scheduledScrollUpdate.scrollPosition, m_scheduledScrollUpdate.layoutViewportOrigin, m_scheduledScrollUpdate.isProgrammaticScroll, m_scheduledScrollUpdate.updateLayerPositionAction);
        updateScrollPositionAfterAsyncScroll(nodeID, scrollPosition, layoutViewportOrigin, programmaticScroll, scrollingLayerPositionAction);
        return;
    }

    m_scheduledScrollUpdate = scrollUpdate;
    m_updateNodeScrollPositionTimer.startOneShot(0_s);
}

void AsyncScrollingCoordinator::updateScrollPositionAfterAsyncScrollTimerFired()
{
    updateScrollPositionAfterAsyncScroll(m_scheduledScrollUpdate.nodeID, m_scheduledScrollUpdate.scrollPosition, m_scheduledScrollUpdate.layoutViewportOrigin, m_scheduledScrollUpdate.isProgrammaticScroll, m_scheduledScrollUpdate.updateLayerPositionAction);
}

FrameView* AsyncScrollingCoordinator::frameViewForScrollingNode(ScrollingNodeID scrollingNodeID) const
{
    if (!m_scrollingStateTree->rootStateNode())
        return nullptr;

    if (scrollingNodeID == m_scrollingStateTree->rootStateNode()->scrollingNodeID())
        return m_page->mainFrame().view();

    auto* stateNode = m_scrollingStateTree->stateNodeForID(scrollingNodeID);
    if (!stateNode)
        return nullptr;

    // Find the enclosing frame scrolling node.
    auto* parentNode = stateNode;
    while (parentNode && !parentNode->isFrameScrollingNode())
        parentNode = parentNode->parent();

    if (!parentNode)
        return nullptr;

    // Walk the frame tree to find the matching FrameView. This is not ideal, but avoids back pointers to FrameViews
    // from ScrollingTreeStateNodes.
    for (Frame* frame = &m_page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (auto* view = frame->view()) {
            if (view->scrollLayerID() == parentNode->scrollingNodeID())
                return view;
        }
    }

    return nullptr;
}

void AsyncScrollingCoordinator::updateScrollPositionAfterAsyncScroll(ScrollingNodeID scrollingNodeID, const FloatPoint& scrollPosition, std::optional<FloatPoint> layoutViewportOrigin, bool programmaticScroll, ScrollingLayerPositionAction scrollingLayerPositionAction)
{
    ASSERT(isMainThread());

    if (!m_page)
        return;

    auto* frameViewPtr = frameViewForScrollingNode(scrollingNodeID);
    if (!frameViewPtr)
        return;

    LOG_WITH_STREAM(Scrolling, stream << "AsyncScrollingCoordinator::updateScrollPositionAfterAsyncScroll node " << scrollingNodeID << " scrollPosition " << scrollPosition << " action " << scrollingLayerPositionAction);

    auto& frameView = *frameViewPtr;

    if (scrollingNodeID == frameView.scrollLayerID()) {
        reconcileScrollingState(frameView, scrollPosition, layoutViewportOrigin, programmaticScroll, ViewportRectStability::Stable, scrollingLayerPositionAction);

#if PLATFORM(COCOA)
        if (m_page->expectsWheelEventTriggers()) {
            frameView.scrollAnimator().setWheelEventTestTrigger(m_page->testTrigger());
            if (const auto& trigger = m_page->testTrigger())
                trigger->removeTestDeferralForReason(reinterpret_cast<WheelEventTestTrigger::ScrollableAreaIdentifier>(scrollingNodeID), WheelEventTestTrigger::ScrollingThreadSyncNeeded);
        }
#endif

        return;
    }

    // Overflow-scroll area.
    if (auto* scrollableArea = frameView.scrollableAreaForScrollLayerID(scrollingNodeID)) {
        scrollableArea->setIsUserScroll(scrollingLayerPositionAction == ScrollingLayerPositionAction::Sync);
        scrollableArea->scrollToOffsetWithoutAnimation(scrollPosition);
        scrollableArea->setIsUserScroll(false);
        if (scrollingLayerPositionAction == ScrollingLayerPositionAction::Set)
            m_page->editorClient().overflowScrollPositionChanged();

#if PLATFORM(COCOA)
        if (m_page->expectsWheelEventTriggers()) {
            frameView.scrollAnimator().setWheelEventTestTrigger(m_page->testTrigger());
            if (const auto& trigger = m_page->testTrigger())
                trigger->removeTestDeferralForReason(reinterpret_cast<WheelEventTestTrigger::ScrollableAreaIdentifier>(scrollingNodeID), WheelEventTestTrigger::ScrollingThreadSyncNeeded);
        }
#endif
    }
}

void AsyncScrollingCoordinator::reconcileScrollingState(FrameView& frameView, const FloatPoint& scrollPosition, const LayoutViewportOriginOrOverrideRect& layoutViewportOriginOrOverrideRect, bool programmaticScroll, ViewportRectStability viewportRectStability, ScrollingLayerPositionAction scrollingLayerPositionAction)
{
    bool oldProgrammaticScroll = frameView.inProgrammaticScroll();
    frameView.setInProgrammaticScroll(programmaticScroll);

    LOG_WITH_STREAM(Scrolling, stream << getpid() << " AsyncScrollingCoordinator " << this << " reconcileScrollingState scrollPosition " << scrollPosition << " programmaticScroll " << programmaticScroll << " stability " << viewportRectStability << " " << scrollingLayerPositionAction);

    std::optional<FloatRect> layoutViewportRect;

    WTF::switchOn(layoutViewportOriginOrOverrideRect,
        [&frameView](std::optional<FloatPoint> origin) {
            if (origin)
                frameView.setBaseLayoutViewportOrigin(LayoutPoint(origin.value()), FrameView::TriggerLayoutOrNot::No);
        }, [&frameView, &layoutViewportRect, viewportRectStability, visualViewportEnabled = visualViewportEnabled()](std::optional<FloatRect> overrideRect) {
            if (!overrideRect)
                return;

            layoutViewportRect = overrideRect;
            if (visualViewportEnabled && viewportRectStability != ViewportRectStability::ChangingObscuredInsetsInteractively)
                frameView.setLayoutViewportOverrideRect(LayoutRect(overrideRect.value()), viewportRectStability == ViewportRectStability::Stable ? FrameView::TriggerLayoutOrNot::Yes : FrameView::TriggerLayoutOrNot::No);
#if PLATFORM(IOS)
            else if (viewportRectStability == ViewportRectStability::Stable)
                frameView.setCustomFixedPositionLayoutRect(enclosingIntRect(overrideRect.value()));
#endif
        }
    );

    frameView.setConstrainsScrollingToContentEdge(false);
    frameView.notifyScrollPositionChanged(roundedIntPoint(scrollPosition));
    frameView.setConstrainsScrollingToContentEdge(true);
    frameView.setInProgrammaticScroll(oldProgrammaticScroll);

    if (!programmaticScroll && scrollingLayerPositionAction != ScrollingLayerPositionAction::Set) {
        if (viewportRectStability == ViewportRectStability::Stable)
            reconcileViewportConstrainedLayerPositions(frameView.rectForFixedPositionLayout(), scrollingLayerPositionAction);
        else if (layoutViewportRect)
            reconcileViewportConstrainedLayerPositions(LayoutRect(layoutViewportRect.value()), scrollingLayerPositionAction);
    }

    auto* scrollLayer = scrollLayerForFrameView(frameView);
    if (!scrollLayer)
        return;

    auto* counterScrollingLayer = counterScrollingLayerForFrameView(frameView);
    auto* insetClipLayer = insetClipLayerForFrameView(frameView);
    auto* contentShadowLayer = contentShadowLayerForFrameView(frameView);
    auto* scrolledContentsLayer = rootContentLayerForFrameView(frameView);
    auto* headerLayer = headerLayerForFrameView(frameView);
    auto* footerLayer = footerLayerForFrameView(frameView);

    ASSERT(frameView.scrollPosition() == roundedIntPoint(scrollPosition));
    LayoutPoint scrollPositionForFixed = frameView.scrollPositionForFixedPosition();
    float topContentInset = frameView.topContentInset();

    FloatPoint positionForInsetClipLayer;
    if (insetClipLayer)
        positionForInsetClipLayer = FloatPoint(insetClipLayer->position().x(), FrameView::yPositionForInsetClipLayer(scrollPosition, topContentInset));
    FloatPoint positionForContentsLayer = frameView.positionForRootContentLayer();

    FloatPoint positionForHeaderLayer = FloatPoint(scrollPositionForFixed.x(), FrameView::yPositionForHeaderLayer(scrollPosition, topContentInset));
    FloatPoint positionForFooterLayer = FloatPoint(scrollPositionForFixed.x(),
        FrameView::yPositionForFooterLayer(scrollPosition, topContentInset, frameView.totalContentsSize().height(), frameView.footerHeight()));

    if (programmaticScroll || scrollingLayerPositionAction == ScrollingLayerPositionAction::Set) {
        scrollLayer->setPosition(-frameView.scrollPosition());
        if (counterScrollingLayer)
            counterScrollingLayer->setPosition(scrollPositionForFixed);
        if (insetClipLayer)
            insetClipLayer->setPosition(positionForInsetClipLayer);
        if (contentShadowLayer)
            contentShadowLayer->setPosition(positionForContentsLayer);
        if (scrolledContentsLayer)
            scrolledContentsLayer->setPosition(positionForContentsLayer);
        if (headerLayer)
            headerLayer->setPosition(positionForHeaderLayer);
        if (footerLayer)
            footerLayer->setPosition(positionForFooterLayer);
    } else {
        scrollLayer->syncPosition(-frameView.scrollPosition());
        if (counterScrollingLayer)
            counterScrollingLayer->syncPosition(scrollPositionForFixed);
        if (insetClipLayer)
            insetClipLayer->syncPosition(positionForInsetClipLayer);
        if (contentShadowLayer)
            contentShadowLayer->syncPosition(positionForContentsLayer);
        if (scrolledContentsLayer)
            scrolledContentsLayer->syncPosition(positionForContentsLayer);
        if (headerLayer)
            headerLayer->syncPosition(positionForHeaderLayer);
        if (footerLayer)
            footerLayer->syncPosition(positionForFooterLayer);
    }
}

void AsyncScrollingCoordinator::scrollableAreaScrollbarLayerDidChange(ScrollableArea& scrollableArea, ScrollbarOrientation orientation)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (&scrollableArea != static_cast<ScrollableArea*>(m_page->mainFrame().view()))
        return;

    if (orientation == VerticalScrollbar)
        scrollableArea.verticalScrollbarLayerDidChange();
    else
        scrollableArea.horizontalScrollbarLayerDidChange();
}

ScrollingNodeID AsyncScrollingCoordinator::attachToStateTree(ScrollingNodeType nodeType, ScrollingNodeID newNodeID, ScrollingNodeID parentID)
{
    return m_scrollingStateTree->attachNode(nodeType, newNodeID, parentID);
}

void AsyncScrollingCoordinator::detachFromStateTree(ScrollingNodeID nodeID)
{
    m_scrollingStateTree->detachNode(nodeID);
}

void AsyncScrollingCoordinator::clearStateTree()
{
    m_scrollingStateTree->clear();
}

void AsyncScrollingCoordinator::reconcileViewportConstrainedLayerPositions(const LayoutRect& viewportRect, ScrollingLayerPositionAction action)
{
    if (!m_scrollingStateTree->rootStateNode())
        return;

    LOG_WITH_STREAM(Scrolling, stream << getpid() << " AsyncScrollingCoordinator::reconcileViewportConstrainedLayerPositions for viewport rect " << viewportRect);

    m_scrollingStateTree->rootStateNode()->reconcileLayerPositionForViewportRect(viewportRect, action);
}

void AsyncScrollingCoordinator::ensureRootStateNodeForFrameView(FrameView& frameView)
{
    ASSERT(frameView.scrollLayerID());
    if (m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()))
        return;

    // For non-main frames, it is only possible to arrive in this function from
    // RenderLayerCompositor::updateBacking where the node has already been created.
    ASSERT(frameView.frame().isMainFrame());
    attachToStateTree(FrameScrollingNode, frameView.scrollLayerID(), 0);
}

void AsyncScrollingCoordinator::updateFrameScrollingNode(ScrollingNodeID nodeID, GraphicsLayer* layer, GraphicsLayer* scrolledContentsLayer, GraphicsLayer* counterScrollingLayer, GraphicsLayer* insetClipLayer, const ScrollingGeometry* scrollingGeometry)
{
    auto* node = downcast<ScrollingStateFrameScrollingNode>(m_scrollingStateTree->stateNodeForID(nodeID));
    ASSERT(node);
    if (!node)
        return;

    node->setLayer(layer);
    node->setInsetClipLayer(insetClipLayer);
    node->setScrolledContentsLayer(scrolledContentsLayer);
    node->setCounterScrollingLayer(counterScrollingLayer);

    if (scrollingGeometry) {
        node->setScrollOrigin(scrollingGeometry->scrollOrigin);
        node->setScrollPosition(scrollingGeometry->scrollPosition);
        node->setTotalContentsSize(scrollingGeometry->contentSize);
        node->setReachableContentsSize(scrollingGeometry->reachableContentSize);
        node->setScrollableAreaSize(scrollingGeometry->scrollableAreaSize);
    }
}

void AsyncScrollingCoordinator::updateOverflowScrollingNode(ScrollingNodeID nodeID, GraphicsLayer* layer, GraphicsLayer* scrolledContentsLayer, const ScrollingGeometry* scrollingGeometry)
{
    auto* node = downcast<ScrollingStateOverflowScrollingNode>(m_scrollingStateTree->stateNodeForID(nodeID));
    ASSERT(node);
    if (!node)
        return;

    node->setLayer(layer);
    node->setScrolledContentsLayer(scrolledContentsLayer);

    if (scrollingGeometry) {
        node->setScrollOrigin(scrollingGeometry->scrollOrigin);
        node->setScrollPosition(scrollingGeometry->scrollPosition);
        node->setTotalContentsSize(scrollingGeometry->contentSize);
        node->setReachableContentsSize(scrollingGeometry->reachableContentSize);
        node->setScrollableAreaSize(scrollingGeometry->scrollableAreaSize);
#if ENABLE(CSS_SCROLL_SNAP)
        setStateScrollingNodeSnapOffsetsAsFloat(*node, ScrollEventAxis::Horizontal, &scrollingGeometry->horizontalSnapOffsets, &scrollingGeometry->horizontalSnapOffsetRanges, m_page->deviceScaleFactor());
        setStateScrollingNodeSnapOffsetsAsFloat(*node, ScrollEventAxis::Vertical, &scrollingGeometry->verticalSnapOffsets, &scrollingGeometry->verticalSnapOffsetRanges, m_page->deviceScaleFactor());
        node->setCurrentHorizontalSnapPointIndex(scrollingGeometry->currentHorizontalSnapPointIndex);
        node->setCurrentVerticalSnapPointIndex(scrollingGeometry->currentVerticalSnapPointIndex);
#endif
    }
}

void AsyncScrollingCoordinator::updateNodeLayer(ScrollingNodeID nodeID, GraphicsLayer* graphicsLayer)
{
    auto* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (!node)
        return;

    node->setLayer(graphicsLayer);
}

void AsyncScrollingCoordinator::updateNodeViewportConstraints(ScrollingNodeID nodeID, const ViewportConstraints& constraints)
{
    auto* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (!node)
        return;

    switch (constraints.constraintType()) {
    case ViewportConstraints::FixedPositionConstraint: {
        auto& fixedNode = downcast<ScrollingStateFixedNode>(*node);
        fixedNode.updateConstraints((const FixedPositionViewportConstraints&)constraints);
        break;
    }
    case ViewportConstraints::StickyPositionConstraint: {
        auto& stickyNode = downcast<ScrollingStateStickyNode>(*node);
        stickyNode.updateConstraints((const StickyPositionViewportConstraints&)constraints);
        break;
    }
    }
}

void AsyncScrollingCoordinator::setSynchronousScrollingReasons(FrameView& frameView, SynchronousScrollingReasons reasons)
{
    auto* scrollingStateNode = static_cast<ScrollingStateFrameScrollingNode*>(m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()));
    if (!scrollingStateNode)
        return;

    // The FrameView's GraphicsLayer is likely to be out-of-synch with the PlatformLayer
    // at this point. So we'll update it before we switch back to main thread scrolling
    // in order to avoid layer positioning bugs.
    if (reasons)
        updateScrollLayerPosition(frameView);
    scrollingStateNode->setSynchronousScrollingReasons(reasons);
}

void AsyncScrollingCoordinator::updateScrollLayerPosition(FrameView& frameView)
{
    ASSERT(isMainThread());
    if (auto* scrollLayer = scrollLayerForFrameView(frameView))
        scrollLayer->setPosition(-frameView.scrollPosition());
}

bool AsyncScrollingCoordinator::isRubberBandInProgress() const
{
    return scrollingTree()->isRubberBandInProgress();
}

void AsyncScrollingCoordinator::setScrollPinningBehavior(ScrollPinningBehavior pinning)
{
    scrollingTree()->setScrollPinningBehavior(pinning);
}

bool AsyncScrollingCoordinator::visualViewportEnabled() const
{
    return m_page->mainFrame().settings().visualViewportEnabled();
}

String AsyncScrollingCoordinator::scrollingStateTreeAsText(ScrollingStateTreeAsTextBehavior behavior) const
{
    if (m_scrollingStateTree->rootStateNode()) {
        if (m_eventTrackingRegionsDirty)
            m_scrollingStateTree->rootStateNode()->setEventTrackingRegions(absoluteEventTrackingRegions());
        return m_scrollingStateTree->rootStateNode()->scrollingStateTreeAsText(behavior);
    }

    return String();
}

#if PLATFORM(COCOA)
void AsyncScrollingCoordinator::setActiveScrollSnapIndices(ScrollingNodeID scrollingNodeID, unsigned horizontalIndex, unsigned verticalIndex)
{
    ASSERT(isMainThread());

    if (!m_page)
        return;

    auto* frameView = frameViewForScrollingNode(scrollingNodeID);
    if (!frameView)
        return;

    if (scrollingNodeID == frameView->scrollLayerID()) {
        frameView->setCurrentHorizontalSnapPointIndex(horizontalIndex);
        frameView->setCurrentVerticalSnapPointIndex(verticalIndex);
        return;
    }

    // Overflow-scroll area.
    if (auto* scrollableArea = frameView->scrollableAreaForScrollLayerID(scrollingNodeID)) {
        scrollableArea->setCurrentHorizontalSnapPointIndex(horizontalIndex);
        scrollableArea->setCurrentVerticalSnapPointIndex(verticalIndex);
    }
}

void AsyncScrollingCoordinator::deferTestsForReason(WheelEventTestTrigger::ScrollableAreaIdentifier identifier, WheelEventTestTrigger::DeferTestTriggerReason reason) const
{
    ASSERT(isMainThread());
    if (!m_page || !m_page->expectsWheelEventTriggers())
        return;

    if (const auto& trigger = m_page->testTrigger()) {
        LOG(WheelEventTestTriggers, "    (!) AsyncScrollingCoordinator::deferTestsForReason: Deferring %p for reason %d.", identifier, reason);
        trigger->deferTestsForReason(identifier, reason);
    }
}

void AsyncScrollingCoordinator::removeTestDeferralForReason(WheelEventTestTrigger::ScrollableAreaIdentifier identifier, WheelEventTestTrigger::DeferTestTriggerReason reason) const
{
    ASSERT(isMainThread());
    if (!m_page || !m_page->expectsWheelEventTriggers())
        return;

    if (const auto& trigger = m_page->testTrigger()) {
        LOG(WheelEventTestTriggers, "    (!) AsyncScrollingCoordinator::removeTestDeferralForReason: Deferring %p for reason %d.", identifier, reason);
        trigger->removeTestDeferralForReason(identifier, reason);
    }
}
#endif

#if ENABLE(CSS_SCROLL_SNAP)
bool AsyncScrollingCoordinator::isScrollSnapInProgress() const
{
    return scrollingTree()->isScrollSnapInProgress();
}

void AsyncScrollingCoordinator::updateScrollSnapPropertiesWithFrameView(const FrameView& frameView)
{
    if (auto node = downcast<ScrollingStateFrameScrollingNode>(m_scrollingStateTree->stateNodeForID(frameView.scrollLayerID()))) {
        setStateScrollingNodeSnapOffsetsAsFloat(*node, ScrollEventAxis::Horizontal, frameView.horizontalSnapOffsets(), frameView.horizontalSnapOffsetRanges(), m_page->deviceScaleFactor());
        setStateScrollingNodeSnapOffsetsAsFloat(*node, ScrollEventAxis::Vertical, frameView.verticalSnapOffsets(), frameView.verticalSnapOffsetRanges(), m_page->deviceScaleFactor());
        node->setCurrentHorizontalSnapPointIndex(frameView.currentHorizontalSnapPointIndex());
        node->setCurrentVerticalSnapPointIndex(frameView.currentVerticalSnapPointIndex());
    }
}
#endif

void AsyncScrollingCoordinator::reportExposedUnfilledArea(MonotonicTime timestamp, unsigned unfilledArea)
{
    if (m_page && m_page->performanceLoggingClient())
        m_page->performanceLoggingClient()->logScrollingEvent(PerformanceLoggingClient::ScrollingEvent::ExposedTilelessArea, timestamp, unfilledArea);
}

void AsyncScrollingCoordinator::reportSynchronousScrollingReasonsChanged(MonotonicTime timestamp, SynchronousScrollingReasons reasons)
{
    if (m_page && m_page->performanceLoggingClient())
        m_page->performanceLoggingClient()->logScrollingEvent(PerformanceLoggingClient::ScrollingEvent::SwitchedScrollingMode, timestamp, reasons);
}

} // namespace WebCore

#endif // ENABLE(ASYNC_SCROLLING)
