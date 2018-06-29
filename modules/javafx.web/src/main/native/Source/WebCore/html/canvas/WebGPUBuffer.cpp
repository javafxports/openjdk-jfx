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
#include "WebGPUBuffer.h"

#if ENABLE(WEBGPU)

#include "GPUBuffer.h"
#include "WebGPURenderingContext.h"
#include <JavaScriptCore/ArrayBuffer.h>

namespace WebCore {

Ref<WebGPUBuffer> WebGPUBuffer::create(WebGPURenderingContext* context, ArrayBufferView* data)
{
    return adoptRef(*new WebGPUBuffer(context, data));
}

WebGPUBuffer::WebGPUBuffer(WebGPURenderingContext* context, ArrayBufferView* data)
    : WebGPUObject(context)
{
    m_buffer = context->device()->createBufferFromData(data);
}

WebGPUBuffer::~WebGPUBuffer() = default;

unsigned long WebGPUBuffer::length() const
{
    if (!m_buffer)
        return 0;

    return m_buffer->length();
}

RefPtr<ArrayBuffer> WebGPUBuffer::contents() const
{
    if (!m_buffer)
        return nullptr;

    return m_buffer->contents();
}

} // namespace WebCore

#endif
