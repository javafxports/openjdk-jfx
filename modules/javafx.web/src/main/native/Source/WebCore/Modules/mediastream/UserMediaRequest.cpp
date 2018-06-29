/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include "config.h"
#include "UserMediaRequest.h"

#if ENABLE(MEDIA_STREAM)

#include "CaptureDeviceManager.h"
#include "DeprecatedGlobalSettings.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "HTMLIFrameElement.h"
#include "HTMLParserIdioms.h"
#include "JSMediaStream.h"
#include "JSOverconstrainedError.h"
#include "Logging.h"
#include "MainFrame.h"
#include "MediaConstraints.h"
#include "RealtimeMediaSourceCenter.h"
#include "SchemeRegistry.h"
#include "Settings.h"
#include "UserMediaController.h"

namespace WebCore {

RefPtr<UserMediaRequest> UserMediaRequest::create(Document& document, MediaStreamRequest&& request, DOMPromiseDeferred<IDLInterface<MediaStream>>&& promise)
{
    auto result = adoptRef(new UserMediaRequest(document, WTFMove(request), WTFMove(promise)));
    result->suspendIfNeeded();
    return result;
}

UserMediaRequest::UserMediaRequest(Document& document, MediaStreamRequest&& request, DOMPromiseDeferred<IDLInterface<MediaStream>>&& promise)
    : ActiveDOMObject(&document)
    , m_promise(WTFMove(promise))
    , m_request(WTFMove(request))
{
}

UserMediaRequest::~UserMediaRequest() = default;

SecurityOrigin* UserMediaRequest::userMediaDocumentOrigin() const
{
    if (!m_scriptExecutionContext)
        return nullptr;
    return m_scriptExecutionContext->securityOrigin();
}

SecurityOrigin* UserMediaRequest::topLevelDocumentOrigin() const
{
    if (!m_scriptExecutionContext)
        return nullptr;
    return &m_scriptExecutionContext->topOrigin();
}

static bool isSecure(DocumentLoader& documentLoader)
{
    auto& response = documentLoader.response();
    if (SecurityOrigin::isLocalHostOrLoopbackIPAddress(documentLoader.response().url().host()))
        return true;
    return SchemeRegistry::shouldTreatURLSchemeAsSecure(response.url().protocol().toStringWithoutCopying())
        && response.certificateInfo()
        && !response.certificateInfo()->containsNonRootSHA1SignedCertificate();
}

static bool isAllowedToUse(Document& document, Document& topDocument, bool requiresAudio, bool requiresVideo)
{
    if (&document == &topDocument)
        return true;

    auto* parentDocument = document.parentDocument();
    if (!parentDocument)
        return false;

    if (document.securityOrigin().isSameSchemeHostPort(parentDocument->securityOrigin()))
        return true;

    auto* element = document.ownerElement();
    ASSERT(element);
    if (!element)
        return false;

    if (!is<HTMLIFrameElement>(*element))
        return false;
    auto& allow = downcast<HTMLIFrameElement>(*element).allow();

    bool allowCameraAccess = false;
    bool allowMicrophoneAccess = false;
    for (auto allowItem : StringView { allow }.split(';')) {
        auto item = allowItem.stripLeadingAndTrailingMatchedCharacters(isHTMLSpace<UChar>);
        if (!allowCameraAccess && item == "camera")
            allowCameraAccess = true;
        else if (!allowMicrophoneAccess && item == "microphone")
            allowMicrophoneAccess = true;
    }
    return (allowCameraAccess || !requiresVideo) && (allowMicrophoneAccess || !requiresAudio);
}

static bool canCallGetUserMedia(Document& document, bool wantsAudio, bool wantsVideo, String& errorMessage)
{
    ASSERT(wantsAudio || wantsVideo);

    bool requiresSecureConnection = DeprecatedGlobalSettings::mediaCaptureRequiresSecureConnection();
    auto& documentLoader = *document.loader();
    if (requiresSecureConnection && !isSecure(documentLoader)) {
        errorMessage = "Trying to call getUserMedia from an insecure document.";
        return false;
    }

    auto& topDocument = document.topDocument();
    if (&document != &topDocument) {
        for (auto* ancestorDocument = &document; ancestorDocument != &topDocument; ancestorDocument = ancestorDocument->parentDocument()) {
            if (requiresSecureConnection && !isSecure(*ancestorDocument->loader())) {
                errorMessage = "Trying to call getUserMedia from a document with an insecure parent frame.";
                return false;
            }

            if (!isAllowedToUse(*ancestorDocument, topDocument, wantsAudio, wantsVideo)) {
                errorMessage = "The top-level frame has prevented a document with a different security origin to call getUserMedia.";
                return false;
            }
        }
    }

    return true;
}

void UserMediaRequest::start()
{
    ASSERT(m_scriptExecutionContext);
    if (!m_scriptExecutionContext) {
        deny(MediaAccessDenialReason::UserMediaDisabled);
        return;
    }

    if (m_request.type == MediaStreamRequest::Type::DisplayMedia) {
        // https://w3c.github.io/mediacapture-screen-share/#constraints
        // 5.2 Constraining Display Surface Selection
        // The getDisplayMedia function does not permit the use of constraints for selection of a source as described
        // in the getUserMedia() algorithm. Prior to invoking the getUserMedia() algorithm, if either of the video
        // and audio attributes are set to a MediaTrackConstraints value (as opposed to being absent or set to a
        // Boolean value), reject the promise with a InvalidAccessError and abort.
        if (m_request.videoConstraints.isValid && !(m_request.videoConstraints.mandatoryConstraints.isEmpty() && m_request.videoConstraints.advancedConstraints.isEmpty())) {
            deny(MediaAccessDenialReason::InvalidAccess);
            return;
        }

        if (m_request.audioConstraints.isValid && !(m_request.audioConstraints.mandatoryConstraints.isEmpty() && m_request.audioConstraints.advancedConstraints.isEmpty())) {
            deny(MediaAccessDenialReason::InvalidAccess);
            return;
        }
    }

    // https://w3c.github.io/mediacapture-main/getusermedia.html#dom-mediadevices-getusermedia()
    // 1. Let constraints be the method's first argument.
    // 2. Let requestedMediaTypes be the set of media types in constraints with either a dictionary
    //    value or a value of "true".
    // 3. If requestedMediaTypes is the empty set, return a promise rejected with a TypeError. The word
    //    "optional" occurs in the WebIDL due to WebIDL rules, but the argument must be supplied in order
    //    for the call to succeed.
    if (!m_request.audioConstraints.isValid && !m_request.videoConstraints.isValid) {
        deny(MediaAccessDenialReason::NoConstraints);
        return;
    }

    // 4. If the current settings object's responsible document is NOT allowed to use the feature indicated by
    //    attribute name allowusermedia, return a promise rejected with a DOMException object whose name
    //    attribute has the value SecurityError.
    auto& document = downcast<Document>(*m_scriptExecutionContext);
    auto* controller = UserMediaController::from(document.page());
    if (!controller) {
        deny(MediaAccessDenialReason::UserMediaDisabled);
        return;
    }

    // 6.3 Optionally, e.g., based on a previously-established user preference, for security reasons,
    //     or due to platform limitations, jump to the step labeled Permission Failure below.
    // ...
    // 6.10 Permission Failure: Reject p with a new DOMException object whose name attribute has
    //      the value NotAllowedError.
    String errorMessage;
    if (!canCallGetUserMedia(document, m_request.audioConstraints.isValid, m_request.videoConstraints.isValid, errorMessage)) {
        deny(MediaAccessDenialReason::PermissionDenied);
        document.domWindow()->printErrorMessage(errorMessage);
        return;
    }

    controller->requestUserMediaAccess(*this);
}

void UserMediaRequest::allow(CaptureDevice&& audioDevice, CaptureDevice&& videoDevice, String&& deviceIdentifierHashSalt)
{
    RELEASE_LOG(MediaStream, "UserMediaRequest::allow %s %s", audioDevice ? audioDevice.persistentId().utf8().data() : "", videoDevice ? videoDevice.persistentId().utf8().data() : "");

    auto callback = [this, protector = makePendingActivity(*this)](RefPtr<MediaStreamPrivate>&& privateStream) mutable {
        if (!m_scriptExecutionContext)
            return;

        if (!privateStream) {
            deny(MediaAccessDenialReason::HardwareError);
            return;
        }
        privateStream->monitorOrientation(downcast<Document>(m_scriptExecutionContext)->orientationNotifier());

        auto stream = MediaStream::create(*m_scriptExecutionContext, privateStream.releaseNonNull());
        if (stream->getTracks().isEmpty()) {
            deny(MediaAccessDenialReason::HardwareError);
            return;
        }

        m_pendingActivationMediaStream = PendingActivationMediaStream::create(WTFMove(protector), *this, WTFMove(stream));
    };

    m_request.audioConstraints.deviceIDHashSalt = deviceIdentifierHashSalt;
    m_request.videoConstraints.deviceIDHashSalt = WTFMove(deviceIdentifierHashSalt);

    RealtimeMediaSourceCenter::singleton().createMediaStream(WTFMove(callback), WTFMove(audioDevice), WTFMove(videoDevice), m_request);

    if (!m_scriptExecutionContext)
        return;

#if ENABLE(WEB_RTC)
    auto& document = downcast<Document>(*m_scriptExecutionContext);
    if (auto* page = document.page())
        page->rtcController().disableICECandidateFilteringForDocument(document);
#endif
}

void UserMediaRequest::deny(MediaAccessDenialReason reason, const String& invalidConstraint)
{
    if (!m_scriptExecutionContext)
        return;

    switch (reason) {
    case MediaAccessDenialReason::NoConstraints:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - no constraints");
        m_promise.reject(TypeError);
        break;
    case MediaAccessDenialReason::UserMediaDisabled:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - user media disabled");
        m_promise.reject(SecurityError);
        break;
    case MediaAccessDenialReason::NoCaptureDevices:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - no capture devices");
        m_promise.reject(NotFoundError);
        break;
    case MediaAccessDenialReason::InvalidConstraint:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - invalid constraint - %s", invalidConstraint.utf8().data());
        m_promise.rejectType<IDLInterface<OverconstrainedError>>(OverconstrainedError::create(invalidConstraint, ASCIILiteral("Invalid constraint")).get());
        break;
    case MediaAccessDenialReason::HardwareError:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - hardware error");
        m_promise.reject(NotReadableError);
        break;
    case MediaAccessDenialReason::OtherFailure:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - other failure");
        m_promise.reject(AbortError);
        break;
    case MediaAccessDenialReason::PermissionDenied:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - permission denied");
        m_promise.reject(NotAllowedError);
        break;
    case MediaAccessDenialReason::InvalidAccess:
        RELEASE_LOG(MediaStream, "UserMediaRequest::deny - invalid access");
        m_promise.reject(InvalidAccessError);
        break;
    }
}

void UserMediaRequest::stop()
{
    // Protecting 'this' since nulling m_pendingActivationMediaStream might destroy it.
    Ref<UserMediaRequest> protectedThis(*this);

    m_pendingActivationMediaStream = nullptr;

    auto& document = downcast<Document>(*m_scriptExecutionContext);
    if (auto* controller = UserMediaController::from(document.page()))
        controller->cancelUserMediaAccessRequest(*this);
}

const char* UserMediaRequest::activeDOMObjectName() const
{
    return "UserMediaRequest";
}

bool UserMediaRequest::canSuspendForDocumentSuspension() const
{
    return !hasPendingActivity();
}

Document* UserMediaRequest::document() const
{
    return downcast<Document>(m_scriptExecutionContext);
}

UserMediaRequest::PendingActivationMediaStream::PendingActivationMediaStream(Ref<PendingActivity<UserMediaRequest>>&& protectingUserMediaRequest, UserMediaRequest& userMediaRequest, Ref<MediaStream>&& stream)
    : m_protectingUserMediaRequest(WTFMove(protectingUserMediaRequest))
    , m_userMediaRequest(userMediaRequest)
    , m_mediaStream(WTFMove(stream))
{
    m_mediaStream->privateStream().addObserver(*this);
    m_mediaStream->startProducingData();
}

UserMediaRequest::PendingActivationMediaStream::~PendingActivationMediaStream()
{
    m_mediaStream->privateStream().removeObserver(*this);
}

void UserMediaRequest::PendingActivationMediaStream::characteristicsChanged()
{
    if (m_mediaStream->privateStream().hasVideo() || m_mediaStream->privateStream().hasAudio())
        m_userMediaRequest.mediaStreamIsReady(m_mediaStream.copyRef());
}

void UserMediaRequest::mediaStreamIsReady(Ref<MediaStream>&& stream)
{
    m_promise.resolve(WTFMove(stream));
    // We are in an observer iterator loop, we do not want to change the observers within this loop.
    callOnMainThread([stream = WTFMove(m_pendingActivationMediaStream)] { });
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
