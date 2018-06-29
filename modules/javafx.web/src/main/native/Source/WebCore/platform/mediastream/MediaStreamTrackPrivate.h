/*
 *  Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 *  Copyright (C) 2015 Ericsson AB. All rights reserved.
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

#pragma once

#if ENABLE(MEDIA_STREAM)

#include "RealtimeMediaSource.h"

namespace WebCore {

class AudioSourceProvider;
class GraphicsContext;
class MediaSample;
class RealtimeMediaSourceCapabilities;
class WebAudioSourceProvider;

class MediaStreamTrackPrivate : public RefCounted<MediaStreamTrackPrivate>, public RealtimeMediaSource::Observer {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void trackStarted(MediaStreamTrackPrivate&) { };
        virtual void trackEnded(MediaStreamTrackPrivate&) = 0;
        virtual void trackMutedChanged(MediaStreamTrackPrivate&) = 0;
        virtual void trackSettingsChanged(MediaStreamTrackPrivate&) = 0;
        virtual void trackEnabledChanged(MediaStreamTrackPrivate&) = 0;
        virtual void sampleBufferUpdated(MediaStreamTrackPrivate&, MediaSample&) { };
        virtual void audioSamplesAvailable(MediaStreamTrackPrivate&, const MediaTime&, const PlatformAudioData&, const AudioStreamDescription&, size_t) { };
        virtual void readyStateChanged(MediaStreamTrackPrivate&) { };
    };

    static Ref<MediaStreamTrackPrivate> create(Ref<RealtimeMediaSource>&&);
    static Ref<MediaStreamTrackPrivate> create(Ref<RealtimeMediaSource>&&, String&& id);

    virtual ~MediaStreamTrackPrivate();

    const String& id() const { return m_id; }
    const String& label() const;

    bool ended() const { return m_isEnded; }

    void startProducingData() { m_source->start(); }
    void stopProducingData() { m_source->stop(); }
    bool isProducingData() { return m_source->isProducingData(); }

    bool isIsolated() const { return m_source->isIsolated(); }

    bool muted() const;
    void setMuted(bool muted) { m_source->setMuted(muted); }

    bool isCaptureTrack() const;

    bool enabled() const { return m_isEnabled; }
    void setEnabled(bool);

    Ref<MediaStreamTrackPrivate> clone();

    RealtimeMediaSource& source() { return m_source.get(); }
    RealtimeMediaSource::Type type() const;

    void endTrack();

    void addObserver(Observer&);
    void removeObserver(Observer&);

    const RealtimeMediaSourceSettings& settings() const;
    const RealtimeMediaSourceCapabilities& capabilities() const;

    void applyConstraints(const MediaConstraints&, RealtimeMediaSource::SuccessHandler&&, RealtimeMediaSource::FailureHandler&&);

    AudioSourceProvider* audioSourceProvider();

    void paintCurrentFrameInContext(GraphicsContext&, const FloatRect&);

    enum class ReadyState { None, Live, Ended };
    ReadyState readyState() const { return m_readyState; }

private:
    MediaStreamTrackPrivate(Ref<RealtimeMediaSource>&&, String&& id);

    // RealtimeMediaSourceObserver
    void sourceStarted() final;
    void sourceStopped() final;
    void sourceMutedChanged() final;
    void sourceSettingsChanged() final;
    bool preventSourceFromStopping() final;
    void videoSampleAvailable(MediaSample&) final;
    void audioSamplesAvailable(const MediaTime&, const PlatformAudioData&, const AudioStreamDescription&, size_t) final;

    void updateReadyState();

    Vector<Observer*> m_observers;
    Ref<RealtimeMediaSource> m_source;

    String m_id;
    ReadyState m_readyState { ReadyState::None };
    bool m_isEnabled { true };
    bool m_isEnded { false };
    bool m_haveProducedData { false };
    RefPtr<WebAudioSourceProvider> m_audioSourceProvider;
};

typedef Vector<RefPtr<MediaStreamTrackPrivate>> MediaStreamTrackPrivateVector;

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
