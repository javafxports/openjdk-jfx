/*
 * Copyright (C) 2017-2018 Apple Inc. All rights reserved.
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

#include "ApplePayLineItem.h"
#include "MockPaymentAddress.h"
#include "PaymentCoordinatorClient.h"
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class MainFrame;
struct ApplePayPaymentMethod;

class MockPaymentCoordinator final : public PaymentCoordinatorClient {
public:
    explicit MockPaymentCoordinator(MainFrame&);

    void setShippingAddress(MockPaymentAddress&& shippingAddress) { m_shippingAddress = WTFMove(shippingAddress); }
    void changeShippingOption(String&& shippingOption);
    void changePaymentMethod(ApplePayPaymentMethod&&);
    void acceptPayment();
    void cancelPayment();

    const ApplePayLineItem& total() const { return m_total; }
    const Vector<ApplePayLineItem>& lineItems() const { return m_lineItems; }

    void ref() const { }
    void deref() const { }

private:
    bool supportsVersion(unsigned) final;
    std::optional<String> validatedPaymentNetwork(const String&) final;
    bool canMakePayments() final;
    void canMakePaymentsWithActiveCard(const String&, const String&, WTF::Function<void(bool)>&&);
    void openPaymentSetup(const String&, const String&, WTF::Function<void(bool)>&&);
    bool showPaymentUI(const URL&, const Vector<URL>&, const ApplePaySessionPaymentRequest&) final;
    void completeMerchantValidation(const PaymentMerchantSession&) final;
    void completeShippingMethodSelection(std::optional<ShippingMethodUpdate>&&) final;
    void completeShippingContactSelection(std::optional<ShippingContactUpdate>&&) final;
    void completePaymentMethodSelection(std::optional<PaymentMethodUpdate>&&) final;
    void completePaymentSession(std::optional<PaymentAuthorizationResult>&&) final;
    void abortPaymentSession() final;
    void cancelPaymentSession() final;
    void paymentCoordinatorDestroyed() final;

    void updateTotalAndLineItems(const ApplePaySessionPaymentRequest::TotalAndLineItems&);

    MainFrame& m_mainFrame;
    ApplePayPaymentContact m_shippingAddress;
    ApplePayLineItem m_total;
    Vector<ApplePayLineItem> m_lineItems;
    HashSet<String, ASCIICaseInsensitiveHash> m_availablePaymentNetworks;
};

} // namespace WebCore

#endif // ENABLE(APPLE_PAY)
