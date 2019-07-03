/*
 * Copyright (C) 2010-2018 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2017 Yusuke Suzuki <utatane.tea@gmail.com>. All rights reserved.
 * Copyright (C) 2017 Mozilla Foundation. All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "config.h"
#include <wtf/text/StringBuilder.h>

#include <wtf/text/WTFString.h>

namespace WTF {

// This table driven escaping is ported from SpiderMonkey.
static const constexpr LChar escapedFormsForJSON[0x100] = {
    'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u',
    'b', 't', 'n', 'u', 'f', 'r', 'u', 'u',
    'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u',
    'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u',
    0,   0,  '\"', 0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,  '\\', 0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
};

template<typename OutputCharacterType, typename InputCharacterType>
ALWAYS_INLINE static void appendQuotedJSONStringInternal(OutputCharacterType*& output, const InputCharacterType* input, unsigned length)
{
    for (auto* end = input + length; input != end; ++input) {
        auto character = *input;
        if (LIKELY(character <= 0xFF)) {
            auto escaped = escapedFormsForJSON[character];
            if (LIKELY(!escaped)) {
                *output++ = character;
                continue;
            }

            *output++ = '\\';
            *output++ = escaped;
            if (UNLIKELY(escaped == 'u')) {
                *output++ = '0';
                *output++ = '0';
                *output++ = upperNibbleToLowercaseASCIIHexDigit(character);
                *output++ = lowerNibbleToLowercaseASCIIHexDigit(character);
            }
            continue;
        }

        if (LIKELY(!U16_IS_SURROGATE(character))) {
            *output++ = character;
            continue;
        }

        auto next = input + 1;
        bool isValidSurrogatePair = U16_IS_SURROGATE_LEAD(character) && next != end && U16_IS_TRAIL(*next);
        if (isValidSurrogatePair) {
            *output++ = character;
            *output++ = *next;
            ++input;
            continue;
        }

        uint8_t upper = static_cast<uint32_t>(character) >> 8;
        uint8_t lower = static_cast<uint8_t>(character);
        *output++ = '\\';
        *output++ = 'u';
        *output++ = upperNibbleToLowercaseASCIIHexDigit(upper);
        *output++ = lowerNibbleToLowercaseASCIIHexDigit(upper);
        *output++ = upperNibbleToLowercaseASCIIHexDigit(lower);
        *output++ = lowerNibbleToLowercaseASCIIHexDigit(lower);
        continue;
    }
}

void StringBuilder::appendQuotedJSONString(const String& string)
{
    if (hasOverflowed())
        return;
    // Make sure we have enough buffer space to append this string without having
    // to worry about reallocating in the middle.
    // The 2 is for the '"' quotes on each end.
    // The 6 is for characters that need to be \uNNNN encoded.
    Checked<unsigned, RecordOverflow> stringLength = string.length();
    Checked<unsigned, RecordOverflow> maximumCapacityRequired = length();
    maximumCapacityRequired += 2 + stringLength * 6;
    unsigned allocationSize;
    if (CheckedState::DidOverflow == maximumCapacityRequired.safeGet(allocationSize))
        return didOverflow();
    // This max() is here to allow us to allocate sizes between the range [2^31, 2^32 - 2] because roundUpToPowerOfTwo(1<<31 + some int smaller than 1<<31) == 0.
    // FIXME: roundUpToPowerOfTwo should take Checked<unsigned> and abort if it fails to round up.
    // https://bugs.webkit.org/show_bug.cgi?id=176086
    allocationSize = std::max(allocationSize, roundUpToPowerOfTwo(allocationSize));

    // Allocating this much will definitely fail.
    if (allocationSize > String::MaxLength)
        return didOverflow();

    if (is8Bit() && !string.is8Bit())
        allocateBufferUpConvert(m_bufferCharacters8, allocationSize);
    else
        reserveCapacity(allocationSize);
    if (UNLIKELY(hasOverflowed()))
        return;
    ASSERT(m_buffer->length() >= allocationSize);

    if (is8Bit()) {
        ASSERT(string.is8Bit());
        LChar* output = m_bufferCharacters8 + m_length.unsafeGet<unsigned>();
        *output++ = '"';
        appendQuotedJSONStringInternal(output, string.characters8(), string.length());
        *output++ = '"';
        m_length = output - m_bufferCharacters8;
    } else {
        UChar* output = m_bufferCharacters16 + m_length.unsafeGet<unsigned>();
        *output++ = '"';
        if (string.is8Bit())
            appendQuotedJSONStringInternal(output, string.characters8(), string.length());
        else
            appendQuotedJSONStringInternal(output, string.characters16(), string.length());
        *output++ = '"';
        m_length = output - m_bufferCharacters16;
    }
    ASSERT(!hasOverflowed());
    ASSERT(m_buffer->length() >= m_length.unsafeGet<unsigned>());
}

} // namespace WTF
