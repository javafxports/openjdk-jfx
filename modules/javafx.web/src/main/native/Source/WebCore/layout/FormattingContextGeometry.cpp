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
#include "FloatingState.h"
#include "FormattingContext.h"
#include "FormattingState.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

namespace WebCore {
namespace Layout {

static LayoutUnit contentHeightForFormattingContextRoot(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.style().logicalHeight().isAuto() && (layoutBox.establishesFormattingContext() || layoutBox.isDocumentBox()));

    // 10.6.7 'Auto' heights for block formatting context roots

    // If it only has inline-level children, the height is the distance between the top of the topmost line box and the bottom of the bottommost line box.
    // If it has block-level children, the height is the distance between the top margin-edge of the topmost block-level
    // child box and the bottom margin-edge of the bottommost block-level child box.

    // In addition, if the element has any floating descendants whose bottom margin edge is below the element's bottom content edge,
    // then the height is increased to include those edges. Only floats that participate in this block formatting context are taken
    // into account, e.g., floats inside absolutely positioned descendants or other floats are not.
    if (!is<Container>(layoutBox) || !downcast<Container>(layoutBox).hasInFlowOrFloatingChild())
        return 0;

    auto& formattingRootContainer = downcast<Container>(layoutBox);
    if (formattingRootContainer.establishesInlineFormattingContext())
        return 0;

    auto* firstDisplayBox = layoutContext.displayBoxForLayoutBox(*formattingRootContainer.firstInFlowChild());
    auto* lastDisplayBox = layoutContext.displayBoxForLayoutBox(*formattingRootContainer.lastInFlowChild());
    auto top = firstDisplayBox->rectWithMargin().top();
    auto bottom = lastDisplayBox->rectWithMargin().bottom();

    auto* formattingContextRoot = &layoutBox;
    // TODO: The document renderer is not a formatting context root by default at all. Need to find out what it is.
    if (!layoutBox.establishesFormattingContext()) {
        ASSERT(layoutBox.isDocumentBox());
        formattingContextRoot = &layoutBox.formattingContextRoot();
    }

    auto floatsBottom = layoutContext.establishedFormattingState(*formattingContextRoot).floatingState().bottom(*formattingContextRoot);
    if (floatsBottom)
        bottom = std::max(*floatsBottom, bottom);

    auto computedHeight = bottom - top;
    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Height] -> content height for formatting context root -> height(" << computedHeight << "px) layoutBox("<< &layoutBox << ")");
    return computedHeight;
}

std::optional<LayoutUnit> FormattingContext::Geometry::computedValueIfNotAuto(const Length& geometryProperty, LayoutUnit containingBlockWidth)
{
    if (geometryProperty.isAuto())
        return std::nullopt;
    return valueForLength(geometryProperty, containingBlockWidth);
}

std::optional<LayoutUnit> FormattingContext::Geometry::fixedValue(const Length& geometryProperty)
{
    if (!geometryProperty.isFixed())
        return std::nullopt;
    return { geometryProperty.value() };
}

static LayoutUnit staticVerticalPositionForOutOfFlowPositioned(const LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned());

    // For the purposes of this section and the next, the term "static position" (of an element) refers, roughly, to the position an element would have
    // had in the normal flow. More precisely, the static position for 'top' is the distance from the top edge of the containing block to the top margin
    // edge of a hypothetical box that would have been the first box of the element if its specified 'position' value had been 'static' and its specified
    // 'float' had been 'none' and its specified 'clear' had been 'none'. (Note that due to the rules in section 9.7 this might require also assuming a different
    // computed value for 'display'.) The value is negative if the hypothetical box is above the containing block.

    // Start with this box's border box offset from the parent's border box.
    LayoutUnit top;
    if (auto* previousInFlowSibling = layoutBox.previousInFlowSibling()) {
        // Add sibling offset
        auto& previousInFlowDisplayBox = *layoutContext.displayBoxForLayoutBox(*previousInFlowSibling);
        top += previousInFlowDisplayBox.bottom() + previousInFlowDisplayBox.nonCollapsedMarginBottom();
    } else {
        ASSERT(layoutBox.parent());
        top = layoutContext.displayBoxForLayoutBox(*layoutBox.parent())->contentBoxTop();
    }

    // Resolve top all the way up to the containing block.
    auto* containingBlock = layoutBox.containingBlock();
    for (auto* container = layoutBox.parent(); container != containingBlock; container = container->containingBlock()) {
        auto& displayBox = *layoutContext.displayBoxForLayoutBox(*container);
        // Display::Box::top is the border box top position in its containing block's coordinate system.
        top += displayBox.top();
        ASSERT(!container->isPositioned());
    }
    // FIXME: floatings need to be taken into account.
    return top;
}

static LayoutUnit staticHorizontalPositionForOutOfFlowPositioned(const LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned());
    // See staticVerticalPositionForOutOfFlowPositioned for the definition of the static position.

    // Start with this box's border box offset from the parent's border box.
    ASSERT(layoutBox.parent());
    auto left = layoutContext.displayBoxForLayoutBox(*layoutBox.parent())->contentBoxLeft();

    // Resolve left all the way up to the containing block.
    auto* containingBlock = layoutBox.containingBlock();
    for (auto* container = layoutBox.parent(); container != containingBlock; container = container->containingBlock()) {
        auto& displayBox = *layoutContext.displayBoxForLayoutBox(*container);
        // Display::Box::left is the border box left position in its containing block's coordinate system.
        left += displayBox.left();
        ASSERT(!container->isPositioned());
    }
    // FIXME: floatings need to be taken into account.
    return left;
}

LayoutUnit FormattingContext::Geometry::shrinkToFitWidth(LayoutContext& layoutContext, const FormattingContext& formattingContext, const Box& layoutBox)
{
    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Width] -> shrink to fit -> unsupported -> width(" << LayoutUnit { } << "px) layoutBox: " << &layoutBox << ")");
    // Calculation of the shrink-to-fit width is similar to calculating the width of a table cell using the automatic table layout algorithm.
    // Roughly: calculate the preferred width by formatting the content without breaking lines other than where explicit line breaks occur,
    // and also calculate the preferred minimum width, e.g., by trying all possible line breaks. CSS 2.2 does not define the exact algorithm.
    // Thirdly, find the available width: in this case, this is the width of the containing block minus the used values of 'margin-left', 'border-left-width',
    // 'padding-left', 'padding-right', 'border-right-width', 'margin-right', and the widths of any relevant scroll bars.

    // Then the shrink-to-fit width is: min(max(preferred minimum width, available width), preferred width).
    auto availableWidth = layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock())->width();
    auto instrinsicWidthConstraints = formattingContext.instrinsicWidthConstraints(layoutContext, layoutBox);
    return std::min(std::max(instrinsicWidthConstraints.minimum, availableWidth), instrinsicWidthConstraints.maximum);
}

VerticalGeometry FormattingContext::Geometry::outOfFlowNonReplacedVerticalGeometry(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned() && !layoutBox.replaced());

    // 10.6.4 Absolutely positioned, non-replaced elements
    //
    // For absolutely positioned elements, the used values of the vertical dimensions must satisfy this constraint:
    // 'top' + 'margin-top' + 'border-top-width' + 'padding-top' + 'height' + 'padding-bottom' + 'border-bottom-width' + 'margin-bottom' + 'bottom'
    // = height of containing block

    // If all three of 'top', 'height', and 'bottom' are auto, set 'top' to the static position and apply rule number three below.

    // If none of the three are 'auto': If both 'margin-top' and 'margin-bottom' are 'auto', solve the equation under the extra
    // constraint that the two margins get equal values. If one of 'margin-top' or 'margin-bottom' is 'auto', solve the equation for that value.
    // If the values are over-constrained, ignore the value for 'bottom' and solve for that value.

    // Otherwise, pick the one of the following six rules that applies.

    // 1. 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then the height is based on the content per 10.6.7,
    //     set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
    // 2. 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then set 'top' to the static position, set 'auto' values for
    //    'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
    // 3. 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then the height is based on the content per 10.6.7, set 'auto'
    //     values for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
    // 4. 'top' is 'auto', 'height' and 'bottom' are not 'auto', then set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
    // 5. 'height' is 'auto', 'top' and 'bottom' are not 'auto', then 'auto' values for 'margin-top' and 'margin-bottom' are set to 0 and solve for 'height'
    // 6. 'bottom' is 'auto', 'top' and 'height' are not 'auto', then set 'auto' values for 'margin-top' and 'margin-bottom' to 0 and solve for 'bottom'

    auto& style = layoutBox.style();
    auto& displayBox = *layoutContext.displayBoxForLayoutBox(layoutBox);
    auto& containingBlockDisplayBox = *layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock());
    auto containingBlockHeight = containingBlockDisplayBox.height();
    auto containingBlockWidth = containingBlockDisplayBox.width();

    auto top = computedValueIfNotAuto(style.logicalTop(), containingBlockWidth);
    auto bottom = computedValueIfNotAuto(style.logicalBottom(), containingBlockWidth);
    auto height = computedValueIfNotAuto(style.logicalHeight(), containingBlockHeight);
    auto marginTop = computedValueIfNotAuto(style.marginTop(), containingBlockWidth);
    auto marginBottom = computedValueIfNotAuto(style.marginBottom(), containingBlockWidth);
    auto paddingTop = displayBox.paddingTop().value_or(0);
    auto paddingBottom = displayBox.paddingBottom().value_or(0);
    auto borderTop = displayBox.borderTop();
    auto borderBottom = displayBox.borderBottom();

    if (!top && !height && !bottom)
        top = staticVerticalPositionForOutOfFlowPositioned(layoutContext, layoutBox);

    if (top && height && bottom) {
        if (!marginTop && !marginBottom) {
            auto marginTopAndBottom = containingBlockHeight - (*top + borderTop + paddingTop + *height + paddingBottom + borderBottom + *bottom);
            marginTop = marginBottom = marginTopAndBottom / 2;
        } else if (!marginTop)
            marginTop = containingBlockHeight - (*top + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom + *bottom);
        else
            marginBottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *bottom);
        // Over-constrained?
        auto boxHeight = *top + *marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom + *bottom;
        if (boxHeight > containingBlockHeight)
            bottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom);
    }

    if (!top && !height && bottom) {
        // #1
        height = contentHeightForFormattingContextRoot(layoutContext, layoutBox);
        marginTop = marginTop.value_or(0);
        marginBottom = marginBottom.value_or(0);
        top = containingBlockHeight - (*marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom + *bottom);
    }

    if (!top && !bottom && height) {
        // #2
        top = staticVerticalPositionForOutOfFlowPositioned(layoutContext, layoutBox);
        marginTop = marginTop.value_or(0);
        marginBottom = marginBottom.value_or(0);
        bottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom);
    }

    if (!height && !bottom && top) {
        // #3
        height = contentHeightForFormattingContextRoot(layoutContext, layoutBox);
        marginTop = marginTop.value_or(0);
        marginBottom = marginBottom.value_or(0);
        bottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom);
    }

    if (!top && height && bottom) {
        // #4
        marginTop = marginTop.value_or(0);
        marginBottom = marginBottom.value_or(0);
        top = containingBlockHeight - (*marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom + *bottom);
    }

    if (!height && top && bottom) {
        // #5
        marginTop = marginTop.value_or(0);
        marginBottom = marginBottom.value_or(0);
        height = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + paddingBottom + borderBottom + *marginBottom + *bottom);
    }

    if (!bottom && top && height) {
        // #6
        marginTop = marginTop.value_or(0);
        marginBottom = marginBottom.value_or(0);
        bottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + *height + paddingBottom + borderBottom + *marginBottom);
    }

    ASSERT(top);
    ASSERT(bottom);
    ASSERT(height);
    ASSERT(marginTop);
    ASSERT(marginBottom);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Position][Height][Margin] -> out-of-flow non-replaced -> top(" << *top << "px) bottom("  << *bottom << "px) height(" << *height << "px) margin(" << *marginTop << "px, "  << *marginBottom << "px) layoutBox(" << &layoutBox << ")");
    return { *top, *bottom, { *height, { *marginTop, *marginBottom }, { } } };
}

HorizontalGeometry FormattingContext::Geometry::outOfFlowNonReplacedHorizontalGeometry(LayoutContext& layoutContext, const FormattingContext& formattingContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned() && !layoutBox.replaced());

    // 10.3.7 Absolutely positioned, non-replaced elements
    //
    // 'left' + 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' + 'right'
    // = width of containing block

    // If all three of 'left', 'width', and 'right' are 'auto': First set any 'auto' values for 'margin-left' and 'margin-right' to 0.
    // Then, if the 'direction' property of the element establishing the static-position containing block is 'ltr' set 'left' to the static
    // position and apply rule number three below; otherwise, set 'right' to the static position and apply rule number one below.
    //
    // If none of the three is 'auto': If both 'margin-left' and 'margin-right' are 'auto', solve the equation under the extra constraint that the two margins get equal values,
    // unless this would make them negative, in which case when direction of the containing block is 'ltr' ('rtl'), set 'margin-left' ('margin-right') to zero and
    // solve for 'margin-right' ('margin-left'). If one of 'margin-left' or 'margin-right' is 'auto', solve the equation for that value.
    // If the values are over-constrained, ignore the value for 'left' (in case the 'direction' property of the containing block is 'rtl') or 'right'
    // (in case 'direction' is 'ltr') and solve for that value.
    //
    // Otherwise, set 'auto' values for 'margin-left' and 'margin-right' to 0, and pick the one of the following six rules that applies.
    //
    // 1. 'left' and 'width' are 'auto' and 'right' is not 'auto', then the width is shrink-to-fit. Then solve for 'left'
    // 2. 'left' and 'right' are 'auto' and 'width' is not 'auto', then if the 'direction' property of the element establishing the static-position
    //    containing block is 'ltr' set 'left' to the static position, otherwise set 'right' to the static position.
    //    Then solve for 'left' (if 'direction is 'rtl') or 'right' (if 'direction' is 'ltr').
    // 3. 'width' and 'right' are 'auto' and 'left' is not 'auto', then the width is shrink-to-fit . Then solve for 'right'
    // 4. 'left' is 'auto', 'width' and 'right' are not 'auto', then solve for 'left'
    // 5. 'width' is 'auto', 'left' and 'right' are not 'auto', then solve for 'width'
    // 6. 'right' is 'auto', 'left' and 'width' are not 'auto', then solve for 'right'

    auto& style = layoutBox.style();
    auto& displayBox = *layoutContext.displayBoxForLayoutBox(layoutBox);
    auto& containingBlock = *layoutBox.containingBlock();
    auto containingBlockWidth = layoutContext.displayBoxForLayoutBox(containingBlock)->contentBoxWidth();
    auto isLeftToRightDirection = containingBlock.style().isLeftToRightDirection();

    auto left = computedValueIfNotAuto(style.logicalLeft(), containingBlockWidth);
    auto right = computedValueIfNotAuto(style.logicalRight(), containingBlockWidth);
    auto width = computedValueIfNotAuto(style.logicalWidth(), containingBlockWidth);
    auto marginLeft = computedValueIfNotAuto(style.marginLeft(), containingBlockWidth);
    auto marginRight = computedValueIfNotAuto(style.marginRight(), containingBlockWidth);
    auto paddingLeft = displayBox.paddingLeft().value_or(0);
    auto paddingRight = displayBox.paddingRight().value_or(0);
    auto borderLeft = displayBox.borderLeft();
    auto borderRight = displayBox.borderRight();

    if (!left && !width && !right) {
        // If all three of 'left', 'width', and 'right' are 'auto': First set any 'auto' values for 'margin-left' and 'margin-right' to 0.
        // Then, if the 'direction' property of the element establishing the static-position containing block is 'ltr' set 'left' to the static
        // position and apply rule number three below; otherwise, set 'right' to the static position and apply rule number one below.
        marginLeft = marginLeft.value_or(0);
        marginRight = marginRight.value_or(0);

        auto staticHorizontalPosition = staticHorizontalPositionForOutOfFlowPositioned(layoutContext, layoutBox);
        if (isLeftToRightDirection)
            left = staticHorizontalPosition;
        else
            right = staticHorizontalPosition;
    } else if (left && width && right) {
        // If none of the three is 'auto': If both 'margin-left' and 'margin-right' are 'auto', solve the equation under the extra constraint that the two margins get equal values,
        // unless this would make them negative, in which case when direction of the containing block is 'ltr' ('rtl'), set 'margin-left' ('margin-right') to zero and
        // solve for 'margin-right' ('margin-left'). If one of 'margin-left' or 'margin-right' is 'auto', solve the equation for that value.
        // If the values are over-constrained, ignore the value for 'left' (in case the 'direction' property of the containing block is 'rtl') or 'right'
        // (in case 'direction' is 'ltr') and solve for that value.
        if (!marginLeft && !marginRight) {
            auto marginLeftAndRight = containingBlockWidth - (*left + borderLeft + paddingLeft + *width + paddingRight + borderRight + *right);
            if (marginLeftAndRight >= 0)
                marginLeft = marginRight = marginLeftAndRight / 2;
            else {
                if (isLeftToRightDirection) {
                    marginLeft = LayoutUnit { 0 };
                    marginRight = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *right);
                } else {
                    marginRight = LayoutUnit { 0 };
                    marginLeft = containingBlockWidth - (*left + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight + *right);
                }
            }
        } else if (!marginLeft) {
            marginLeft = containingBlockWidth - (*left + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight + *right);
            // Overconstrained? Ignore right (left).
            if (*marginLeft < 0) {
                if (isLeftToRightDirection)
                    marginLeft = containingBlockWidth - (*left + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight);
                else
                    marginLeft = containingBlockWidth - (borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight + *right);
            }
        } else if (!marginRight) {
            marginRight = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *right);
            // Overconstrained? Ignore right (left).
            if (*marginRight < 0) {
                if (isLeftToRightDirection)
                    marginRight = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight);
                else
                    marginRight = containingBlockWidth - (*marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *right);
            }
        }
    } else {
        // Otherwise, set 'auto' values for 'margin-left' and 'margin-right' to 0, and pick the one of the following six rules that applies.
        marginLeft = marginLeft.value_or(0);
        marginRight = marginRight.value_or(0);
    }

    ASSERT(marginLeft);
    ASSERT(marginRight);

    if (!left && !width && right) {
        // #1
        width = shrinkToFitWidth(layoutContext, formattingContext, layoutBox);
        left = containingBlockWidth - (*marginLeft + borderLeft + paddingLeft + *width + paddingRight  + borderRight + *marginRight + *right);
    } else if (!left && !right && width) {
        // #2
        auto staticHorizontalPosition = staticHorizontalPositionForOutOfFlowPositioned(layoutContext, layoutBox);
        if (isLeftToRightDirection) {
            left = staticHorizontalPosition;
            right = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight);
        } else {
            right = staticHorizontalPosition;
            left = containingBlockWidth - (*marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight + *right);
        }
    } else if (!width && !right && left) {
        // #3
        width = shrinkToFitWidth(layoutContext, formattingContext, layoutBox);
        right = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight);
    } else if (!left && width && right) {
        // #4
        left = containingBlockWidth - (*marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight + *right);
    } else if (!width && left && right) {
        // #5
        width = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + paddingRight  + borderRight + *marginRight + *right);
    } else if (!right && left && width) {
        // #6
        right = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + *width + paddingRight + borderRight + *marginRight);
    }

    ASSERT(left);
    ASSERT(right);
    ASSERT(width);
    ASSERT(marginLeft);
    ASSERT(marginRight);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Position][Width][Margin] -> out-of-flow non-replaced -> left(" << *left << "px) right("  << *right << "px) width(" << *width << "px) margin(" << *marginLeft << "px, "  << *marginRight << "px) layoutBox(" << &layoutBox << ")");
    return { *left, *right, { *width, { *marginLeft, *marginRight } } };
}

VerticalGeometry FormattingContext::Geometry::outOfFlowReplacedVerticalGeometry(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned() && layoutBox.replaced());

    // 10.6.5 Absolutely positioned, replaced elements
    //
    // The used value of 'height' is determined as for inline replaced elements.
    // If 'margin-top' or 'margin-bottom' is specified as 'auto' its used value is determined by the rules below.
    // 1. If both 'top' and 'bottom' have the value 'auto', replace 'top' with the element's static position.
    // 2. If 'bottom' is 'auto', replace any 'auto' on 'margin-top' or 'margin-bottom' with '0'.
    // 3. If at this point both 'margin-top' and 'margin-bottom' are still 'auto', solve the equation under the extra constraint that the two margins must get equal values.
    // 4. If at this point there is only one 'auto' left, solve the equation for that value.
    // 5. If at this point the values are over-constrained, ignore the value for 'bottom' and solve for that value.

    auto& style = layoutBox.style();
    auto& displayBox = *layoutContext.displayBoxForLayoutBox(layoutBox);
    auto& containingBlockDisplayBox = *layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock());
    auto containingBlockHeight = containingBlockDisplayBox.height();
    auto containingBlockWidth = containingBlockDisplayBox.width();

    auto top = computedValueIfNotAuto(style.logicalTop(), containingBlockWidth);
    auto bottom = computedValueIfNotAuto(style.logicalBottom(), containingBlockWidth);
    auto height = inlineReplacedHeightAndMargin(layoutContext, layoutBox).height;
    auto marginTop = computedValueIfNotAuto(style.marginTop(), containingBlockWidth);
    auto marginBottom = computedValueIfNotAuto(style.marginBottom(), containingBlockWidth);
    auto paddingTop = displayBox.paddingTop().value_or(0);
    auto paddingBottom = displayBox.paddingBottom().value_or(0);
    auto borderTop = displayBox.borderTop();
    auto borderBottom = displayBox.borderBottom();

    if (!top && !bottom) {
        // #1
        top = staticVerticalPositionForOutOfFlowPositioned(layoutContext, layoutBox);
    }

    if (!bottom) {
        // #2
        marginTop = marginTop.value_or(0);
        marginBottom = marginBottom.value_or(0);
    }

    if (!marginTop && !marginBottom) {
        // #3
        auto marginTopAndBottom = containingBlockHeight - (*top + borderTop + paddingTop + height + paddingBottom + borderBottom + *bottom);
        marginTop = marginBottom = marginTopAndBottom / 2;
    }

    // #4
    if (!top)
        top = containingBlockHeight - (*marginTop + borderTop + paddingTop + height + paddingBottom + borderBottom + *marginBottom + *bottom);

    if (!bottom)
        bottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + height + paddingBottom + borderBottom + *marginBottom);

    if (!marginTop)
        marginTop = containingBlockHeight - (*top + borderTop + paddingTop + height + paddingBottom + borderBottom + *marginBottom + *bottom);

    if (!marginBottom)
        marginBottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + height + paddingBottom + borderBottom + *bottom);

    // #5
    auto boxHeight = *top + *marginTop + borderTop + paddingTop + height + paddingBottom + borderBottom + *marginBottom + *bottom;
    if (boxHeight > containingBlockHeight)
        bottom = containingBlockHeight - (*top + *marginTop + borderTop + paddingTop + height + paddingBottom + borderBottom + *marginBottom);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Position][Height][Margin] -> out-of-flow replaced -> top(" << *top << "px) bottom("  << *bottom << "px) height(" << height << "px) margin(" << *marginTop << "px, "  << *marginBottom << "px) layoutBox(" << &layoutBox << ")");
    return { *top, *bottom, { height, { *marginTop, *marginBottom }, { } } };
}

HorizontalGeometry FormattingContext::Geometry::outOfFlowReplacedHorizontalGeometry(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned() && layoutBox.replaced());

    // 10.3.8 Absolutely positioned, replaced elements
    // In this case, section 10.3.7 applies up through and including the constraint equation, but the rest of section 10.3.7 is replaced by the following rules:
    //
    // The used value of 'width' is determined as for inline replaced elements. If 'margin-left' or 'margin-right' is specified as 'auto' its used value is determined by the rules below.
    // 1. If both 'left' and 'right' have the value 'auto', then if the 'direction' property of the element establishing the static-position containing block is 'ltr',
    //   set 'left' to the static position; else if 'direction' is 'rtl', set 'right' to the static position.
    // 2. If 'left' or 'right' are 'auto', replace any 'auto' on 'margin-left' or 'margin-right' with '0'.
    // 3. If at this point both 'margin-left' and 'margin-right' are still 'auto', solve the equation under the extra constraint that the two margins must get equal values,
    //   unless this would make them negative, in which case when the direction of the containing block is 'ltr' ('rtl'), set 'margin-left' ('margin-right') to zero and
    //   solve for 'margin-right' ('margin-left').
    // 4. If at this point there is an 'auto' left, solve the equation for that value.
    // 5. If at this point the values are over-constrained, ignore the value for either 'left' (in case the 'direction' property of the containing block is 'rtl') or
    //   'right' (in case 'direction' is 'ltr') and solve for that value.

    auto& style = layoutBox.style();
    auto& displayBox = *layoutContext.displayBoxForLayoutBox(layoutBox);
    auto& containingBlock = *layoutBox.containingBlock();
    auto containingBlockWidth = layoutContext.displayBoxForLayoutBox(containingBlock)->contentBoxWidth();
    auto isLeftToRightDirection = containingBlock.style().isLeftToRightDirection();

    auto left = computedValueIfNotAuto(style.logicalLeft(), containingBlockWidth);
    auto right = computedValueIfNotAuto(style.logicalRight(), containingBlockWidth);
    auto marginLeft = computedValueIfNotAuto(style.marginLeft(), containingBlockWidth);
    auto marginRight = computedValueIfNotAuto(style.marginRight(), containingBlockWidth);
    auto width = inlineReplacedWidthAndMargin(layoutContext, layoutBox).width;
    auto paddingLeft = displayBox.paddingLeft().value_or(0);
    auto paddingRight = displayBox.paddingRight().value_or(0);
    auto borderLeft = displayBox.borderLeft();
    auto borderRight = displayBox.borderRight();

    if (!left && !right) {
        // #1
        auto staticHorizontalPosition = staticHorizontalPositionForOutOfFlowPositioned(layoutContext, layoutBox);
        if (isLeftToRightDirection)
            left = staticHorizontalPosition;
        else
            right = staticHorizontalPosition;
    }

    if (!left || !right) {
        // #2
        marginLeft = marginLeft.value_or(0);
        marginRight = marginRight.value_or(0);
    }

    if (!marginLeft && !marginRight) {
        // #3
        auto marginLeftAndRight = containingBlockWidth - (*left + borderLeft + paddingLeft + width + paddingRight + borderRight + *right);
        if (marginLeftAndRight >= 0)
            marginLeft = marginRight = marginLeftAndRight / 2;
        else {
            if (isLeftToRightDirection) {
                marginLeft = LayoutUnit { 0 };
                marginRight = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + *right);
            } else {
                marginRight = LayoutUnit { 0 };
                marginLeft = containingBlockWidth - (*left + borderLeft + paddingLeft + width + paddingRight + borderRight + *marginRight + *right);
            }
        }
    }

    // #4
    if (!left)
        left = containingBlockWidth - (*marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + *marginRight + *right);

    if (!right)
        right = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + *marginRight);

    if (!marginLeft)
        marginLeft = containingBlockWidth - (*left + borderLeft + paddingLeft + width + paddingRight + borderRight + *marginRight + *right);

    if (!marginRight)
        marginRight = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + *right);

    auto boxWidth = (*left + *marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + *marginRight + *right);
    if (boxWidth > containingBlockWidth) {
        // #5 Over-constrained?
        if (isLeftToRightDirection)
            right = containingBlockWidth - (*left + *marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + *marginRight);
        else
            left = containingBlockWidth - (*marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + *marginRight + *right);
    }

    ASSERT(left);
    ASSERT(right);
    ASSERT(marginLeft);
    ASSERT(marginRight);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Position][Width][Margin] -> out-of-flow replaced -> left(" << *left << "px) right("  << *right << "px) width(" << width << "px) margin(" << *marginLeft << "px, "  << *marginRight << "px) layoutBox(" << &layoutBox << ")");
    return { *left, *right, { width, { *marginLeft, *marginRight } } };
}

HeightAndMargin FormattingContext::Geometry::complicatedCases(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(!layoutBox.replaced());
    // TODO: Use complicated-case for document renderer for now (see BlockFormattingContext::Geometry::inFlowHeightAndMargin).
    ASSERT((layoutBox.isBlockLevelBox() && layoutBox.isInFlow() && !layoutBox.isOverflowVisible()) || layoutBox.isInlineBlockBox() || layoutBox.isFloatingPositioned() || layoutBox.isDocumentBox());

    // 10.6.6 Complicated cases
    //
    // Block-level, non-replaced elements in normal flow when 'overflow' does not compute to 'visible' (except if the 'overflow' property's value has been propagated to the viewport).
    // 'Inline-block', non-replaced elements.
    // Floating, non-replaced elements.
    //
    // 1. If 'margin-top', or 'margin-bottom' are 'auto', their used value is 0.
    // 2. If 'height' is 'auto', the height depends on the element's descendants per 10.6.7.

    auto& style = layoutBox.style();
    auto& containingBlock = *layoutBox.containingBlock();
    auto& containingBlockDisplayBox = *layoutContext.displayBoxForLayoutBox(containingBlock);
    auto containingBlockWidth = containingBlockDisplayBox.contentBoxWidth();

    auto height = fixedValue(style.logicalHeight());
    auto marginTop = computedValueIfNotAuto(style.marginTop(), containingBlockWidth);
    auto marginBottom = computedValueIfNotAuto(style.marginBottom(), containingBlockWidth);

    // #1
    marginTop = marginTop.value_or(0);
    marginBottom = marginBottom.value_or(0);
    // #2
    if (!height) {
        if (style.logicalHeight().isAuto())
            height = contentHeightForFormattingContextRoot(layoutContext, layoutBox);
        else
            ASSERT_NOT_IMPLEMENTED_YET();
    }

    ASSERT(height);
    ASSERT(marginTop);
    ASSERT(marginBottom);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Height][Margin] -> floating non-replaced -> height(" << *height << "px) margin(" << *marginTop << "px, " << *marginBottom << "px) -> layoutBox(" << &layoutBox << ")");
    return HeightAndMargin { *height, { *marginTop, *marginBottom }, { } };
}

WidthAndMargin FormattingContext::Geometry::floatingNonReplacedWidthAndMargin(LayoutContext& layoutContext, const FormattingContext& formattingContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isFloatingPositioned() && !layoutBox.replaced());

    // 10.3.5 Floating, non-replaced elements
    //
    // 1. If 'margin-left', or 'margin-right' are computed as 'auto', their used value is '0'.
    // 2. If 'width' is computed as 'auto', the used value is the "shrink-to-fit" width.

    auto& containingBlock = *layoutBox.containingBlock();
    auto containingBlockWidth = layoutContext.displayBoxForLayoutBox(containingBlock)->contentBoxWidth();

    // #1
    auto margin = computedNonCollapsedHorizontalMarginValue(layoutContext, layoutBox);
      // #2
    auto width = computedValueIfNotAuto(layoutBox.style().logicalWidth(), containingBlockWidth);
    if (!width)
        width = shrinkToFitWidth(layoutContext, formattingContext, layoutBox);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Width][Margin] -> floating non-replaced -> width(" << *width << "px) margin(" << margin.left << "px, " << margin.right << "px) -> layoutBox(" << &layoutBox << ")");
    return WidthAndMargin { *width, margin };
}

HeightAndMargin FormattingContext::Geometry::floatingReplacedHeightAndMargin(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isFloatingPositioned() && layoutBox.replaced());

    // 10.6.2 Inline replaced elements, block-level replaced elements in normal flow, 'inline-block'
    // replaced elements in normal flow and floating replaced elements
    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Height][Margin] -> floating replaced -> redirected to inline replaced");
    return inlineReplacedHeightAndMargin(layoutContext, layoutBox);
}

WidthAndMargin FormattingContext::Geometry::floatingReplacedWidthAndMargin(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isFloatingPositioned() && layoutBox.replaced());

    // 10.3.6 Floating, replaced elements
    //
    // 1. If 'margin-left' or 'margin-right' are computed as 'auto', their used value is '0'.
    // 2. The used value of 'width' is determined as for inline replaced elements.
    auto margin = computedNonCollapsedHorizontalMarginValue(layoutContext, layoutBox);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Height][Margin] -> floating replaced -> redirected to inline replaced");
    return inlineReplacedWidthAndMargin(layoutContext, layoutBox, margin.left, margin.right);
}

VerticalGeometry FormattingContext::Geometry::outOfFlowVerticalGeometry(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned());

    if (!layoutBox.replaced())
        return outOfFlowNonReplacedVerticalGeometry(layoutContext, layoutBox);
    return outOfFlowReplacedVerticalGeometry(layoutContext, layoutBox);
}

HorizontalGeometry FormattingContext::Geometry::outOfFlowHorizontalGeometry(LayoutContext& layoutContext, const FormattingContext& formattingContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isOutOfFlowPositioned());

    if (!layoutBox.replaced())
        return outOfFlowNonReplacedHorizontalGeometry(layoutContext, formattingContext, layoutBox);
    return outOfFlowReplacedHorizontalGeometry(layoutContext, layoutBox);
}

HeightAndMargin FormattingContext::Geometry::floatingHeightAndMargin(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isFloatingPositioned());

    if (!layoutBox.replaced())
        return complicatedCases(layoutContext, layoutBox);
    return floatingReplacedHeightAndMargin(layoutContext, layoutBox);
}

WidthAndMargin FormattingContext::Geometry::floatingWidthAndMargin(LayoutContext& layoutContext, const FormattingContext& formattingContext, const Box& layoutBox)
{
    ASSERT(layoutBox.isFloatingPositioned());

    if (!layoutBox.replaced())
        return floatingNonReplacedWidthAndMargin(layoutContext, formattingContext, layoutBox);
    return floatingReplacedWidthAndMargin(layoutContext, layoutBox);
}

HeightAndMargin FormattingContext::Geometry::inlineReplacedHeightAndMargin(LayoutContext& layoutContext, const Box& layoutBox)
{
    ASSERT((layoutBox.isOutOfFlowPositioned() || layoutBox.isFloatingPositioned() || layoutBox.isInFlow()) && layoutBox.replaced());

    // 10.6.2 Inline replaced elements, block-level replaced elements in normal flow, 'inline-block' replaced elements in normal flow and floating replaced elements
    //
    // 1. If 'margin-top', or 'margin-bottom' are 'auto', their used value is 0.
    // 2. If 'height' and 'width' both have computed values of 'auto' and the element also has an intrinsic height, then that intrinsic height is the used value of 'height'.
    // 3. Otherwise, if 'height' has a computed value of 'auto', and the element has an intrinsic ratio then the used value of 'height' is:
    //    (used width) / (intrinsic ratio)
    // 4. Otherwise, if 'height' has a computed value of 'auto', and the element has an intrinsic height, then that intrinsic height is the used value of 'height'.
    // 5. Otherwise, if 'height' has a computed value of 'auto', but none of the conditions above are met, then the used value of 'height' must be set to
    //    the height of the largest rectangle that has a 2:1 ratio, has a height not greater than 150px, and has a width not greater than the device width.

    // #1
    auto margin = computedNonCollapsedVerticalMarginValue(layoutContext, layoutBox);

    auto& style = layoutBox.style();
    auto replaced = layoutBox.replaced();
    auto& containingBlockDisplayBox = *layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock());
    auto containingBlockWidth = containingBlockDisplayBox.width();

    auto height = fixedValue(style.logicalHeight());
    auto heightIsAuto = style.logicalHeight().isAuto();
    auto width = computedValueIfNotAuto(style.logicalWidth(), containingBlockWidth);

    if (!height && !heightIsAuto)
        ASSERT_NOT_IMPLEMENTED_YET();

    if (heightIsAuto && !width && replaced->hasIntrinsicHeight()) {
        // #2
        height = replaced->intrinsicHeight();
    } else if (heightIsAuto && replaced->hasIntrinsicRatio()) {
        // #3
        height = *width / replaced->intrinsicRatio();
    } else if (heightIsAuto && replaced->hasIntrinsicHeight()) {
        // #4
        height = replaced->intrinsicHeight();
    } else if (heightIsAuto) {
        // #5
        height = { 150 };
    }

    ASSERT(height);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Height][Margin] -> inflow replaced -> height(" << *height << "px) margin(" << margin.top << "px, " << margin.bottom << "px) -> layoutBox(" << &layoutBox << ")");
    return { *height, margin, { } };
}

WidthAndMargin FormattingContext::Geometry::inlineReplacedWidthAndMargin(LayoutContext& layoutContext, const Box& layoutBox,
    std::optional<LayoutUnit> precomputedMarginLeft, std::optional<LayoutUnit> precomputedMarginRight)
{
    ASSERT((layoutBox.isOutOfFlowPositioned() || layoutBox.isFloatingPositioned() || layoutBox.isInFlow()) && layoutBox.replaced());

    // 10.3.2 Inline, replaced elements
    //
    // A computed value of 'auto' for 'margin-left' or 'margin-right' becomes a used value of '0'.
    //
    // 1. If 'height' and 'width' both have computed values of 'auto' and the element also has an intrinsic width, then that intrinsic width is the used value of 'width'.
    //
    // 2. If 'height' and 'width' both have computed values of 'auto' and the element has no intrinsic width, but does have an intrinsic height and intrinsic ratio;
    //    or if 'width' has a computed value of 'auto', 'height' has some other computed value, and the element does have an intrinsic ratio;
    //    then the used value of 'width' is: (used height) * (intrinsic ratio)
    //
    // 3. If 'height' and 'width' both have computed values of 'auto' and the element has an intrinsic ratio but no intrinsic height or width,
    //    then the used value of 'width' is undefined in CSS 2.2. However, it is suggested that, if the containing block's width does not itself depend on the replaced
    //    element's width, then the used value of 'width' is calculated from the constraint equation used for block-level, non-replaced elements in normal flow.
    //
    // 4. Otherwise, if 'width' has a computed value of 'auto', and the element has an intrinsic width, then that intrinsic width is the used value of 'width'.
    //
    // 5. Otherwise, if 'width' has a computed value of 'auto', but none of the conditions above are met, then the used value of 'width' becomes 300px.
    //    If 300px is too wide to fit the device, UAs should use the width of the largest rectangle that has a 2:1 ratio and fits the device instead.

    auto& style = layoutBox.style();
    auto& containingBlockDisplayBox = *layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock());
    auto containingBlockWidth = containingBlockDisplayBox.width();

    auto computeMarginRight = [&]() {
        if (precomputedMarginRight)
            return precomputedMarginRight.value();
        auto marginRight = computedValueIfNotAuto(style.marginRight(), containingBlockWidth);
        return marginRight.value_or(LayoutUnit { 0 });
    };

    auto computeMarginLeft = [&]() {
        if (precomputedMarginLeft)
            return precomputedMarginLeft.value();
        auto marginLeft = computedValueIfNotAuto(style.marginLeft(), containingBlockWidth);
        return marginLeft.value_or(LayoutUnit { 0 });
    };

    auto replaced = layoutBox.replaced();
    ASSERT(replaced);

    auto marginLeft = computeMarginLeft();
    auto marginRight = computeMarginRight();
    auto width = computedValueIfNotAuto(style.logicalWidth(), containingBlockWidth);

    auto heightIsAuto = style.logicalHeight().isAuto();
    auto height = fixedValue(style.logicalHeight());
    if (!height && !heightIsAuto)
        ASSERT_NOT_IMPLEMENTED_YET();

    if (!width && heightIsAuto && replaced->hasIntrinsicWidth()) {
        // #1
        width = replaced->intrinsicWidth();
    } else if ((!width && heightIsAuto && !replaced->hasIntrinsicWidth() && replaced->hasIntrinsicHeight() && replaced->hasIntrinsicRatio())
        || (!width && height && replaced->hasIntrinsicRatio())) {
        // #2
        width = height.value_or(replaced->hasIntrinsicHeight()) * replaced->intrinsicRatio();
    } else if (!width && heightIsAuto && replaced->hasIntrinsicRatio() && !replaced->hasIntrinsicWidth() && !replaced->hasIntrinsicHeight()) {
        // #3
        // FIXME: undefined but surely doable.
        ASSERT_NOT_IMPLEMENTED_YET();
    } else if (!width && replaced->hasIntrinsicWidth()) {
        // #4
        width = replaced->intrinsicWidth();
    } else {
        // #5
        width = { 300 };
    }

    ASSERT(width);

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Width][Margin] -> inflow replaced -> width(" << *width << "px) margin(" << marginLeft << "px, " << marginRight << "px) -> layoutBox(" << &layoutBox << ")");
    return { *width, { marginLeft, marginRight } };
}

Edges FormattingContext::Geometry::computedBorder(LayoutContext&, const Box& layoutBox)
{
    auto& style = layoutBox.style();
    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Border] -> layoutBox: " << &layoutBox);
    return {
        { style.borderLeft().boxModelWidth(), style.borderRight().boxModelWidth() },
        { style.borderTop().boxModelWidth(), style.borderBottom().boxModelWidth() }
    };
}

std::optional<Edges> FormattingContext::Geometry::computedPadding(LayoutContext& layoutContext, const Box& layoutBox)
{
    if (!layoutBox.isPaddingApplicable())
        return std::nullopt;

    auto& style = layoutBox.style();
    auto containingBlockWidth = layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock())->contentBoxWidth();
    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Padding] -> layoutBox: " << &layoutBox);
    return Edges {
        { valueForLength(style.paddingLeft(), containingBlockWidth), valueForLength(style.paddingRight(), containingBlockWidth) },
        { valueForLength(style.paddingTop(), containingBlockWidth), valueForLength(style.paddingBottom(), containingBlockWidth) }
    };
}

HorizontalEdges FormattingContext::Geometry::computedNonCollapsedHorizontalMarginValue(const LayoutContext& layoutContext, const Box& layoutBox)
{
    auto& style = layoutBox.style();
    auto containingBlockWidth = layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock())->contentBoxWidth();

    auto marginLeft = computedValueIfNotAuto(style.marginLeft(), containingBlockWidth).value_or(LayoutUnit { 0 });
    auto marginRight = computedValueIfNotAuto(style.marginRight(), containingBlockWidth).value_or(LayoutUnit { 0 });

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Margin] -> non collapsed horizontal -> margin(" << marginLeft << "px, " << marginRight << "px) -> layoutBox: " << &layoutBox);
    return { marginLeft, marginRight };
}

VerticalEdges FormattingContext::Geometry::computedNonCollapsedVerticalMarginValue(const LayoutContext& layoutContext, const Box& layoutBox)
{
    auto& style = layoutBox.style();
    auto containingBlockWidth = layoutContext.displayBoxForLayoutBox(*layoutBox.containingBlock())->contentBoxWidth();

    auto marginTop = computedValueIfNotAuto(style.marginTop(), containingBlockWidth).value_or(LayoutUnit { 0 });
    auto marginBottom = computedValueIfNotAuto(style.marginBottom(), containingBlockWidth).value_or(LayoutUnit { 0 });

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Margin] -> non collapsed vertical -> margin(" << marginTop << "px, " << marginBottom << "px) -> layoutBox: " << &layoutBox);
    return { marginTop, marginBottom };
}

}
}
#endif
