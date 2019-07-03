/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#if ENABLE(WEBGPU)

#include "GPUBindGroupLayoutDescriptor.h"

#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

#if USE(METAL)
OBJC_PROTOCOL(MTLArgumentEncoder);
OBJC_PROTOCOL(MTLBuffer);
#endif // USE(METAL)

namespace WebCore {

class GPUDevice;

class GPUBindGroupLayout : public RefCounted<GPUBindGroupLayout> {
public:
    static RefPtr<GPUBindGroupLayout> tryCreate(const GPUDevice&, GPUBindGroupLayoutDescriptor&&);

    using BindingsMapType = HashMap<unsigned long long, GPUBindGroupLayoutBinding, WTF::IntHash<unsigned long long>, WTF::UnsignedWithZeroKeyHashTraits<unsigned long long>>;
    const BindingsMapType& bindingsMap() const { return m_bindingsMap; }
#if USE(METAL)
    struct ArgumentEncoderBuffer {
        RetainPtr<MTLArgumentEncoder> encoder;
        RetainPtr<MTLBuffer> buffer;

        bool isValid() const { return encoder && buffer; }
    };
    const ArgumentEncoderBuffer& vertexArguments() const { return m_vertexArguments; }
    const ArgumentEncoderBuffer& fragmentArguments() const { return m_fragmentArguments; }
    const ArgumentEncoderBuffer& computeArguments() const { return m_computeArguments; }
#endif // USE(METAL)

private:
#if USE(METAL)
    GPUBindGroupLayout(BindingsMapType&&, ArgumentEncoderBuffer&& vertex, ArgumentEncoderBuffer&& fragment, ArgumentEncoderBuffer&& compute);

    ArgumentEncoderBuffer m_vertexArguments;
    ArgumentEncoderBuffer m_fragmentArguments;
    ArgumentEncoderBuffer m_computeArguments;
#endif // USE(METAL)
    const BindingsMapType m_bindingsMap;
};

} // namespace WebCore

#endif // ENABLE(WEBGPU)
