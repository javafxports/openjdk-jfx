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
#include "SWServerToContextConnection.h"

#if ENABLE(SERVICE_WORKER)

#include "SWServer.h"
#include "SWServerWorker.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

static SWServerToContextConnectionIdentifier generateServerToContextConnectionIdentifier()
{
    return generateObjectIdentifier<SWServerToContextConnectionIdentifierType>();
}

static HashMap<SWServerToContextConnectionIdentifier, SWServerToContextConnection*>& allConnections()
{
    static NeverDestroyed<HashMap<SWServerToContextConnectionIdentifier, SWServerToContextConnection*>> connections;
    return connections;
}

SWServerToContextConnection::SWServerToContextConnection()
    : m_identifier(generateServerToContextConnectionIdentifier())
{
    auto result = allConnections().add(m_identifier, this);
    ASSERT_UNUSED(result, result.isNewEntry);
}

SWServerToContextConnection::~SWServerToContextConnection()
{
    auto result = allConnections().remove(m_identifier);
    ASSERT_UNUSED(result, result);
}

SWServerToContextConnection* SWServerToContextConnection::connectionForIdentifier(SWServerToContextConnectionIdentifier identifier)
{
    return allConnections().get(identifier);
}

SWServerToContextConnection* SWServerToContextConnection::globalServerToContextConnection()
{
    if (allConnections().isEmpty())
        return nullptr;

    ASSERT(allConnections().size() == 1);
    return allConnections().begin()->value;
}

void SWServerToContextConnection::scriptContextFailedToStart(const std::optional<ServiceWorkerJobDataIdentifier>& jobDataIdentifier, ServiceWorkerIdentifier serviceWorkerIdentifier, const String& message)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        worker->scriptContextFailedToStart(jobDataIdentifier, message);
}

void SWServerToContextConnection::scriptContextStarted(const std::optional<ServiceWorkerJobDataIdentifier>& jobDataIdentifier, ServiceWorkerIdentifier serviceWorkerIdentifier)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        worker->scriptContextStarted(jobDataIdentifier);
}

void SWServerToContextConnection::didFinishInstall(const std::optional<ServiceWorkerJobDataIdentifier>& jobDataIdentifier, ServiceWorkerIdentifier serviceWorkerIdentifier, bool wasSuccessful)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        worker->didFinishInstall(jobDataIdentifier, wasSuccessful);
}

void SWServerToContextConnection::didFinishActivation(ServiceWorkerIdentifier serviceWorkerIdentifier)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        worker->didFinishActivation();
}

void SWServerToContextConnection::setServiceWorkerHasPendingEvents(ServiceWorkerIdentifier serviceWorkerIdentifier, bool hasPendingEvents)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        worker->setHasPendingEvents(hasPendingEvents);
}

void SWServerToContextConnection::workerTerminated(ServiceWorkerIdentifier serviceWorkerIdentifier)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        worker->contextTerminated();
}

void SWServerToContextConnection::findClientByIdentifier(uint64_t requestIdentifier, ServiceWorkerIdentifier serviceWorkerIdentifier, ServiceWorkerClientIdentifier clientId)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        globalServerToContextConnection()->findClientByIdentifierCompleted(requestIdentifier, worker->findClientByIdentifier(clientId), false);
}

void SWServerToContextConnection::matchAll(uint64_t requestIdentifier, ServiceWorkerIdentifier serviceWorkerIdentifier, const ServiceWorkerClientQueryOptions& options)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier)) {
        worker->matchAll(options, [requestIdentifier] (auto&& data) {
            globalServerToContextConnection()->matchAllCompleted(requestIdentifier, data);
        });
    }
}

void SWServerToContextConnection::claim(uint64_t requestIdentifier, ServiceWorkerIdentifier serviceWorkerIdentifier)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier)) {
        worker->claim();
        globalServerToContextConnection()->claimCompleted(requestIdentifier);
    }
}

void SWServerToContextConnection::skipWaiting(ServiceWorkerIdentifier serviceWorkerIdentifier, uint64_t callbackID)
{
    if (auto* worker = SWServerWorker::existingWorkerForIdentifier(serviceWorkerIdentifier))
        worker->skipWaiting();

    didFinishSkipWaiting(callbackID);
}

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
