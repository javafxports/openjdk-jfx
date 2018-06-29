/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#if ENABLE(MEDIA_STREAM)
#include "RealtimeMediaSourceSettings.h"

#include <wtf/NeverDestroyed.h>

namespace WebCore {

String RealtimeMediaSourceSettings::facingMode(RealtimeMediaSourceSettings::VideoFacingMode mode)
{
    static const NeverDestroyed<String> values[] = {
        MAKE_STATIC_STRING_IMPL("unknown"),
        MAKE_STATIC_STRING_IMPL("user"),
        MAKE_STATIC_STRING_IMPL("environment"),
        MAKE_STATIC_STRING_IMPL("left"),
        MAKE_STATIC_STRING_IMPL("right"),
    };
    static_assert(static_cast<size_t>(RealtimeMediaSourceSettings::VideoFacingMode::Unknown) == 0, "RealtimeMediaSourceSettings::VideoFacingMode::Unknown is not 0 as expected");
    static_assert(static_cast<size_t>(RealtimeMediaSourceSettings::VideoFacingMode::User) == 1, "RealtimeMediaSourceSettings::VideoFacingMode::User is not 1 as expected");
    static_assert(static_cast<size_t>(RealtimeMediaSourceSettings::VideoFacingMode::Environment) == 2, "RealtimeMediaSourceSettings::VideoFacingMode::Environment is not 2 as expected");
    static_assert(static_cast<size_t>(RealtimeMediaSourceSettings::VideoFacingMode::Left) == 3, "RealtimeMediaSourceSettings::VideoFacingMode::Left is not 3 as expected");
    static_assert(static_cast<size_t>(RealtimeMediaSourceSettings::VideoFacingMode::Right) == 4, "RealtimeMediaSourceSettings::VideoFacingMode::Right is not 4 as expected");
    ASSERT(static_cast<size_t>(mode) < WTF_ARRAY_LENGTH(values));
    return values[static_cast<size_t>(mode)];
}

RealtimeMediaSourceSettings::VideoFacingMode RealtimeMediaSourceSettings::videoFacingModeEnum(const String& mode)
{
    if (mode == "user")
        return RealtimeMediaSourceSettings::VideoFacingMode::User;
    if (mode == "environment")
        return RealtimeMediaSourceSettings::VideoFacingMode::Environment;
    if (mode == "left")
        return RealtimeMediaSourceSettings::VideoFacingMode::Left;
    if (mode == "right")
        return RealtimeMediaSourceSettings::VideoFacingMode::Right;

    return RealtimeMediaSourceSettings::Unknown;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
