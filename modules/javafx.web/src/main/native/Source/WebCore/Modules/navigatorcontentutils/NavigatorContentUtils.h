/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#pragma once

#if ENABLE(NAVIGATOR_CONTENT_UTILS)

#include "ExceptionOr.h"
#include "NavigatorContentUtilsClient.h"
#include "Supplementable.h"

namespace WebCore {

class Page;
class Navigator;

typedef int ExceptionCode;

class NavigatorContentUtils final : public Supplement<Page> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit NavigatorContentUtils(std::unique_ptr<NavigatorContentUtilsClient> client)
        : m_client(WTFMove(client))
    { }

    virtual ~NavigatorContentUtils();

    static const char* supplementName();
    static NavigatorContentUtils* from(Page*);

    static ExceptionOr<void> registerProtocolHandler(Navigator&, const String& scheme, const String& url, const String& title);

#if ENABLE(CUSTOM_SCHEME_HANDLER)
    static ExceptionOr<String> isProtocolHandlerRegistered(Navigator&, const String& scheme, const String& url);
    static ExceptionOr<void> unregisterProtocolHandler(Navigator&, const String& scheme, const String& url);
#endif

private:
    NavigatorContentUtilsClient* client() { return m_client.get(); }

    std::unique_ptr<NavigatorContentUtilsClient> m_client;
};

} // namespace WebCore

#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)
