/**
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2007, 2013 Apple Inc. All rights reserved.
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
#include "KeyboardEvent.h"

#include "Document.h"
#include "Editor.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "PlatformKeyboardEvent.h"
#include "WindowsKeyboardCodes.h"

namespace WebCore {

static inline const AtomicString& eventTypeForKeyboardEventType(PlatformEvent::Type type)
{
    switch (type) {
        case PlatformEvent::KeyUp:
            return eventNames().keyupEvent;
        case PlatformEvent::RawKeyDown:
            return eventNames().keydownEvent;
        case PlatformEvent::Char:
            return eventNames().keypressEvent;
        case PlatformEvent::KeyDown:
            // The caller should disambiguate the combined event into RawKeyDown or Char events.
            break;
        default:
            break;
    }
    ASSERT_NOT_REACHED();
    return eventNames().keydownEvent;
}

static inline int windowsVirtualKeyCodeWithoutLocation(int keycode)
{
    switch (keycode) {
    case VK_LCONTROL:
    case VK_RCONTROL:
        return VK_CONTROL;
    case VK_LSHIFT:
    case VK_RSHIFT:
        return VK_SHIFT;
    case VK_LMENU:
    case VK_RMENU:
        return VK_MENU;
    default:
        return keycode;
    }
}

static inline KeyboardEvent::KeyLocationCode keyLocationCode(const PlatformKeyboardEvent& key)
{
    if (key.isKeypad())
        return KeyboardEvent::DOM_KEY_LOCATION_NUMPAD;

    switch (key.windowsVirtualKeyCode()) {
    case VK_LCONTROL:
    case VK_LSHIFT:
    case VK_LMENU:
    case VK_LWIN:
        return KeyboardEvent::DOM_KEY_LOCATION_LEFT;
    case VK_RCONTROL:
    case VK_RSHIFT:
    case VK_RMENU:
    case VK_RWIN:
        return KeyboardEvent::DOM_KEY_LOCATION_RIGHT;
    default:
        return KeyboardEvent::DOM_KEY_LOCATION_STANDARD;
    }
}

KeyboardEvent::KeyboardEvent() = default;

KeyboardEvent::KeyboardEvent(const PlatformKeyboardEvent& key, DOMWindow* view)
    : UIEventWithKeyState(eventTypeForKeyboardEventType(key.type())
        , true, true, key.timestamp().approximateMonotonicTime(), view, 0, key.ctrlKey(), key.altKey(), key.shiftKey()
        , key.metaKey(), false, key.modifiers().contains(PlatformEvent::Modifier::CapsLockKey))
    , m_keyEvent(std::make_unique<PlatformKeyboardEvent>(key))
#if ENABLE(KEYBOARD_KEY_ATTRIBUTE)
    , m_key(key.key())
#endif
#if ENABLE(KEYBOARD_CODE_ATTRIBUTE)
    , m_code(key.code())
#endif
    , m_keyIdentifier(key.keyIdentifier())
    , m_location(keyLocationCode(key))
    , m_repeat(key.isAutoRepeat())
    , m_isComposing(view && view->frame() && view->frame()->editor().hasComposition())
#if PLATFORM(COCOA)
#if USE(APPKIT)
    , m_handledByInputMethod(key.handledByInputMethod())
    , m_keypressCommands(key.commands())
#else
    , m_handledByInputMethod(false)
#endif
#endif
{
}

// FIXME: This method should be get ride of in the future.
// DO NOT USE IT!
KeyboardEvent::KeyboardEvent(WTF::HashTableDeletedValueType)
{
}

KeyboardEvent::KeyboardEvent(const AtomicString& eventType, const Init& initializer, IsTrusted isTrusted)
    : UIEventWithKeyState(eventType, initializer, isTrusted)
#if ENABLE(KEYBOARD_KEY_ATTRIBUTE)
    , m_key(initializer.key)
#endif
#if ENABLE(KEYBOARD_CODE_ATTRIBUTE)
    , m_code(initializer.code)
#endif
    , m_keyIdentifier(initializer.keyIdentifier)
    , m_location(initializer.keyLocation ? *initializer.keyLocation : initializer.location)
    , m_repeat(initializer.repeat)
    , m_isComposing(initializer.isComposing)
    , m_charCode(initializer.charCode)
    , m_keyCode(initializer.keyCode)
    , m_which(initializer.which)
#if PLATFORM(COCOA)
    , m_handledByInputMethod(false)
#endif
{
}

KeyboardEvent::~KeyboardEvent() = default;

void KeyboardEvent::initKeyboardEvent(const AtomicString& type, bool canBubble, bool cancelable, DOMWindow* view,
                                      const String &keyIdentifier, unsigned location,
                                      bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool altGraphKey)
{
    if (isBeingDispatched())
        return;

    initUIEvent(type, canBubble, cancelable, view, 0);

    m_keyEvent = nullptr;
    m_keyIdentifier = keyIdentifier;
    m_location = location;
    m_ctrlKey = ctrlKey;
    m_shiftKey = shiftKey;
    m_altKey = altKey;
    m_metaKey = metaKey;
    m_altGraphKey = altGraphKey;

#if PLATFORM(COCOA)
    m_handledByInputMethod = false;
    m_keypressCommands = { };
#endif
}

bool KeyboardEvent::getModifierState(const String& keyIdentifier) const
{
    if (keyIdentifier == "Control")
        return ctrlKey();
    if (keyIdentifier == "Shift")
        return shiftKey();
    if (keyIdentifier == "Alt")
        return altKey();
    if (keyIdentifier == "Meta")
        return metaKey();
    if (keyIdentifier == "AltGraph")
        return altGraphKey();
    if (keyIdentifier == "CapsLock")
        return capsLockKey();
    // FIXME: The specification also has Fn, FnLock, Hyper, NumLock, Super, ScrollLock, Symbol, SymbolLock.
    return false;
}

int KeyboardEvent::keyCode() const
{
    if (m_keyCode)
        return m_keyCode.value();

    // IE: virtual key code for keyup/keydown, character code for keypress
    // Firefox: virtual key code for keyup/keydown, zero for keypress
    // We match IE.
    if (!m_keyEvent)
        return 0;
    if (type() == eventNames().keydownEvent || type() == eventNames().keyupEvent)
        return windowsVirtualKeyCodeWithoutLocation(m_keyEvent->windowsVirtualKeyCode());

    return charCode();
}

int KeyboardEvent::charCode() const
{
    if (m_charCode)
        return m_charCode.value();

    // IE: not supported
    // Firefox: 0 for keydown/keyup events, character code for keypress
    // We match Firefox, unless in backward compatibility mode, where we always return the character code.
    bool backwardCompatibilityMode = false;
    if (view() && view()->frame())
        backwardCompatibilityMode = view()->frame()->eventHandler().needsKeyboardEventDisambiguationQuirks();

    if (!m_keyEvent || (type() != eventNames().keypressEvent && !backwardCompatibilityMode))
        return 0;
    String text = m_keyEvent->text();
    return static_cast<int>(text.characterStartingAt(0));
}

EventInterface KeyboardEvent::eventInterface() const
{
    return KeyboardEventInterfaceType;
}

bool KeyboardEvent::isKeyboardEvent() const
{
    return true;
}

int KeyboardEvent::which() const
{
    // Netscape's "which" returns a virtual key code for keydown and keyup, and a character code for keypress.
    // That's exactly what IE's "keyCode" returns. So they are the same for keyboard events.
    if (m_which)
        return m_which.value();
    return keyCode();
}

KeyboardEvent* findKeyboardEvent(Event* event)
{
    for (Event* e = event; e; e = e->underlyingEvent())
        if (is<KeyboardEvent>(*e))
            return downcast<KeyboardEvent>(e);
    return nullptr;
}

} // namespace WebCore
