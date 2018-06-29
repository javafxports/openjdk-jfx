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

#include "config.h"
#include "LibWebRTCProvider.h"

#if PLATFORM(COCOA)
#include "LibWebRTCProviderCocoa.h"
#endif

#if USE(LIBWEBRTC)
#include "LibWebRTCAudioModule.h"
#include "Logging.h"
#include "VideoToolBoxDecoderFactory.h"
#include "VideoToolBoxEncoderFactory.h"
#include <dlfcn.h>
#include <webrtc/api/peerconnectionfactoryproxy.h>
#include <webrtc/base/physicalsocketserver.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include <webrtc/pc/peerconnectionfactory.h>
#include <wtf/Function.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/darwin/WeakLinking.h>
#endif

namespace WebCore {

UniqueRef<LibWebRTCProvider> LibWebRTCProvider::create()
{
#if USE(LIBWEBRTC) && PLATFORM(COCOA)
    return makeUniqueRef<LibWebRTCProviderCocoa>();
#else
    return makeUniqueRef<LibWebRTCProvider>();
#endif
}

#if USE(LIBWEBRTC)
struct PeerConnectionFactoryAndThreads : public rtc::MessageHandler {
    std::unique_ptr<rtc::Thread> networkThread;
    std::unique_ptr<rtc::Thread> signalingThread;
    bool networkThreadWithSocketServer { false };
    std::unique_ptr<LibWebRTCAudioModule> audioDeviceModule;

private:
    void OnMessage(rtc::Message*);
};

static void initializePeerConnectionFactoryAndThreads(PeerConnectionFactoryAndThreads& factoryAndThreads)
{
    ASSERT(!factoryAndThreads.networkThread);

#if defined(NDEBUG)
#if !LOG_DISABLED || !RELEASE_LOG_DISABLED
    rtc::LogMessage::LogToDebug(LogWebRTC.state != WTFLogChannelOn ? rtc::LS_NONE : rtc::LS_INFO);
#else
    rtc::LogMessage::LogToDebug(rtc::LS_NONE);
#endif
#else
    rtc::LogMessage::LogToDebug(LogWebRTC.state != WTFLogChannelOn ? rtc::LS_WARNING : rtc::LS_INFO);
#endif

    factoryAndThreads.networkThread = factoryAndThreads.networkThreadWithSocketServer ? rtc::Thread::CreateWithSocketServer() : rtc::Thread::Create();
    factoryAndThreads.networkThread->SetName("WebKitWebRTCNetwork", nullptr);
    bool result = factoryAndThreads.networkThread->Start();
    ASSERT_UNUSED(result, result);

    factoryAndThreads.signalingThread = rtc::Thread::Create();
    factoryAndThreads.signalingThread->SetName("WebKitWebRTCSignaling", nullptr);

    result = factoryAndThreads.signalingThread->Start();
    ASSERT(result);

    factoryAndThreads.audioDeviceModule = std::make_unique<LibWebRTCAudioModule>();
}

static inline PeerConnectionFactoryAndThreads& staticFactoryAndThreads()
{
    static NeverDestroyed<PeerConnectionFactoryAndThreads> factoryAndThreads;
    return factoryAndThreads.get();
}

static inline PeerConnectionFactoryAndThreads& getStaticFactoryAndThreads(bool useNetworkThreadWithSocketServer)
{
    auto& factoryAndThreads = staticFactoryAndThreads();

    ASSERT(!factoryAndThreads.networkThread || factoryAndThreads.networkThreadWithSocketServer == useNetworkThreadWithSocketServer);

    if (!factoryAndThreads.networkThread) {
        factoryAndThreads.networkThreadWithSocketServer = useNetworkThreadWithSocketServer;
        initializePeerConnectionFactoryAndThreads(factoryAndThreads);
    }
    return factoryAndThreads;
}

struct ThreadMessageData : public rtc::MessageData {
    ThreadMessageData(Function<void()>&& callback)
        : callback(WTFMove(callback))
    { }
    Function<void()> callback;
};

void PeerConnectionFactoryAndThreads::OnMessage(rtc::Message* message)
{
    ASSERT(message->message_id == 1);
    auto* data = static_cast<ThreadMessageData*>(message->pdata);
    data->callback();
    delete data;
}

void LibWebRTCProvider::callOnWebRTCNetworkThread(Function<void()>&& callback)
{
    PeerConnectionFactoryAndThreads& threads = staticFactoryAndThreads();
    threads.networkThread->Post(RTC_FROM_HERE, &threads, 1, new ThreadMessageData(WTFMove(callback)));
}

void LibWebRTCProvider::callOnWebRTCSignalingThread(Function<void()>&& callback)
{
    PeerConnectionFactoryAndThreads& threads = staticFactoryAndThreads();
    threads.signalingThread->Post(RTC_FROM_HERE, &threads, 1, new ThreadMessageData(WTFMove(callback)));
}

webrtc::PeerConnectionFactoryInterface* LibWebRTCProvider::factory()
{
    if (m_factory)
        return m_factory.get();

    if (!webRTCAvailable())
        return nullptr;

    auto& factoryAndThreads = getStaticFactoryAndThreads(m_useNetworkThreadWithSocketServer);

    m_factory = createPeerConnectionFactory(factoryAndThreads.networkThread.get(), factoryAndThreads.networkThread.get(), factoryAndThreads.audioDeviceModule.get());

    return m_factory;
}

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> LibWebRTCProvider::createPeerConnectionFactory(rtc::Thread* networkThread, rtc::Thread* signalingThread, LibWebRTCAudioModule* audioModule)
{
    return webrtc::CreatePeerConnectionFactory(networkThread, networkThread, signalingThread, audioModule, createEncoderFactory().release(), createDecoderFactory().release());
}

void LibWebRTCProvider::setPeerConnectionFactory(rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>&& factory)
{
    m_factory = webrtc::PeerConnectionFactoryProxy::Create(getStaticFactoryAndThreads(m_useNetworkThreadWithSocketServer).signalingThread.get(), WTFMove(factory));
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface> LibWebRTCProvider::createPeerConnection(webrtc::PeerConnectionObserver& observer, webrtc::PeerConnectionInterface::RTCConfiguration&& configuration)
{
    // Default WK1 implementation.
    ASSERT(m_useNetworkThreadWithSocketServer);
    auto* factory = this->factory();
    if (!factory)
        return nullptr;

    return m_factory->CreatePeerConnection(configuration, nullptr, nullptr, &observer);
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface> LibWebRTCProvider::createPeerConnection(webrtc::PeerConnectionObserver& observer, rtc::NetworkManager& networkManager, rtc::PacketSocketFactory& packetSocketFactory, webrtc::PeerConnectionInterface::RTCConfiguration&& configuration)
{
    ASSERT(!m_useNetworkThreadWithSocketServer);

    auto& factoryAndThreads = getStaticFactoryAndThreads(m_useNetworkThreadWithSocketServer);

    std::unique_ptr<cricket::BasicPortAllocator> portAllocator;
    factoryAndThreads.signalingThread->Invoke<void>(RTC_FROM_HERE, [&]() {
        auto basicPortAllocator = std::make_unique<cricket::BasicPortAllocator>(&networkManager, &packetSocketFactory);
        if (!m_enableEnumeratingAllNetworkInterfaces)
            basicPortAllocator->set_flags(basicPortAllocator->flags() | cricket::PORTALLOCATOR_DISABLE_ADAPTER_ENUMERATION);
        portAllocator = WTFMove(basicPortAllocator);
    });

    auto* factory = this->factory();
    if (!factory)
        return nullptr;

    return m_factory->CreatePeerConnection(configuration, WTFMove(portAllocator), nullptr, &observer);
}

#endif // USE(LIBWEBRTC)

bool LibWebRTCProvider::webRTCAvailable()
{
#if USE(LIBWEBRTC)
    return !isNullFunctionPointer(rtc::LogMessage::LogToDebug);
#else
    return true;
#endif
}

} // namespace WebCore
