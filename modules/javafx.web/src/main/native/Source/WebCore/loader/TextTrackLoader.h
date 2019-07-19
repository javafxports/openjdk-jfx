/*
 * Copyright (C) 2011, 2014 Apple Inc. All rights reserved.
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

#if ENABLE(VIDEO_TRACK)

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "Timer.h"
#include "WebVTTParser.h"
#include <memory>

namespace WebCore {

class CachedTextTrack;
class Document;
class HTMLTrackElement;
class TextTrackLoader;
class ScriptExecutionContext;

class TextTrackLoaderClient {
public:
    virtual ~TextTrackLoaderClient() = default;

    virtual void newCuesAvailable(TextTrackLoader&) = 0;
    virtual void cueLoadingCompleted(TextTrackLoader&, bool loadingFailed) = 0;
    virtual void newRegionsAvailable(TextTrackLoader&) = 0;
    virtual void newStyleSheetsAvailable(TextTrackLoader&) = 0;
};

class TextTrackLoader : public CachedResourceClient, private WebVTTParserClient {
    WTF_MAKE_NONCOPYABLE(TextTrackLoader);
    WTF_MAKE_FAST_ALLOCATED;
public:
    TextTrackLoader(TextTrackLoaderClient&, ScriptExecutionContext*);
    virtual ~TextTrackLoader();

    bool load(const URL&, HTMLTrackElement&);
    void cancelLoad();
    void getNewCues(Vector<RefPtr<TextTrackCue>>& outputCues);
    void getNewRegions(Vector<RefPtr<VTTRegion>>& outputRegions);
    Vector<String> getNewStyleSheets();
private:

    // CachedResourceClient
    void notifyFinished(CachedResource&) override;
    void deprecatedDidReceiveCachedResource(CachedResource&) override;

    // WebVTTParserClient
    void newCuesParsed() override;
    void newRegionsParsed() override;
    void newStyleSheetsParsed() final;
    void fileFailedToParse() override;

    void processNewCueData(CachedResource&);
    void cueLoadTimerFired();
    void corsPolicyPreventedLoad();

    enum State { Idle, Loading, Finished, Failed };

    TextTrackLoaderClient& m_client;
    std::unique_ptr<WebVTTParser> m_cueParser;
    CachedResourceHandle<CachedTextTrack> m_resource;
    ScriptExecutionContext* m_scriptExecutionContext;
    Timer m_cueLoadTimer;
    State m_state;
    unsigned m_parseOffset;
    bool m_newCuesAvailable;
};

} // namespace WebCore

#endif // ENABLE(VIDEO_TRACK)
