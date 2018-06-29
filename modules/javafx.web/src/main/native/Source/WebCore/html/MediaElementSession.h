/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(VIDEO)

#include "MediaPlayer.h"
#include "MediaProducer.h"
#include "PlatformMediaSession.h"
#include "SuccessOr.h"
#include "Timer.h"
#include <wtf/LoggerHelper.h>
#include <wtf/TypeCasts.h>

namespace WebCore {

enum class MediaSessionMainContentPurpose {
    MediaControls,
    Autoplay
};

enum class MediaPlaybackDenialReason {
    UserGestureRequired,
    FullscreenRequired,
    PageConsentRequired,
};

class Document;
class HTMLMediaElement;
class SourceBuffer;

class MediaElementSession final : public PlatformMediaSession
#if !RELEASE_LOG_DISABLED
    , private LoggerHelper
#endif
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit MediaElementSession(HTMLMediaElement&);
    virtual ~MediaElementSession() = default;

    void registerWithDocument(Document&);
    void unregisterWithDocument(Document&);

    SuccessOr<MediaPlaybackDenialReason> playbackPermitted(const HTMLMediaElement&) const;
    bool autoplayPermitted() const;
    bool dataLoadingPermitted(const HTMLMediaElement&) const;
    bool fullscreenPermitted(const HTMLMediaElement&) const;
    bool pageAllowsDataLoading(const HTMLMediaElement&) const;
    bool pageAllowsPlaybackAfterResuming(const HTMLMediaElement&) const;

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void showPlaybackTargetPicker(const HTMLMediaElement&);
    bool hasWirelessPlaybackTargets(const HTMLMediaElement&) const;

    bool wirelessVideoPlaybackDisabled(const HTMLMediaElement&) const;
    void setWirelessVideoPlaybackDisabled(const HTMLMediaElement&, bool);

    void setHasPlaybackTargetAvailabilityListeners(const HTMLMediaElement&, bool);

    bool canPlayToWirelessPlaybackTarget() const override;
    bool isPlayingToWirelessPlaybackTarget() const override;

    void mediaStateDidChange(const HTMLMediaElement&, MediaProducer::MediaStateFlags);
#endif

    bool requiresFullscreenForVideoPlayback(const HTMLMediaElement&) const;
    WEBCORE_EXPORT bool allowsPictureInPicture(const HTMLMediaElement&) const;
    MediaPlayer::Preload effectivePreloadForElement(const HTMLMediaElement&) const;
    bool allowsAutomaticMediaDataLoading(const HTMLMediaElement&) const;

    void mediaEngineUpdated(const HTMLMediaElement&);

    void resetPlaybackSessionState() override;

    // Restrictions to modify default behaviors.
    enum BehaviorRestrictionFlags : unsigned {
        NoRestrictions = 0,
        RequireUserGestureForLoad = 1 << 0,
        RequireUserGestureForVideoRateChange = 1 << 1,
        RequireUserGestureForFullscreen = 1 << 2,
        RequirePageConsentToLoadMedia = 1 << 3,
        RequirePageConsentToResumeMedia = 1 << 4,
        RequireUserGestureForAudioRateChange = 1 << 5,
        RequireUserGestureToShowPlaybackTargetPicker = 1 << 6,
        WirelessVideoPlaybackDisabled =  1 << 7,
        RequireUserGestureToAutoplayToExternalDevice = 1 << 8,
        MetadataPreloadingNotPermitted = 1 << 9,
        AutoPreloadingNotPermitted = 1 << 10,
        InvisibleAutoplayNotPermitted = 1 << 11,
        OverrideUserGestureRequirementForMainContent = 1 << 12,
        RequireUserGestureToControlControlsManager = 1 << 13,
        RequirePlaybackToControlControlsManager = 1 << 14,
        RequireUserGestureForVideoDueToLowPowerMode = 1 << 15,
        AllRestrictions = ~NoRestrictions,
    };
    typedef unsigned BehaviorRestrictions;

    WEBCORE_EXPORT BehaviorRestrictions behaviorRestrictions() const { return m_restrictions; }
    WEBCORE_EXPORT void addBehaviorRestriction(BehaviorRestrictions);
    WEBCORE_EXPORT void removeBehaviorRestriction(BehaviorRestrictions);
    bool hasBehaviorRestriction(BehaviorRestrictions restriction) const { return restriction & m_restrictions; }

#if ENABLE(MEDIA_SOURCE)
    size_t maximumMediaSourceBufferSize(const SourceBuffer&) const;
#endif

    HTMLMediaElement& element() const { return m_element; }

    bool wantsToObserveViewportVisibilityForMediaControls() const;
    bool wantsToObserveViewportVisibilityForAutoplay() const;

    enum class PlaybackControlsPurpose { ControlsManager, NowPlaying };
    bool canShowControlsManager(PlaybackControlsPurpose) const;
    bool isLargeEnoughForMainContent(MediaSessionMainContentPurpose) const;
    double mostRecentUserInteractionTime() const;

    bool allowsPlaybackControlsForAutoplayingAudio() const;
    bool allowsNowPlayingControlsVisibility() const override;

    static bool isMediaElementSessionMediaType(MediaType type)
    {
        return type == Video
            || type == Audio
            || type == VideoAudio;
    }

#if !RELEASE_LOG_DISABLED
    const Logger& logger() const final;
    const void* logIdentifier() const final;
    const char* logClassName() const final { return "MediaElementSession"; }
    WTFLogChannel& logChannel() const final;
#endif
    bool willLog(WTFLogLevel) const;

private:

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void targetAvailabilityChangedTimerFired();

    // MediaPlaybackTargetClient
    void setPlaybackTarget(Ref<MediaPlaybackTarget>&&) override;
    void externalOutputDeviceAvailableDidChange(bool) override;
    void setShouldPlayToPlaybackTarget(bool) override;
#endif
#if PLATFORM(IOS)
    bool requiresPlaybackTargetRouteMonitoring() const override;
#endif
    bool updateIsMainContent() const;
    void mainContentCheckTimerFired();

    HTMLMediaElement& m_element;
    BehaviorRestrictions m_restrictions;

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    mutable Timer m_targetAvailabilityChangedTimer;
    RefPtr<MediaPlaybackTarget> m_playbackTarget;
    bool m_shouldPlayToPlaybackTarget { false };
    mutable bool m_hasPlaybackTargets { false };
#endif
#if PLATFORM(IOS)
    bool m_hasPlaybackTargetAvailabilityListeners { false };
#endif

    double m_mostRecentUserInteractionTime { 0 };

    mutable bool m_isMainContent { false };
    Timer m_mainContentCheckTimer;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::MediaElementSession)
static bool isType(const WebCore::PlatformMediaSession& session) { return WebCore::MediaElementSession::isMediaElementSessionMediaType(session.mediaType()); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // ENABLE(VIDEO)
