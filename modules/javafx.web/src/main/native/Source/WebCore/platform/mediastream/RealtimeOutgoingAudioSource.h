/*
 * Copyright (C) 2017 Apple Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once


#if USE(LIBWEBRTC)

#include "LibWebRTCMacros.h"
#include "MediaStreamTrackPrivate.h"
#include "Timer.h"
#include <webrtc/api/mediastreaminterface.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace webrtc {
class AudioTrackInterface;
class AudioTrackSinkInterface;
}

namespace WebCore {

class RealtimeOutgoingAudioSource : public ThreadSafeRefCounted<RealtimeOutgoingAudioSource>, public webrtc::AudioSourceInterface, private MediaStreamTrackPrivate::Observer {
public:
    static Ref<RealtimeOutgoingAudioSource> create(Ref<MediaStreamTrackPrivate>&& audioSource);

    ~RealtimeOutgoingAudioSource() { stop(); }

    void stop();

    bool setSource(Ref<MediaStreamTrackPrivate>&&);
    MediaStreamTrackPrivate& source() const { return m_audioSource.get(); }

protected:
    explicit RealtimeOutgoingAudioSource(Ref<MediaStreamTrackPrivate>&&);

    virtual void handleMutedIfNeeded();
    virtual void sendSilence() { };
    virtual void pullAudioData() { };

    Vector<webrtc::AudioTrackSinkInterface*> m_sinks;
    bool m_muted { false };
    bool m_enabled { true };

private:
    virtual void AddSink(webrtc::AudioTrackSinkInterface* sink) { m_sinks.append(sink); }
    virtual void RemoveSink(webrtc::AudioTrackSinkInterface* sink) { m_sinks.removeFirst(sink); }

    int AddRef() const final { ref(); return refCount(); }
    int Release() const final { deref(); return refCount(); }
    SourceState state() const final { return kLive; }
    bool remote() const final { return false; }
    void RegisterObserver(webrtc::ObserverInterface*) final { }
    void UnregisterObserver(webrtc::ObserverInterface*) final { }

    void sourceMutedChanged();
    void sourceEnabledChanged();
    virtual void audioSamplesAvailable(const MediaTime&, const PlatformAudioData&, const AudioStreamDescription&, size_t) { };

    virtual bool isReachingBufferedAudioDataHighLimit() { return false; };
    virtual bool isReachingBufferedAudioDataLowLimit() { return false; };
    virtual bool hasBufferedEnoughData() { return false; };

    // MediaStreamTrackPrivate::Observer API
    void trackMutedChanged(MediaStreamTrackPrivate&) final { sourceMutedChanged(); }
    void trackEnabledChanged(MediaStreamTrackPrivate&) final { sourceEnabledChanged(); }
    void audioSamplesAvailable(MediaStreamTrackPrivate&, const MediaTime& mediaTime, const PlatformAudioData& data, const AudioStreamDescription& description, size_t sampleCount) { audioSamplesAvailable(mediaTime, data, description, sampleCount); }
    void trackEnded(MediaStreamTrackPrivate&) final { }
    void trackSettingsChanged(MediaStreamTrackPrivate&) final { }

    void initializeConverter();

    Ref<MediaStreamTrackPrivate> m_audioSource;

    Timer m_silenceAudioTimer;
};

} // namespace WebCore

#endif // USE(LIBWEBRTC)
