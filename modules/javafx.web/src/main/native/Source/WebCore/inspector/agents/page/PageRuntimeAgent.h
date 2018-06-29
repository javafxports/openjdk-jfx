/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#pragma once

#include "InspectorWebAgentBase.h"
#include <JavaScriptCore/InspectorFrontendDispatchers.h>
#include <JavaScriptCore/InspectorRuntimeAgent.h>

namespace JSC {
class ExecState;
}

namespace WebCore {

class InspectorPageAgent;
class Frame;
class Page;
class SecurityOrigin;
typedef String ErrorString;

class PageRuntimeAgent final : public Inspector::InspectorRuntimeAgent {
    WTF_MAKE_NONCOPYABLE(PageRuntimeAgent);
    WTF_MAKE_FAST_ALLOCATED;
public:
    PageRuntimeAgent(PageAgentContext&, InspectorPageAgent*);
    virtual ~PageRuntimeAgent() = default;

    void didCreateFrontendAndBackend(Inspector::FrontendRouter*, Inspector::BackendDispatcher*) override;
    void willDestroyFrontendAndBackend(Inspector::DisconnectReason) override;
    void enable(ErrorString&) override;
    void disable(ErrorString&) override;

    // InspectorInstrumentation
    void didCreateMainWorldContext(Frame&);

private:
    Inspector::InjectedScript injectedScriptForEval(ErrorString&, const int* executionContextId) override;
    void muteConsole() override;
    void unmuteConsole() override;
    void reportExecutionContextCreation();
    void notifyContextCreated(const String& frameId, JSC::ExecState*, SecurityOrigin*, bool isPageContext);

    std::unique_ptr<Inspector::RuntimeFrontendDispatcher> m_frontendDispatcher;
    RefPtr<Inspector::RuntimeBackendDispatcher> m_backendDispatcher;
    InspectorPageAgent* m_pageAgent;

    Page& m_inspectedPage;

    bool m_mainWorldContextCreated { false };
};

} // namespace WebCore
