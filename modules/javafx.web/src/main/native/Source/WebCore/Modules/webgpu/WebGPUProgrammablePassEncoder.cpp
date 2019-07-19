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

#include "config.h"
#include "WebGPUProgrammablePassEncoder.h"

#if ENABLE(WEBGPU)

#include "GPUProgrammablePassEncoder.h"
#include "WebGPUBindGroup.h"
#include "WebGPURenderPipeline.h"

namespace WebCore {

WebGPUProgrammablePassEncoder::WebGPUProgrammablePassEncoder(Ref<WebGPUCommandBuffer>&& creator)
    : m_commandBuffer(WTFMove(creator))
{
}

Ref<WebGPUCommandBuffer> WebGPUProgrammablePassEncoder::endPass()
{
    passEncoder().endPass();
    return m_commandBuffer.copyRef();
}

void WebGPUProgrammablePassEncoder::setBindGroup(unsigned long index, const WebGPUBindGroup& bindGroup) const
{
    // Maximum number of bind groups supported in Web GPU.
    if (index >= 4) {
        LOG(WebGPU, "WebGPUProgrammablePassEncoder::setBindGroup(): Invalid index!");
        return;
    }
    if (!bindGroup.bindGroup()) {
        LOG(WebGPU, "WebGPUProgrammablePassEncoder::setBindGroup(): Invalid WebGPUBindGroup!");
        return;
    }
    // FIXME: Any validation (e.g. index duplicates, not in pipeline layout).
    passEncoder().setBindGroup(index, *bindGroup.bindGroup());
}

void WebGPUProgrammablePassEncoder::setPipeline(Ref<WebGPURenderPipeline>&& pipeline)
{
    passEncoder().setPipeline(pipeline->renderPipeline());
}

} // namespace WebCore

#endif // ENABLE(WEBGPU)
