/*
 * CSS Media Query Evaluator
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
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

#include "MediaQueryExpression.h"
#include <wtf/WeakPtr.h>

namespace WebCore {

class Document;
class Frame;
class MediaQuerySet;
class RenderStyle;
class StyleResolver;

struct MediaQueryResult {
    MediaQueryExpression expression;
    bool result;
};

// Some of the constructors are used for cases where the device characteristics are not known.
// These  can be used to prune the loading of stylesheets to only those which are not already known to not match.

class MediaQueryEvaluator {
public:
    // Creates evaluator which evaluates only simple media queries.
    // Evaluator returns true for "all", and returns value of \mediaFeatureResult for any media features.
    explicit MediaQueryEvaluator(bool mediaFeatureResult = false);

    // Creates evaluator which evaluates only simple media queries.
    // Evaluator returns true for acceptedMediaType and returns value of \mediaFeatureResult for any media features.
    MediaQueryEvaluator(const String& acceptedMediaType, bool mediaFeatureResult = false);

    // Creates evaluator which evaluates full media queries.
    WEBCORE_EXPORT MediaQueryEvaluator(const String& acceptedMediaType, const Document&, const RenderStyle*);

    bool mediaTypeMatch(const String& mediaTypeToMatch) const;
    bool mediaTypeMatchSpecific(const char* mediaTypeToMatch) const;

    // Evaluates a list of media queries.
    WEBCORE_EXPORT bool evaluate(const MediaQuerySet&, StyleResolver* = nullptr) const;

    // Evaluates media query subexpression, ie "and (media-feature: value)" part.
    bool evaluate(const MediaQueryExpression&) const;

    // Evaluates a list of media queries and fills in a vector with any viewport-dependent results found.
    bool evaluate(const MediaQuerySet&, Vector<MediaQueryResult>&) const;

    static bool mediaAttributeMatches(Document&, const String& attributeValue);

private:
    String m_mediaType;
    WeakPtr<Document> m_document;
    const RenderStyle* m_style { nullptr };
    bool m_fallbackResult { false };
};

} // namespace
