/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
#include "MediaCapabilities.h"

#include "ContentType.h"
#include "MediaDecodingConfiguration.h"
#include "MediaEncodingConfiguration.h"
#include <wtf/HashSet.h>

namespace WebCore {

static const HashSet<String>& bucketMIMETypes()
{
    // A "bucket" MIME types is one whose container type does not uniquely specify a codec.
    // See: https://tools.ietf.org/html/rfc6381
    static NeverDestroyed<HashSet<String>> bucketMIMETypes = HashSet<String>({
        "audio/3gpp",
        "video/3gpp",
        "audio/3gpp2",
        "video/3gpp2",
        "audio/mp4",
        "video/mp4",
        "application/mp4",
        "video/quicktime",
        "application/mp21",
        "audio/vnd.apple.mpegurl",
        "video/vnd.apple.mpegurl",
        "audio/ogg",
        "video/ogg",
        "video/webm",
        "audio/webm",
    });
    return bucketMIMETypes;
}

static bool isValidMediaMIMEType(const ContentType& contentType)
{
    // 2.1.4. MIME types
    // https://wicg.github.io/media-capabilities/#valid-media-mime-type
    // A valid media MIME type is a string that is a valid MIME type per [mimesniff]. If the MIME type does
    // not imply a codec, the string MUST also have one and only one parameter that is named codecs with a
    // value describing a single media codec. Otherwise, it MUST contain no parameters.
    if (contentType.isEmpty())
        return false;

    auto codecs = contentType.codecs();

    // FIXME: The spec requires that the "codecs" parameter is the only parameter present.
    if (bucketMIMETypes().contains(contentType.containerType()))
        return codecs.size() == 1;
    return !codecs.size();
}

static bool isValidVideoMIMEType(const ContentType& contentType)
{
    // 2.1.4 MIME Types
    // https://wicg.github.io/media-capabilities/#valid-video-mime-type
    // A valid video MIME type is a string that is a valid media MIME type and for which the type per [RFC7231]
    // is either video or application.
    if (!isValidMediaMIMEType(contentType))
        return false;

    auto containerType = contentType.containerType();
    if (!startsWithLettersIgnoringASCIICase(containerType, "video/") && !startsWithLettersIgnoringASCIICase(containerType, "application/"))
        return false;

    return true;
}

static bool isValidAudioMIMEType(const ContentType& contentType)
{
    // 2.1.4 MIME Types
    // https://wicg.github.io/media-capabilities/#valid-audio-mime-type
    // A valid audio MIME type is a string that is a valid media MIME type and for which the type per [RFC7231]
    // is either audio or application.
    if (!isValidMediaMIMEType(contentType))
        return false;

    auto containerType = contentType.containerType();
    if (!startsWithLettersIgnoringASCIICase(containerType, "audio/") && !startsWithLettersIgnoringASCIICase(containerType, "application/"))
        return false;

    return true;
}

static bool isValidVideoConfiguration(const VideoConfiguration& configuration)
{
    // 2.1.5. VideoConfiguration
    // https://wicg.github.io/media-capabilities/#valid-video-configuration
    // 1. If configuration’s contentType is not a valid video MIME type, return false and abort these steps.
    if (!isValidVideoMIMEType(ContentType(configuration.contentType)))
        return false;

    // 2. If none of the following is true, return false and abort these steps:
    //   o. Applying the rules for parsing floating-point number values to configuration’s framerate
    //      results in a number that is finite and greater than 0.
    bool ok = false;
    double framerate = configuration.framerate.toDouble(&ok);
    if (ok && std::isfinite(framerate) && framerate > 0)
        return true;

    //   o. Configuration’s framerate contains one occurence of U+002F SLASH character (/) and the substrings
    //      before and after this character, when applying the rules for parsing floating-point number values
    //      results in a number that is finite and greater than 0.
    auto frameratePieces = configuration.framerate.split('/');
    if (frameratePieces.size() != 2)
        return false;

    double numerator = frameratePieces[0].toDouble(&ok);
    if (!ok)
        return false;

    double denominator = frameratePieces[1].toDouble(&ok);
    if (!ok)
        return false;

    if (!std::isfinite(numerator) || !std::isfinite(denominator))
        return false;

    framerate = numerator / denominator;
    if (!std::isfinite(framerate) || framerate <= 0)
        return false;

    // 3. Return true.
    return true;
}

static bool isValidAudioConfiguration(const AudioConfiguration& configuration)
{
    // 2.1.6. AudioConfiguration
    // https://wicg.github.io/media-capabilities/#audioconfiguration
    // 1. If configuration’s contentType is not a valid audio MIME type, return false and abort these steps.
    if (!isValidAudioMIMEType(ContentType(configuration.contentType)))
        return false;

    // 2. Return true.
    return true;
}

static bool isValidMediaConfiguration(const MediaConfiguration& configuration)
{
    // 2.1.1. MediaConfiguration
    // https://wicg.github.io/media-capabilities/#mediaconfiguration
    // For a MediaConfiguration to be a valid MediaConfiguration, audio or video MUST be present.
    if (!configuration.video && !configuration.audio)
        return false;

    if (configuration.video && !isValidVideoConfiguration(configuration.video.value()))
        return false;

    if (configuration.audio && !isValidAudioConfiguration(configuration.audio.value()))
        return false;

    return true;
}

void MediaCapabilities::decodingInfo(MediaDecodingConfiguration&& configuration, Ref<DeferredPromise>&& promise)
{
    // 2.4 Media Capabilities Interface
    // https://wicg.github.io/media-capabilities/#media-capabilities-interface

    // 1. If configuration is not a valid MediaConfiguration, return a Promise rejected with a TypeError.
    // 2. If configuration.video is present and is not a valid video configuration, return a Promise rejected with a TypeError.
    // 3. If configuration.audio is present and is not a valid audio configuration, return a Promise rejected with a TypeError.
    if (!isValidMediaConfiguration(configuration)) {
        promise->reject(TypeError);
        return;
    }

    // 4. Let p be a new promise.
    // 5. In parallel, run the create a MediaCapabilitiesInfo algorithm with configuration and resolve p with its result.
    // 6. Return p.
    m_taskQueue.enqueueTask([configuration = WTFMove(configuration), promise = WTFMove(promise)] () mutable {
        UNUSED_PARAM(configuration);
        UNUSED_PARAM(promise);
    });
}

void MediaCapabilities::encodingInfo(MediaEncodingConfiguration&& configuration, Ref<DeferredPromise>&& promise)
{
    // 2.4 Media Capabilities Interface
    // https://wicg.github.io/media-capabilities/#media-capabilities-interface

    // 1. If configuration is not a valid MediaConfiguration, return a Promise rejected with a TypeError.
    // 2. If configuration.video is present and is not a valid video configuration, return a Promise rejected with a TypeError.
    // 3. If configuration.audio is present and is not a valid audio configuration, return a Promise rejected with a TypeError.
    if (!isValidMediaConfiguration(configuration)) {
        promise->reject(TypeError);
        return;
    }

    // 4. Let p be a new promise.
    // 5. In parallel, run the create a MediaCapabilitiesInfo algorithm with configuration and resolve p with its result.
    // 6. Return p.
    m_taskQueue.enqueueTask([configuration = WTFMove(configuration), promise = WTFMove(promise)] () mutable {
        UNUSED_PARAM(configuration);
        UNUSED_PARAM(promise);
    });
}

}
