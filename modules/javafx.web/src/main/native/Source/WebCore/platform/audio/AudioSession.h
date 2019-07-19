/*
 * Copyright (C) 2013-2019 Apple Inc. All rights reserved.
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

#if USE(AUDIO_SESSION)

#include <memory>
#include <wtf/HashSet.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class AudioSessionPrivate;

enum class RouteSharingPolicy : uint8_t {
    Default,
    LongForm,
    Independent,
};

class AudioSession {
    WTF_MAKE_NONCOPYABLE(AudioSession);
public:
    WEBCORE_EXPORT static AudioSession& sharedSession();

    enum CategoryType {
        None,
        AmbientSound,
        SoloAmbientSound,
        MediaPlayback,
        RecordAudio,
        PlayAndRecord,
        AudioProcessing,
    };
    WEBCORE_EXPORT void setCategory(CategoryType);
    WEBCORE_EXPORT CategoryType category() const;

    void setCategoryOverride(CategoryType);
    CategoryType categoryOverride() const;

    RouteSharingPolicy routeSharingPolicy() const;
    String routingContextUID() const;

    float sampleRate() const;
    size_t bufferSize() const;
    size_t numberOfOutputChannels() const;

    bool tryToSetActive(bool);

    WEBCORE_EXPORT size_t preferredBufferSize() const;
    void setPreferredBufferSize(size_t);

    class MutedStateObserver {
    public:
        virtual ~MutedStateObserver() = default;

        virtual void hardwareMutedStateDidChange(AudioSession*) = 0;
    };

    void addMutedStateObserver(MutedStateObserver*);
    void removeMutedStateObserver(MutedStateObserver*);

    bool isMuted() const;
    void handleMutedStateChange();

    bool isActive() const { return m_active; }

private:
    friend class NeverDestroyed<AudioSession>;
    AudioSession();
    ~AudioSession();

    bool tryToSetActiveInternal(bool);

    std::unique_ptr<AudioSessionPrivate> m_private;
    HashSet<MutedStateObserver*> m_observers;
    bool m_active { false }; // Used only for testing.
};

}

namespace WTF {
template<> struct EnumTraits<WebCore::RouteSharingPolicy> {
    using values = EnumValues<
    WebCore::RouteSharingPolicy,
    WebCore::RouteSharingPolicy::Default,
    WebCore::RouteSharingPolicy::LongForm,
    WebCore::RouteSharingPolicy::Independent
    >;
};
}

#endif // USE(AUDIO_SESSION)
