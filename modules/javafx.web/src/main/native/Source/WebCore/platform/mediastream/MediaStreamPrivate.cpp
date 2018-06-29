/*
 * Copyright (C) 2011, 2015 Ericsson AB. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "MediaStreamPrivate.h"

#if ENABLE(MEDIA_STREAM)

#include "GraphicsContext.h"
#include "IntRect.h"
#include <wtf/MainThread.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

Ref<MediaStreamPrivate> MediaStreamPrivate::create(const Vector<Ref<RealtimeMediaSource>>& audioSources, const Vector<Ref<RealtimeMediaSource>>& videoSources)
{
    MediaStreamTrackPrivateVector tracks;
    tracks.reserveInitialCapacity(audioSources.size() + videoSources.size());

    for (auto& source : audioSources)
        tracks.uncheckedAppend(MediaStreamTrackPrivate::create(source.copyRef()));

    for (auto& source : videoSources)
        tracks.uncheckedAppend(MediaStreamTrackPrivate::create(source.copyRef()));

    return MediaStreamPrivate::create(tracks);
}

MediaStreamPrivate::MediaStreamPrivate(const MediaStreamTrackPrivateVector& tracks, String&& id)
    : m_id(WTFMove(id))
{
    ASSERT(!m_id.isEmpty());

    for (auto& track : tracks) {
        track->addObserver(*this);
        m_trackSet.add(track->id(), track);
    }

    updateActiveState(NotifyClientOption::DontNotify);
}

MediaStreamPrivate::~MediaStreamPrivate()
{
    for (auto& track : m_trackSet.values())
        track->removeObserver(*this);
}

void MediaStreamPrivate::addObserver(MediaStreamPrivate::Observer& observer)
{
    m_observers.append(&observer);
}

void MediaStreamPrivate::removeObserver(MediaStreamPrivate::Observer& observer)
{
    size_t pos = m_observers.find(&observer);
    if (pos != notFound)
        m_observers.remove(pos);
}

MediaStreamTrackPrivateVector MediaStreamPrivate::tracks() const
{
    return copyToVector(m_trackSet.values());
}

void MediaStreamPrivate::updateActiveState(NotifyClientOption notifyClientOption)
{
    bool newActiveState = false;
    for (auto& track : m_trackSet.values()) {
        if (!track->ended()) {
            newActiveState = true;
            break;
        }
    }

    updateActiveVideoTrack();

    // A stream is active if it has at least one un-ended track.
    if (newActiveState == m_isActive)
        return;

    m_isActive = newActiveState;

    if (notifyClientOption == NotifyClientOption::Notify) {
        for (auto& observer : m_observers)
            observer->activeStatusChanged();
    }
}

void MediaStreamPrivate::addTrack(RefPtr<MediaStreamTrackPrivate>&& track, NotifyClientOption notifyClientOption)
{
    if (m_trackSet.contains(track->id()))
        return;

    track->addObserver(*this);
    m_trackSet.add(track->id(), track);

    if (notifyClientOption == NotifyClientOption::Notify) {
        for (auto& observer : m_observers)
            observer->didAddTrack(*track.get());
    }

    updateActiveState(notifyClientOption);
    characteristicsChanged();
}

void MediaStreamPrivate::removeTrack(MediaStreamTrackPrivate& track, NotifyClientOption notifyClientOption)
{
    if (!m_trackSet.remove(track.id()))
        return;

    track.removeObserver(*this);

    if (notifyClientOption == NotifyClientOption::Notify) {
        for (auto& observer : m_observers)
            observer->didRemoveTrack(track);
    }

    updateActiveState(NotifyClientOption::Notify);
    characteristicsChanged();
}

void MediaStreamPrivate::startProducingData()
{
    for (auto& track : m_trackSet.values())
        track->startProducingData();
}

void MediaStreamPrivate::stopProducingData()
{
    for (auto& track : m_trackSet.values())
        track->stopProducingData();
}

bool MediaStreamPrivate::isProducingData() const
{
    for (auto& track : m_trackSet.values()) {
        if (track->isProducingData())
            return true;
    }
    return false;
}

void MediaStreamPrivate::setCaptureTracksMuted(bool muted)
{
    for (auto& track : m_trackSet.values()) {
        if (track->isCaptureTrack())
            track->setMuted(muted);
    }
}

bool MediaStreamPrivate::hasVideo() const
{
    for (auto& track : m_trackSet.values()) {
        if (track->type() == RealtimeMediaSource::Type::Video && track->enabled() && !track->ended())
            return true;
    }
    return false;
}

bool MediaStreamPrivate::hasAudio() const
{
    for (auto& track : m_trackSet.values()) {
        if (track->type() == RealtimeMediaSource::Type::Audio && track->enabled() && !track->ended())
            return true;
    }
    return false;
}

bool MediaStreamPrivate::hasCaptureVideoSource() const
{
    for (auto& track : m_trackSet.values()) {
        if (track->type() == RealtimeMediaSource::Type::Video && track->isCaptureTrack())
            return true;
    }
    return false;
}

bool MediaStreamPrivate::hasCaptureAudioSource() const
{
    for (auto& track : m_trackSet.values()) {
        if (track->type() == RealtimeMediaSource::Type::Audio && track->isCaptureTrack())
            return true;
    }
    return false;
}

bool MediaStreamPrivate::muted() const
{
    for (auto& track : m_trackSet.values()) {
        if (!track->muted() && !track->ended())
            return false;
    }
    return true;
}

FloatSize MediaStreamPrivate::intrinsicSize() const
{
    FloatSize size;

    if (m_activeVideoTrack) {
        const RealtimeMediaSourceSettings& setting = m_activeVideoTrack->settings();
        size.setWidth(setting.width());
        size.setHeight(setting.height());
    }

    return size;
}

void MediaStreamPrivate::updateActiveVideoTrack()
{
    m_activeVideoTrack = nullptr;
    for (auto& track : m_trackSet.values()) {
        if (!track->ended() && track->type() == RealtimeMediaSource::Type::Video) {
            m_activeVideoTrack = track.get();
            break;
        }
    }
}

void MediaStreamPrivate::characteristicsChanged()
{
    for (auto& observer : m_observers)
        observer->characteristicsChanged();
}

void MediaStreamPrivate::trackMutedChanged(MediaStreamTrackPrivate&)
{
    scheduleDeferredTask([this] {
        characteristicsChanged();
    });
}

void MediaStreamPrivate::trackSettingsChanged(MediaStreamTrackPrivate&)
{
    characteristicsChanged();
}

void MediaStreamPrivate::trackEnabledChanged(MediaStreamTrackPrivate&)
{
    updateActiveVideoTrack();

    scheduleDeferredTask([this] {
        characteristicsChanged();
    });
}

void MediaStreamPrivate::trackStarted(MediaStreamTrackPrivate&)
{
    scheduleDeferredTask([this] {
        characteristicsChanged();
    });
}

void MediaStreamPrivate::trackEnded(MediaStreamTrackPrivate&)
{
    scheduleDeferredTask([this] {
        updateActiveState(NotifyClientOption::Notify);
        characteristicsChanged();
    });
}

void MediaStreamPrivate::scheduleDeferredTask(Function<void ()>&& function)
{
    ASSERT(function);
    callOnMainThread([weakThis = createWeakPtr(), function = WTFMove(function)] {
        if (!weakThis)
            return;

        function();
    });
}

void MediaStreamPrivate::monitorOrientation(OrientationNotifier& notifier)
{
    for (auto& track : m_trackSet.values()) {
        if (track->source().isCaptureSource() && track->type() == RealtimeMediaSource::Type::Video)
            track->source().monitorOrientation(notifier);
    }
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
