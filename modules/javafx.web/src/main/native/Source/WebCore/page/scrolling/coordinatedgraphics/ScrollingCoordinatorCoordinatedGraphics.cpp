/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollingCoordinatorCoordinatedGraphics.h"

#if USE(COORDINATED_GRAPHICS)

#include "CoordinatedGraphicsLayer.h"
#include "FrameView.h"
#include "HostWindow.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "ScrollingConstraints.h"
#include "ScrollingStateFixedNode.h"
#include "ScrollingStateScrollingNode.h"
#include "ScrollingStateStickyNode.h"
#include "ScrollingStateTree.h"

namespace WebCore {

ScrollingCoordinatorCoordinatedGraphics::ScrollingCoordinatorCoordinatedGraphics(Page* page)
    : ScrollingCoordinator(page)
    , m_scrollingStateTree(std::make_unique<ScrollingStateTree>())
{
}

ScrollingCoordinatorCoordinatedGraphics::~ScrollingCoordinatorCoordinatedGraphics() = default;

ScrollingNodeID ScrollingCoordinatorCoordinatedGraphics::attachToStateTree(ScrollingNodeType nodeType, ScrollingNodeID newNodeID, ScrollingNodeID parentID)
{
    return m_scrollingStateTree->attachNode(nodeType, newNodeID, parentID);
}

void ScrollingCoordinatorCoordinatedGraphics::detachFromStateTree(ScrollingNodeID nodeID)
{
    auto* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (node && node->nodeType() == FixedNode)
        downcast<CoordinatedGraphicsLayer>(*static_cast<GraphicsLayer*>(node->layer())).setFixedToViewport(false);

    m_scrollingStateTree->detachNode(nodeID);
}

void ScrollingCoordinatorCoordinatedGraphics::clearStateTree()
{
    m_scrollingStateTree->clear();
}

void ScrollingCoordinatorCoordinatedGraphics::updateNodeLayer(ScrollingNodeID nodeID, GraphicsLayer* graphicsLayer)
{
    auto* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (!node)
        return;

    node->setLayer(graphicsLayer);
}

void ScrollingCoordinatorCoordinatedGraphics::updateNodeViewportConstraints(ScrollingNodeID nodeID, const ViewportConstraints& constraints)
{
    auto* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (!node)
        return;

    switch (constraints.constraintType()) {
    case ViewportConstraints::FixedPositionConstraint: {
        auto& layer = node->layer();
        if (layer.representsGraphicsLayer())
            downcast<CoordinatedGraphicsLayer>(static_cast<GraphicsLayer*>(layer))->setFixedToViewport(true);
        break;
    }
    case ViewportConstraints::StickyPositionConstraint:
        break; // FIXME : Support sticky elements.
    default:
        ASSERT_NOT_REACHED();
    }
}

void ScrollingCoordinatorCoordinatedGraphics::scrollableAreaScrollLayerDidChange(ScrollableArea& scrollableArea)
{
    auto* layer = downcast<CoordinatedGraphicsLayer>(scrollLayerForScrollableArea(scrollableArea));
    if (!layer)
        return;

    layer->setScrollableArea(&scrollableArea);
}

void ScrollingCoordinatorCoordinatedGraphics::willDestroyScrollableArea(ScrollableArea& scrollableArea)
{
    auto* layer = downcast<CoordinatedGraphicsLayer>(scrollLayerForScrollableArea(scrollableArea));
    if (!layer)
        return;

    layer->setScrollableArea(nullptr);
}

bool ScrollingCoordinatorCoordinatedGraphics::requestScrollPositionUpdate(FrameView& frameView, const IntPoint& scrollPosition)
{
    if (!frameView.delegatesScrolling())
        return false;

    frameView.hostWindow()->delegatedScrollRequested(scrollPosition);
    return true;
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
