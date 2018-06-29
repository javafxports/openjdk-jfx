/*
 * Copyright (C) 2016-2017 Apple Inc. All rights reserved.
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
#include "CommonVM.h"

#include "DeprecatedGlobalSettings.h"
#include "Frame.h"
#include "ScriptController.h"
#include "WebCoreJSClientData.h"
#include <JavaScriptCore/HeapInlines.h>
#include <JavaScriptCore/MachineStackMarker.h>
#include <JavaScriptCore/VM.h>
#include <wtf/MainThread.h>
#include <wtf/text/AtomicString.h>

#if PLATFORM(IOS)
#include "WebCoreThreadInternal.h"
#endif


namespace WebCore {
using namespace JSC;

VM* g_commonVMOrNull;

VM& commonVMSlow()
{
    ASSERT(isMainThread());
    ASSERT(!g_commonVMOrNull);

    ScriptController::initializeThreading();
    g_commonVMOrNull = &VM::createLeaked(LargeHeap).leakRef();
    g_commonVMOrNull->heap.acquireAccess(); // At any time, we may do things that affect the GC.
#if PLATFORM(IOS)
    g_commonVMOrNull->setRunLoop(WebThreadRunLoop());
    g_commonVMOrNull->heap.machineThreads().addCurrentThread();
#endif

    g_commonVMOrNull->setGlobalConstRedeclarationShouldThrow(DeprecatedGlobalSettings::globalConstRedeclarationShouldThrow());

    JSVMClientData::initNormalWorld(g_commonVMOrNull);

    return *g_commonVMOrNull;
}

Frame* lexicalFrameFromCommonVM()
{
    if (auto* topCallFrame = commonVM().topCallFrame) {
        if (auto* globalObject = JSC::jsCast<JSDOMGlobalObject*>(topCallFrame->lexicalGlobalObject())) {
            if (auto* window = jsDynamicDowncast<JSDOMWindow*>(commonVM(), globalObject)) {
                if (auto* frame = window->wrapped().frame())
                    return frame;
            }
        }
    }

    return nullptr;
}

void addImpureProperty(const AtomicString& propertyName)
{
    commonVM().addImpureProperty(propertyName);
}

} // namespace WebCore

