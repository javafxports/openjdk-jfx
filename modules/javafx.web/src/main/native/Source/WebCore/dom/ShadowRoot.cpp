/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ShadowRoot.h"

#include "CSSStyleSheet.h"
#include "ElementTraversal.h"
#include "HTMLSlotElement.h"
#include "RenderElement.h"
#include "RuntimeEnabledFeatures.h"
#include "SlotAssignment.h"
#include "StyleResolver.h"
#include "StyleScope.h"
#include "markup.h"

namespace WebCore {

struct SameSizeAsShadowRoot : public DocumentFragment, public TreeScope {
    unsigned countersAndFlags[1];
    void* styleScope;
    void* host;
    void* slotAssignment;
};

COMPILE_ASSERT(sizeof(ShadowRoot) == sizeof(SameSizeAsShadowRoot), shadowroot_should_stay_small);

ShadowRoot::ShadowRoot(Document& document, ShadowRootMode type)
    : DocumentFragment(document, CreateShadowRoot)
    , TreeScope(*this, document)
    , m_type(type)
    , m_styleScope(std::make_unique<Style::Scope>(*this))
{
}


ShadowRoot::ShadowRoot(Document& document, std::unique_ptr<SlotAssignment>&& slotAssignment)
    : DocumentFragment(document, CreateShadowRoot)
    , TreeScope(*this, document)
    , m_type(ShadowRootMode::UserAgent)
    , m_styleScope(std::make_unique<Style::Scope>(*this))
    , m_slotAssignment(WTFMove(slotAssignment))
{
}


ShadowRoot::~ShadowRoot()
{
    if (isConnected())
        document().didRemoveInDocumentShadowRoot(*this);

    // We cannot let ContainerNode destructor call willBeDeletedFrom()
    // for this ShadowRoot instance because TreeScope destructor
    // clears Node::m_treeScope thus ContainerNode is no longer able
    // to access it Document reference after that.
    willBeDeletedFrom(document());

    // We must remove all of our children first before the TreeScope destructor
    // runs so we don't go through Node::setTreeScopeRecursively for each child with a
    // destructed tree scope in each descendant.
    removeDetachedChildren();
}

Node::InsertedIntoAncestorResult ShadowRoot::insertedIntoAncestor(InsertionType insertionType, ContainerNode& parentOfInsertedTree)
{
    DocumentFragment::insertedIntoAncestor(insertionType, parentOfInsertedTree);
    if (insertionType.connectedToDocument)
        document().didInsertInDocumentShadowRoot(*this);
    return InsertedIntoAncestorResult::Done;
}

void ShadowRoot::removedFromAncestor(RemovalType removalType, ContainerNode& oldParentOfRemovedTree)
{
    DocumentFragment::removedFromAncestor(removalType, oldParentOfRemovedTree);
    if (removalType.disconnectedFromDocument)
        document().didRemoveInDocumentShadowRoot(*this);
}

void ShadowRoot::moveShadowRootToNewParentScope(TreeScope& newScope, Document& newDocument)
{
    setParentTreeScope(newScope);
    moveShadowRootToNewDocument(newDocument);
}

void ShadowRoot::moveShadowRootToNewDocument(Document& newDocument)
{
    setDocumentScope(newDocument);
    RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(!parentTreeScope() || &parentTreeScope()->documentScope() == &newDocument);

    // Style scopes are document specific.
    m_styleScope = std::make_unique<Style::Scope>(*this);
    RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(&m_styleScope->document() == &newDocument);
}

Style::Scope& ShadowRoot::styleScope()
{
    return *m_styleScope;
}

String ShadowRoot::innerHTML() const
{
    return createMarkup(*this, ChildrenOnly);
}

ExceptionOr<void> ShadowRoot::setInnerHTML(const String& markup)
{
    if (isOrphan())
        return Exception { InvalidAccessError };
    auto fragment = createFragmentForInnerOuterHTML(*host(), markup, AllowScriptingContent);
    if (fragment.hasException())
        return fragment.releaseException();
    return replaceChildrenWithFragment(*this, fragment.releaseReturnValue());
}

bool ShadowRoot::childTypeAllowed(NodeType type) const
{
    switch (type) {
    case ELEMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
    case COMMENT_NODE:
    case TEXT_NODE:
    case CDATA_SECTION_NODE:
        return true;
    default:
        return false;
    }
}

void ShadowRoot::setResetStyleInheritance(bool value)
{
    // If this was ever changed after initialization, child styles would need to be invalidated here.
    m_resetStyleInheritance = value;
}

Ref<Node> ShadowRoot::cloneNodeInternal(Document&, CloningOperation)
{
    RELEASE_ASSERT_NOT_REACHED();
    return *static_cast<Node*>(nullptr); // ShadowRoots should never be cloned.
}

void ShadowRoot::removeAllEventListeners()
{
    DocumentFragment::removeAllEventListeners();
    for (Node* node = firstChild(); node; node = NodeTraversal::next(*node))
        node->removeAllEventListeners();
}


HTMLSlotElement* ShadowRoot::findAssignedSlot(const Node& node)
{
    ASSERT(node.parentNode() == host());
    if (!m_slotAssignment)
        return nullptr;
    return m_slotAssignment->findAssignedSlot(node, *this);
}

void ShadowRoot::addSlotElementByName(const AtomicString& name, HTMLSlotElement& slot)
{
    if (!m_slotAssignment)
        m_slotAssignment = std::make_unique<SlotAssignment>();

    return m_slotAssignment->addSlotElementByName(name, slot, *this);
}

void ShadowRoot::removeSlotElementByName(const AtomicString& name, HTMLSlotElement& slot)
{
    return m_slotAssignment->removeSlotElementByName(name, slot, *this);
}

const Vector<Node*>* ShadowRoot::assignedNodesForSlot(const HTMLSlotElement& slot)
{
    if (!m_slotAssignment)
        return nullptr;
    return m_slotAssignment->assignedNodesForSlot(slot, *this);
}

Vector<ShadowRoot*> assignedShadowRootsIfSlotted(const Node& node)
{
    Vector<ShadowRoot*> result;
    for (auto* slot = node.assignedSlot(); slot; slot = slot->assignedSlot()) {
        ASSERT(slot->containingShadowRoot());
        result.append(slot->containingShadowRoot());
    }
    return result;
}

}
