/*
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009, 2010, 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Motorola Mobility. All rights reserved.
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

#include "config.h"
#include "markup.h"

#include "ArchiveResource.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CSSValue.h"
#include "CSSValueKeywords.h"
#include "CacheStorageProvider.h"
#include "ChildListMutationScope.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "DocumentType.h"
#include "Editing.h"
#include "Editor.h"
#include "EditorClient.h"
#include "ElementIterator.h"
#include "EmptyClients.h"
#include "File.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLAttachmentElement.h"
#include "HTMLBRElement.h"
#include "HTMLBodyElement.h"
#include "HTMLDivElement.h"
#include "HTMLHeadElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLTextFormControlElement.h"
#include "LibWebRTCProvider.h"
#include "MainFrame.h"
#include "MarkupAccumulator.h"
#include "NodeList.h"
#include "Page.h"
#include "PageConfiguration.h"
#include "Range.h"
#include "RenderBlock.h"
#include "Settings.h"
#include "SocketProvider.h"
#include "StyleProperties.h"
#include "TextIterator.h"
#include "TypedElementDescendantIterator.h"
#include "URL.h"
#include "URLParser.h"
#include "VisibleSelection.h"
#include "VisibleUnits.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

static bool propertyMissingOrEqualToNone(StyleProperties*, CSSPropertyID);

class AttributeChange {
public:
    AttributeChange()
        : m_name(nullAtom(), nullAtom(), nullAtom())
    {
    }

    AttributeChange(Element* element, const QualifiedName& name, const String& value)
        : m_element(element), m_name(name), m_value(value)
    {
    }

    void apply()
    {
        m_element->setAttribute(m_name, m_value);
    }

private:
    RefPtr<Element> m_element;
    QualifiedName m_name;
    String m_value;
};

static void completeURLs(DocumentFragment* fragment, const String& baseURL)
{
    Vector<AttributeChange> changes;

    URL parsedBaseURL(ParsedURLString, baseURL);

    for (auto& element : descendantsOfType<Element>(*fragment)) {
        if (!element.hasAttributes())
            continue;
        for (const Attribute& attribute : element.attributesIterator()) {
            if (element.attributeContainsURL(attribute) && !attribute.value().isEmpty())
                changes.append(AttributeChange(&element, attribute.name(), element.completeURLsInAttributeValue(parsedBaseURL, attribute)));
        }
    }

    for (auto& change : changes)
        change.apply();
}

void replaceSubresourceURLs(Ref<DocumentFragment>&& fragment, HashMap<AtomicString, AtomicString>&& replacementMap)
{
    Vector<AttributeChange> changes;
    for (auto& element : descendantsOfType<Element>(fragment)) {
        if (!element.hasAttributes())
            continue;
        for (const Attribute& attribute : element.attributesIterator()) {
            // FIXME: This won't work for srcset.
            if (element.attributeContainsURL(attribute) && !attribute.value().isEmpty()) {
                auto replacement = replacementMap.get(attribute.value());
                if (!replacement.isNull())
                    changes.append({ &element, attribute.name(), replacement });
            }
        }
    }
    for (auto& change : changes)
        change.apply();
}

struct ElementAttribute {
    Ref<Element> element;
    QualifiedName attributeName;
};

void removeSubresourceURLAttributes(Ref<DocumentFragment>&& fragment, WTF::Function<bool(const URL&)> shouldRemoveURL)
{
    Vector<ElementAttribute> attributesToRemove;
    for (auto& element : descendantsOfType<Element>(fragment)) {
        if (!element.hasAttributes())
            continue;
        for (const Attribute& attribute : element.attributesIterator()) {
            // FIXME: This won't work for srcset.
            if (element.attributeContainsURL(attribute) && !attribute.value().isEmpty()) {
                URL url = URLParser { attribute.value() }.result();
                if (shouldRemoveURL(url))
                    attributesToRemove.append({ element, attribute.name() });
            }
        }
    }
    for (auto& item : attributesToRemove)
        item.element->removeAttribute(item.attributeName);
}

std::unique_ptr<Page> createPageForSanitizingWebContent()
{
    PageConfiguration pageConfiguration(createEmptyEditorClient(), SocketProvider::create(), LibWebRTCProvider::create(), CacheStorageProvider::create());

    fillWithEmptyClients(pageConfiguration);

    auto page = std::make_unique<Page>(WTFMove(pageConfiguration));
    page->settings().setMediaEnabled(false);
    page->settings().setScriptEnabled(false);
    page->settings().setPluginsEnabled(false);
    page->settings().setAcceleratedCompositingEnabled(false);

    Frame& frame = page->mainFrame();
    frame.setView(FrameView::create(frame));
    frame.init();

    FrameLoader& loader = frame.loader();
    static char markup[] = "<!DOCTYPE html><html><body></body></html>";
    ASSERT(loader.activeDocumentLoader());
    loader.activeDocumentLoader()->writer().setMIMEType("text/html");
    loader.activeDocumentLoader()->writer().begin();
    loader.activeDocumentLoader()->writer().addData(markup, sizeof(markup));
    loader.activeDocumentLoader()->writer().end();

    return page;
}


String sanitizeMarkup(const String& rawHTML, std::optional<WTF::Function<void(DocumentFragment&)>> fragmentSanitizer)
{
    auto page = createPageForSanitizingWebContent();
    Document* stagingDocument = page->mainFrame().document();
    ASSERT(stagingDocument);
    auto* bodyElement = stagingDocument->body();
    ASSERT(bodyElement);

    auto fragment = createFragmentFromMarkup(*stagingDocument, rawHTML, emptyString(), DisallowScriptingAndPluginContent);

    if (fragmentSanitizer)
        (*fragmentSanitizer)(fragment);

    bodyElement->appendChild(fragment.get());

    auto range = Range::create(*stagingDocument);
    range->selectNodeContents(*bodyElement);
    return createMarkup(range.get(), nullptr, AnnotateForInterchange, false, ResolveNonLocalURLs);
}


class StyledMarkupAccumulator final : public MarkupAccumulator {
public:
    enum RangeFullySelectsNode { DoesFullySelectNode, DoesNotFullySelectNode };

    StyledMarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs, EAnnotateForInterchange, const Range*, bool needsPositionStyleConversion, Node* highestNodeToBeSerialized = nullptr);

    Node* serializeNodes(Node* startNode, Node* pastEnd);
    void wrapWithNode(Node&, bool convertBlocksToInlines = false, RangeFullySelectsNode = DoesFullySelectNode);
    void wrapWithStyleNode(StyleProperties*, Document&, bool isBlock = false);
    String takeResults();

    bool needRelativeStyleWrapper() const { return m_needRelativeStyleWrapper; }
    bool needClearingDiv() const { return m_needClearingDiv; }

    using MarkupAccumulator::appendString;

private:
    void appendStyleNodeOpenTag(StringBuilder&, StyleProperties*, Document&, bool isBlock = false);
    const String& styleNodeCloseTag(bool isBlock = false);

    String renderedText(const Node&, const Range*);
    String stringValueForRange(const Node&, const Range*);

    void appendElement(StringBuilder& out, const Element&, bool addDisplayInline, RangeFullySelectsNode);
    void appendCustomAttributes(StringBuilder&, const Element&, Namespaces*) override;

    void appendText(StringBuilder& out, const Text&) override;
    void appendElement(StringBuilder& out, const Element& element, Namespaces*) override
    {
        appendElement(out, element, false, DoesFullySelectNode);
    }

    enum NodeTraversalMode { EmitString, DoNotEmitString };
    Node* traverseNodesForSerialization(Node* startNode, Node* pastEnd, NodeTraversalMode);

    bool shouldAnnotate()
    {
        return m_shouldAnnotate == AnnotateForInterchange;
    }

    bool shouldApplyWrappingStyle(const Node& node) const
    {
        return m_highestNodeToBeSerialized && m_highestNodeToBeSerialized->parentNode() == node.parentNode() && m_wrappingStyle && m_wrappingStyle->style();
    }

    Vector<String> m_reversedPrecedingMarkup;
    const EAnnotateForInterchange m_shouldAnnotate;
    Node* m_highestNodeToBeSerialized;
    RefPtr<EditingStyle> m_wrappingStyle;
    bool m_needRelativeStyleWrapper;
    bool m_needsPositionStyleConversion;
    bool m_needClearingDiv;
};

inline StyledMarkupAccumulator::StyledMarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, EAnnotateForInterchange shouldAnnotate, const Range* range, bool needsPositionStyleConversion, Node* highestNodeToBeSerialized)
    : MarkupAccumulator(nodes, shouldResolveURLs, range)
    , m_shouldAnnotate(shouldAnnotate)
    , m_highestNodeToBeSerialized(highestNodeToBeSerialized)
    , m_needRelativeStyleWrapper(false)
    , m_needsPositionStyleConversion(needsPositionStyleConversion)
    , m_needClearingDiv(false)
{
}

void StyledMarkupAccumulator::wrapWithNode(Node& node, bool convertBlocksToInlines, RangeFullySelectsNode rangeFullySelectsNode)
{
    StringBuilder markup;
    if (is<Element>(node))
        appendElement(markup, downcast<Element>(node), convertBlocksToInlines && isBlock(&node), rangeFullySelectsNode);
    else
        appendStartMarkup(markup, node, nullptr);
    m_reversedPrecedingMarkup.append(markup.toString());
    appendEndTag(node);
    if (m_nodes)
        m_nodes->append(&node);
}

void StyledMarkupAccumulator::wrapWithStyleNode(StyleProperties* style, Document& document, bool isBlock)
{
    StringBuilder openTag;
    appendStyleNodeOpenTag(openTag, style, document, isBlock);
    m_reversedPrecedingMarkup.append(openTag.toString());
    appendString(styleNodeCloseTag(isBlock));
}

void StyledMarkupAccumulator::appendStyleNodeOpenTag(StringBuilder& out, StyleProperties* style, Document& document, bool isBlock)
{
    // wrappingStyleForSerialization should have removed -webkit-text-decorations-in-effect
    ASSERT(propertyMissingOrEqualToNone(style, CSSPropertyWebkitTextDecorationsInEffect));
    if (isBlock)
        out.appendLiteral("<div style=\"");
    else
        out.appendLiteral("<span style=\"");
    appendAttributeValue(out, style->asText(), document.isHTMLDocument());
    out.appendLiteral("\">");
}

const String& StyledMarkupAccumulator::styleNodeCloseTag(bool isBlock)
{
    static NeverDestroyed<const String> divClose(MAKE_STATIC_STRING_IMPL("</div>"));
    static NeverDestroyed<const String> styleSpanClose(MAKE_STATIC_STRING_IMPL("</span>"));
    return isBlock ? divClose : styleSpanClose;
}

String StyledMarkupAccumulator::takeResults()
{
    StringBuilder result;
    result.reserveCapacity(totalLength(m_reversedPrecedingMarkup) + length());

    for (size_t i = m_reversedPrecedingMarkup.size(); i > 0; --i)
        result.append(m_reversedPrecedingMarkup[i - 1]);

    concatenateMarkup(result);

    // We remove '\0' characters because they are not visibly rendered to the user.
    return result.toString().replaceWithLiteral('\0', "");
}

void StyledMarkupAccumulator::appendText(StringBuilder& out, const Text& text)
{
    const bool parentIsTextarea = is<HTMLTextAreaElement>(text.parentElement());
    const bool wrappingSpan = shouldApplyWrappingStyle(text) && !parentIsTextarea;
    if (wrappingSpan) {
        RefPtr<EditingStyle> wrappingStyle = m_wrappingStyle->copy();
        // FIXME: <rdar://problem/5371536> Style rules that match pasted content can change it's appearance
        // Make sure spans are inline style in paste side e.g. span { display: block }.
        wrappingStyle->forceInline();
        // FIXME: Should this be included in forceInline?
        wrappingStyle->style()->setProperty(CSSPropertyFloat, CSSValueNone);

        appendStyleNodeOpenTag(out, wrappingStyle->style(), text.document());
    }

    if (!shouldAnnotate() || parentIsTextarea)
        MarkupAccumulator::appendText(out, text);
    else {
        const bool useRenderedText = !enclosingElementWithTag(firstPositionInNode(const_cast<Text*>(&text)), selectTag);
        String content = useRenderedText ? renderedText(text, m_range) : stringValueForRange(text, m_range);
        StringBuilder buffer;
        appendCharactersReplacingEntities(buffer, content, 0, content.length(), EntityMaskInPCDATA);
        out.append(convertHTMLTextToInterchangeFormat(buffer.toString(), &text));
    }

    if (wrappingSpan)
        out.append(styleNodeCloseTag());
}

String StyledMarkupAccumulator::renderedText(const Node& node, const Range* range)
{
    if (!is<Text>(node))
        return String();

    const Text& textNode = downcast<Text>(node);
    unsigned startOffset = 0;
    unsigned endOffset = textNode.length();

    TextIteratorBehavior behavior = TextIteratorDefaultBehavior;
    if (range && &node == &range->startContainer())
        startOffset = range->startOffset();
    if (range && &node == &range->endContainer())
        endOffset = range->endOffset();
    else if (range)
        behavior = TextIteratorBehavesAsIfNodesFollowing;

    Position start = createLegacyEditingPosition(const_cast<Node*>(&node), startOffset);
    Position end = createLegacyEditingPosition(const_cast<Node*>(&node), endOffset);
    return plainText(Range::create(node.document(), start, end).ptr(), behavior);
}

String StyledMarkupAccumulator::stringValueForRange(const Node& node, const Range* range)
{
    if (!range)
        return node.nodeValue();

    String nodeValue = node.nodeValue();
    if (&node == &range->endContainer())
        nodeValue.truncate(range->endOffset());
    if (&node == &range->startContainer())
        nodeValue.remove(0, range->startOffset());
    return nodeValue;
}

void StyledMarkupAccumulator::appendCustomAttributes(StringBuilder& out, const Element& element, Namespaces* namespaces)
{
#if ENABLE(ATTACHMENT_ELEMENT)
    if (!is<HTMLAttachmentElement>(element))
        return;

    const HTMLAttachmentElement& attachment = downcast<HTMLAttachmentElement>(element);
    appendAttribute(out, element, { webkitattachmentidAttr, attachment.uniqueIdentifier() }, namespaces);
    if (auto* file = attachment.file()) {
        // These attributes are only intended for File deserialization, and are removed from the generated attachment
        // element after we've deserialized and set its backing File.
        appendAttribute(out, element, { webkitattachmentpathAttr, file->path() }, namespaces);
        appendAttribute(out, element, { webkitattachmentbloburlAttr, file->url().string() }, namespaces);
    }
#else
    UNUSED_PARAM(out);
    UNUSED_PARAM(element);
    UNUSED_PARAM(namespaces);
#endif
}

void StyledMarkupAccumulator::appendElement(StringBuilder& out, const Element& element, bool addDisplayInline, RangeFullySelectsNode rangeFullySelectsNode)
{
    const bool documentIsHTML = element.document().isHTMLDocument();
    appendOpenTag(out, element, 0);

    appendCustomAttributes(out, element, nullptr);

    const bool shouldAnnotateOrForceInline = element.isHTMLElement() && (shouldAnnotate() || addDisplayInline);
    const bool shouldOverrideStyleAttr = shouldAnnotateOrForceInline || shouldApplyWrappingStyle(element);
    if (element.hasAttributes()) {
        for (const Attribute& attribute : element.attributesIterator()) {
            // We'll handle the style attribute separately, below.
            if (attribute.name() == styleAttr && shouldOverrideStyleAttr)
                continue;
            if (element.isEventHandlerAttribute(attribute) || element.isJavaScriptURLAttribute(attribute))
                continue;
            appendAttribute(out, element, attribute, 0);
        }
    }

    if (shouldOverrideStyleAttr) {
        RefPtr<EditingStyle> newInlineStyle;

        if (shouldApplyWrappingStyle(element)) {
            newInlineStyle = m_wrappingStyle->copy();
            newInlineStyle->removePropertiesInElementDefaultStyle(*const_cast<Element*>(&element));
            newInlineStyle->removeStyleConflictingWithStyleOfNode(*const_cast<Element*>(&element));
        } else
            newInlineStyle = EditingStyle::create();

        if (is<StyledElement>(element) && downcast<StyledElement>(element).inlineStyle())
            newInlineStyle->overrideWithStyle(downcast<StyledElement>(element).inlineStyle());

        if (shouldAnnotateOrForceInline) {
            if (shouldAnnotate())
                newInlineStyle->mergeStyleFromRulesForSerialization(downcast<HTMLElement>(*const_cast<Element*>(&element)));

            if (addDisplayInline)
                newInlineStyle->forceInline();

            if (m_needsPositionStyleConversion) {
                m_needRelativeStyleWrapper |= newInlineStyle->convertPositionStyle();
                m_needClearingDiv |= newInlineStyle->isFloating();
            }

            // If the node is not fully selected by the range, then we don't want to keep styles that affect its relationship to the nodes around it
            // only the ones that affect it and the nodes within it.
            if (rangeFullySelectsNode == DoesNotFullySelectNode && newInlineStyle->style())
                newInlineStyle->style()->removeProperty(CSSPropertyFloat);
        }

        if (!newInlineStyle->isEmpty()) {
            out.appendLiteral(" style=\"");
            appendAttributeValue(out, newInlineStyle->style()->asText(), documentIsHTML);
            out.append('\"');
        }
    }

    appendCloseTag(out, element);
}

Node* StyledMarkupAccumulator::serializeNodes(Node* startNode, Node* pastEnd)
{
    if (!m_highestNodeToBeSerialized) {
        Node* lastClosed = traverseNodesForSerialization(startNode, pastEnd, DoNotEmitString);
        m_highestNodeToBeSerialized = lastClosed;
    }

    if (m_highestNodeToBeSerialized && m_highestNodeToBeSerialized->parentNode())
        m_wrappingStyle = EditingStyle::wrappingStyleForSerialization(m_highestNodeToBeSerialized->parentNode(), shouldAnnotate());

    return traverseNodesForSerialization(startNode, pastEnd, EmitString);
}

Node* StyledMarkupAccumulator::traverseNodesForSerialization(Node* startNode, Node* pastEnd, NodeTraversalMode traversalMode)
{
    const bool shouldEmit = traversalMode == EmitString;
    Vector<Node*> ancestorsToClose;
    Node* next;
    Node* lastClosed = nullptr;
    for (Node* n = startNode; n != pastEnd; n = next) {
        // According to <rdar://problem/5730668>, it is possible for n to blow
        // past pastEnd and become null here. This shouldn't be possible.
        // This null check will prevent crashes (but create too much markup)
        // and the ASSERT will hopefully lead us to understanding the problem.
        ASSERT(n);
        if (!n)
            break;

        next = NodeTraversal::next(*n);
        bool openedTag = false;

        if (isBlock(n) && canHaveChildrenForEditing(*n) && next == pastEnd) {
            // Don't write out empty block containers that aren't fully selected.
            continue;
        }

        if (!n->renderer() && !enclosingElementWithTag(firstPositionInOrBeforeNode(n), selectTag)) {
            next = NodeTraversal::nextSkippingChildren(*n);
            // Don't skip over pastEnd.
            if (pastEnd && pastEnd->isDescendantOf(*n))
                next = pastEnd;
        } else {
            // Add the node to the markup if we're not skipping the descendants
            if (shouldEmit)
                appendStartTag(*n);

            // If node has no children, close the tag now.
            if (!n->hasChildNodes()) {
                if (shouldEmit)
                    appendEndTag(*n);
                lastClosed = n;
            } else {
                openedTag = true;
                ancestorsToClose.append(n);
            }
        }

        // If we didn't insert open tag and there's no more siblings or we're at the end of the traversal, take care of ancestors.
        // FIXME: What happens if we just inserted open tag and reached the end?
        if (!openedTag && (!n->nextSibling() || next == pastEnd)) {
            // Close up the ancestors.
            while (!ancestorsToClose.isEmpty()) {
                Node* ancestor = ancestorsToClose.last();
                if (next != pastEnd && next->isDescendantOf(ancestor))
                    break;
                // Not at the end of the range, close ancestors up to sibling of next node.
                if (shouldEmit)
                    appendEndTag(*ancestor);
                lastClosed = ancestor;
                ancestorsToClose.removeLast();
            }

            // Surround the currently accumulated markup with markup for ancestors we never opened as we leave the subtree(s) rooted at those ancestors.
            ContainerNode* nextParent = next ? next->parentNode() : 0;
            if (next != pastEnd && n != nextParent) {
                Node* lastAncestorClosedOrSelf = n->isDescendantOf(lastClosed) ? lastClosed : n;
                for (ContainerNode* parent = lastAncestorClosedOrSelf->parentNode(); parent && parent != nextParent; parent = parent->parentNode()) {
                    // All ancestors that aren't in the ancestorsToClose list should either be a) unrendered:
                    if (!parent->renderer())
                        continue;
                    // or b) ancestors that we never encountered during a pre-order traversal starting at startNode:
                    ASSERT(startNode->isDescendantOf(*parent));
                    if (shouldEmit)
                        wrapWithNode(*parent);
                    lastClosed = parent;
                }
            }
        }
    }

    return lastClosed;
}

static Node* ancestorToRetainStructureAndAppearanceForBlock(Node* commonAncestorBlock)
{
    if (!commonAncestorBlock)
        return nullptr;

    if (commonAncestorBlock->hasTagName(tbodyTag) || commonAncestorBlock->hasTagName(trTag)) {
        ContainerNode* table = commonAncestorBlock->parentNode();
        while (table && !is<HTMLTableElement>(*table))
            table = table->parentNode();

        return table;
    }

    if (isNonTableCellHTMLBlockElement(commonAncestorBlock))
        return commonAncestorBlock;

    return nullptr;
}

static inline Node* ancestorToRetainStructureAndAppearance(Node* commonAncestor)
{
    return ancestorToRetainStructureAndAppearanceForBlock(enclosingBlock(commonAncestor));
}

static bool propertyMissingOrEqualToNone(StyleProperties* style, CSSPropertyID propertyID)
{
    if (!style)
        return false;
    RefPtr<CSSValue> value = style->getPropertyCSSValue(propertyID);
    if (!value)
        return true;
    if (!is<CSSPrimitiveValue>(*value))
        return false;
    return downcast<CSSPrimitiveValue>(*value).valueID() == CSSValueNone;
}

static bool needInterchangeNewlineAfter(const VisiblePosition& v)
{
    VisiblePosition next = v.next();
    Node* upstreamNode = next.deepEquivalent().upstream().deprecatedNode();
    Node* downstreamNode = v.deepEquivalent().downstream().deprecatedNode();
    // Add an interchange newline if a paragraph break is selected and a br won't already be added to the markup to represent it.
    return isEndOfParagraph(v) && isStartOfParagraph(next) && !(upstreamNode->hasTagName(brTag) && upstreamNode == downstreamNode);
}

static RefPtr<EditingStyle> styleFromMatchedRulesAndInlineDecl(Node& node)
{
    if (!is<HTMLElement>(node))
        return nullptr;

    auto& element = downcast<HTMLElement>(node);
    RefPtr<EditingStyle> style = EditingStyle::create(element.inlineStyle());
    style->mergeStyleFromRules(element);
    return style;
}

static bool isElementPresentational(const Node* node)
{
    return node->hasTagName(uTag) || node->hasTagName(sTag) || node->hasTagName(strikeTag)
        || node->hasTagName(iTag) || node->hasTagName(emTag) || node->hasTagName(bTag) || node->hasTagName(strongTag);
}

static Node* highestAncestorToWrapMarkup(const Range* range, EAnnotateForInterchange shouldAnnotate)
{
    auto* commonAncestor = range->commonAncestorContainer();
    ASSERT(commonAncestor);
    Node* specialCommonAncestor = nullptr;
    if (shouldAnnotate == AnnotateForInterchange) {
        // Include ancestors that aren't completely inside the range but are required to retain
        // the structure and appearance of the copied markup.
        specialCommonAncestor = ancestorToRetainStructureAndAppearance(commonAncestor);

        if (auto* parentListNode = enclosingNodeOfType(firstPositionInOrBeforeNode(range->firstNode()), isListItem)) {
            if (!editingIgnoresContent(*parentListNode) && WebCore::areRangesEqual(VisibleSelection::selectionFromContentsOfNode(parentListNode).toNormalizedRange().get(), range)) {
                specialCommonAncestor = parentListNode->parentNode();
                while (specialCommonAncestor && !isListHTMLElement(specialCommonAncestor))
                    specialCommonAncestor = specialCommonAncestor->parentNode();
            }
        }

        // Retain the Mail quote level by including all ancestor mail block quotes.
        if (Node* highestMailBlockquote = highestEnclosingNodeOfType(firstPositionInOrBeforeNode(range->firstNode()), isMailBlockquote, CanCrossEditingBoundary))
            specialCommonAncestor = highestMailBlockquote;
    }

    auto* checkAncestor = specialCommonAncestor ? specialCommonAncestor : commonAncestor;
    if (checkAncestor->renderer() && checkAncestor->renderer()->containingBlock()) {
        Node* newSpecialCommonAncestor = highestEnclosingNodeOfType(firstPositionInNode(checkAncestor), &isElementPresentational, CanCrossEditingBoundary, checkAncestor->renderer()->containingBlock()->element());
        if (newSpecialCommonAncestor)
            specialCommonAncestor = newSpecialCommonAncestor;
    }

    // If a single tab is selected, commonAncestor will be a text node inside a tab span.
    // If two or more tabs are selected, commonAncestor will be the tab span.
    // In either case, if there is a specialCommonAncestor already, it will necessarily be above
    // any tab span that needs to be included.
    if (!specialCommonAncestor && isTabSpanTextNode(commonAncestor))
        specialCommonAncestor = commonAncestor->parentNode();
    if (!specialCommonAncestor && isTabSpanNode(commonAncestor))
        specialCommonAncestor = commonAncestor;

    if (auto* enclosingAnchor = enclosingElementWithTag(firstPositionInNode(specialCommonAncestor ? specialCommonAncestor : commonAncestor), aTag))
        specialCommonAncestor = enclosingAnchor;

    return specialCommonAncestor;
}

// FIXME: Shouldn't we omit style info when annotate == DoNotAnnotateForInterchange?
// FIXME: At least, annotation and style info should probably not be included in range.markupString()
static String createMarkupInternal(Document& document, const Range& range, Vector<Node*>* nodes,
    EAnnotateForInterchange shouldAnnotate, bool convertBlocksToInlines, EAbsoluteURLs shouldResolveURLs)
{
    static NeverDestroyed<const String> interchangeNewlineString(MAKE_STATIC_STRING_IMPL("<br class=\"" AppleInterchangeNewline "\">"));

    bool collapsed = range.collapsed();
    if (collapsed)
        return emptyString();
    RefPtr<Node> commonAncestor = range.commonAncestorContainer();
    if (!commonAncestor)
        return emptyString();

    document.updateLayoutIgnorePendingStylesheets();

    auto* body = enclosingElementWithTag(firstPositionInNode(commonAncestor.get()), bodyTag);
    Element* fullySelectedRoot = nullptr;
    // FIXME: Do this for all fully selected blocks, not just the body.
    if (body && VisiblePosition(firstPositionInNode(body)) == VisiblePosition(range.startPosition())
        && VisiblePosition(lastPositionInNode(body)) == VisiblePosition(range.endPosition()))
        fullySelectedRoot = body;
    Node* specialCommonAncestor = highestAncestorToWrapMarkup(&range, shouldAnnotate);

    bool needsPositionStyleConversion = body && fullySelectedRoot == body
        && document.settings().shouldConvertPositionStyleOnCopy();
    StyledMarkupAccumulator accumulator(nodes, shouldResolveURLs, shouldAnnotate, &range, needsPositionStyleConversion, specialCommonAncestor);
    Node* pastEnd = range.pastLastNode();

    Node* startNode = range.firstNode();
    VisiblePosition visibleStart(range.startPosition(), VP_DEFAULT_AFFINITY);
    VisiblePosition visibleEnd(range.endPosition(), VP_DEFAULT_AFFINITY);
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleStart)) {
        if (visibleStart == visibleEnd.previous())
            return interchangeNewlineString;

        accumulator.appendString(interchangeNewlineString);
        startNode = visibleStart.next().deepEquivalent().deprecatedNode();

        if (pastEnd && Range::compareBoundaryPoints(startNode, 0, pastEnd, 0).releaseReturnValue() >= 0)
            return interchangeNewlineString;
    }

    Node* lastClosed = accumulator.serializeNodes(startNode, pastEnd);

    if (specialCommonAncestor && lastClosed) {
        // Also include all of the ancestors of lastClosed up to this special ancestor.
        for (ContainerNode* ancestor = lastClosed->parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (ancestor == fullySelectedRoot && !convertBlocksToInlines) {
                RefPtr<EditingStyle> fullySelectedRootStyle = styleFromMatchedRulesAndInlineDecl(*fullySelectedRoot);

                // Bring the background attribute over, but not as an attribute because a background attribute on a div
                // appears to have no effect.
                if ((!fullySelectedRootStyle || !fullySelectedRootStyle->style() || !fullySelectedRootStyle->style()->getPropertyCSSValue(CSSPropertyBackgroundImage))
                    && fullySelectedRoot->hasAttributeWithoutSynchronization(backgroundAttr))
                    fullySelectedRootStyle->style()->setProperty(CSSPropertyBackgroundImage, "url('" + fullySelectedRoot->getAttribute(backgroundAttr) + "')");

                if (fullySelectedRootStyle->style()) {
                    // Reset the CSS properties to avoid an assertion error in addStyleMarkup().
                    // This assertion is caused at least when we select all text of a <body> element whose
                    // 'text-decoration' property is "inherit", and copy it.
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle->style(), CSSPropertyTextDecoration))
                        fullySelectedRootStyle->style()->setProperty(CSSPropertyTextDecoration, CSSValueNone);
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle->style(), CSSPropertyWebkitTextDecorationsInEffect))
                        fullySelectedRootStyle->style()->setProperty(CSSPropertyWebkitTextDecorationsInEffect, CSSValueNone);
                    accumulator.wrapWithStyleNode(fullySelectedRootStyle->style(), document, true);
                }
            } else {
                // Since this node and all the other ancestors are not in the selection we want to set RangeFullySelectsNode to DoesNotFullySelectNode
                // so that styles that affect the exterior of the node are not included.
                accumulator.wrapWithNode(*ancestor, convertBlocksToInlines, StyledMarkupAccumulator::DoesNotFullySelectNode);
            }
            if (nodes)
                nodes->append(ancestor);

            if (ancestor == specialCommonAncestor)
                break;
        }
    }

    if (accumulator.needRelativeStyleWrapper() && needsPositionStyleConversion) {
        if (accumulator.needClearingDiv())
            accumulator.appendString("<div style=\"clear: both;\"></div>");
        RefPtr<EditingStyle> positionRelativeStyle = styleFromMatchedRulesAndInlineDecl(*body);
        positionRelativeStyle->style()->setProperty(CSSPropertyPosition, CSSValueRelative);
        accumulator.wrapWithStyleNode(positionRelativeStyle->style(), document, true);
    }

    // FIXME: The interchange newline should be placed in the block that it's in, not after all of the content, unconditionally.
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleEnd.previous()))
        accumulator.appendString(interchangeNewlineString);

    return accumulator.takeResults();
}

String createMarkup(const Range& range, Vector<Node*>* nodes, EAnnotateForInterchange shouldAnnotate, bool convertBlocksToInlines, EAbsoluteURLs shouldResolveURLs)
{
    return createMarkupInternal(range.ownerDocument(), range, nodes, shouldAnnotate, convertBlocksToInlines, shouldResolveURLs);
}

Ref<DocumentFragment> createFragmentFromMarkup(Document& document, const String& markup, const String& baseURL, ParserContentPolicy parserContentPolicy)
{
    // We use a fake body element here to trick the HTML parser to using the InBody insertion mode.
    auto fakeBody = HTMLBodyElement::create(document);
    auto fragment = DocumentFragment::create(document);

    fragment->parseHTML(markup, fakeBody.ptr(), parserContentPolicy);

#if ENABLE(ATTACHMENT_ELEMENT)
    // When creating a fragment we must strip the webkit-attachment-path attribute after restoring the File object.
    Vector<Ref<HTMLAttachmentElement>> attachments;
    for (auto& attachment : descendantsOfType<HTMLAttachmentElement>(fragment))
        attachments.append(attachment);

    for (auto& attachment : attachments) {
        attachment->setUniqueIdentifier(attachment->attributeWithoutSynchronization(webkitattachmentidAttr));

        auto attachmentPath = attachment->attachmentPath();
        auto blobURL = attachment->blobURL();
        if (!attachmentPath.isEmpty())
            attachment->setFile(File::create(attachmentPath));
        else if (!blobURL.isEmpty())
            attachment->setFile(File::deserialize({ }, blobURL, attachment->attachmentType(), attachment->attachmentTitle()));

        attachment->removeAttribute(webkitattachmentidAttr);
        attachment->removeAttribute(webkitattachmentpathAttr);
        attachment->removeAttribute(webkitattachmentbloburlAttr);
    }
#endif
    if (!baseURL.isEmpty() && baseURL != blankURL() && baseURL != document.baseURL())
        completeURLs(fragment.ptr(), baseURL);

    return fragment;
}

String createMarkup(const Node& node, EChildrenOnly childrenOnly, Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, Vector<QualifiedName>* tagNamesToSkip, EFragmentSerialization fragmentSerialization)
{
    MarkupAccumulator accumulator(nodes, shouldResolveURLs, 0, fragmentSerialization);
    return accumulator.serializeNodes(const_cast<Node&>(node), childrenOnly, tagNamesToSkip);
}

static void fillContainerFromString(ContainerNode& paragraph, const String& string)
{
    Document& document = paragraph.document();

    if (string.isEmpty()) {
        paragraph.appendChild(createBlockPlaceholderElement(document));
        return;
    }

    ASSERT(string.find('\n') == notFound);

    Vector<String> tabList;
    string.split('\t', true, tabList);
    String tabText = emptyString();
    bool first = true;
    size_t numEntries = tabList.size();
    for (size_t i = 0; i < numEntries; ++i) {
        const String& s = tabList[i];

        // append the non-tab textual part
        if (!s.isEmpty()) {
            if (!tabText.isEmpty()) {
                paragraph.appendChild(createTabSpanElement(document, tabText));
                tabText = emptyString();
            }
            Ref<Node> textNode = document.createTextNode(stringWithRebalancedWhitespace(s, first, i + 1 == numEntries));
            paragraph.appendChild(textNode);
        }

        // there is a tab after every entry, except the last entry
        // (if the last character is a tab, the list gets an extra empty entry)
        if (i + 1 != numEntries)
            tabText.append('\t');
        else if (!tabText.isEmpty())
            paragraph.appendChild(createTabSpanElement(document, tabText));

        first = false;
    }
}

bool isPlainTextMarkup(Node* node)
{
    ASSERT(node);
    if (!is<HTMLDivElement>(*node))
        return false;

    HTMLDivElement& element = downcast<HTMLDivElement>(*node);
    if (element.hasAttributes())
        return false;

    Node* firstChild = element.firstChild();
    if (!firstChild)
        return false;

    Node* secondChild = firstChild->nextSibling();
    if (!secondChild)
        return firstChild->isTextNode() || firstChild->firstChild();

    if (secondChild->nextSibling())
        return false;

    return isTabSpanTextNode(firstChild->firstChild()) && secondChild->isTextNode();
}

static bool contextPreservesNewline(const Range& context)
{
    VisiblePosition position(context.startPosition());
    Node* container = position.deepEquivalent().containerNode();
    if (!container || !container->renderer())
        return false;

    return container->renderer()->style().preserveNewline();
}

Ref<DocumentFragment> createFragmentFromText(Range& context, const String& text)
{
    Document& document = context.ownerDocument();
    Ref<DocumentFragment> fragment = document.createDocumentFragment();

    if (text.isEmpty())
        return fragment;

    String string = text;
    string.replace("\r\n", "\n");
    string.replace('\r', '\n');

    if (contextPreservesNewline(context)) {
        fragment->appendChild(document.createTextNode(string));
        if (string.endsWith('\n')) {
            auto element = HTMLBRElement::create(document);
            element->setAttributeWithoutSynchronization(classAttr, AppleInterchangeNewline);
            fragment->appendChild(element);
        }
        return fragment;
    }

    // A string with no newlines gets added inline, rather than being put into a paragraph.
    if (string.find('\n') == notFound) {
        fillContainerFromString(fragment, string);
        return fragment;
    }

    // Break string into paragraphs. Extra line breaks turn into empty paragraphs.
    Node* blockNode = enclosingBlock(context.firstNode());
    Element* block = downcast<Element>(blockNode);
    bool useClonesOfEnclosingBlock = blockNode
        && blockNode->isElementNode()
        && !block->hasTagName(bodyTag)
        && !block->hasTagName(htmlTag)
        && block != editableRootForPosition(context.startPosition());
    bool useLineBreak = enclosingTextFormControl(context.startPosition());

    Vector<String> list;
    string.split('\n', true, list); // true gets us empty strings in the list
    size_t numLines = list.size();
    for (size_t i = 0; i < numLines; ++i) {
        const String& s = list[i];

        RefPtr<Element> element;
        if (s.isEmpty() && i + 1 == numLines) {
            // For last line, use the "magic BR" rather than a P.
            element = HTMLBRElement::create(document);
            element->setAttributeWithoutSynchronization(classAttr, AppleInterchangeNewline);
        } else if (useLineBreak) {
            element = HTMLBRElement::create(document);
            fillContainerFromString(fragment, s);
        } else {
            if (useClonesOfEnclosingBlock)
                element = block->cloneElementWithoutChildren(document);
            else
                element = createDefaultParagraphElement(document);
            fillContainerFromString(*element, s);
        }
        fragment->appendChild(*element);
    }
    return fragment;
}

String documentTypeString(const Document& document)
{
    DocumentType* documentType = document.doctype();
    if (!documentType)
        return emptyString();
    return createMarkup(*documentType);
}

String createFullMarkup(const Node& node)
{
    // FIXME: This is never "for interchange". Is that right?
    String markupString = createMarkup(node, IncludeNode, 0);

    Node::NodeType nodeType = node.nodeType();
    if (nodeType != Node::DOCUMENT_NODE && nodeType != Node::DOCUMENT_TYPE_NODE)
        markupString = documentTypeString(node.document()) + markupString;

    return markupString;
}

String createFullMarkup(const Range& range)
{
    // FIXME: This is always "for interchange". Is that right?
    return documentTypeString(range.startContainer().document()) + createMarkup(range, 0, AnnotateForInterchange);
}

String urlToMarkup(const URL& url, const String& title)
{
    StringBuilder markup;
    markup.appendLiteral("<a href=\"");
    markup.append(url.string());
    markup.appendLiteral("\">");
    MarkupAccumulator::appendCharactersReplacingEntities(markup, title, 0, title.length(), EntityMaskInPCDATA);
    markup.appendLiteral("</a>");
    return markup.toString();
}

ExceptionOr<Ref<DocumentFragment>> createFragmentForInnerOuterHTML(Element& contextElement, const String& markup, ParserContentPolicy parserContentPolicy)
{
    auto* document = &contextElement.document();
    if (contextElement.hasTagName(templateTag))
        document = &document->ensureTemplateDocument();
    auto fragment = DocumentFragment::create(*document);

    if (document->isHTMLDocument()) {
        fragment->parseHTML(markup, &contextElement, parserContentPolicy);
        return WTFMove(fragment);
    }

    bool wasValid = fragment->parseXML(markup, &contextElement, parserContentPolicy);
    if (!wasValid)
        return Exception { SyntaxError };
    return WTFMove(fragment);
}

RefPtr<DocumentFragment> createFragmentForTransformToFragment(Document& outputDoc, const String& sourceString, const String& sourceMIMEType)
{
    RefPtr<DocumentFragment> fragment = outputDoc.createDocumentFragment();

    if (sourceMIMEType == "text/html") {
        // As far as I can tell, there isn't a spec for how transformToFragment is supposed to work.
        // Based on the documentation I can find, it looks like we want to start parsing the fragment in the InBody insertion mode.
        // Unfortunately, that's an implementation detail of the parser.
        // We achieve that effect here by passing in a fake body element as context for the fragment.
        RefPtr<HTMLBodyElement> fakeBody = HTMLBodyElement::create(outputDoc);
        fragment->parseHTML(sourceString, fakeBody.get());
    } else if (sourceMIMEType == "text/plain")
        fragment->parserAppendChild(Text::create(outputDoc, sourceString));
    else {
        bool successfulParse = fragment->parseXML(sourceString, 0);
        if (!successfulParse)
            return nullptr;
    }

    // FIXME: Do we need to mess with URLs here?

    return fragment;
}

Ref<DocumentFragment> createFragmentForImageAndURL(Document& document, const String& url)
{
    auto imageElement = HTMLImageElement::create(document);
    imageElement->setAttributeWithoutSynchronization(HTMLNames::srcAttr, url);

    auto fragment = document.createDocumentFragment();
    fragment->appendChild(imageElement);

    return fragment;
}

static Vector<Ref<HTMLElement>> collectElementsToRemoveFromFragment(ContainerNode& container)
{
    Vector<Ref<HTMLElement>> toRemove;
    for (auto& element : childrenOfType<HTMLElement>(container)) {
        if (is<HTMLHtmlElement>(element)) {
            toRemove.append(element);
            collectElementsToRemoveFromFragment(element);
            continue;
        }
        if (is<HTMLHeadElement>(element) || is<HTMLBodyElement>(element))
            toRemove.append(element);
    }
    return toRemove;
}

static void removeElementFromFragmentPreservingChildren(DocumentFragment& fragment, HTMLElement& element)
{
    RefPtr<Node> nextChild;
    for (RefPtr<Node> child = element.firstChild(); child; child = nextChild) {
        nextChild = child->nextSibling();
        element.removeChild(*child);
        fragment.insertBefore(*child, &element);
    }
    fragment.removeChild(element);
}

ExceptionOr<Ref<DocumentFragment>> createContextualFragment(Element& element, const String& markup, ParserContentPolicy parserContentPolicy)
{
    auto result = createFragmentForInnerOuterHTML(element, markup, parserContentPolicy);
    if (result.hasException())
        return result.releaseException();

    auto fragment = result.releaseReturnValue();

    // We need to pop <html> and <body> elements and remove <head> to
    // accommodate folks passing complete HTML documents to make the
    // child of an element.
    auto toRemove = collectElementsToRemoveFromFragment(fragment);
    for (auto& element : toRemove)
        removeElementFromFragmentPreservingChildren(fragment, element);

    return WTFMove(fragment);
}

static inline bool hasOneChild(ContainerNode& node)
{
    Node* firstChild = node.firstChild();
    return firstChild && !firstChild->nextSibling();
}

static inline bool hasOneTextChild(ContainerNode& node)
{
    return hasOneChild(node) && node.firstChild()->isTextNode();
}

static inline bool hasMutationEventListeners(const Document& document)
{
    return document.hasListenerType(Document::DOMSUBTREEMODIFIED_LISTENER)
        || document.hasListenerType(Document::DOMNODEINSERTED_LISTENER)
        || document.hasListenerType(Document::DOMNODEREMOVED_LISTENER)
        || document.hasListenerType(Document::DOMNODEREMOVEDFROMDOCUMENT_LISTENER)
        || document.hasListenerType(Document::DOMCHARACTERDATAMODIFIED_LISTENER);
}

// We can use setData instead of replacing Text node as long as script can't observe the difference.
static inline bool canUseSetDataOptimization(const Text& containerChild, const ChildListMutationScope& mutationScope)
{
    bool authorScriptMayHaveReference = containerChild.refCount();
    return !authorScriptMayHaveReference && !mutationScope.canObserve() && !hasMutationEventListeners(containerChild.document());
}

ExceptionOr<void> replaceChildrenWithFragment(ContainerNode& container, Ref<DocumentFragment>&& fragment)
{
    Ref<ContainerNode> containerNode(container);
    ChildListMutationScope mutation(containerNode);

    if (!fragment->firstChild()) {
        containerNode->removeChildren();
        return { };
    }

    auto* containerChild = containerNode->firstChild();
    if (containerChild && !containerChild->nextSibling()) {
        if (is<Text>(*containerChild) && hasOneTextChild(fragment) && canUseSetDataOptimization(downcast<Text>(*containerChild), mutation)) {
            ASSERT(!fragment->firstChild()->refCount());
            downcast<Text>(*containerChild).setData(downcast<Text>(*fragment->firstChild()).data());
            return { };
        }

        return containerNode->replaceChild(fragment, *containerChild);
    }

    containerNode->removeChildren();
    return containerNode->appendChild(fragment);
}

}
