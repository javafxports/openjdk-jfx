/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "PixelBufferConformerCV.h"

#if HAVE(CORE_VIDEO)

#include "GraphicsContextCG.h"
#include <wtf/SoftLinking.h>

#include "CoreVideoSoftLink.h"

#if USE(VIDEOTOOLBOX)
SOFT_LINK_FRAMEWORK_OPTIONAL(VideoToolbox)
SOFT_LINK(VideoToolbox, VTPixelBufferConformerCreateWithAttributes, OSStatus, (CFAllocatorRef allocator, CFDictionaryRef attributes, VTPixelBufferConformerRef* conformerOut), (allocator, attributes, conformerOut));
SOFT_LINK(VideoToolbox, VTPixelBufferConformerIsConformantPixelBuffer, Boolean, (VTPixelBufferConformerRef conformer, CVPixelBufferRef pixBuf), (conformer, pixBuf))
SOFT_LINK(VideoToolbox, VTPixelBufferConformerCopyConformedPixelBuffer, OSStatus, (VTPixelBufferConformerRef conformer, CVPixelBufferRef sourceBuffer, Boolean ensureModifiable, CVPixelBufferRef* conformedBufferOut), (conformer, sourceBuffer, ensureModifiable, conformedBufferOut))
#endif

namespace WebCore {

PixelBufferConformerCV::PixelBufferConformerCV(CFDictionaryRef attributes)
{
#if USE(VIDEOTOOLBOX)
    VTPixelBufferConformerRef conformer = nullptr;
    VTPixelBufferConformerCreateWithAttributes(kCFAllocatorDefault, attributes, &conformer);
    ASSERT(conformer);
    m_pixelConformer = adoptCF(conformer);
#else
    UNUSED_PARAM(attributes);
    ASSERT(!attributes);
#endif
}

static const void* CVPixelBufferGetBytePointerCallback(void* info)
{
    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(info);
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    return CVPixelBufferGetBaseAddress(pixelBuffer);
}

static void CVPixelBufferReleaseBytePointerCallback(void* info, const void*)
{
    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(info);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
}

static void CVPixelBufferReleaseInfoCallback(void* info)
{
    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(info);
    CFRelease(pixelBuffer);
}

RetainPtr<CVPixelBufferRef> PixelBufferConformerCV::convert(CVPixelBufferRef rawBuffer)
{
#if USE(VIDEOTOOLBOX)
    RetainPtr<CVPixelBufferRef> buffer { rawBuffer };

    if (!VTPixelBufferConformerIsConformantPixelBuffer(m_pixelConformer.get(), buffer.get())) {
        CVPixelBufferRef outputBuffer = nullptr;
        OSStatus status = VTPixelBufferConformerCopyConformedPixelBuffer(m_pixelConformer.get(), buffer.get(), false, &outputBuffer);
        if (status != noErr || !outputBuffer)
            return nullptr;
        return adoptCF(outputBuffer);
    }
#else
    UNUSED_PARAM(rawBuffer);
#endif
    return nullptr;
}

RetainPtr<CGImageRef> PixelBufferConformerCV::createImageFromPixelBuffer(CVPixelBufferRef rawBuffer)
{
    RetainPtr<CVPixelBufferRef> buffer { rawBuffer };
    size_t width = CVPixelBufferGetWidth(buffer.get());
    size_t height = CVPixelBufferGetHeight(buffer.get());

#if USE(VIDEOTOOLBOX)
    if (!VTPixelBufferConformerIsConformantPixelBuffer(m_pixelConformer.get(), buffer.get())) {
        CVPixelBufferRef outputBuffer = nullptr;
        OSStatus status = VTPixelBufferConformerCopyConformedPixelBuffer(m_pixelConformer.get(), buffer.get(), false, &outputBuffer);
        if (status != noErr || !outputBuffer)
            return nullptr;
        buffer = adoptCF(outputBuffer);
    }
#endif

    CGBitmapInfo bitmapInfo = kCGBitmapByteOrder32Little | kCGImageAlphaFirst;
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(buffer.get());
    size_t byteLength = CVPixelBufferGetDataSize(buffer.get());

    CFRetain(buffer.get()); // Balanced by CVPixelBufferReleaseInfoCallback in providerCallbacks.
    CGDataProviderDirectCallbacks providerCallbacks = { 0, CVPixelBufferGetBytePointerCallback, CVPixelBufferReleaseBytePointerCallback, 0, CVPixelBufferReleaseInfoCallback };
    RetainPtr<CGDataProviderRef> provider = adoptCF(CGDataProviderCreateDirect(buffer.get(), byteLength, &providerCallbacks));

    return adoptCF(CGImageCreate(width, height, 8, 32, bytesPerRow, sRGBColorSpaceRef(), bitmapInfo, provider.get(), nullptr, false, kCGRenderingIntentDefault));
}

}

#endif // HAVE(CORE_VIDEO)
