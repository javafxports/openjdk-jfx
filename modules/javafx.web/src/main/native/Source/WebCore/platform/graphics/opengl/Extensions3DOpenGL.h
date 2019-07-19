/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Extensions3DOpenGLCommon.h"

#include "GraphicsContext3D.h"
#include <wtf/text/StringHash.h>

namespace WebCore {

class Extensions3DOpenGL : public Extensions3DOpenGLCommon {
    WTF_MAKE_FAST_ALLOCATED;
public:
    // This class only needs to be instantiated by GraphicsContext3D implementations.
    Extensions3DOpenGL(GraphicsContext3D*, bool useIndexedGetString);
    virtual ~Extensions3DOpenGL();

    // Extensions3D methods.
    void blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter) override;
    void renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height) override;

    Platform3DObject createVertexArrayOES() override;
    void deleteVertexArrayOES(Platform3DObject) override;
    GC3Dboolean isVertexArrayOES(Platform3DObject) override;
    void bindVertexArrayOES(Platform3DObject) override;
    void insertEventMarkerEXT(const String&) override;
    void pushGroupMarkerEXT(const String&) override;
    void popGroupMarkerEXT(void) override;
    void drawBuffersEXT(GC3Dsizei, const GC3Denum*) override;

    void drawArraysInstanced(GC3Denum mode, GC3Dint first, GC3Dsizei count, GC3Dsizei primcount) override;
    void drawElementsInstanced(GC3Denum mode, GC3Dsizei count, GC3Denum type, long long offset, GC3Dsizei primcount) override;
    void vertexAttribDivisor(GC3Duint index, GC3Duint divisor) override;

protected:
    bool supportsExtension(const WTF::String&) override;
    String getExtensions() override;

private:
#if PLATFORM(GTK) || PLATFORM(WIN) || (PLATFORM(COCOA) && USE(OPENGL_ES))
    bool isVertexArrayObjectSupported();
#endif
};

} // namespace WebCore
