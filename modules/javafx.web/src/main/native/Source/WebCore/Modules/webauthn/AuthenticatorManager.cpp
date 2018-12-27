/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
#include "AuthenticatorManager.h"

#if ENABLE(WEB_AUTHN)

#include "AbortSignal.h"
#include "AuthenticatorAssertionResponse.h"
#include "AuthenticatorAttestationResponse.h"
#include "CredentialsMessenger.h"
#include "JSBasicCredential.h"
#include "PublicKeyCredential.h"
#include "PublicKeyCredentialCreationOptions.h"
#include "PublicKeyCredentialRequestOptions.h"
#include "SecurityOrigin.h"
#include "Timer.h"
#include <pal/crypto/CryptoDigest.h>
#include <wtf/JSONValues.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/Base64.h>

namespace WebCore {

namespace AuthenticatorManagerInternal {

enum class ClientDataType {
    Create,
    Get
};

// FIXME(181948): Add token binding ID and extensions.
static Ref<ArrayBuffer> produceClientDataJson(ClientDataType type, const BufferSource& challenge, const SecurityOrigin& origin)
{
    auto object = JSON::Object::create();
    switch (type) {
    case ClientDataType::Create:
        object->setString("type"_s, "webauthn.create"_s);
        break;
    case ClientDataType::Get:
        object->setString("type"_s, "webauthn.get"_s);
        break;
    }
    object->setString("challenge"_s, WTF::base64URLEncode(challenge.data(), challenge.length()));
    object->setString("origin"_s, origin.toRawString());
    // FIXME: This might be platform dependent.
    object->setString("hashAlgorithm"_s, "SHA-256"_s);

    auto utf8JSONString = object->toJSONString().utf8();
    return ArrayBuffer::create(utf8JSONString.data(), utf8JSONString.length());
}

static Vector<uint8_t> produceClientDataJsonHash(const ArrayBuffer& clientDataJson)
{
    // FIXME: This might be platform dependent.
    auto crypto = PAL::CryptoDigest::create(PAL::CryptoDigest::Algorithm::SHA_256);
    crypto->addBytes(clientDataJson.data(), clientDataJson.byteLength());
    return crypto->computeHash();
}

// FIXME(181947): We should probably trim timeOutInMs to some max allowable number.
static std::unique_ptr<Timer> initTimeoutTimer(std::optional<unsigned long> timeOutInMs, const CredentialPromise& promise)
{
    if (!timeOutInMs)
        return nullptr;

    auto timer = std::make_unique<Timer>([promise = promise] () mutable {
        promise.reject(Exception { NotAllowedError, "Operation timed out."_s });
    });
    timer->startOneShot(Seconds::fromMilliseconds(*timeOutInMs));
    return timer;
}

static bool didTimeoutTimerFire(Timer* timer)
{
    if (!timer)
        return false;
    if (!timer->isActive())
        return true;
    timer->stop();
    return false;
}

} // namespace AuthenticatorManagerInternal

AuthenticatorManager& AuthenticatorManager::singleton()
{
    ASSERT(isMainThread());
    static NeverDestroyed<AuthenticatorManager> authenticator;
    return authenticator;
}

void AuthenticatorManager::setMessenger(CredentialsMessenger& messenger)
{
    m_messenger = makeWeakPtr(messenger);
}

void AuthenticatorManager::create(const SecurityOrigin& callerOrigin, const PublicKeyCredentialCreationOptions& options, bool sameOriginWithAncestors, RefPtr<AbortSignal>&& abortSignal, CredentialPromise&& promise) const
{
    using namespace AuthenticatorManagerInternal;

    // The following implements https://www.w3.org/TR/webauthn/#createCredential as of 5 December 2017.
    // FIXME: Extensions are not supported yet. Skip Step 11-12.
    // Step 1, 3, 16 are handled by the caller.
    // Step 2.
    if (!sameOriginWithAncestors) {
        promise.reject(Exception { NotAllowedError, "The origin of the document is not the same as its ancestors."_s });
        return;
    }

    // Step 4 & 17.
    std::unique_ptr<Timer> timeoutTimer = initTimeoutTimer(options.timeout, promise);

    // Step 5-7.
    // FIXME(181950): We lack fundamental support from SecurityOrigin to determine if a host is a valid domain or not.
    // Step 6 is therefore skipped. Also, we lack the support to determine whether a domain is a registrable
    // domain suffix of another domain. Hence restrict the comparison to equal in Step 7.
    if (!options.rp.id.isEmpty() && callerOrigin.host() != options.rp.id) {
        promise.reject(Exception { SecurityError, "The origin of the document is not a registrable domain suffix of the provided RP ID."_s });
        return;
    }
    if (options.rp.id.isEmpty())
        options.rp.id = callerOrigin.host();

    // Step 8-10.
    // Most of the jobs are done by bindings. However, we can't know if the JSValue of options.pubKeyCredParams
    // is empty or not. Return NotSupportedError as long as it is empty.
    if (options.pubKeyCredParams.isEmpty()) {
        promise.reject(Exception { NotSupportedError, "No desired properties of the to be created credential are provided."_s });
        return;
    }

    // Step 13-15.
    auto clientDataJson = produceClientDataJson(ClientDataType::Create, options.challenge, callerOrigin);
    auto clientDataJsonHash = produceClientDataJsonHash(clientDataJson);

    // Step 18-21.
    // Only platform attachments will be supported at this stage. Assuming one authenticator per device.
    // Also, resident keys, user verifications and direct attestation are enforced at this tage.
    // For better performance, transports of options.excludeCredentials are checked in LocalAuthenticator.
    if (!m_messenger)  {
        promise.reject(Exception { UnknownError, "Unknown internal error."_s });
        return;
    }

    auto completionHandler = [clientDataJson = WTFMove(clientDataJson), promise = WTFMove(promise), timeoutTimer = WTFMove(timeoutTimer), abortSignal = WTFMove(abortSignal)] (ExceptionOr<CreationReturnBundle>&& result) mutable {
        if (didTimeoutTimerFire(timeoutTimer.get()))
            return;
        if (abortSignal && abortSignal->aborted()) {
            promise.reject(Exception { AbortError, "Aborted by AbortSignal."_s });
            return;
        }
        if (result.hasException()) {
            promise.reject(result.exception());
            return;
        }

        auto bundle = result.releaseReturnValue();
        promise.resolve(PublicKeyCredential::create(WTFMove(bundle.credentialId), AuthenticatorAttestationResponse::create(WTFMove(clientDataJson), ArrayBuffer::create(WTFMove(bundle.attestationObject)))).ptr());
    };
    // Async operations are dispatched and handled in the messenger.
    m_messenger->makeCredential(clientDataJsonHash, options, WTFMove(completionHandler));
}

void AuthenticatorManager::discoverFromExternalSource(const SecurityOrigin& callerOrigin, const PublicKeyCredentialRequestOptions& options, bool sameOriginWithAncestors, RefPtr<AbortSignal>&& abortSignal, CredentialPromise&& promise) const
{
    using namespace AuthenticatorManagerInternal;

    // The following implements https://www.w3.org/TR/webauthn/#createCredential as of 5 December 2017.
    // FIXME: Extensions are not supported yet. Skip Step 8-9.
    // Step 1, 3, 13 are handled by the caller.
    // Step 2.
    if (!sameOriginWithAncestors) {
        promise.reject(Exception { NotAllowedError, "The origin of the document is not the same as its ancestors."_s });
        return;
    }

    // Step 4 & 16.
    std::unique_ptr<Timer> timeoutTimer = initTimeoutTimer(options.timeout, promise);

    // Step 5-7.
    // FIXME(181950): We lack fundamental support from SecurityOrigin to determine if a host is a valid domain or not.
    // Step 6 is therefore skipped. Also, we lack the support to determine whether a domain is a registrable
    // domain suffix of another domain. Hence restrict the comparison to equal in Step 7.
    if (!options.rpId.isEmpty() && callerOrigin.host() != options.rpId) {
        promise.reject(Exception { SecurityError, "The origin of the document is not a registrable domain suffix of the provided RP ID."_s });
        return;
    }
    if (options.rpId.isEmpty())
        options.rpId = callerOrigin.host();

    // Step 10-12.
    auto clientDataJson = produceClientDataJson(ClientDataType::Get, options.challenge, callerOrigin);
    auto clientDataJsonHash = produceClientDataJsonHash(clientDataJson);

    // Step 14-15, 17-19.
    // Only platform attachments will be supported at this stage. Assuming one authenticator per device.
    // Also, resident keys, user verifications and direct attestation are enforced at this tage.
    // For better performance, filtering of options.allowCredentials is done in LocalAuthenticator.
    if (!m_messenger)  {
        promise.reject(Exception { UnknownError, "Unknown internal error."_s });
        return;
    }

    auto completionHandler = [clientDataJson = WTFMove(clientDataJson), promise = WTFMove(promise), timeoutTimer = WTFMove(timeoutTimer), abortSignal = WTFMove(abortSignal)] (ExceptionOr<AssertionReturnBundle>&& result) mutable {
        if (didTimeoutTimerFire(timeoutTimer.get()))
            return;
        if (abortSignal && abortSignal->aborted()) {
            promise.reject(Exception { AbortError, "Aborted by AbortSignal."_s });
            return;
        }
        if (result.hasException()) {
            promise.reject(result.exception());
            return;
        }

        auto bundle = result.releaseReturnValue();
        promise.resolve(PublicKeyCredential::create(WTFMove(bundle.credentialId), AuthenticatorAssertionResponse::create(WTFMove(clientDataJson), WTFMove(bundle.authenticatorData), WTFMove(bundle.signature), WTFMove(bundle.userHandle))).ptr());
    };
    // Async operations are dispatched and handled in the messenger.
    m_messenger->getAssertion(clientDataJsonHash, options, WTFMove(completionHandler));
}

void AuthenticatorManager::isUserVerifyingPlatformAuthenticatorAvailable(DOMPromiseDeferred<IDLBoolean>&& promise) const
{
    // The following implements https://www.w3.org/TR/webauthn/#isUserVerifyingPlatformAuthenticatorAvailable
    // as of 5 December 2017.
    if (!m_messenger)  {
        promise.reject(Exception { UnknownError, "Unknown internal error."_s });
        return;
    }

    // FIXME(182767): We should consider more on the assessment of the return value. Right now, we return true/false
    // immediately according to platform specific procedures.
    auto completionHandler = [promise = WTFMove(promise)] (bool result) mutable {
        promise.resolve(result);
    };
    // Async operation are dispatched and handled in the messenger.
    m_messenger->isUserVerifyingPlatformAuthenticatorAvailable(WTFMove(completionHandler));
}

} // namespace WebCore

#endif // ENABLE(WEB_AUTHN)
