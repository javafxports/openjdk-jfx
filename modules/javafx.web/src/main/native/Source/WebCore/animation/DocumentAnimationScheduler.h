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

#pragma once

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)

#include "DisplayRefreshMonitorClient.h"
#include "PlatformScreen.h"
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Seconds.h>

namespace WebCore {

class Document;

class DocumentAnimationScheduler : public RefCounted<DocumentAnimationScheduler>
    , public DisplayRefreshMonitorClient {
public:
    static Ref<DocumentAnimationScheduler> create(Document&, PlatformDisplayID);
    ~DocumentAnimationScheduler();

    void detachFromDocument();
    void windowScreenDidChange(PlatformDisplayID);

    bool scheduleWebAnimationsResolution();
    void unscheduleWebAnimationsResolution();
    bool scheduleScriptedAnimationResolution();

    Seconds lastTimestamp() { return m_lastTimestamp; }
    bool isFiring() const { return m_isFiring; }

private:
    DocumentAnimationScheduler(Document&, PlatformDisplayID);

    RefPtr<Document> m_document;
    bool m_scheduledWebAnimationsResolution { false };
    bool m_scheduledScriptedAnimationResolution { false };
    bool m_isFiring { false };
    Seconds m_lastTimestamp { 0_s };

    void displayRefreshFired() override;
    RefPtr<DisplayRefreshMonitor> createDisplayRefreshMonitor(PlatformDisplayID) const override;
};

} // namespace WebCore

#endif // USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
