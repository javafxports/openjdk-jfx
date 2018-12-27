/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "AnimationTimeline.h"
#include "DocumentTimelineOptions.h"
#include "GenericTaskQueue.h"
#include "Timer.h"
#include <wtf/Ref.h>

namespace WebCore {

class AnimationPlaybackEvent;
class RenderElement;

class DocumentTimeline final : public AnimationTimeline
{
public:
    static Ref<DocumentTimeline> create(Document&);
    static Ref<DocumentTimeline> create(Document&, DocumentTimelineOptions&&);
    ~DocumentTimeline();

    Document* document() const { return m_document.get(); }

    std::optional<Seconds> currentTime() override;
    void pause() override;

    void timingModelDidChange() override;

    // If possible, compute the visual extent of any transform animation on the given renderer
    // using the given rect, returning the result in the rect. Return false if there is some
    // transform animation but we were unable to cheaply compute its effect on the extent.
    bool computeExtentOfAnimation(RenderElement&, LayoutRect&) const;
    std::unique_ptr<RenderStyle> animatedStyleForRenderer(RenderElement& renderer);
    bool isRunningAnimationOnRenderer(RenderElement&, CSSPropertyID) const;
    bool isRunningAcceleratedAnimationOnRenderer(RenderElement&, CSSPropertyID) const;
    void animationAcceleratedRunningStateDidChange(WebAnimation&);
    void applyPendingAcceleratedAnimations();
    bool runningAnimationsForElementAreAllAccelerated(Element&);
    bool resolveAnimationsForElement(Element&, RenderStyle&);
    void detachFromDocument();

    void enqueueAnimationPlaybackEvent(AnimationPlaybackEvent&);

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    void documentAnimationSchedulerDidFire();
#endif

    void updateThrottlingState();
    WEBCORE_EXPORT Seconds animationInterval() const;
    WEBCORE_EXPORT void suspendAnimations();
    WEBCORE_EXPORT void resumeAnimations();
    WEBCORE_EXPORT bool animationsAreSuspended();
    WEBCORE_EXPORT unsigned numberOfActiveAnimationsForTesting() const;

private:
    DocumentTimeline(Document&, Seconds);

    void scheduleInvalidationTaskIfNeeded();
    void performInvalidationTask();
    void updateAnimationSchedule();
    void animationScheduleTimerFired();
    void scheduleAnimationResolution();
    void updateAnimations();
    void performEventDispatchTask();
    void maybeClearCachedCurrentTime();

    RefPtr<Document> m_document;
    Seconds m_originTime;
    bool m_paused { false };
    bool m_isSuspended { false };
    bool m_waitingOnVMIdle { false };
    std::optional<Seconds> m_cachedCurrentTime;
    GenericTaskQueue<Timer> m_invalidationTaskQueue;
    GenericTaskQueue<Timer> m_eventDispatchTaskQueue;
    bool m_needsUpdateAnimationSchedule { false };
    Timer m_animationScheduleTimer;
    HashSet<RefPtr<WebAnimation>> m_acceleratedAnimationsPendingRunningStateChange;
    Vector<Ref<AnimationPlaybackEvent>> m_pendingAnimationEvents;

#if !USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    void animationResolutionTimerFired();
    Timer m_animationResolutionTimer;
#endif
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_ANIMATION_TIMELINE(DocumentTimeline, isDocumentTimeline())
