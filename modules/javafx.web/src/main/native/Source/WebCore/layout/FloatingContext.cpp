/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
#include "FloatingContext.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "DisplayBox.h"
#include "LayoutBox.h"
#include "LayoutContainer.h"
#include "LayoutContext.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {
namespace Layout {

WTF_MAKE_ISO_ALLOCATED_IMPL(FloatingContext);

// Finding the top/left position for a new floating(F)
//  ____  ____  _____               _______
// |    || L2 ||     | <-----1---->|       |
// |    ||____||  L3 |             |   R1  |
// | L1 |      |_____|             |       |
// |____| <-------------2--------->|       |
//                                 |       |
//                                 |_______|
//
// 1. Compute the initial vertical position for (F) -> (1)
// 2. Find the corresponding floating pair (L3-R1)
// 3. Align (F) horizontally with (L3-R1) depending whether (F) is left/right positioned
// 4. Intersect (F) with (L3-R1)
// 5. If (F) does not fit, find the next floating pair (L1-R1)
// 6. Repeat until either (F) fits/no more floats.
// Note that all coordinates are in the coordinate system of the formatting root.
// The formatting root here is always the one that establishes the floating context (see inherited floating context).
// (It simply means that the float box's formatting root is not necessarily the same as the FormattingContext's root.)

class Iterator;

class FloatingPair {
public:
    bool isEmpty() const { return !m_leftIndex && !m_rightIndex; }
    const Display::Box* left() const;
    const Display::Box* right() const;
    bool intersects(const Display::Box::Rect&) const;
    LayoutUnit verticalPosition() const { return m_verticalPosition; }
    LayoutUnit bottom() const;
    bool operator==(const FloatingPair&) const;

private:
    friend class Iterator;
    FloatingPair(const FloatingState::FloatList&);

    const FloatingState::FloatList& m_floats;

    std::optional<unsigned> m_leftIndex;
    std::optional<unsigned> m_rightIndex;
    LayoutUnit m_verticalPosition;
};

class Iterator {
public:
    Iterator(const FloatingState::FloatList&, std::optional<LayoutUnit> verticalPosition);

    const FloatingPair& operator*() const { return m_current; }
    Iterator& operator++();
    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;

private:
    void set(LayoutUnit verticalPosition);

    const FloatingState::FloatList& m_floats;
    FloatingPair m_current;
};

static Iterator begin(const FloatingState& floatingState, LayoutUnit initialVerticalPosition)
{
    // Start with the inner-most floating pair for the initial vertical position.
    return Iterator(floatingState.floats(), initialVerticalPosition);
}

static Iterator end(const FloatingState& floatingState)
{
    return Iterator(floatingState.floats(), std::nullopt);
}

FloatingContext::FloatingContext(FloatingState& floatingState)
    : m_floatingState(floatingState)
{
}

Position FloatingContext::positionForFloat(const Box& layoutBox) const
{
    ASSERT(layoutBox.isFloatingPositioned());
    FloatingState::FloatItem floatItem = { layoutBox, m_floatingState };

    Position floatPosition;
    if (m_floatingState.isEmpty()) {
        // No float box on the context yet -> align it with the containing block's left/right edge.
        auto& displayBox = floatItem.displayBox();
        floatPosition = { alignWithContainingBlock(floatItem) + displayBox.marginLeft(), displayBox.top() };
    } else {
        // Find the top most position where the float box fits.
        floatPosition = floatingPosition(floatItem);
    }

    return toContainingBlock(floatItem, floatPosition);
}

std::optional<LayoutUnit> FloatingContext::verticalPositionWithClearance(const Box& layoutBox) const
{
    ASSERT(layoutBox.hasFloatClear());
    ASSERT(layoutBox.isBlockLevelBox());

    if (m_floatingState.isEmpty())
        return { };

    auto bottom = [&](std::optional<LayoutUnit> floatBottom) -> std::optional<LayoutUnit> {
        // 'bottom' is in the formatting root's coordinate system.
        if (!floatBottom)
            return { };

        // 9.5.2 Controlling flow next to floats: the 'clear' property
        // Then the amount of clearance is set to the greater of:
        //
        // 1. The amount necessary to place the border edge of the block even with the bottom outer edge of the lowest float that is to be cleared.
        // 2. The amount necessary to place the top border edge of the block at its hypothetical position.

        auto& layoutContext = this->layoutContext();
        auto& displayBox = *layoutContext.displayBoxForLayoutBox(layoutBox);
        auto rootRelativeTop = FormattingContext::mapTopLeftToAncestor(layoutContext, layoutBox, downcast<Container>(m_floatingState.root())).y;
        auto clearance = *floatBottom - rootRelativeTop;
        if (clearance <= 0)
            return { };

        // Clearance inhibits margin collapsing. Let's reset the relevant adjoining margins.
        if (auto* previousInFlowSibling = layoutBox.previousInFlowSibling()) {
            auto& previousInFlowDisplayBox = *layoutContext.displayBoxForLayoutBox(*previousInFlowSibling);

            // Since the previous inflow sibling has already been laid out, its margin is collapsed by now.
            ASSERT(!previousInFlowDisplayBox.marginBottom());
            auto collapsedMargin = displayBox.marginTop();

            // Reset previous bottom and current top margins to non-collapsing.
            previousInFlowDisplayBox.setVerticalMargin({ previousInFlowDisplayBox.marginTop(), previousInFlowDisplayBox.nonCollapsedMarginBottom() });
            displayBox.setVerticalMargin({ displayBox.nonCollapsedMarginTop(), displayBox.marginBottom() });

            auto nonCollapsedMargin = previousInFlowDisplayBox.marginBottom() + displayBox.marginTop();
            auto marginOffset = nonCollapsedMargin - collapsedMargin;
            // Move the box to the position where it would be with non-collapsed margins.
            rootRelativeTop += marginOffset;

            // Having negative clearance is also normal. It just means that the box with the non-collapsed margins is now lower than it needs to be.
            clearance -= marginOffset;
        }
        // Now adjust the box's position with the clearance.
        rootRelativeTop += clearance;
        ASSERT(*floatBottom == rootRelativeTop);

        // The return vertical position is in the containing block's coordinate system.
        auto containingBlockRootRelativeTop = FormattingContext::mapTopLeftToAncestor(layoutContext, *layoutBox.containingBlock(), downcast<Container>(m_floatingState.root())).y;
        return rootRelativeTop - containingBlockRootRelativeTop;
    };

    auto clear = layoutBox.style().clear();
    auto& formattingContextRoot = layoutBox.formattingContextRoot();

    if (clear == Clear::Left)
        return bottom(m_floatingState.leftBottom(formattingContextRoot));

    if (clear == Clear::Right)
        return bottom(m_floatingState.rightBottom(formattingContextRoot));

    if (clear == Clear::Both)
        return bottom(m_floatingState.bottom(formattingContextRoot));

    ASSERT_NOT_REACHED();
    return { };
}

Position FloatingContext::floatingPosition(const FloatingState::FloatItem& floatItem) const
{
    auto initialVerticalPosition = this->initialVerticalPosition(floatItem);
    auto& displayBox = floatItem.displayBox();
    auto marginBoxSize = displayBox.marginBox().size();

    auto end = Layout::end(m_floatingState);
    auto top = initialVerticalPosition;
    auto bottomMost = top;
    for (auto iterator = begin(m_floatingState, initialVerticalPosition); iterator != end; ++iterator) {
        ASSERT(!(*iterator).isEmpty());

        auto floats = *iterator;
        top = floats.verticalPosition();

        // Move the box horizontally so that it aligns with the current floating pair.
        auto left = alignWithFloatings(floats, floatItem);
        // Check if the box fits at this vertical position.
        if (!floats.intersects({ top, left, marginBoxSize.width(), marginBoxSize.height() }))
            return { left + displayBox.marginLeft(), top + displayBox.marginTop() };

        bottomMost = floats.bottom();
        // Move to the next floating pair.
    }

    // Passed all the floats and still does not fit?
    return { alignWithContainingBlock(floatItem) + displayBox.marginLeft(), bottomMost + displayBox.marginTop() };
}

LayoutUnit FloatingContext::initialVerticalPosition(const FloatingState::FloatItem& floatItem) const
{
    // Incoming floating cannot be placed higher than existing floats.
    // Take the static position (where the box would go if it wasn't floating) and adjust it with the last floating.
    auto marginBoxTop = floatItem.displayBox().rectWithMargin().top();

    if (auto lastFloat = m_floatingState.last())
        return std::max(marginBoxTop, lastFloat->displayBox().rectWithMargin().top());

    return marginBoxTop;
}

LayoutUnit FloatingContext::alignWithContainingBlock(const FloatingState::FloatItem& floatItem) const
{
    // If there is no floating to align with, push the box to the left/right edge of its containing block's content box.
    // (Either there's no floats at all or this box does not fit at any vertical positions where the floats are.)
    auto& containingBlockDisplayBox = floatItem.containingBlockDisplayBox();
    auto containingBlockContentBoxLeft = containingBlockDisplayBox.left() + containingBlockDisplayBox.contentBoxLeft();

    if (floatItem.layoutBox().isLeftFloatingPositioned())
        return containingBlockContentBoxLeft;

    return containingBlockContentBoxLeft + containingBlockDisplayBox.contentBoxWidth() - floatItem.displayBox().marginBox().width();
}

LayoutUnit FloatingContext::alignWithFloatings(const FloatingPair& floatingPair, const FloatingState::FloatItem& floatItem) const
{
    // Compute the horizontal position for the new floating by taking both the contining block and the current left/right floats into account.
    auto& containingBlockDisplayBox = floatItem.containingBlockDisplayBox();
    auto containingBlockContentBoxLeft = containingBlockDisplayBox.left() + containingBlockDisplayBox.contentBoxLeft();
    auto containingBlockContentBoxRight = containingBlockDisplayBox.left() + containingBlockDisplayBox.contentBoxRight();
    auto marginBoxWidth = floatItem.displayBox().marginBox().width();

    auto leftAlignedBoxLeft = containingBlockContentBoxLeft;
    auto rightAlignedBoxLeft = containingBlockContentBoxRight - marginBoxWidth;

    if (floatingPair.isEmpty()) {
        ASSERT_NOT_REACHED();
        return floatItem.layoutBox().isLeftFloatingPositioned() ? leftAlignedBoxLeft : rightAlignedBoxLeft;
    }

    if (floatItem.layoutBox().isLeftFloatingPositioned()) {
        if (auto* leftDisplayBox = floatingPair.left()) {
            auto leftFloatingBoxRight = leftDisplayBox->rectWithMargin().right();
            return std::min(std::max(leftAlignedBoxLeft, leftFloatingBoxRight), rightAlignedBoxLeft);
        }

        return leftAlignedBoxLeft;
    }

    ASSERT(floatItem.layoutBox().isRightFloatingPositioned());

    if (auto* rightDisplayBox = floatingPair.right()) {
        auto rightFloatingBoxLeft = rightDisplayBox->rectWithMargin().left();
        return std::max(std::min(rightAlignedBoxLeft, rightFloatingBoxLeft - marginBoxWidth), leftAlignedBoxLeft);
    }

    return rightAlignedBoxLeft;
}

// FIXME: find a better place for this.
Position FloatingContext::toContainingBlock(const FloatingState::FloatItem& floatItem, Position position) const
{
    // From formatting root coordinate system back to containing block's.
    if (&floatItem.containingBlock() == &m_floatingState.root())
        return position;

    auto& containingBlockDisplayBox = floatItem.containingBlockDisplayBox();
    return { position.x - containingBlockDisplayBox.left(), position.y - containingBlockDisplayBox.top() };
}

FloatingPair::FloatingPair(const FloatingState::FloatList& floats)
    : m_floats(floats)
{
}

const Display::Box* FloatingPair::left() const
{
    if (!m_leftIndex)
        return nullptr;

    ASSERT(m_floats[*m_leftIndex].layoutBox().isLeftFloatingPositioned());
    return &m_floats[*m_leftIndex].displayBox();
}

const Display::Box* FloatingPair::right() const
{
    if (!m_rightIndex)
        return nullptr;

    ASSERT(m_floats[*m_rightIndex].layoutBox().isRightFloatingPositioned());
    return &m_floats[*m_rightIndex].displayBox();
}

bool FloatingPair::intersects(const Display::Box::Rect& candidateRect) const
{
    auto intersects = [&](const Display::Box* floating, Float floatingType) {
        if (!floating)
            return false;

        auto marginRect = floating->rectWithMargin();
        // Before intersecting, check if the candidate position is too far to the left/right.
        // The new float's containing block could push the candidate position beyond the current float horizontally.
        if ((floatingType == Float::Left && candidateRect.left() < marginRect.right())
            || (floatingType == Float::Right && candidateRect.right() > marginRect.left()))
            return true;
        return marginRect.intersects(candidateRect);
    };

    if (!m_leftIndex && !m_rightIndex) {
        ASSERT_NOT_REACHED();
        return false;
    }

    if (intersects(left(), Float::Left))
        return true;

    if (intersects(right(), Float::Right))
        return true;

    return false;
}

bool FloatingPair::operator ==(const FloatingPair& other) const
{
    return m_leftIndex == other.m_leftIndex && m_rightIndex == other.m_rightIndex;
}

LayoutUnit FloatingPair::bottom() const
{
    auto* left = this->left();
    auto* right = this->right();
    ASSERT(left || right);

    auto leftBottom = left ? std::optional<LayoutUnit>(left->rectWithMargin().bottom()) : std::nullopt;
    auto rightBottom = right ? std::optional<LayoutUnit>(right->rectWithMargin().bottom()) : std::nullopt;

    if (leftBottom && rightBottom)
        return std::max(*leftBottom, *rightBottom);

    if (leftBottom)
        return *leftBottom;

    return *rightBottom;
}

Iterator::Iterator(const FloatingState::FloatList& floats, std::optional<LayoutUnit> verticalPosition)
    : m_floats(floats)
    , m_current(floats)
{
    if (verticalPosition)
        set(*verticalPosition);
}

inline static std::optional<unsigned> previousFloatingIndex(Float floatingType, const FloatingState::FloatList& floats, unsigned currentIndex)
{
    RELEASE_ASSERT(currentIndex <= floats.size());

    while (currentIndex) {
        auto& floating = floats[--currentIndex].layoutBox();
        if ((floatingType == Float::Left && floating.isLeftFloatingPositioned()) || (floatingType == Float::Right && floating.isRightFloatingPositioned()))
            return currentIndex;
    }

    return { };
}

Iterator& Iterator::operator++()
{
    if (m_current.isEmpty()) {
        ASSERT_NOT_REACHED();
        return *this;
    }

    auto findPreviousFloatingWithLowerBottom = [&](Float floatingType, unsigned currentIndex) -> std::optional<unsigned> {

        RELEASE_ASSERT(currentIndex < m_floats.size());

        // Last floating? There's certainly no previous floating at this point.
        if (!currentIndex)
            return { };

        auto currentBottom = m_floats[currentIndex].displayBox().rectWithMargin().bottom();

        std::optional<unsigned> index = currentIndex;
        while (true) {
            index = previousFloatingIndex(floatingType, m_floats, *index);
            if (!index)
                return { };

            if (m_floats[*index].displayBox().rectWithMargin().bottom() > currentBottom)
                return index;
        }

        ASSERT_NOT_REACHED();
        return { };
    };

    // 1. Take the current floating from left and right and check which one's bottom edge is positioned higher (they could be on the same vertical position too).
    // The current floats from left and right are considered the inner-most pair for the current vertical position.
    // 2. Move away from inner-most pair by picking one of the previous floats in the list(#1)
    // Ensure that the new floating's bottom edge is positioned lower than the current one -which essentially means skipping in-between floats that are positioned higher).
    // 3. Reset the vertical position and align it with the new left-right pair. These floats are now the inner-most boxes for the current vertical position.
    // As the result we have more horizontal space on the current vertical position.
    auto leftBottom = m_current.left() ? std::optional<LayoutUnit>(m_current.left()->bottom()) : std::nullopt;
    auto rightBottom = m_current.right() ? std::optional<LayoutUnit>(m_current.right()->bottom()) : std::nullopt;

    auto updateLeft = (leftBottom == rightBottom) || (!rightBottom || (leftBottom && leftBottom < rightBottom));
    auto updateRight = (leftBottom == rightBottom) || (!leftBottom || (rightBottom && leftBottom > rightBottom));

    if (updateLeft) {
        ASSERT(m_current.m_leftIndex);
        m_current.m_verticalPosition = *leftBottom;
        m_current.m_leftIndex = findPreviousFloatingWithLowerBottom(Float::Left, *m_current.m_leftIndex);
    }

    if (updateRight) {
        ASSERT(m_current.m_rightIndex);
        m_current.m_verticalPosition = *rightBottom;
        m_current.m_rightIndex = findPreviousFloatingWithLowerBottom(Float::Right, *m_current.m_rightIndex);
    }

    return *this;
}

void Iterator::set(LayoutUnit verticalPosition)
{
    // Move the iterator to the initial vertical position by starting at the inner-most floating pair (last floats on left/right).
    // 1. Check if the inner-most pair covers the vertical position.
    // 2. Move outwards from the inner-most pair until the vertical postion intersects.
    // (Note that verticalPosition has already been adjusted with the top of the last float.)

    m_current.m_verticalPosition = verticalPosition;
    // No floats at all?
    if (m_floats.isEmpty()) {
        ASSERT_NOT_REACHED();

        m_current.m_leftIndex = { };
        m_current.m_rightIndex = { };
        return;
    }

    auto findFloatingBelow = [&](Float floatingType) -> std::optional<unsigned> {

        ASSERT(!m_floats.isEmpty());

        auto index = floatingType == Float::Left ? m_current.m_leftIndex : m_current.m_rightIndex;
        // Start from the end if we don't have current yet.
        index = index.value_or(m_floats.size());
        while (true) {
            index = previousFloatingIndex(floatingType, m_floats, *index);
            if (!index)
                return { };

            auto bottom = m_floats[*index].displayBox().rectWithMargin().bottom();
            // Is this floating intrusive on this position?
            if (bottom > verticalPosition)
                return index;
        }

        return { };
    };

    m_current.m_leftIndex = findFloatingBelow(Float::Left);
    m_current.m_rightIndex = findFloatingBelow(Float::Right);

    ASSERT(!m_current.m_leftIndex || (*m_current.m_leftIndex < m_floats.size() && m_floats[*m_current.m_leftIndex].layoutBox().isLeftFloatingPositioned()));
    ASSERT(!m_current.m_rightIndex || (*m_current.m_rightIndex < m_floats.size() && m_floats[*m_current.m_rightIndex].layoutBox().isRightFloatingPositioned()));
}

bool Iterator::operator==(const Iterator& other) const
{
    return m_current == other.m_current;
}

bool Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

}
}
#endif
