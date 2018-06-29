/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#pragma once

#include "CachePolicy.h"
#include "CacheValidation.h"
#include "FrameLoaderTypes.h"
#include "ResourceError.h"
#include "ResourceLoadPriority.h"
#include "ResourceLoaderOptions.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "Timer.h"
#include <pal/SessionID.h>
#include <time.h>
#include <wtf/HashCountedSet.h>
#include <wtf/HashSet.h>
#include <wtf/TypeCasts.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CachedResourceClient;
class CachedResourceHandleBase;
class CachedResourceLoader;
class CachedResourceRequest;
class LoadTiming;
class MemoryCache;
class SecurityOrigin;
class SharedBuffer;
class SubresourceLoader;
class TextResourceDecoder;

// A resource that is held in the cache. Classes who want to use this object should derive
// from CachedResourceClient, to get the function calls in case the requested data has arrived.
// This class also does the actual communication with the loader to obtain the resource from the network.
class CachedResource {
    WTF_MAKE_NONCOPYABLE(CachedResource); WTF_MAKE_FAST_ALLOCATED;
    friend class MemoryCache;

public:
    enum Type {
        MainResource,
        ImageResource,
        CSSStyleSheet,
        Script,
        FontResource,
#if ENABLE(SVG_FONTS)
        SVGFontResource,
#endif
        MediaResource,
        RawResource,
        Icon,
        Beacon,
        SVGDocumentResource
#if ENABLE(XSLT)
        , XSLStyleSheet
#endif
#if ENABLE(LINK_PREFETCH)
        , LinkPrefetch
        , LinkSubresource
#endif
#if ENABLE(VIDEO_TRACK)
        , TextTrackResource
#endif
#if ENABLE(APPLICATION_MANIFEST)
        , ApplicationManifest
#endif
    };

    enum Status {
        Unknown,      // let cache decide what to do with it
        Pending,      // only partially loaded
        Cached,       // regular case
        LoadError,
        DecodeError
    };

    CachedResource(CachedResourceRequest&&, Type, PAL::SessionID);
    virtual ~CachedResource();

    virtual void load(CachedResourceLoader&);

    virtual void setEncoding(const String&) { }
    virtual String encoding() const { return String(); }
    virtual const TextResourceDecoder* textResourceDecoder() const { return nullptr; }
    virtual void updateBuffer(SharedBuffer&);
    virtual void updateData(const char* data, unsigned length);
    virtual void finishLoading(SharedBuffer*);
    virtual void error(CachedResource::Status);

    void setResourceError(const ResourceError& error) { m_error = error; }
    const ResourceError& resourceError() const { return m_error; }

    virtual bool shouldIgnoreHTTPStatusCodeErrors() const { return false; }

    const ResourceRequest& resourceRequest() const { return m_resourceRequest; }
    const URL& url() const { return m_resourceRequest.url();}
    const String& cachePartition() const { return m_resourceRequest.cachePartition(); }
    PAL::SessionID sessionID() const { return m_sessionID; }
    Type type() const { return m_type; }
    String mimeType() const { return m_response.mimeType(); }
    long long expectedContentLength() const { return m_response.expectedContentLength(); }

    static bool shouldUsePingLoad(Type type) { return type == Type::Beacon; }

    ResourceLoadPriority loadPriority() const { return m_loadPriority; }
    void setLoadPriority(const std::optional<ResourceLoadPriority>&);

    WEBCORE_EXPORT void addClient(CachedResourceClient&);
    WEBCORE_EXPORT void removeClient(CachedResourceClient&);
    bool hasClients() const { return !m_clients.isEmpty() || !m_clientsAwaitingCallback.isEmpty(); }
    bool hasClient(CachedResourceClient& client) { return m_clients.contains(&client) || m_clientsAwaitingCallback.contains(&client); }
    bool deleteIfPossible();

    enum PreloadResult {
        PreloadNotReferenced,
        PreloadReferenced,
        PreloadReferencedWhileLoading,
        PreloadReferencedWhileComplete
    };
    PreloadResult preloadResult() const { return static_cast<PreloadResult>(m_preloadResult); }

    virtual void didAddClient(CachedResourceClient&);
    virtual void didRemoveClient(CachedResourceClient&) { }
    virtual void allClientsRemoved() { }
    void destroyDecodedDataIfNeeded();

    unsigned count() const { return m_clients.size(); }

    Status status() const { return static_cast<Status>(m_status); }
    void setStatus(Status status) { m_status = status; }

    unsigned size() const { return encodedSize() + decodedSize() + overheadSize(); }
    unsigned encodedSize() const { return m_encodedSize; }
    unsigned decodedSize() const { return m_decodedSize; }
    unsigned overheadSize() const;

    bool isLoaded() const { return !m_loading; } // FIXME. Method name is inaccurate. Loading might not have started yet.

    bool isLoading() const { return m_loading; }
    void setLoading(bool b) { m_loading = b; }
    virtual bool stillNeedsLoad() const { return false; }

    SubresourceLoader* loader() { return m_loader.get(); }

    bool areAllClientsXMLHttpRequests() const;

    bool isImage() const { return type() == ImageResource; }
    // FIXME: CachedRawResource could be a main resource, an audio/video resource, or a raw XHR/icon resource.
    bool isMainOrMediaOrIconOrRawResource() const { return type() == MainResource || type() == MediaResource || type() == Icon || type() == RawResource || type() == Beacon; }

    // Whether this request should impact request counting and delay window.onload.
    bool ignoreForRequestCount() const
    {
        return m_ignoreForRequestCount
            || type() == MainResource
#if ENABLE(LINK_PREFETCH)
            || type() == LinkPrefetch
            || type() == LinkSubresource
#endif
            || type() == Icon
            || type() == RawResource;
    }

    void setIgnoreForRequestCount(bool ignoreForRequestCount) { m_ignoreForRequestCount = ignoreForRequestCount; }

    unsigned accessCount() const { return m_accessCount; }
    void increaseAccessCount() { m_accessCount++; }

    // Computes the status of an object after loading.
    // Updates the expire date on the cache entry file
    void finish();

    // Called by the cache if the object has been removed from the cache
    // while still being referenced. This means the object should delete itself
    // if the number of clients observing it ever drops to 0.
    // The resource can be brought back to cache after successful revalidation.
    void setInCache(bool inCache) { m_inCache = inCache; }
    bool inCache() const { return m_inCache; }

    void clearLoader();

    SharedBuffer* resourceBuffer() const { return m_data.get(); }

    virtual void redirectReceived(ResourceRequest&&, const ResourceResponse&, CompletionHandler<void(ResourceRequest&&)>&&);
    virtual void responseReceived(const ResourceResponse&);
    virtual bool shouldCacheResponse(const ResourceResponse&) { return true; }
    void setResponse(const ResourceResponse&);
    const ResourceResponse& response() const { return m_response; }

    void setCrossOrigin();
    bool isCrossOrigin() const;
    bool isCORSSameOrigin() const;
    ResourceResponse::Tainting responseTainting() const { return m_responseTainting; }

    void loadFrom(const CachedResource&);

    SecurityOrigin* origin() const { return m_origin.get(); }
    AtomicString initiatorName() const { return m_initiatorName; }

    bool canDelete() const { return !hasClients() && !m_loader && !m_preloadCount && !m_handleCount && !m_resourceToRevalidate && !m_proxyResource; }
    bool hasOneHandle() const { return m_handleCount == 1; }

    bool isExpired() const;

    void cancelLoad();
    bool wasCanceled() const { return m_error.isCancellation(); }
    bool errorOccurred() const { return m_status == LoadError || m_status == DecodeError; }
    bool loadFailedOrCanceled() const { return !m_error.isNull(); }

    bool shouldSendResourceLoadCallbacks() const { return m_options.sendLoadCallbacks == SendCallbacks; }
    DataBufferingPolicy dataBufferingPolicy() const { return m_options.dataBufferingPolicy; }

    bool allowsCaching() const { return m_options.cachingPolicy == CachingPolicy::AllowCaching; }
    const ResourceLoaderOptions& options() const { return m_options; }

    virtual void destroyDecodedData() { }

    void setOwningCachedResourceLoader(CachedResourceLoader* cachedResourceLoader) { m_owningCachedResourceLoader = cachedResourceLoader; }

    bool isPreloaded() const { return m_preloadCount; }
    void increasePreloadCount() { ++m_preloadCount; }
    void decreasePreloadCount() { ASSERT(m_preloadCount); --m_preloadCount; }
    bool isLinkPreload() { return m_isLinkPreload; }
    void setLinkPreload() { m_isLinkPreload = true; }
    bool hasUnknownEncoding() { return m_hasUnknownEncoding; }
    void setHasUnknownEncoding(bool hasUnknownEncoding) { m_hasUnknownEncoding = hasUnknownEncoding; }

    void registerHandle(CachedResourceHandleBase*);
    WEBCORE_EXPORT void unregisterHandle(CachedResourceHandleBase*);

    bool canUseCacheValidator() const;

    enum class RevalidationDecision { No, YesDueToCachePolicy, YesDueToNoStore, YesDueToNoCache, YesDueToExpired };
    virtual RevalidationDecision makeRevalidationDecision(CachePolicy) const;
    bool redirectChainAllowsReuse(ReuseExpiredRedirectionOrNot) const;
    bool hasRedirections() const { return m_redirectChainCacheStatus.status != RedirectChainCacheStatus::Status::NoRedirection;  }

    bool varyHeaderValuesMatch(const ResourceRequest&);

    bool isCacheValidator() const { return m_resourceToRevalidate; }
    CachedResource* resourceToRevalidate() const { return m_resourceToRevalidate; }

    // HTTP revalidation support methods for CachedResourceLoader.
    void setResourceToRevalidate(CachedResource*);
    virtual void switchClientsToRevalidatedResource();
    void clearResourceToRevalidate();
    void updateResponseAfterRevalidation(const ResourceResponse& validatingResponse);
    bool validationInProgress() const { return m_proxyResource; }
    bool validationCompleting() const { return m_proxyResource && m_proxyResource->m_switchingClientsToRevalidatedResource; }

    virtual void didSendData(unsigned long long /* bytesSent */, unsigned long long /* totalBytesToBeSent */) { }

    virtual void didRetrieveDerivedDataFromCache(const String& /* type */, SharedBuffer&) { }

#if USE(FOUNDATION) || USE(SOUP)
    WEBCORE_EXPORT void tryReplaceEncodedData(SharedBuffer&);
#endif

    unsigned long identifierForLoadWithoutResourceLoader() const { return m_identifierForLoadWithoutResourceLoader; }
    static ResourceLoadPriority defaultPriorityForResourceType(Type);

    void setOriginalRequest(std::unique_ptr<ResourceRequest>&& originalRequest) { m_originalRequest = WTFMove(originalRequest); }
    const std::unique_ptr<ResourceRequest>& originalRequest() const { return m_originalRequest; }

protected:
    // CachedResource constructor that may be used when the CachedResource can already be filled with response data.
    CachedResource(const URL&, Type, PAL::SessionID);

    void setEncodedSize(unsigned);
    void setDecodedSize(unsigned);
    void didAccessDecodedData(double timeStamp);

    virtual void didReplaceSharedBufferContents() { }

    virtual void setBodyDataFrom(const CachedResource&);

    // FIXME: Make the rest of these data members private and use functions in derived classes instead.
    HashCountedSet<CachedResourceClient*> m_clients;
    ResourceRequest m_resourceRequest;
    std::unique_ptr<ResourceRequest> m_originalRequest; // Needed by Ping loads.
    RefPtr<SubresourceLoader> m_loader;
    ResourceLoaderOptions m_options;
    ResourceResponse m_response;
    ResourceResponse::Tainting m_responseTainting { ResourceResponse::Tainting::Basic };
    RefPtr<SharedBuffer> m_data;
    DeferrableOneShotTimer m_decodedDataDeletionTimer;

private:
    class Callback;

    bool addClientToSet(CachedResourceClient&);

    void decodedDataDeletionTimerFired();

    virtual void checkNotify();
    virtual bool mayTryReplaceEncodedData() const { return false; }

    Seconds freshnessLifetime(const ResourceResponse&) const;

    void addAdditionalRequestHeaders(CachedResourceLoader&);
    void failBeforeStarting();

    HashMap<CachedResourceClient*, std::unique_ptr<Callback>> m_clientsAwaitingCallback;
    PAL::SessionID m_sessionID;
    ResourceLoadPriority m_loadPriority;
    WallTime m_responseTimestamp;

    String m_fragmentIdentifierForRequest;

    ResourceError m_error;
    RefPtr<SecurityOrigin> m_origin;
    AtomicString m_initiatorName;

    double m_lastDecodedAccessTime { 0 }; // Used as a "thrash guard" in the cache

    unsigned m_encodedSize { 0 };
    unsigned m_decodedSize { 0 };
    unsigned m_accessCount { 0 };
    unsigned m_handleCount { 0 };
    unsigned m_preloadCount { 0 };

    PreloadResult m_preloadResult { PreloadNotReferenced };

    bool m_requestedFromNetworkingLayer { false };

    bool m_inCache { false };
    bool m_loading { false };
    bool m_isLinkPreload { false };
    bool m_hasUnknownEncoding { false };

    bool m_switchingClientsToRevalidatedResource { false };

    Type m_type; // Type
    unsigned m_status { Pending }; // Status

#ifndef NDEBUG
    bool m_deleted { false };
    unsigned m_lruIndex { 0 };
#endif

    CachedResourceLoader* m_owningCachedResourceLoader { nullptr }; // only non-null for resources that are not in the cache

    // If this field is non-null we are using the resource as a proxy for checking whether an existing resource is still up to date
    // using HTTP If-Modified-Since/If-None-Match headers. If the response is 304 all clients of this resource are moved
    // to to be clients of m_resourceToRevalidate and the resource is deleted. If not, the field is zeroed and this
    // resources becomes normal resource load.
    CachedResource* m_resourceToRevalidate { nullptr };

    // If this field is non-null, the resource has a proxy for checking whether it is still up to date (see m_resourceToRevalidate).
    CachedResource* m_proxyResource { nullptr };

    // These handles will need to be updated to point to the m_resourceToRevalidate in case we get 304 response.
    HashSet<CachedResourceHandleBase*> m_handlesToRevalidate;

    RedirectChainCacheStatus m_redirectChainCacheStatus;

    Vector<std::pair<String, String>> m_varyingHeaderValues;

    unsigned long m_identifierForLoadWithoutResourceLoader { 0 };
    bool m_ignoreForRequestCount { false };
};

class CachedResource::Callback {
#if !COMPILER(MSVC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    Callback(CachedResource&, CachedResourceClient&);

    void cancel();

private:
    void timerFired();

    CachedResource& m_resource;
    CachedResourceClient& m_client;
    Timer m_timer;
};

} // namespace WebCore

#define SPECIALIZE_TYPE_TRAITS_CACHED_RESOURCE(ToClassName, CachedResourceType) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ToClassName) \
    static bool isType(const WebCore::CachedResource& resource) { return resource.type() == WebCore::CachedResourceType; } \
SPECIALIZE_TYPE_TRAITS_END()
