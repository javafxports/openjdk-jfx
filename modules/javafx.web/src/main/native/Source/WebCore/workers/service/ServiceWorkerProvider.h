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

#pragma once

#if ENABLE(SERVICE_WORKER)

namespace PAL {
class SessionID;
}

namespace WebCore {

class SWClientConnection;
class ServiceWorkerJob;

class WEBCORE_EXPORT ServiceWorkerProvider {
public:
    virtual ~ServiceWorkerProvider() = default;

    WEBCORE_EXPORT static ServiceWorkerProvider& singleton();
    WEBCORE_EXPORT static void setSharedProvider(ServiceWorkerProvider&);

    bool mayHaveServiceWorkerRegisteredForOrigin(PAL::SessionID, const WebCore::SecurityOrigin&);
    virtual SWClientConnection* existingServiceWorkerConnectionForSession(PAL::SessionID) = 0;
    virtual SWClientConnection& serviceWorkerConnectionForSession(PAL::SessionID) = 0;

    WEBCORE_EXPORT void registerServiceWorkerClients(PAL::SessionID);

    void setHasRegisteredServiceWorkers(bool value) { m_hasRegisteredServiceWorkers = value; }

private:
    bool m_hasRegisteredServiceWorkers { true };
};

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
