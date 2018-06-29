/*
 * Copyright (C) 2012-2018 Apple Inc. All rights reserved.
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

#include "CredentialStorage.h"
#include <pal/SessionID.h>
#include <wtf/Function.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(COCOA) || USE(CFURLCONNECTION)
#include <pal/spi/cf/CFNetworkSPI.h>
#include <wtf/RetainPtr.h>
#endif

#if USE(SOUP)
#include <wtf/Function.h>
#include <wtf/glib/GRefPtr.h>
typedef struct _SoupCookieJar SoupCookieJar;
#endif

#if USE(CURL)
#include "CookieJarCurl.h"
#include "CookieJarDB.h"
#include <wtf/UniqueRef.h>
#endif

#ifdef __OBJC__
#include <objc/objc.h>
#endif

#if PLATFORM(COCOA)
#include "CookieStorageObserver.h"
#endif

namespace WebCore {

class NetworkingContext;
class ResourceRequest;
class SoupNetworkSession;

struct Cookie;

class NetworkStorageSession {
    WTF_MAKE_NONCOPYABLE(NetworkStorageSession); WTF_MAKE_FAST_ALLOCATED;
public:
    WEBCORE_EXPORT static NetworkStorageSession& defaultStorageSession();
    WEBCORE_EXPORT static NetworkStorageSession* storageSession(PAL::SessionID);
    WEBCORE_EXPORT static void ensureSession(PAL::SessionID, const String& identifierBase = String());
    WEBCORE_EXPORT static void destroySession(PAL::SessionID);
    WEBCORE_EXPORT static void forEach(const WTF::Function<void(const WebCore::NetworkStorageSession&)>&);

    WEBCORE_EXPORT static void switchToNewTestingSession();

    PAL::SessionID sessionID() const { return m_sessionID; }
    CredentialStorage& credentialStorage() { return m_credentialStorage; }

#ifdef __OBJC__
    WEBCORE_EXPORT NSHTTPCookieStorage *nsCookieStorage() const;
#endif

    const String& cacheStorageDirectory() const { return m_cacheStorageDirectory; }
    void setCacheStorageDirectory(String&& path) { m_cacheStorageDirectory = WTFMove(path); }
    uint64_t cacheStoragePerOriginQuota() const { return m_cacheStoragePerOriginQuota; }
    void setCacheStoragePerOriginQuota(uint64_t quota) { m_cacheStoragePerOriginQuota = quota; }

#if PLATFORM(COCOA) || USE(CFURLCONNECTION)
    WEBCORE_EXPORT static void ensureSession(PAL::SessionID, const String& identifierBase, RetainPtr<CFHTTPCookieStorageRef>&&);
    NetworkStorageSession(PAL::SessionID, RetainPtr<CFURLStorageSessionRef>&&, RetainPtr<CFHTTPCookieStorageRef>&&);

    // May be null, in which case a Foundation default should be used.
    CFURLStorageSessionRef platformSession() { return m_platformSession.get(); }
    WEBCORE_EXPORT RetainPtr<CFHTTPCookieStorageRef> cookieStorage() const;
    WEBCORE_EXPORT static void setCookieStoragePartitioningEnabled(bool);
    WEBCORE_EXPORT static void setStorageAccessAPIEnabled(bool);
#if HAVE(CFNETWORK_STORAGE_PARTITIONING)
    WEBCORE_EXPORT String cookieStoragePartition(const ResourceRequest&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID) const;
    WEBCORE_EXPORT bool shouldBlockCookies(const ResourceRequest&) const;
    bool shouldBlockCookies(const URL& firstPartyForCookies, const URL& resource) const;
    String cookieStoragePartition(const URL& firstPartyForCookies, const URL& resource, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID) const;
    WEBCORE_EXPORT void setPrevalentDomainsToPartitionOrBlockCookies(const Vector<String>& domainsToPartition, const Vector<String>& domainsToBlock, const Vector<String>& domainsToNeitherPartitionNorBlock, bool clearFirst);
    WEBCORE_EXPORT void removePrevalentDomains(const Vector<String>& domains);
    WEBCORE_EXPORT bool hasStorageAccessForFrame(const String& resourceDomain, const String& firstPartyDomain, uint64_t frameID, uint64_t pageID) const;
    WEBCORE_EXPORT bool hasStorageAccessForFrame(const ResourceRequest&, uint64_t frameID, uint64_t pageID) const;
    WEBCORE_EXPORT Vector<String> getAllStorageAccessEntries() const;
    WEBCORE_EXPORT void grantStorageAccessForFrame(const String& resourceDomain, const String& firstPartyDomain, uint64_t frameID, uint64_t pageID);
    WEBCORE_EXPORT void removeStorageAccessForFrame(uint64_t frameID, uint64_t pageID);
    WEBCORE_EXPORT void removeStorageAccessForAllFramesOnPage(uint64_t pageID);
#endif
#elif USE(SOUP)
    NetworkStorageSession(PAL::SessionID, std::unique_ptr<SoupNetworkSession>&&);
    ~NetworkStorageSession();

    SoupNetworkSession* soupNetworkSession() const { return m_session.get(); };
    SoupNetworkSession& getOrCreateSoupNetworkSession() const;
    void clearSoupNetworkSessionAndCookieStorage();
    SoupCookieJar* cookieStorage() const;
    void setCookieStorage(SoupCookieJar*);
    void setCookieObserverHandler(Function<void ()>&&);
    void getCredentialFromPersistentStorage(const ProtectionSpace&, GCancellable*, Function<void (Credential&&)>&& completionHandler);
    void saveCredentialToPersistentStorage(const ProtectionSpace&, const Credential&);
#elif USE(CURL)
    NetworkStorageSession(PAL::SessionID, NetworkingContext*);
    ~NetworkStorageSession();

    const CookieJarCurl& cookieStorage() const { return m_cookieStorage; };
    CookieJarDB& cookieDatabase() const { return m_cookieDatabase; };

    NetworkingContext* context() const;
#else
    NetworkStorageSession(PAL::SessionID, NetworkingContext*);
    ~NetworkStorageSession();

    NetworkingContext* context() const;
#endif

    WEBCORE_EXPORT void setCookie(const Cookie&);
    WEBCORE_EXPORT void setCookies(const Vector<Cookie>&, const URL&, const URL& mainDocumentURL);
    WEBCORE_EXPORT void deleteCookie(const Cookie&);
    WEBCORE_EXPORT Vector<Cookie> getAllCookies();
    WEBCORE_EXPORT Vector<Cookie> getCookies(const URL&);
    WEBCORE_EXPORT void flushCookieStore();

private:
    static HashMap<PAL::SessionID, std::unique_ptr<NetworkStorageSession>>& globalSessionMap();
    PAL::SessionID m_sessionID;

#if PLATFORM(COCOA) || USE(CFURLCONNECTION)
    RetainPtr<CFURLStorageSessionRef> m_platformSession;
    RetainPtr<CFHTTPCookieStorageRef> m_platformCookieStorage;
#elif USE(SOUP)
    static void cookiesDidChange(NetworkStorageSession*);

    mutable std::unique_ptr<SoupNetworkSession> m_session;
    GRefPtr<SoupCookieJar> m_cookieStorage;
    Function<void ()> m_cookieObserverHandler;
#elif USE(CURL)
    UniqueRef<CookieJarCurl> m_cookieStorage;
    mutable CookieJarDB m_cookieDatabase;

    RefPtr<NetworkingContext> m_context;
#else
    RefPtr<NetworkingContext> m_context;
#endif

    CredentialStorage m_credentialStorage;

    String m_cacheStorageDirectory;
    uint64_t m_cacheStoragePerOriginQuota { 0 };

#if HAVE(CFNETWORK_STORAGE_PARTITIONING)
    bool shouldPartitionCookies(const String& topPrivatelyControlledDomain) const;
    bool shouldBlockThirdPartyCookies(const String& topPrivatelyControlledDomain) const;
    HashSet<String> m_topPrivatelyControlledDomainsToPartition;
    HashSet<String> m_topPrivatelyControlledDomainsToBlock;
    HashMap<uint64_t, HashMap<uint64_t, String, DefaultHash<uint64_t>::Hash, WTF::UnsignedWithZeroKeyHashTraits<uint64_t>>, DefaultHash<uint64_t>::Hash, WTF::UnsignedWithZeroKeyHashTraits<uint64_t>> m_framesGrantedStorageAccess;
#endif

#if PLATFORM(COCOA)
public:
    CookieStorageObserver& cookieStorageObserver() const;

private:
    mutable RefPtr<CookieStorageObserver> m_cookieStorageObserver;
#endif
};

#if PLATFORM(COCOA)
WEBCORE_EXPORT CFURLStorageSessionRef createPrivateStorageSession(CFStringRef identifier);
#endif

}
