/*
 * Copyright (C) 2009-2017 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009, 2011 Google Inc. All Rights Reserved.
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

#include "config.h"
#include "WorkerScriptLoader.h"

#include "ContentSecurityPolicy.h"
#include "ResourceResponse.h"
#include "ScriptExecutionContext.h"
#include "ServiceWorker.h"
#include "TextResourceDecoder.h"
#include "WorkerGlobalScope.h"
#include "WorkerScriptLoaderClient.h"
#include "WorkerThreadableLoader.h"
#include <wtf/Ref.h>

namespace WebCore {

WorkerScriptLoader::WorkerScriptLoader() = default;

WorkerScriptLoader::~WorkerScriptLoader() = default;

void WorkerScriptLoader::loadSynchronously(ScriptExecutionContext* scriptExecutionContext, const URL& url, FetchOptions::Mode mode, FetchOptions::Cache cachePolicy, ContentSecurityPolicyEnforcement contentSecurityPolicyEnforcement, const String& initiatorIdentifier)
{
    ASSERT(scriptExecutionContext);
    auto& workerGlobalScope = downcast<WorkerGlobalScope>(*scriptExecutionContext);

    m_url = url;

    std::unique_ptr<ResourceRequest> request(createResourceRequest(initiatorIdentifier));
    if (!request)
        return;

    ASSERT_WITH_SECURITY_IMPLICATION(is<WorkerGlobalScope>(scriptExecutionContext));

    // Only used for importScripts that prescribes NoCors mode.
    ASSERT(mode == FetchOptions::Mode::NoCors);

    ThreadableLoaderOptions options;
    options.credentials = FetchOptions::Credentials::Include;
    options.mode = mode;
    options.cache = cachePolicy;
    options.sendLoadCallbacks = SendCallbacks;
    options.contentSecurityPolicyEnforcement = contentSecurityPolicyEnforcement;
#if ENABLE(SERVICE_WORKER)
    options.serviceWorkersMode = workerGlobalScope.isServiceWorkerGlobalScope() ? ServiceWorkersMode::None : ServiceWorkersMode::All;
    if (auto* activeServiceWorker = workerGlobalScope.activeServiceWorker())
        options.serviceWorkerRegistrationIdentifier = activeServiceWorker->registrationIdentifier();
#endif
    WorkerThreadableLoader::loadResourceSynchronously(workerGlobalScope, WTFMove(*request), *this, options);
}

void WorkerScriptLoader::loadAsynchronously(ScriptExecutionContext& scriptExecutionContext, ResourceRequest&& scriptRequest, FetchOptions::Mode mode, FetchOptions::Cache cachePolicy, FetchOptions::Redirect redirectPolicy, ContentSecurityPolicyEnforcement contentSecurityPolicyEnforcement, WorkerScriptLoaderClient& client)
{
    m_client = &client;
    m_url = scriptRequest.url();

    ASSERT(scriptRequest.httpMethod() == "GET");

    auto request = std::make_unique<ResourceRequest>(WTFMove(scriptRequest));
    if (!request)
        return;

    // Only used for loading worker scripts in classic mode.
    // FIXME: We should add an option to set credential mode.
    ASSERT(mode == FetchOptions::Mode::SameOrigin);

    ThreadableLoaderOptions options;
    options.credentials = FetchOptions::Credentials::SameOrigin;
    options.mode = mode;
    options.cache = cachePolicy;
    options.redirect = redirectPolicy;
    options.sendLoadCallbacks = SendCallbacks;
    options.contentSecurityPolicyEnforcement = contentSecurityPolicyEnforcement;
#if ENABLE(SERVICE_WORKER)
    options.serviceWorkersMode = m_client->isServiceWorkerClient() ? ServiceWorkersMode::None : ServiceWorkersMode::All;
    if (auto* activeServiceWorker = scriptExecutionContext.activeServiceWorker())
        options.serviceWorkerRegistrationIdentifier = activeServiceWorker->registrationIdentifier();
#endif
    // During create, callbacks may happen which remove the last reference to this object.
    Ref<WorkerScriptLoader> protectedThis(*this);
    m_threadableLoader = ThreadableLoader::create(scriptExecutionContext, *this, WTFMove(*request), options);
}

const URL& WorkerScriptLoader::responseURL() const
{
    ASSERT(!failed());
    return m_responseURL;
}

std::unique_ptr<ResourceRequest> WorkerScriptLoader::createResourceRequest(const String& initiatorIdentifier)
{
    auto request = std::make_unique<ResourceRequest>(m_url);
    request->setHTTPMethod(ASCIILiteral("GET"));
    request->setInitiatorIdentifier(initiatorIdentifier);
    return request;
}

void WorkerScriptLoader::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    if (response.httpStatusCode() / 100 != 2 && response.httpStatusCode()) {
        m_failed = true;
        return;
    }

    if (!isScriptAllowedByNosniff(response)) {
        m_failed = true;
        return;
    }

    m_responseURL = response.url();
    m_responseMIMEType = response.mimeType();
    m_responseEncoding = response.textEncodingName();
    m_contentSecurityPolicy = ContentSecurityPolicyResponseHeaders { response };
    if (m_client)
        m_client->didReceiveResponse(identifier, response);
}

void WorkerScriptLoader::didReceiveData(const char* data, int len)
{
    if (m_failed)
        return;

    if (!m_decoder) {
        if (!m_responseEncoding.isEmpty())
            m_decoder = TextResourceDecoder::create(ASCIILiteral("text/javascript"), m_responseEncoding);
        else
            m_decoder = TextResourceDecoder::create(ASCIILiteral("text/javascript"), "UTF-8");
    }

    if (!len)
        return;

    if (len == -1)
        len = strlen(data);

    m_script.append(m_decoder->decode(data, len));
}

void WorkerScriptLoader::didFinishLoading(unsigned long identifier)
{
    if (m_failed) {
        notifyError();
        return;
    }

    if (m_decoder)
        m_script.append(m_decoder->flush());

    m_identifier = identifier;
    notifyFinished();
}

void WorkerScriptLoader::didFail(const ResourceError& error)
{
    m_error = error;
    notifyError();
}

void WorkerScriptLoader::notifyError()
{
    m_failed = true;
    if (m_error.isNull())
        m_error = ResourceError { errorDomainWebKitInternal, 0, url(), "Failed to load script", ResourceError::Type::General };
    notifyFinished();
}

String WorkerScriptLoader::script()
{
    return m_script.toString();
}

void WorkerScriptLoader::notifyFinished()
{
    m_threadableLoader = nullptr;
    if (!m_client || m_finishing)
        return;

    m_finishing = true;
    m_client->notifyFinished();
}

void WorkerScriptLoader::cancel()
{
    if (!m_threadableLoader)
        return;

    m_client = nullptr;
    m_threadableLoader->cancel();
    m_threadableLoader = nullptr;
}

} // namespace WebCore
