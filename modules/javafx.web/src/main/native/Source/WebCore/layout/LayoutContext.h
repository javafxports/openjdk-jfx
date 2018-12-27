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

#pragma once

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "FormattingContext.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/IsoMalloc.h>
#include <wtf/OptionSet.h>

namespace WebCore {

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)
class RenderView;
#endif

namespace Display {
class Box;
}

namespace Layout {

enum class StyleDiff;
class Box;
class Container;
class FormattingState;

// LayoutContext is the entry point for layout. It takes a (formatting root)container which acts as the root of the layout context.
// LayoutContext::layout() generates the display tree for the root container's subtree (it does not run layout on the root though).
// Note, while the root container is suppposed to be the entry point for the initial layout, it does not necessarily need to be the entry point of any
// subsequent layouts (subtree layout). A non-initial, subtree layout could be initiated on multiple formatting contexts.
// Each formatting context has an entry point for layout, which potenitally means multiple entry points per layout frame.
// LayoutContext also holds the formatting states. They cache formatting context specific data to enable performant incremental layouts.
class LayoutContext {
    WTF_MAKE_ISO_ALLOCATED(LayoutContext);
public:
    LayoutContext();

    void initializeRoot(const Container&, const LayoutSize&);
    void updateLayout();
    void styleChanged(const Box&, StyleDiff);
    void setInQuirksMode(bool inQuirksMode) { m_inQuirksMode = inQuirksMode; }

    enum class UpdateType {
        Overflow = 1 << 0,
        Position = 1 << 1,
        Size     = 1 << 2,
        All      = Overflow | Position | Size
    };
    void markNeedsUpdate(const Box&, OptionSet<UpdateType>);
    bool needsUpdate(const Box&) const;

    std::unique_ptr<FormattingContext> formattingContext(const Box& formattingContextRoot);

    FormattingState& formattingStateForBox(const Box&) const;
    FormattingState& establishedFormattingState(const Box& formattingRoot);

    Display::Box& createDisplayBox(const Box&);
    Display::Box* displayBoxForLayoutBox(const Box& layoutBox) const { return m_layoutToDisplayBox.get(&layoutBox); }

    bool inQuirksMode() const { return m_inQuirksMode; }
    // For testing purposes only
    void verifyAndOutputMismatchingLayoutTree(const RenderView&) const;

private:
    void layoutFormattingContextSubtree(const Box&);

    WeakPtr<Container> m_root;
    HashSet<const Container*> m_formattingContextRootListForLayout;
    HashMap<const Box*, std::unique_ptr<FormattingState>> m_formattingStates;
    HashMap<const Box*, std::unique_ptr<Display::Box>> m_layoutToDisplayBox;
    bool m_inQuirksMode { false };
};

}
}
#endif
