/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
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
 *
 */

#include "config.h"
#include "Animation.h"

#include <wtf/NeverDestroyed.h>

namespace WebCore {

Animation::Animation()
    : m_name(initialName())
    , m_property(CSSPropertyInvalid)
    , m_mode(AnimateAll)
    , m_iterationCount(initialIterationCount())
    , m_delay(initialDelay())
    , m_duration(initialDuration())
    , m_timingFunction(initialTimingFunction())
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    , m_trigger(initialTrigger())
#endif
    , m_direction(initialDirection())
    , m_fillMode(initialFillMode())
    , m_playState(initialPlayState())
    , m_delaySet(false)
    , m_directionSet(false)
    , m_durationSet(false)
    , m_fillModeSet(false)
    , m_iterationCountSet(false)
    , m_nameSet(false)
    , m_playStateSet(false)
    , m_propertySet(false)
    , m_timingFunctionSet(false)
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    , m_triggerSet(false)
#endif
    , m_isNone(false)
{
}

Animation::Animation(const Animation& o)
    : RefCounted<Animation>()
    , m_name(o.m_name)
    , m_nameStyleScopeOrdinal(o.m_nameStyleScopeOrdinal)
    , m_property(o.m_property)
    , m_mode(o.m_mode)
    , m_iterationCount(o.m_iterationCount)
    , m_delay(o.m_delay)
    , m_duration(o.m_duration)
    , m_timingFunction(o.m_timingFunction)
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    , m_trigger(o.m_trigger)
#endif
    , m_direction(o.m_direction)
    , m_fillMode(o.m_fillMode)
    , m_playState(o.m_playState)
    , m_delaySet(o.m_delaySet)
    , m_directionSet(o.m_directionSet)
    , m_durationSet(o.m_durationSet)
    , m_fillModeSet(o.m_fillModeSet)
    , m_iterationCountSet(o.m_iterationCountSet)
    , m_nameSet(o.m_nameSet)
    , m_playStateSet(o.m_playStateSet)
    , m_propertySet(o.m_propertySet)
    , m_timingFunctionSet(o.m_timingFunctionSet)
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    , m_triggerSet(o.m_triggerSet)
#endif
    , m_isNone(o.m_isNone)
{
}

Animation& Animation::operator=(const Animation& o)
{
    m_name = o.m_name;
    m_nameStyleScopeOrdinal = o.m_nameStyleScopeOrdinal;
    m_property = o.m_property;
    m_mode = o.m_mode;
    m_iterationCount = o.m_iterationCount;
    m_delay = o.m_delay;
    m_duration = o.m_duration;
    m_timingFunction = o.m_timingFunction;
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    m_trigger = o.m_trigger;
#endif
    m_direction = o.m_direction;
    m_fillMode = o.m_fillMode;
    m_playState = o.m_playState;
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    m_trigger = o.m_trigger;
#endif

    m_delaySet = o.m_delaySet;
    m_directionSet = o.m_directionSet;
    m_durationSet = o.m_durationSet;
    m_fillModeSet = o.m_fillModeSet;
    m_iterationCountSet = o.m_iterationCountSet;
    m_nameSet = o.m_nameSet;
    m_playStateSet = o.m_playStateSet;
    m_propertySet = o.m_propertySet;
    m_timingFunctionSet = o.m_timingFunctionSet;
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    m_triggerSet = o.m_triggerSet;
#endif
    m_isNone = o.m_isNone;

    return *this;
}

Animation::~Animation() = default;

bool Animation::animationsMatch(const Animation& other, bool matchPlayStates) const
{
    bool result = m_name == other.m_name
        && m_nameStyleScopeOrdinal == other.m_nameStyleScopeOrdinal
        && m_property == other.m_property
        && m_mode == other.m_mode
        && m_iterationCount == other.m_iterationCount
        && m_delay == other.m_delay
        && m_duration == other.m_duration
        && *(m_timingFunction.get()) == *(other.m_timingFunction.get())
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
        && *(m_trigger.get()) == *(other.m_trigger.get())
#endif
        && m_direction == other.m_direction
        && m_fillMode == other.m_fillMode
        && m_delaySet == other.m_delaySet
        && m_directionSet == other.m_directionSet
        && m_durationSet == other.m_durationSet
        && m_fillModeSet == other.m_fillModeSet
        && m_iterationCountSet == other.m_iterationCountSet
        && m_nameSet == other.m_nameSet
        && m_propertySet == other.m_propertySet
        && m_timingFunctionSet == other.m_timingFunctionSet
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
        && m_triggerSet == other.m_triggerSet
#endif
        && m_isNone == other.m_isNone;

    if (!result)
        return false;

    return !matchPlayStates || (m_playState == other.m_playState && m_playStateSet == other.m_playStateSet);
}

const String& Animation::initialName()
{
    static NeverDestroyed<String> initialValue(MAKE_STATIC_STRING_IMPL("none"));
    return initialValue;
}

} // namespace WebCore
