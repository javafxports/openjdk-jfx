/*
 * Copyright (C) 2016-2017 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "ResourceLoadStatistics.h"

#include "KeyedCoding.h"
#include "PublicSuffix.h"
#include <wtf/MainThread.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

static Seconds timestampResolution { 5_s };

typedef WTF::HashMap<String, unsigned, StringHash, HashTraits<String>, HashTraits<unsigned>>::KeyValuePairType ResourceLoadStatisticsValue;

static void encodeHashCountedSet(KeyedEncoder& encoder, const String& label, const HashCountedSet<String>& hashCountedSet)
{
    if (hashCountedSet.isEmpty())
        return;

    encoder.encodeObjects(label, hashCountedSet.begin(), hashCountedSet.end(), [](KeyedEncoder& encoderInner, const ResourceLoadStatisticsValue& origin) {
        encoderInner.encodeString("origin", origin.key);
        encoderInner.encodeUInt32("count", origin.value);
    });
}

static void encodeHashSet(KeyedEncoder& encoder, const String& label, const HashSet<String>& hashSet)
{
    if (hashSet.isEmpty())
        return;

    encoder.encodeObjects(label, hashSet.begin(), hashSet.end(), [](KeyedEncoder& encoderInner, const String& origin) {
        encoderInner.encodeString("origin", origin);
    });
}

void ResourceLoadStatistics::encode(KeyedEncoder& encoder) const
{
    encoder.encodeString("PrevalentResourceOrigin", highLevelDomain);

    encoder.encodeDouble("lastSeen", lastSeen.secondsSinceEpoch().value());

    // User interaction
    encoder.encodeBool("hadUserInteraction", hadUserInteraction);
    encoder.encodeDouble("mostRecentUserInteraction", mostRecentUserInteractionTime.secondsSinceEpoch().value());
    encoder.encodeBool("grandfathered", grandfathered);

    // Storage access
    encodeHashSet(encoder, "storageAccessUnderTopFrameOrigins", storageAccessUnderTopFrameOrigins);

    // Top frame stats
    encodeHashCountedSet(encoder, "topFrameUniqueRedirectsTo", topFrameUniqueRedirectsTo);
    encodeHashCountedSet(encoder, "topFrameUniqueRedirectsFrom", topFrameUniqueRedirectsFrom);

    // Subframe stats
    encodeHashCountedSet(encoder, "subframeUnderTopFrameOrigins", subframeUnderTopFrameOrigins);

    // Subresource stats
    encodeHashCountedSet(encoder, "subresourceUnderTopFrameOrigins", subresourceUnderTopFrameOrigins);
    encodeHashCountedSet(encoder, "subresourceUniqueRedirectsTo", subresourceUniqueRedirectsTo);
    encodeHashCountedSet(encoder, "subresourceUniqueRedirectsFrom", subresourceUniqueRedirectsFrom);

    // Prevalent Resource
    encoder.encodeBool("isPrevalentResource", isPrevalentResource);
    encoder.encodeBool("isVeryPrevalentResource", isVeryPrevalentResource);
    encoder.encodeUInt32("dataRecordsRemoved", dataRecordsRemoved);

    encoder.encodeUInt32("timesAccessedAsFirstPartyDueToUserInteraction", timesAccessedAsFirstPartyDueToUserInteraction);
    encoder.encodeUInt32("timesAccessedAsFirstPartyDueToStorageAccessAPI", timesAccessedAsFirstPartyDueToStorageAccessAPI);
}

static void decodeHashCountedSet(KeyedDecoder& decoder, const String& label, HashCountedSet<String>& hashCountedSet)
{
    Vector<String> ignore;
    decoder.decodeObjects(label, ignore, [&hashCountedSet](KeyedDecoder& decoderInner, String& origin) {
        if (!decoderInner.decodeString("origin", origin))
            return false;

        unsigned count;
        if (!decoderInner.decodeUInt32("count", count))
            return false;

        hashCountedSet.add(origin, count);
        return true;
    });
}

static void decodeHashSet(KeyedDecoder& decoder, const String& label, HashSet<String>& hashSet)
{
    Vector<String> ignore;
    decoder.decodeObjects(label, ignore, [&hashSet](KeyedDecoder& decoderInner, String& origin) {
        if (!decoderInner.decodeString("origin", origin))
            return false;

        hashSet.add(origin);
        return true;
    });
}

bool ResourceLoadStatistics::decode(KeyedDecoder& decoder, unsigned modelVersion)
{
    if (!decoder.decodeString("PrevalentResourceOrigin", highLevelDomain))
        return false;

    // User interaction
    if (!decoder.decodeBool("hadUserInteraction", hadUserInteraction))
        return false;

    // Storage access
    decodeHashSet(decoder, "storageAccessUnderTopFrameOrigins", storageAccessUnderTopFrameOrigins);

    // Top frame stats
    if (modelVersion >= 11) {
        decodeHashCountedSet(decoder, "topFrameUniqueRedirectsTo", topFrameUniqueRedirectsTo);
        decodeHashCountedSet(decoder, "topFrameUniqueRedirectsFrom", topFrameUniqueRedirectsFrom);
    }

    // Subframe stats
    decodeHashCountedSet(decoder, "subframeUnderTopFrameOrigins", subframeUnderTopFrameOrigins);

    // Subresource stats
    decodeHashCountedSet(decoder, "subresourceUnderTopFrameOrigins", subresourceUnderTopFrameOrigins);
    decodeHashCountedSet(decoder, "subresourceUniqueRedirectsTo", subresourceUniqueRedirectsTo);
    if (modelVersion >= 11)
        decodeHashCountedSet(decoder, "subresourceUniqueRedirectsFrom", subresourceUniqueRedirectsFrom);

    // Prevalent Resource
    if (!decoder.decodeBool("isPrevalentResource", isPrevalentResource))
        return false;

    if (modelVersion >= 12) {
        if (!decoder.decodeBool("isVeryPrevalentResource", isVeryPrevalentResource))
            return false;
    }

    if (!decoder.decodeUInt32("dataRecordsRemoved", dataRecordsRemoved))
        return false;

    double mostRecentUserInteractionTimeAsDouble;
    if (!decoder.decodeDouble("mostRecentUserInteraction", mostRecentUserInteractionTimeAsDouble))
        return false;
    mostRecentUserInteractionTime = WallTime::fromRawSeconds(mostRecentUserInteractionTimeAsDouble);

    if (!decoder.decodeBool("grandfathered", grandfathered))
        return false;

    double lastSeenTimeAsDouble;
    if (!decoder.decodeDouble("lastSeen", lastSeenTimeAsDouble))
        return false;
    lastSeen = WallTime::fromRawSeconds(lastSeenTimeAsDouble);

    if (modelVersion >= 11) {
        if (!decoder.decodeUInt32("timesAccessedAsFirstPartyDueToUserInteraction", timesAccessedAsFirstPartyDueToUserInteraction))
            timesAccessedAsFirstPartyDueToUserInteraction = 0;
        if (!decoder.decodeUInt32("timesAccessedAsFirstPartyDueToStorageAccessAPI", timesAccessedAsFirstPartyDueToStorageAccessAPI))
            timesAccessedAsFirstPartyDueToStorageAccessAPI = 0;
    }
    return true;
}

static void appendBoolean(StringBuilder& builder, const String& label, bool flag)
{
    builder.appendLiteral("    ");
    builder.append(label);
    builder.appendLiteral(": ");
    builder.append(flag ? "Yes" : "No");
}

static void appendHashCountedSet(StringBuilder& builder, const String& label, const HashCountedSet<String>& hashCountedSet)
{
    if (hashCountedSet.isEmpty())
        return;

    builder.appendLiteral("    ");
    builder.append(label);
    builder.appendLiteral(":\n");

    for (auto& entry : hashCountedSet) {
        builder.appendLiteral("        ");
        builder.append(entry.key);
        builder.appendLiteral(": ");
        builder.appendNumber(entry.value);
        builder.append('\n');
    }
}

static void appendHashSet(StringBuilder& builder, const String& label, const HashSet<String>& hashSet)
{
    if (hashSet.isEmpty())
        return;

    builder.appendLiteral("    ");
    builder.append(label);
    builder.appendLiteral(":\n");

    for (auto& entry : hashSet) {
        builder.appendLiteral("        ");
        builder.append(entry);
        builder.append('\n');
    }
}

String ResourceLoadStatistics::toString() const
{
    StringBuilder builder;

    builder.appendLiteral("lastSeen");
    builder.appendNumber(lastSeen.secondsSinceEpoch().value());
    builder.append('\n');

    // User interaction
    appendBoolean(builder, "hadUserInteraction", hadUserInteraction);
    builder.append('\n');
    builder.appendLiteral("    mostRecentUserInteraction: ");
    builder.appendNumber(mostRecentUserInteractionTime.secondsSinceEpoch().value());
    builder.append('\n');
    appendBoolean(builder, "    grandfathered", grandfathered);
    builder.append('\n');

    // Storage access
    appendHashSet(builder, "storageAccessUnderTopFrameOrigins", storageAccessUnderTopFrameOrigins);

    // Top frame stats
    appendHashCountedSet(builder, "topFrameUniqueRedirectsTo", topFrameUniqueRedirectsTo);
    appendHashCountedSet(builder, "topFrameUniqueRedirectsFrom", topFrameUniqueRedirectsFrom);

    // Subframe stats
    appendHashCountedSet(builder, "subframeUnderTopFrameOrigins", subframeUnderTopFrameOrigins);

    // Subresource stats
    appendHashCountedSet(builder, "subresourceUnderTopFrameOrigins", subresourceUnderTopFrameOrigins);
    appendHashCountedSet(builder, "subresourceUniqueRedirectsTo", subresourceUniqueRedirectsTo);
    appendHashCountedSet(builder, "subresourceUniqueRedirectsFrom", subresourceUniqueRedirectsFrom);

    // Prevalent Resource
    appendBoolean(builder, "isPrevalentResource", isPrevalentResource);
    appendBoolean(builder, "    isVeryPrevalentResource", isVeryPrevalentResource);
    builder.appendLiteral("    dataRecordsRemoved: ");
    builder.appendNumber(dataRecordsRemoved);
    builder.append('\n');

    // In-memory only
    appendBoolean(builder, "isMarkedForCookieBlocking", isMarkedForCookieBlocking);
    builder.append('\n');

    builder.append('\n');

    return builder.toString();
}

template <typename T>
static void mergeHashCountedSet(HashCountedSet<T>& to, const HashCountedSet<T>& from)
{
    for (auto& entry : from)
        to.add(entry.key, entry.value);
}

template <typename T>
static void mergeHashSet(HashSet<T>& to, const HashSet<T>& from)
{
    for (auto& entry : from)
        to.add(entry);
}

void ResourceLoadStatistics::merge(const ResourceLoadStatistics& other)
{
    ASSERT(other.highLevelDomain == highLevelDomain);

    if (lastSeen < other.lastSeen)
        lastSeen = other.lastSeen;

    if (!other.hadUserInteraction) {
        // If user interaction has been reset do so here too.
        // Else, do nothing.
        if (!other.mostRecentUserInteractionTime) {
            hadUserInteraction = false;
            mostRecentUserInteractionTime = { };
        }
    } else {
        hadUserInteraction = true;
        if (mostRecentUserInteractionTime < other.mostRecentUserInteractionTime)
            mostRecentUserInteractionTime = other.mostRecentUserInteractionTime;
    }
    grandfathered |= other.grandfathered;

    // Storage access
    mergeHashSet(storageAccessUnderTopFrameOrigins, other.storageAccessUnderTopFrameOrigins);

    // Top frame stats
    mergeHashCountedSet(topFrameUniqueRedirectsTo, other.topFrameUniqueRedirectsTo);
    mergeHashCountedSet(topFrameUniqueRedirectsFrom, other.topFrameUniqueRedirectsFrom);

    // Subframe stats
    mergeHashCountedSet(subframeUnderTopFrameOrigins, other.subframeUnderTopFrameOrigins);

    // Subresource stats
    mergeHashCountedSet(subresourceUnderTopFrameOrigins, other.subresourceUnderTopFrameOrigins);
    mergeHashCountedSet(subresourceUniqueRedirectsTo, other.subresourceUniqueRedirectsTo);
    mergeHashCountedSet(subresourceUniqueRedirectsFrom, other.subresourceUniqueRedirectsFrom);

    // Prevalent resource stats
    isPrevalentResource |= other.isPrevalentResource;
    isVeryPrevalentResource |= other.isVeryPrevalentResource;
    dataRecordsRemoved = std::max(dataRecordsRemoved, other.dataRecordsRemoved);

    // In-memory only
    isMarkedForCookieBlocking |= other.isMarkedForCookieBlocking;
}

String ResourceLoadStatistics::primaryDomain(const URL& url)
{
    return primaryDomain(url.host());
}

String ResourceLoadStatistics::primaryDomain(StringView host)
{
    if (host.isNull() || host.isEmpty())
        return "nullOrigin"_s;

    String hostString = host.toString();
#if ENABLE(PUBLIC_SUFFIX_LIST)
    String primaryDomain = topPrivatelyControlledDomain(hostString);
    // We will have an empty string here if there is no TLD. Use the host as a fallback.
    if (!primaryDomain.isEmpty())
        return primaryDomain;
#endif

    return hostString;
}

// FIXME: Temporary fix for <rdar://problem/32343256> until content can be updated.
bool ResourceLoadStatistics::areDomainsAssociated(bool needsSiteSpecificQuirks, const String& firstDomain, const String& secondDomain)
{
    ASSERT(isMainThread());

    static NeverDestroyed<HashMap<String, unsigned>> metaDomainIdentifiers = [] {
        HashMap<String, unsigned> map;

        // Domains owned by Dow Jones & Company, Inc.
        const unsigned dowJonesIdentifier = 1;
        map.add("dowjones.com"_s, dowJonesIdentifier);
        map.add("wsj.com"_s, dowJonesIdentifier);
        map.add("barrons.com"_s, dowJonesIdentifier);
        map.add("marketwatch.com"_s, dowJonesIdentifier);
        map.add("wsjplus.com"_s, dowJonesIdentifier);

        return map;
    }();

    if (firstDomain == secondDomain)
        return true;

    ASSERT(!equalIgnoringASCIICase(firstDomain, secondDomain));

    if (!needsSiteSpecificQuirks)
        return false;

    unsigned firstMetaDomainIdentifier = metaDomainIdentifiers.get().get(firstDomain);
    if (!firstMetaDomainIdentifier)
        return false;

    return firstMetaDomainIdentifier == metaDomainIdentifiers.get().get(secondDomain);
}

WallTime ResourceLoadStatistics::reduceTimeResolution(WallTime time)
{
    return WallTime::fromRawSeconds(std::floor(time.secondsSinceEpoch() / timestampResolution) * timestampResolution.seconds());
}

}
