/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2013 Apple Inc. All rights reserved.
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
 *
 */

#pragma once

#include "DOMWindow.h"
#include "Event.h"
#include "UIEventInit.h"

namespace WebCore {

// FIXME: Remove this when no one is depending on it anymore.
typedef DOMWindow AbstractView;

class UIEvent : public Event {
public:
    static Ref<UIEvent> create(const AtomicString& type, bool canBubble, bool cancelable, DOMWindow* view, int detail)
    {
        return adoptRef(*new UIEvent(type, canBubble, cancelable, view, detail));
    }
    static Ref<UIEvent> createForBindings()
    {
        return adoptRef(*new UIEvent);
    }
    static Ref<UIEvent> create(const AtomicString& type, const UIEventInit& initializer, IsTrusted isTrusted = IsTrusted::No)
    {
        return adoptRef(*new UIEvent(type, initializer, isTrusted));
    }
    virtual ~UIEvent();

    WEBCORE_EXPORT void initUIEvent(const AtomicString& type, bool canBubble, bool cancelable, DOMWindow*, int detail);

    DOMWindow* view() const { return m_view.get(); }
    int detail() const { return m_detail; }

    EventInterface eventInterface() const override;

    virtual int layerX();
    virtual int layerY();

    virtual int pageX() const;
    virtual int pageY() const;

    virtual int which() const;

protected:
    UIEvent();
    UIEvent(const AtomicString& type, bool canBubble, bool cancelable, DOMWindow*, int detail);
    UIEvent(const AtomicString& type, bool canBubble, bool cancelable, MonotonicTime timestamp, DOMWindow*, int detail);
    UIEvent(const AtomicString&, const UIEventInit&, IsTrusted);

private:
    bool isUIEvent() const final;

    RefPtr<DOMWindow> m_view;
    int m_detail;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_EVENT(UIEvent)
