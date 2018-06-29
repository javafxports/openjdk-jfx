/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Copyright (C) 2016 Akamai Technologies Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CachedCSSStyleSheet.h"
#include "CachedFont.h"
#include "CachedFontClient.h"
#include "CachedImage.h"
#include "CachedImageClient.h"
#include "CachedRawResource.h"
#include "CachedRawResourceClient.h"
#include "CachedResourceHandle.h"
#include "CachedScript.h"
#include "CachedStyleSheetClient.h"
#include "CachedTextTrack.h"

#include <wtf/WeakPtr.h>

namespace WebCore {

class LinkLoader;

class LinkPreloadResourceClient {
public:
    virtual ~LinkPreloadResourceClient() = default;

    void triggerEvents(const CachedResource&);

    virtual void clear() = 0;

protected:

    LinkPreloadResourceClient(LinkLoader&, CachedResource&);

    void addResource(CachedResourceClient& client)
    {
        m_resource->addClient(client);
    }

    void clearResource(CachedResourceClient& client)
    {
        if (m_resource) {
            m_resource->cancelLoad();
            m_resource->removeClient(client);
        }
        m_resource = nullptr;
    }

    CachedResource* ownedResource() { return m_resource.get(); }

private:
    WeakPtr<LinkLoader> m_loader;
    CachedResourceHandle<CachedResource> m_resource;
};

class LinkPreloadDefaultResourceClient: public LinkPreloadResourceClient, CachedResourceClient {
public:
    static std::unique_ptr<LinkPreloadDefaultResourceClient> create(LinkLoader& loader, CachedScript& resource)
    {
        return std::unique_ptr<LinkPreloadDefaultResourceClient>(new LinkPreloadDefaultResourceClient(loader, resource));
    }

#if ENABLE(VIDEO_TRACK)
    static std::unique_ptr<LinkPreloadDefaultResourceClient> create(LinkLoader& loader, CachedTextTrack& resource)
    {
        return std::unique_ptr<LinkPreloadDefaultResourceClient>(new LinkPreloadDefaultResourceClient(loader, resource));
    }
#endif

    virtual ~LinkPreloadDefaultResourceClient() = default;


    void notifyFinished(CachedResource& resource) override { triggerEvents(resource); }

    void clear() override { clearResource(*this); }
    bool shouldMarkAsReferenced() const override { return false; }

private:
    LinkPreloadDefaultResourceClient(LinkLoader& loader, CachedResource& resource)
        : LinkPreloadResourceClient(loader, resource)
    {
        addResource(*this);
    }
};

class LinkPreloadStyleResourceClient: public LinkPreloadResourceClient, public CachedStyleSheetClient {
public:
    static std::unique_ptr<LinkPreloadStyleResourceClient> create(LinkLoader& loader, CachedCSSStyleSheet& resource)
    {
        return std::unique_ptr<LinkPreloadStyleResourceClient>(new LinkPreloadStyleResourceClient(loader, resource));
    }

    virtual ~LinkPreloadStyleResourceClient() = default;

    void setCSSStyleSheet(const String&, const URL&, const String&, const CachedCSSStyleSheet* resource) override
    {
        ASSERT(resource);
        ASSERT(ownedResource() == resource);
        triggerEvents(*resource);
    }

    void clear() override { clearResource(*this); }
    bool shouldMarkAsReferenced() const override { return false; }

private:
    LinkPreloadStyleResourceClient(LinkLoader& loader, CachedCSSStyleSheet& resource)
        : LinkPreloadResourceClient(loader, resource)
    {
        addResource(*this);
    }
};

class LinkPreloadImageResourceClient: public LinkPreloadResourceClient, public CachedImageClient {
public:
    static std::unique_ptr<LinkPreloadImageResourceClient> create(LinkLoader& loader, CachedImage& resource)
    {
        return std::unique_ptr<LinkPreloadImageResourceClient>(new LinkPreloadImageResourceClient(loader, resource));
    }

    virtual ~LinkPreloadImageResourceClient() = default;

    void notifyFinished(CachedResource& resource) override { triggerEvents(resource); }

    void clear() override { clearResource(*this); }
    bool shouldMarkAsReferenced() const override { return false; }

private:
    LinkPreloadImageResourceClient(LinkLoader& loader, CachedImage& resource)
        : LinkPreloadResourceClient(loader, dynamic_cast<CachedResource&>(resource))
    {
        addResource(*this);
    }
};

class LinkPreloadFontResourceClient: public LinkPreloadResourceClient, public CachedFontClient {
public:
    static std::unique_ptr<LinkPreloadFontResourceClient> create(LinkLoader& loader, CachedFont& resource)
    {
        return std::unique_ptr<LinkPreloadFontResourceClient>(new LinkPreloadFontResourceClient(loader, resource));
    }

    virtual ~LinkPreloadFontResourceClient() = default;

    void fontLoaded(CachedFont& resource) override
    {
        ASSERT(ownedResource() == &resource);
        triggerEvents(resource);
    }

    void clear() override { clearResource(*this); }
    bool shouldMarkAsReferenced() const override { return false; }

private:
    LinkPreloadFontResourceClient(LinkLoader& loader, CachedFont& resource)
        : LinkPreloadResourceClient(loader, resource)
    {
        addResource(*this);
    }
};

class LinkPreloadRawResourceClient: public LinkPreloadResourceClient, public CachedRawResourceClient {
public:
    static std::unique_ptr<LinkPreloadRawResourceClient> create(LinkLoader& loader, CachedRawResource& resource)
    {
        return std::unique_ptr<LinkPreloadRawResourceClient>(new LinkPreloadRawResourceClient(loader, resource));
    }

    virtual ~LinkPreloadRawResourceClient() = default;

    void notifyFinished(CachedResource& resource) override { triggerEvents(resource); }

    void clear() override { clearResource(*this); }
    bool shouldMarkAsReferenced() const override { return false; }

private:
    LinkPreloadRawResourceClient(LinkLoader& loader, CachedRawResource& resource)
        : LinkPreloadResourceClient(loader, resource)
    {
        addResource(*this);
    }
};

}
