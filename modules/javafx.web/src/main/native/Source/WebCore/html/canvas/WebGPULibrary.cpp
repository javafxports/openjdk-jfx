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
#include "WebGPULibrary.h"

#if ENABLE(WEBGPU)

#include "GPULibrary.h"
#include "WebGPUFunction.h"
#include "WebGPURenderingContext.h"

namespace WebCore {

Ref<WebGPULibrary> WebGPULibrary::create(WebGPURenderingContext* context, const String& sourceCode)
{
    return adoptRef(*new WebGPULibrary(context, sourceCode));
}

WebGPULibrary::WebGPULibrary(WebGPURenderingContext* context, const String& sourceCode)
    : WebGPUObject(context)
    , m_sourceCode(sourceCode)
{
    m_library = context->device()->createLibrary(sourceCode);
}

WebGPULibrary::~WebGPULibrary() = default;

String WebGPULibrary::label() const
{
    if (!m_library)
        return emptyString();

    return m_library->label();
}

void WebGPULibrary::setLabel(const String& label)
{
    if (!m_library)
        return;

    m_library->setLabel(label);
}

Vector<String> WebGPULibrary::functionNames()
{
    if (!m_library)
        return Vector<String>();

    return m_library->functionNames();
}

RefPtr<WebGPUFunction> WebGPULibrary::functionWithName(const String& name)
{
    if (!m_library)
        return nullptr;

    RefPtr<WebGPUFunction> function = WebGPUFunction::create(this->context(), this, name);
    if (!function->function())
        return nullptr;
    return function;
}

} // namespace WebCore

#endif
