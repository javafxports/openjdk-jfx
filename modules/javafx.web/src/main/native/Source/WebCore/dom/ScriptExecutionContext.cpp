/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Google Inc. All Rights Reserved.
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
 *
 */

#include "config.h"
#include "ScriptExecutionContext.h"

#include "CachedScript.h"
#include "CommonVM.h"
#include "DOMTimer.h"
#include "DOMWindow.h"
#include "DatabaseContext.h"
#include "Document.h"
#include "ErrorEvent.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMWindow.h"
#include "MessagePort.h"
#include "Navigator.h"
#include "PublicURLManager.h"
#include "RejectedPromiseTracker.h"
#include "ResourceRequest.h"
#include "SWClientConnection.h"
#include "SWContextManager.h"
#include "SchemeRegistry.h"
#include "ScriptDisallowedScope.h"
#include "ScriptState.h"
#include "ServiceWorker.h"
#include "ServiceWorkerGlobalScope.h"
#include "ServiceWorkerProvider.h"
#include "Settings.h"
#include "WorkerGlobalScope.h"
#include "WorkerNavigator.h"
#include "WorkerThread.h"
#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/Exception.h>
#include <JavaScriptCore/JSPromise.h>
#include <JavaScriptCore/ScriptCallStack.h>
#include <JavaScriptCore/StrongInlines.h>
#include <wtf/MainThread.h>
#include <wtf/Ref.h>

namespace WebCore {
using namespace Inspector;

struct ScriptExecutionContext::PendingException {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PendingException(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL, RefPtr<ScriptCallStack>&& callStack)
        : m_errorMessage(errorMessage)
        , m_lineNumber(lineNumber)
        , m_columnNumber(columnNumber)
        , m_sourceURL(sourceURL)
        , m_callStack(WTFMove(callStack))
    {
    }
    String m_errorMessage;
    int m_lineNumber;
    int m_columnNumber;
    String m_sourceURL;
    RefPtr<ScriptCallStack> m_callStack;
};

ScriptExecutionContext::ScriptExecutionContext()
{
}

#if ASSERT_DISABLED

inline void ScriptExecutionContext::checkConsistency() const
{
}

#else

void ScriptExecutionContext::checkConsistency() const
{
    for (auto* messagePort : m_messagePorts)
        ASSERT(messagePort->scriptExecutionContext() == this);

    for (auto* destructionObserver : m_destructionObservers)
        ASSERT(destructionObserver->scriptExecutionContext() == this);

    for (auto* activeDOMObject : m_activeDOMObjects) {
        ASSERT(activeDOMObject->scriptExecutionContext() == this);
        activeDOMObject->assertSuspendIfNeededWasCalled();
    }
}

#endif

ScriptExecutionContext::~ScriptExecutionContext()
{
    checkConsistency();

#if !ASSERT_DISABLED
    m_inScriptExecutionContextDestructor = true;
#endif

#if ENABLE(SERVICE_WORKER)
    setActiveServiceWorker(nullptr);
#endif

    while (auto* destructionObserver = m_destructionObservers.takeAny())
        destructionObserver->contextDestroyed();

#if !ASSERT_DISABLED
    m_inScriptExecutionContextDestructor = false;
#endif
}

void ScriptExecutionContext::processMessageWithMessagePortsSoon()
{
    if (m_willprocessMessageWithMessagePortsSoon)
        return;

    m_willprocessMessageWithMessagePortsSoon = true;
    postTask([] (ScriptExecutionContext& context) {
        context.dispatchMessagePortEvents();
    });
}

void ScriptExecutionContext::dispatchMessagePortEvents()
{
    checkConsistency();

    Ref<ScriptExecutionContext> protectedThis(*this);
    ASSERT(m_willprocessMessageWithMessagePortsSoon);
    m_willprocessMessageWithMessagePortsSoon = false;

    // Make a frozen copy of the ports so we can iterate while new ones might be added or destroyed.
    for (auto* messagePort : copyToVector(m_messagePorts)) {
        // The port may be destroyed, and another one created at the same address,
        // but this is harmless. The worst that can happen as a result is that
        // dispatchMessages() will be called needlessly.
        if (m_messagePorts.contains(messagePort) && messagePort->started())
            messagePort->dispatchMessages();
    }
}

void ScriptExecutionContext::createdMessagePort(MessagePort& messagePort)
{
    ASSERT((is<Document>(*this) && isMainThread())
        || (is<WorkerGlobalScope>(*this) && downcast<WorkerGlobalScope>(*this).thread().thread() == &Thread::current()));

    m_messagePorts.add(&messagePort);
}

void ScriptExecutionContext::destroyedMessagePort(MessagePort& messagePort)
{
    ASSERT((is<Document>(*this) && isMainThread())
        || (is<WorkerGlobalScope>(*this) && downcast<WorkerGlobalScope>(*this).thread().thread() == &Thread::current()));

    m_messagePorts.remove(&messagePort);
}

void ScriptExecutionContext::didLoadResourceSynchronously()
{
}

bool ScriptExecutionContext::canSuspendActiveDOMObjectsForDocumentSuspension(Vector<ActiveDOMObject*>* unsuspendableObjects)
{
    checkConsistency();

    bool canSuspend = true;

    m_activeDOMObjectAdditionForbidden = true;
#if !ASSERT_DISABLED || ENABLE(SECURITY_ASSERTIONS)
    m_activeDOMObjectRemovalForbidden = true;
#endif

    // We assume that m_activeDOMObjects will not change during iteration: canSuspend
    // functions should not add new active DOM objects, nor execute arbitrary JavaScript.
    // An ASSERT_WITH_SECURITY_IMPLICATION or RELEASE_ASSERT will fire if this happens, but it's important to code
    // canSuspend functions so it will not happen!
    ScriptDisallowedScope::InMainThread scriptDisallowedScope;
    for (auto* activeDOMObject : m_activeDOMObjects) {
        if (!activeDOMObject->canSuspendForDocumentSuspension()) {
            canSuspend = false;
            if (unsuspendableObjects)
                unsuspendableObjects->append(activeDOMObject);
            else
                break;
        }
    }

    m_activeDOMObjectAdditionForbidden = false;
#if !ASSERT_DISABLED || ENABLE(SECURITY_ASSERTIONS)
    m_activeDOMObjectRemovalForbidden = false;
#endif

    return canSuspend;
}

void ScriptExecutionContext::suspendActiveDOMObjects(ActiveDOMObject::ReasonForSuspension why)
{
    checkConsistency();

    if (m_activeDOMObjectsAreSuspended) {
        // A page may subsequently suspend DOM objects, say as part of entering the page cache, after the embedding
        // client requested the page be suspended. We ignore such requests so long as the embedding client requested
        // the suspension first. See <rdar://problem/13754896> for more details.
        ASSERT(m_reasonForSuspendingActiveDOMObjects == ActiveDOMObject::PageWillBeSuspended);
        return;
    }

    m_activeDOMObjectsAreSuspended = true;

    m_activeDOMObjectAdditionForbidden = true;
#if !ASSERT_DISABLED || ENABLE(SECURITY_ASSERTIONS)
    m_activeDOMObjectRemovalForbidden = true;
#endif

    // We assume that m_activeDOMObjects will not change during iteration: suspend
    // functions should not add new active DOM objects, nor execute arbitrary JavaScript.
    // An ASSERT_WITH_SECURITY_IMPLICATION or RELEASE_ASSERT will fire if this happens, but it's important to code
    // suspend functions so it will not happen!
    ScriptDisallowedScope::InMainThread scriptDisallowedScope;
    for (auto* activeDOMObject : m_activeDOMObjects)
        activeDOMObject->suspend(why);

    m_activeDOMObjectAdditionForbidden = false;
#if !ASSERT_DISABLED || ENABLE(SECURITY_ASSERTIONS)
    m_activeDOMObjectRemovalForbidden = false;
#endif

    m_reasonForSuspendingActiveDOMObjects = why;
}

void ScriptExecutionContext::resumeActiveDOMObjects(ActiveDOMObject::ReasonForSuspension why)
{
    checkConsistency();

    if (m_reasonForSuspendingActiveDOMObjects != why)
        return;
    m_activeDOMObjectsAreSuspended = false;

    m_activeDOMObjectAdditionForbidden = true;
#if !ASSERT_DISABLED || ENABLE(SECURITY_ASSERTIONS)
    m_activeDOMObjectRemovalForbidden = true;
#endif

    // We assume that m_activeDOMObjects will not change during iteration: resume
    // functions should not add new active DOM objects, nor execute arbitrary JavaScript.
    // An ASSERT_WITH_SECURITY_IMPLICATION or RELEASE_ASSERT will fire if this happens, but it's important to code
    // resume functions so it will not happen!
    ScriptDisallowedScope::InMainThread scriptDisallowedScope;
    for (auto* activeDOMObject : m_activeDOMObjects)
        activeDOMObject->resume();

    m_activeDOMObjectAdditionForbidden = false;
#if !ASSERT_DISABLED || ENABLE(SECURITY_ASSERTIONS)
    m_activeDOMObjectRemovalForbidden = false;
#endif
}

void ScriptExecutionContext::stopActiveDOMObjects()
{
    checkConsistency();

    if (m_activeDOMObjectsAreStopped)
        return;
    m_activeDOMObjectsAreStopped = true;

    // Make a frozen copy of the objects so we can iterate while new ones might be destroyed.
    auto possibleActiveDOMObjects = copyToVector(m_activeDOMObjects);

    m_activeDOMObjectAdditionForbidden = true;

    // We assume that new objects will not be added to m_activeDOMObjects during iteration:
    // stop functions should not add new active DOM objects, nor execute arbitrary JavaScript.
    // An ASSERT_WITH_SECURITY_IMPLICATION or RELEASE_ASSERT will fire if this happens, but it's important to code stop functions
    // so it will not happen!
    ScriptDisallowedScope scriptDisallowedScope;
    for (auto* activeDOMObject : possibleActiveDOMObjects) {
        // Check if this object was deleted already. If so, just skip it.
        // Calling contains on a possibly-already-deleted object is OK because we guarantee
        // no new object can be added, so even if a new object ends up allocated with the
        // same address, that will be *after* this function exits.
        if (!m_activeDOMObjects.contains(activeDOMObject))
            continue;
        activeDOMObject->stop();
    }

    m_activeDOMObjectAdditionForbidden = false;
}

void ScriptExecutionContext::suspendActiveDOMObjectIfNeeded(ActiveDOMObject& activeDOMObject)
{
    ASSERT(m_activeDOMObjects.contains(&activeDOMObject));
    if (m_activeDOMObjectsAreSuspended)
        activeDOMObject.suspend(m_reasonForSuspendingActiveDOMObjects);
    if (m_activeDOMObjectsAreStopped)
        activeDOMObject.stop();
}

void ScriptExecutionContext::didCreateActiveDOMObject(ActiveDOMObject& activeDOMObject)
{
    // The m_activeDOMObjectAdditionForbidden check is a RELEASE_ASSERT because of the
    // consequences of having an ActiveDOMObject that is not correctly reflected in the set.
    // If we do have one of those, it can possibly be a security vulnerability. So we'd
    // rather have a crash than continue running with the set possibly compromised.
    ASSERT(!m_inScriptExecutionContextDestructor);
    RELEASE_ASSERT(!m_activeDOMObjectAdditionForbidden);
    m_activeDOMObjects.add(&activeDOMObject);
}

void ScriptExecutionContext::willDestroyActiveDOMObject(ActiveDOMObject& activeDOMObject)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!m_activeDOMObjectRemovalForbidden);
    m_activeDOMObjects.remove(&activeDOMObject);
}

void ScriptExecutionContext::didCreateDestructionObserver(ContextDestructionObserver& observer)
{
    ASSERT(!m_inScriptExecutionContextDestructor);
    m_destructionObservers.add(&observer);
}

void ScriptExecutionContext::willDestroyDestructionObserver(ContextDestructionObserver& observer)
{
    m_destructionObservers.remove(&observer);
}

bool ScriptExecutionContext::sanitizeScriptError(String& errorMessage, int& lineNumber, int& columnNumber, String& sourceURL, JSC::Strong<JSC::Unknown>& error, CachedScript* cachedScript)
{
    ASSERT(securityOrigin());
    if (cachedScript) {
        ASSERT(cachedScript->origin());
        ASSERT(securityOrigin()->toString() == cachedScript->origin()->toString());
        if (cachedScript->isCORSSameOrigin())
            return false;
    } else if (securityOrigin()->canRequest(completeURL(sourceURL)))
        return false;

    errorMessage = ASCIILiteral { "Script error." };
    sourceURL = { };
    lineNumber = 0;
    columnNumber = 0;
    error = { };
    return true;
}

void ScriptExecutionContext::reportException(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL, JSC::Exception* exception, RefPtr<ScriptCallStack>&& callStack, CachedScript* cachedScript)
{
    if (m_inDispatchErrorEvent) {
        if (!m_pendingExceptions)
            m_pendingExceptions = std::make_unique<Vector<std::unique_ptr<PendingException>>>();
        m_pendingExceptions->append(std::make_unique<PendingException>(errorMessage, lineNumber, columnNumber, sourceURL, WTFMove(callStack)));
        return;
    }

    // First report the original exception and only then all the nested ones.
    if (!dispatchErrorEvent(errorMessage, lineNumber, columnNumber, sourceURL, exception, cachedScript))
        logExceptionToConsole(errorMessage, sourceURL, lineNumber, columnNumber, callStack.copyRef());

    if (!m_pendingExceptions)
        return;

    auto pendingExceptions = WTFMove(m_pendingExceptions);
    for (auto& exception : *pendingExceptions)
        logExceptionToConsole(exception->m_errorMessage, exception->m_sourceURL, exception->m_lineNumber, exception->m_columnNumber, WTFMove(exception->m_callStack));
}

void ScriptExecutionContext::reportUnhandledPromiseRejection(JSC::ExecState& state, JSC::JSPromise& promise, RefPtr<Inspector::ScriptCallStack>&& callStack)
{
    JSC::VM& vm = state.vm();
    auto scope = DECLARE_CATCH_SCOPE(vm);

    int lineNumber = 0;
    int columnNumber = 0;
    String sourceURL;

    JSC::JSValue result = promise.result(vm);
    String resultMessage = retrieveErrorMessage(state, vm, result, scope);
    String errorMessage = makeString("Unhandled Promise Rejection: ", resultMessage);
    if (callStack) {
        if (const ScriptCallFrame* callFrame = callStack->firstNonNativeCallFrame()) {
            lineNumber = callFrame->lineNumber();
            columnNumber = callFrame->columnNumber();
            sourceURL = callFrame->sourceURL();
        }
    }

    logExceptionToConsole(errorMessage, sourceURL, lineNumber, columnNumber, WTFMove(callStack));
}

void ScriptExecutionContext::addConsoleMessage(MessageSource source, MessageLevel level, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, JSC::ExecState* state, unsigned long requestIdentifier)
{
    addMessage(source, level, message, sourceURL, lineNumber, columnNumber, 0, state, requestIdentifier);
}

bool ScriptExecutionContext::dispatchErrorEvent(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL, JSC::Exception* exception, CachedScript* cachedScript)
{
    EventTarget* target = errorEventTarget();
    if (!target)
        return false;

    String message = errorMessage;
    int line = lineNumber;
    int column = columnNumber;
    String sourceName = sourceURL;
    JSC::Strong<JSC::Unknown> error = exception && exception->value() ? JSC::Strong<JSC::Unknown>(vm(), exception->value()) : JSC::Strong<JSC::Unknown>();
    sanitizeScriptError(message, line, column, sourceName, error, cachedScript);

    ASSERT(!m_inDispatchErrorEvent);
    m_inDispatchErrorEvent = true;
    Ref<ErrorEvent> errorEvent = ErrorEvent::create(message, sourceName, line, column, error);
    target->dispatchEvent(errorEvent);
    m_inDispatchErrorEvent = false;
    return errorEvent->defaultPrevented();
}

int ScriptExecutionContext::circularSequentialID()
{
    ++m_circularSequentialID;
    if (m_circularSequentialID <= 0)
        m_circularSequentialID = 1;
    return m_circularSequentialID;
}

PublicURLManager& ScriptExecutionContext::publicURLManager()
{
    if (!m_publicURLManager)
        m_publicURLManager = PublicURLManager::create(this);
    return *m_publicURLManager;
}

void ScriptExecutionContext::adjustMinimumDOMTimerInterval(Seconds oldMinimumTimerInterval)
{
    if (minimumDOMTimerInterval() != oldMinimumTimerInterval) {
        for (auto& timer : m_timeouts.values())
            timer->updateTimerIntervalIfNecessary();
    }
}

Seconds ScriptExecutionContext::minimumDOMTimerInterval() const
{
    // The default implementation returns the DOMTimer's default
    // minimum timer interval. FIXME: to make it work with dedicated
    // workers, we will have to override it in the appropriate
    // subclass, and provide a way to enumerate a Document's dedicated
    // workers so we can update them all.
    return DOMTimer::defaultMinimumInterval();
}

void ScriptExecutionContext::didChangeTimerAlignmentInterval()
{
    for (auto& timer : m_timeouts.values())
        timer->didChangeAlignmentInterval();
}

Seconds ScriptExecutionContext::domTimerAlignmentInterval(bool) const
{
    return DOMTimer::defaultAlignmentInterval();
}

JSC::VM& ScriptExecutionContext::vm()
{
    if (is<Document>(*this))
        return commonVM();

    return downcast<WorkerGlobalScope>(*this).script()->vm();
}

RejectedPromiseTracker& ScriptExecutionContext::ensureRejectedPromiseTrackerSlow()
{
    // ScriptExecutionContext::vm() in Worker is only available after WorkerGlobalScope initialization is done.
    // When initializing ScriptExecutionContext, vm() is not ready.

    ASSERT(!m_rejectedPromiseTracker);
    m_rejectedPromiseTracker = std::make_unique<RejectedPromiseTracker>(*this, vm());
    return *m_rejectedPromiseTracker.get();
}

void ScriptExecutionContext::setDatabaseContext(DatabaseContext* databaseContext)
{
    m_databaseContext = databaseContext;
}

bool ScriptExecutionContext::hasPendingActivity() const
{
    checkConsistency();

    for (auto* activeDOMObject : m_activeDOMObjects) {
        if (activeDOMObject->hasPendingActivity())
            return true;
    }

    return false;
}

JSC::ExecState* ScriptExecutionContext::execState()
{
    if (is<Document>(*this)) {
        Document& document = downcast<Document>(*this);
        return execStateFromPage(mainThreadNormalWorld(), document.page());
    }

    WorkerGlobalScope* workerGlobalScope = static_cast<WorkerGlobalScope*>(this);
    return execStateFromWorkerGlobalScope(workerGlobalScope);
}

String ScriptExecutionContext::domainForCachePartition() const
{
    return m_domainForCachePartition.isNull() ? topOrigin().domainForCachePartition() : m_domainForCachePartition;
}

#if ENABLE(SERVICE_WORKER)

bool ScriptExecutionContext::hasServiceWorkerScheme()
{
    ASSERT(securityOrigin());
    return SchemeRegistry::isServiceWorkerContainerCustomScheme(securityOrigin()->protocol());
}

ServiceWorker* ScriptExecutionContext::activeServiceWorker() const
{
    return m_activeServiceWorker.get();
}

void ScriptExecutionContext::setActiveServiceWorker(RefPtr<ServiceWorker>&& serviceWorker)
{
    m_activeServiceWorker = WTFMove(serviceWorker);
}

void ScriptExecutionContext::registerServiceWorker(ServiceWorker& serviceWorker)
{
    auto addResult = m_serviceWorkers.add(serviceWorker.identifier(), &serviceWorker);
    ASSERT_UNUSED(addResult, addResult.isNewEntry);
}

void ScriptExecutionContext::unregisterServiceWorker(ServiceWorker& serviceWorker)
{
    m_serviceWorkers.remove(serviceWorker.identifier());
}

ServiceWorkerContainer* ScriptExecutionContext::serviceWorkerContainer()
{
    NavigatorBase* navigator = nullptr;
    if (is<Document>(*this)) {
        if (auto* window = downcast<Document>(*this).domWindow())
            navigator = window->optionalNavigator();
    } else
        navigator = downcast<WorkerGlobalScope>(*this).optionalNavigator();

    return navigator ? &navigator->serviceWorker() : nullptr;
}

bool ScriptExecutionContext::postTaskTo(const DocumentOrWorkerIdentifier& contextIdentifier, WTF::Function<void(ScriptExecutionContext&)>&& task)
{
    ASSERT(isMainThread());

    bool wasPosted = false;
    switchOn(contextIdentifier, [&] (DocumentIdentifier identifier) {
        auto* document = Document::allDocumentsMap().get(identifier);
        if (!document)
            return;
        document->postTask([task = WTFMove(task)](auto& scope) {
            task(scope);
        });
        wasPosted= true;
    }, [&](ServiceWorkerIdentifier identifier) {
        wasPosted = SWContextManager::singleton().postTaskToServiceWorker(identifier, [task = WTFMove(task)](auto& scope) {
            task(scope);
        });
    });
    return wasPosted;
}
#endif

} // namespace WebCore
