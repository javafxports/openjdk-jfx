/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "HTMLFrameElementBase.h"

#include "Document.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "JSDOMBindingSecurity.h"
#include "Page.h"
#include "RenderWidget.h"
#include "ScriptController.h"
#include "Settings.h"
#include "SubframeLoader.h"
#include "URL.h"

namespace WebCore {

using namespace HTMLNames;

HTMLFrameElementBase::HTMLFrameElementBase(const QualifiedName& tagName, Document& document)
    : HTMLFrameOwnerElement(tagName, document)
    , m_scrolling(ScrollbarAuto)
    , m_marginWidth(-1)
    , m_marginHeight(-1)
{
    setHasCustomStyleResolveCallbacks();
}

bool HTMLFrameElementBase::isURLAllowed() const
{
    if (m_URL.isEmpty())
        return true;

    return isURLAllowed(document().completeURL(m_URL));
}

bool HTMLFrameElementBase::isURLAllowed(const URL& completeURL) const
{
    if (document().page() && document().page()->subframeCount() >= Page::maxNumberOfFrames)
        return false;

    if (completeURL.isEmpty())
        return true;

    if (protocolIsJavaScript(completeURL)) {
        RefPtr<Document> contentDoc = this->contentDocument();
        if (contentDoc && !ScriptController::canAccessFromCurrentOrigin(contentDoc->frame()))
            return false;
    }

    RefPtr<Frame> parentFrame = document().frame();
    if (parentFrame)
        return parentFrame->isURLAllowed(completeURL);

    return true;
}

void HTMLFrameElementBase::openURL(LockHistory lockHistory, LockBackForwardList lockBackForwardList)
{
    if (!isURLAllowed())
        return;

    if (m_URL.isEmpty())
        m_URL = blankURL().string();

    RefPtr<Frame> parentFrame = document().frame();
    if (!parentFrame)
        return;

    parentFrame->loader().subframeLoader().requestFrame(*this, m_URL, m_frameName, lockHistory, lockBackForwardList);
}

void HTMLFrameElementBase::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == srcdocAttr)
        setLocation("about:srcdoc");
    else if (name == srcAttr && !hasAttributeWithoutSynchronization(srcdocAttr))
        setLocation(stripLeadingAndTrailingHTMLSpaces(value));
    else if (name == idAttr) {
        HTMLFrameOwnerElement::parseAttribute(name, value);
        // Falling back to using the 'id' attribute is not standard but some content relies on this behavior.
        if (!hasAttributeWithoutSynchronization(nameAttr))
            m_frameName = value;
    } else if (name == nameAttr) {
        m_frameName = value;
        // FIXME: If we are already attached, this doesn't actually change the frame's name.
        // FIXME: If we are already attached, this doesn't check for frame name
        // conflicts and generate a unique frame name.
    } else if (name == marginwidthAttr) {
        m_marginWidth = value.toInt();
        // FIXME: If we are already attached, this has no effect.
    } else if (name == marginheightAttr) {
        m_marginHeight = value.toInt();
        // FIXME: If we are already attached, this has no effect.
    } else if (name == scrollingAttr) {
        // Auto and yes both simply mean "allow scrolling." No means "don't allow scrolling."
        if (equalLettersIgnoringASCIICase(value, "auto") || equalLettersIgnoringASCIICase(value, "yes"))
            m_scrolling = ScrollbarAuto;
        else if (equalLettersIgnoringASCIICase(value, "no"))
            m_scrolling = ScrollbarAlwaysOff;
        // FIXME: If we are already attached, this has no effect.
    } else
        HTMLFrameOwnerElement::parseAttribute(name, value);
}

void HTMLFrameElementBase::setNameAndOpenURL()
{
    m_frameName = getNameAttribute();
    // Falling back to using the 'id' attribute is not standard but some content relies on this behavior.
    if (m_frameName.isNull())
        m_frameName = getIdAttribute();
    openURL();
}

Node::InsertedIntoAncestorResult HTMLFrameElementBase::insertedIntoAncestor(InsertionType insertionType, ContainerNode& parentOfInsertedTree)
{
    HTMLFrameOwnerElement::insertedIntoAncestor(insertionType, parentOfInsertedTree);
    if (insertionType.connectedToDocument)
        return InsertedIntoAncestorResult::NeedsPostInsertionCallback;
    return InsertedIntoAncestorResult::Done;
}

void HTMLFrameElementBase::didFinishInsertingNode()
{
    if (!isConnected())
        return;

    // DocumentFragments don't kick off any loads.
    if (!document().frame())
        return;

    if (!SubframeLoadingDisabler::canLoadFrame(*this))
        return;

    if (!renderer())
        invalidateStyleAndRenderersForSubtree();
    setNameAndOpenURL();
}

void HTMLFrameElementBase::didAttachRenderers()
{
    if (RenderWidget* part = renderWidget()) {
        if (RefPtr<Frame> frame = contentFrame())
            part->setWidget(frame->view());
    }
}

URL HTMLFrameElementBase::location() const
{
    if (hasAttributeWithoutSynchronization(srcdocAttr))
        return URL(ParsedURLString, "about:srcdoc");
    return document().completeURL(attributeWithoutSynchronization(srcAttr));
}

void HTMLFrameElementBase::setLocation(const String& str)
{
    if (document().settings().needsAcrobatFrameReloadingQuirk() && m_URL == str)
        return;

    m_URL = AtomicString(str);

    if (isConnected())
        openURL(LockHistory::No, LockBackForwardList::No);
}

void HTMLFrameElementBase::setLocation(JSC::ExecState& state, const String& newLocation)
{
    if (protocolIsJavaScript(stripLeadingAndTrailingHTMLSpaces(newLocation))) {
        if (!BindingSecurity::shouldAllowAccessToNode(state, contentDocument()))
            return;
    }

    setLocation(newLocation);
}

bool HTMLFrameElementBase::supportsFocus() const
{
    return true;
}

void HTMLFrameElementBase::setFocus(bool received)
{
    HTMLFrameOwnerElement::setFocus(received);
    if (Page* page = document().page()) {
        if (received)
            page->focusController().setFocusedFrame(contentFrame());
        else if (page->focusController().focusedFrame() == contentFrame()) // Focus may have already been given to another frame, don't take it away.
            page->focusController().setFocusedFrame(0);
    }
}

bool HTMLFrameElementBase::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcAttr || attribute.name() == longdescAttr || HTMLFrameOwnerElement::isURLAttribute(attribute);
}

bool HTMLFrameElementBase::isHTMLContentAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcdocAttr || HTMLFrameOwnerElement::isHTMLContentAttribute(attribute);
}

int HTMLFrameElementBase::width()
{
    document().updateLayoutIgnorePendingStylesheets();
    if (!renderBox())
        return 0;
    return renderBox()->width();
}

int HTMLFrameElementBase::height()
{
    document().updateLayoutIgnorePendingStylesheets();
    if (!renderBox())
        return 0;
    return renderBox()->height();
}

} // namespace WebCore
