/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All Rights Reserved.
 * Copyright 2010, The Android Open Source Project
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

#if ENABLE(GEOLOCATION)

#include "ActiveDOMObject.h"
#include "Document.h"
#include "Geoposition.h"
#include "PositionCallback.h"
#include "PositionError.h"
#include "PositionErrorCallback.h"
#include "PositionOptions.h"
#include "ScriptWrappable.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>

namespace WebCore {

class Frame;
class GeoNotifier;
class GeolocationError;
class Page;
class ScriptExecutionContext;
class SecurityOrigin;
struct PositionOptions;

class Geolocation : public ScriptWrappable, public RefCounted<Geolocation>, public ActiveDOMObject {
    friend class GeoNotifier;
public:
    static Ref<Geolocation> create(ScriptExecutionContext*);
    WEBCORE_EXPORT ~Geolocation();

    WEBCORE_EXPORT void resetAllGeolocationPermission();
    Document* document() const { return downcast<Document>(scriptExecutionContext()); }
    Frame* frame() const { return document() ? document()->frame() : nullptr; }

    void getCurrentPosition(Ref<PositionCallback>&&, RefPtr<PositionErrorCallback>&&, PositionOptions&&);
    int watchPosition(Ref<PositionCallback>&&, RefPtr<PositionErrorCallback>&&, PositionOptions&&);
    void clearWatch(int watchID);

    WEBCORE_EXPORT void setIsAllowed(bool);
    void resetIsAllowed() { m_allowGeolocation = Unknown; }
    bool isAllowed() const { return m_allowGeolocation == Yes; }

    void positionChanged();
    void setError(GeolocationError&);
    bool shouldBlockGeolocationRequests();

private:
    explicit Geolocation(ScriptExecutionContext*);

    Geoposition* lastPosition();

    // ActiveDOMObject
    void stop() override;
    bool canSuspendForDocumentSuspension() const override;
    void suspend(ReasonForSuspension) override;
    void resume() override;
    const char* activeDOMObjectName() const override;

    bool isDenied() const { return m_allowGeolocation == No; }

    Page* page() const;
    SecurityOrigin* securityOrigin() const;

    typedef Vector<RefPtr<GeoNotifier>> GeoNotifierVector;
    typedef HashSet<RefPtr<GeoNotifier>> GeoNotifierSet;

    class Watchers {
    public:
        bool add(int id, RefPtr<GeoNotifier>&&);
        GeoNotifier* find(int id);
        void remove(int id);
        void remove(GeoNotifier*);
        bool contains(GeoNotifier*) const;
        void clear();
        bool isEmpty() const;
        void getNotifiersVector(GeoNotifierVector&) const;
    private:
        typedef HashMap<int, RefPtr<GeoNotifier>> IdToNotifierMap;
        typedef HashMap<RefPtr<GeoNotifier>, int> NotifierToIdMap;
        IdToNotifierMap m_idToNotifierMap;
        NotifierToIdMap m_notifierToIdMap;
    };

    bool hasListeners() const { return !m_oneShots.isEmpty() || !m_watchers.isEmpty(); }

    void sendError(GeoNotifierVector&, PositionError&);
    void sendPosition(GeoNotifierVector&, Geoposition&);

    static void extractNotifiersWithCachedPosition(GeoNotifierVector& notifiers, GeoNotifierVector* cached);
    static void copyToSet(const GeoNotifierVector&, GeoNotifierSet&);

    static void stopTimer(GeoNotifierVector&);
    void stopTimersForOneShots();
    void stopTimersForWatchers();
    void stopTimers();

    void cancelRequests(GeoNotifierVector&);
    void cancelAllRequests();

    void makeSuccessCallbacks(Geoposition&);
    void handleError(PositionError&);

    void requestPermission();

    bool startUpdating(GeoNotifier*);
    void stopUpdating();

    void handlePendingPermissionNotifiers();

    void startRequest(GeoNotifier*);

    void fatalErrorOccurred(GeoNotifier*);
    void requestTimedOut(GeoNotifier*);
    void requestUsesCachedPosition(GeoNotifier*);
    bool haveSuitableCachedPosition(const PositionOptions&);
    void makeCachedPositionCallbacks();

    GeoNotifierSet m_oneShots;
    Watchers m_watchers;
    GeoNotifierSet m_pendingForPermissionNotifiers;
    RefPtr<Geoposition> m_lastPosition;

    enum {
        Unknown,
        InProgress,
        Yes,
        No
    } m_allowGeolocation;
    bool m_isSuspended;
    bool m_resetOnResume;
    bool m_hasChangedPosition;
    RefPtr<PositionError> m_errorWaitingForResume;

    void resumeTimerFired();
    Timer m_resumeTimer;

    GeoNotifierSet m_requestsAwaitingCachedPosition;
};

} // namespace WebCore

#endif // ENABLE(GEOLOCATION)
