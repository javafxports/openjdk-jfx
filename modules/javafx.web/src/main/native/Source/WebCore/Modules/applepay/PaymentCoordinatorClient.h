/*
 * Copyright (C) 2015-2018 Apple Inc. All rights reserved.
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

#if ENABLE(APPLE_PAY)

#include "ApplePaySessionPaymentRequest.h"
#include <wtf/Forward.h>
#include <wtf/Function.h>

namespace WebCore {

class PaymentMerchantSession;
class URL;
struct PaymentAuthorizationResult;
struct PaymentMethodUpdate;
struct ShippingContactUpdate;
struct ShippingMethodUpdate;

class PaymentCoordinatorClient {
public:
    virtual bool supportsVersion(unsigned version) = 0;
    virtual std::optional<String> validatedPaymentNetwork(const String&) = 0;
    virtual bool canMakePayments() = 0;
    virtual void canMakePaymentsWithActiveCard(const String& merchantIdentifier, const String& domainName, WTF::Function<void (bool)>&& completionHandler) = 0;
    virtual void openPaymentSetup(const String& merchantIdentifier, const String& domainName, WTF::Function<void (bool)>&& completionHandler) = 0;

    virtual bool showPaymentUI(const URL& originatingURL, const Vector<URL>& linkIconURLs, const ApplePaySessionPaymentRequest&) = 0;
    virtual void completeMerchantValidation(const PaymentMerchantSession&) = 0;
    virtual void completeShippingMethodSelection(std::optional<ShippingMethodUpdate>&&) = 0;
    virtual void completeShippingContactSelection(std::optional<ShippingContactUpdate>&&) = 0;
    virtual void completePaymentMethodSelection(std::optional<PaymentMethodUpdate>&&) = 0;
    virtual void completePaymentSession(std::optional<PaymentAuthorizationResult>&&) = 0;
    virtual void abortPaymentSession() = 0;
    virtual void cancelPaymentSession() = 0;
    virtual void paymentCoordinatorDestroyed() = 0;

protected:
    virtual ~PaymentCoordinatorClient() = default;
};

}

#endif
