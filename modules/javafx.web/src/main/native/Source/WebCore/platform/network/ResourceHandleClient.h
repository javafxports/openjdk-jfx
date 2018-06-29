/*
 * Copyright (C) 2006-2017 Apple Inc. All rights reserved.
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

#pragma once

#include <wtf/Forward.h>
#include <wtf/Ref.h>

#if USE(CFURLCONNECTION)
#include <pal/spi/cf/CFNetworkSPI.h>
#endif

#if PLATFORM(IOS) || USE(CFURLCONNECTION)
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(COCOA)
OBJC_CLASS NSCachedURLResponse;
#endif

namespace WebCore {
class AuthenticationChallenge;
class Credential;
class URL;
class ProtectionSpace;
class ResourceHandle;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
class SharedBuffer;

enum CacheStoragePolicy {
    StorageAllowed,
    StorageAllowedInMemoryOnly,
    StorageNotAllowed
};

class ResourceHandleClient {
public:
    WEBCORE_EXPORT ResourceHandleClient();
    WEBCORE_EXPORT virtual ~ResourceHandleClient();

    virtual void didSendData(ResourceHandle*, unsigned long long /*bytesSent*/, unsigned long long /*totalBytesToBeSent*/) { }

    virtual void didReceiveData(ResourceHandle*, const char*, unsigned, int /*encodedDataLength*/) { }
    WEBCORE_EXPORT virtual void didReceiveBuffer(ResourceHandle*, Ref<SharedBuffer>&&, int encodedDataLength);

    virtual void didFinishLoading(ResourceHandle*) { }
    virtual void didFail(ResourceHandle*, const ResourceError&) { }
    virtual void wasBlocked(ResourceHandle*) { }
    virtual void cannotShowURL(ResourceHandle*) { }

    virtual bool loadingSynchronousXHR() { return false; }

    WEBCORE_EXPORT virtual void willSendRequestAsync(ResourceHandle*, ResourceRequest&&, ResourceResponse&&, CompletionHandler<void(ResourceRequest&&)>&&) = 0;

    WEBCORE_EXPORT virtual void didReceiveResponseAsync(ResourceHandle*, ResourceResponse&&, CompletionHandler<void()>&&) = 0;

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    // Client will pass an updated request using ResourceHandle::continueCanAuthenticateAgainstProtectionSpace() when ready.
    WEBCORE_EXPORT virtual void canAuthenticateAgainstProtectionSpaceAsync(ResourceHandle*, const ProtectionSpace&) = 0;
#endif
    // Client will pass an updated request using ResourceHandle::continueWillCacheResponse() when ready.
#if USE(CFURLCONNECTION)
    WEBCORE_EXPORT virtual void willCacheResponseAsync(ResourceHandle*, CFCachedURLResponseRef);
#elif PLATFORM(COCOA)
    WEBCORE_EXPORT virtual void willCacheResponseAsync(ResourceHandle*, NSCachedURLResponse *);
#endif

    virtual bool shouldUseCredentialStorage(ResourceHandle*) { return false; }
    virtual void didReceiveAuthenticationChallenge(ResourceHandle*, const AuthenticationChallenge&) { }
    virtual void receivedCancellation(ResourceHandle*, const AuthenticationChallenge&) { }

#if PLATFORM(IOS) || USE(CFURLCONNECTION)
    virtual RetainPtr<CFDictionaryRef> connectionProperties(ResourceHandle*) { return nullptr; }
#endif

#if USE(CFURLCONNECTION)
    virtual CFCachedURLResponseRef willCacheResponse(ResourceHandle*, CFCachedURLResponseRef response) { return response; }
#if PLATFORM(WIN)
    virtual bool shouldCacheResponse(ResourceHandle*, CFCachedURLResponseRef) { return true; }
#endif // PLATFORM(WIN)

#elif PLATFORM(COCOA)
    virtual NSCachedURLResponse *willCacheResponse(ResourceHandle*, NSCachedURLResponse *response) { return response; }
#endif
};

}
