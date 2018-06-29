/*
 * Copyright (C) 2013 Adobe Systems Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorNodeFinder.h"

#include "Attr.h"
#include "Document.h"
#include "Element.h"
#include "HTMLFrameOwnerElement.h"
#include "NodeList.h"
#include "NodeTraversal.h"
#include "XPathNSResolver.h"
#include "XPathResult.h"

namespace WebCore {

static String stripCharacters(const String& string, const char startCharacter, const char endCharacter, bool& startCharFound, bool& endCharFound)
{
    startCharFound = string.startsWith(startCharacter);
    endCharFound = string.endsWith(endCharacter);

    unsigned start = startCharFound ? 1 : 0;
    unsigned end = string.length() - (endCharFound ? 1 : 0);
    return string.substring(start, end - start);
}

InspectorNodeFinder::InspectorNodeFinder(const String& whitespaceTrimmedQuery)
    : m_whitespaceTrimmedQuery(whitespaceTrimmedQuery)
{
    m_tagNameQuery = stripCharacters(whitespaceTrimmedQuery, '<', '>', m_startTagFound, m_endTagFound);

    bool startQuoteFound, endQuoteFound;
    m_attributeQuery = stripCharacters(whitespaceTrimmedQuery, '"', '"', startQuoteFound, endQuoteFound);
    m_exactAttributeMatch = startQuoteFound && endQuoteFound;
}

void InspectorNodeFinder::performSearch(Node* parentNode)
{
    if (!parentNode)
        return;

    searchUsingXPath(*parentNode);
    searchUsingCSSSelectors(*parentNode);

    // Keep the DOM tree traversal last. This way iframe content will come after their parents.
    searchUsingDOMTreeTraversal(*parentNode);
}

void InspectorNodeFinder::searchUsingDOMTreeTraversal(Node& parentNode)
{
    // Manual plain text search.
    for (auto* node = &parentNode; node; node = NodeTraversal::next(*node, &parentNode)) {
        switch (node->nodeType()) {
        case Node::TEXT_NODE:
        case Node::COMMENT_NODE:
        case Node::CDATA_SECTION_NODE:
            if (node->nodeValue().containsIgnoringASCIICase(m_whitespaceTrimmedQuery))
                m_results.add(node);
            break;
        case Node::ELEMENT_NODE:
            if (matchesElement(downcast<Element>(*node)))
                m_results.add(node);
            if (is<HTMLFrameOwnerElement>(downcast<Element>(*node)))
                performSearch(downcast<HTMLFrameOwnerElement>(*node).contentDocument());
            break;
        default:
            break;
        }
    }
}

bool InspectorNodeFinder::matchesAttribute(const Attribute& attribute)
{
    if (attribute.localName().string().containsIgnoringASCIICase(m_whitespaceTrimmedQuery))
        return true;
    return m_exactAttributeMatch ? attribute.value() == m_attributeQuery : attribute.value().string().containsIgnoringASCIICase(m_attributeQuery);
}

bool InspectorNodeFinder::matchesElement(const Element& element)
{
    String nodeName = element.nodeName();
    if ((!m_startTagFound && !m_endTagFound && nodeName.containsIgnoringASCIICase(m_tagNameQuery))
        || (m_startTagFound && m_endTagFound && equalIgnoringASCIICase(nodeName, m_tagNameQuery))
        || (m_startTagFound && !m_endTagFound && nodeName.startsWithIgnoringASCIICase(m_tagNameQuery))
        || (!m_startTagFound && m_endTagFound && nodeName.endsWithIgnoringASCIICase(m_tagNameQuery)))
        return true;

    if (!element.hasAttributes())
        return false;

    for (const Attribute& attribute : element.attributesIterator()) {
        if (matchesAttribute(attribute))
            return true;
    }

    return false;
}

void InspectorNodeFinder::searchUsingXPath(Node& parentNode)
{
    auto evaluateResult = parentNode.document().evaluate(m_whitespaceTrimmedQuery, &parentNode, nullptr, XPathResult::ORDERED_NODE_SNAPSHOT_TYPE, nullptr);
    if (evaluateResult.hasException())
        return;
    auto result = evaluateResult.releaseReturnValue();

    auto snapshotLengthResult = result->snapshotLength();
    if (snapshotLengthResult.hasException())
        return;
    unsigned size = snapshotLengthResult.releaseReturnValue();

    for (unsigned i = 0; i < size; ++i) {
        auto snapshotItemResult = result->snapshotItem(i);
        if (snapshotItemResult.hasException())
            return;
        Node* node = snapshotItemResult.releaseReturnValue();

        if (is<Attr>(*node))
            node = downcast<Attr>(*node).ownerElement();

        // XPath can get out of the context node that we pass as the starting point to evaluate, so we need to filter for just the nodes we care about.
        if (parentNode.contains(node))
            m_results.add(node);
    }
}

void InspectorNodeFinder::searchUsingCSSSelectors(Node& parentNode)
{
    if (!is<ContainerNode>(parentNode))
        return;

    auto queryResult = downcast<ContainerNode>(parentNode).querySelectorAll(m_whitespaceTrimmedQuery);
    if (queryResult.hasException())
        return;

    auto nodeList = queryResult.releaseReturnValue();
    unsigned size = nodeList->length();
    for (unsigned i = 0; i < size; ++i)
        m_results.add(nodeList->item(i));
}

} // namespace WebCore
