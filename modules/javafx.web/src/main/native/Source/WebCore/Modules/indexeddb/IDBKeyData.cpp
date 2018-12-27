/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "IDBKeyData.h"

#if ENABLE(INDEXED_DATABASE)

#include "KeyedCoding.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

IDBKeyData::IDBKeyData(const IDBKey* key)
    : m_type(KeyType::Invalid)
{
    if (!key) {
        m_isNull = true;
        return;
    }

    m_type = key->type();

    switch (m_type) {
    case KeyType::Invalid:
        break;
    case KeyType::Array: {
        m_value = Vector<IDBKeyData>();
        auto& array = WTF::get<Vector<IDBKeyData>>(m_value);
        for (auto& key2 : key->array())
            array.append(IDBKeyData(key2.get()));
        break;
    }
    case KeyType::Binary:
        m_value = key->binary();
        break;
    case KeyType::String:
        m_value = key->string();
        break;
    case KeyType::Date:
        m_value = key->date();
        break;
    case KeyType::Number:
        m_value = key->number();
        break;
    case KeyType::Max:
    case KeyType::Min:
        break;
    }
}

RefPtr<IDBKey> IDBKeyData::maybeCreateIDBKey() const
{
    if (m_isNull)
        return nullptr;

    switch (m_type) {
    case KeyType::Invalid:
        return IDBKey::createInvalid();
    case KeyType::Array: {
        Vector<RefPtr<IDBKey>> array;
        for (auto& keyData : WTF::get<Vector<IDBKeyData>>(m_value)) {
            array.append(keyData.maybeCreateIDBKey());
            ASSERT(array.last());
        }
        return IDBKey::createArray(array);
    }
    case KeyType::Binary:
        return IDBKey::createBinary(WTF::get<ThreadSafeDataBuffer>(m_value));
    case KeyType::String:
        return IDBKey::createString(WTF::get<String>(m_value));
    case KeyType::Date:
        return IDBKey::createDate(WTF::get<double>(m_value));
    case KeyType::Number:
        return IDBKey::createNumber(WTF::get<double>(m_value));
    case KeyType::Max:
    case KeyType::Min:
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

IDBKeyData::IDBKeyData(const IDBKeyData& that, IsolatedCopyTag)
{
    isolatedCopy(that, *this);
}

IDBKeyData IDBKeyData::isolatedCopy() const
{
    return { *this, IsolatedCopy };
}

void IDBKeyData::isolatedCopy(const IDBKeyData& source, IDBKeyData& destination)
{
    destination.m_type = source.m_type;
    destination.m_isNull = source.m_isNull;

    switch (source.m_type) {
    case KeyType::Invalid:
        return;
    case KeyType::Array: {
        destination.m_value = Vector<IDBKeyData>();
        auto& destinationArray = WTF::get<Vector<IDBKeyData>>(destination.m_value);
        for (auto& key : WTF::get<Vector<IDBKeyData>>(source.m_value))
            destinationArray.append(key.isolatedCopy());
        return;
    }
    case KeyType::Binary:
        destination.m_value = WTF::get<ThreadSafeDataBuffer>(source.m_value);
        return;
    case KeyType::String:
        destination.m_value = WTF::get<String>(source.m_value).isolatedCopy();
        return;
    case KeyType::Date:
    case KeyType::Number:
        destination.m_value = WTF::get<double>(source.m_value);
        return;
    case KeyType::Max:
    case KeyType::Min:
        return;
    }

    ASSERT_NOT_REACHED();
}

void IDBKeyData::encode(KeyedEncoder& encoder) const
{
    encoder.encodeBool("null", m_isNull);
    if (m_isNull)
        return;

    encoder.encodeEnum("type", m_type);

    switch (m_type) {
    case KeyType::Invalid:
        return;
    case KeyType::Array: {
        auto& array = WTF::get<Vector<IDBKeyData>>(m_value);
        encoder.encodeObjects("array", array.begin(), array.end(), [](KeyedEncoder& encoder, const IDBKeyData& key) {
            key.encode(encoder);
        });
        return;
    }
    case KeyType::Binary: {
        auto* data = WTF::get<ThreadSafeDataBuffer>(m_value).data();
        encoder.encodeBool("hasBinary", !!data);
        if (data)
            encoder.encodeBytes("binary", data->data(), data->size());
        return;
    }
    case KeyType::String:
        encoder.encodeString("string", WTF::get<String>(m_value));
        return;
    case KeyType::Date:
    case KeyType::Number:
        encoder.encodeDouble("number", WTF::get<double>(m_value));
        return;
    case KeyType::Max:
    case KeyType::Min:
        return;
    }

    ASSERT_NOT_REACHED();
}

bool IDBKeyData::decode(KeyedDecoder& decoder, IDBKeyData& result)
{
    if (!decoder.decodeBool("null", result.m_isNull))
        return false;

    if (result.m_isNull)
        return true;

    auto enumFunction = [](int64_t value) {
        return value == KeyType::Max
            || value == KeyType::Invalid
            || value == KeyType::Array
            || value == KeyType::Binary
            || value == KeyType::String
            || value == KeyType::Date
            || value == KeyType::Number
            || value == KeyType::Min;
    };
    if (!decoder.decodeEnum("type", result.m_type, enumFunction))
        return false;

    if (result.m_type == KeyType::Invalid)
        return true;

    if (result.m_type == KeyType::Max)
        return true;

    if (result.m_type == KeyType::Min)
        return true;

    if (result.m_type == KeyType::String) {
        result.m_value = String();
        return decoder.decodeString("string", WTF::get<String>(result.m_value));
    }

    if (result.m_type == KeyType::Number || result.m_type == KeyType::Date) {
        result.m_value = 0.0;
        return decoder.decodeDouble("number", WTF::get<double>(result.m_value));
    }

    if (result.m_type == KeyType::Binary) {
        result.m_value = ThreadSafeDataBuffer();

        bool hasBinaryData;
        if (!decoder.decodeBool("hasBinary", hasBinaryData))
            return false;

        if (!hasBinaryData)
            return true;

        Vector<uint8_t> bytes;
        if (!decoder.decodeBytes("binary", bytes))
            return false;

        result.m_value = ThreadSafeDataBuffer::create(WTFMove(bytes));
        return true;
    }

    ASSERT(result.m_type == KeyType::Array);

    auto arrayFunction = [](KeyedDecoder& decoder, IDBKeyData& result) {
        return decode(decoder, result);
    };

    result.m_value = Vector<IDBKeyData>();
    return decoder.decodeObjects("array", WTF::get<Vector<IDBKeyData>>(result.m_value), arrayFunction);
}

int IDBKeyData::compare(const IDBKeyData& other) const
{
    if (m_type == KeyType::Invalid) {
        if (other.m_type != KeyType::Invalid)
            return -1;
        if (other.m_type == KeyType::Invalid)
            return 0;
    } else if (other.m_type == KeyType::Invalid)
        return 1;

    // The IDBKey::m_type enum is in reverse sort order.
    if (m_type != other.m_type)
        return m_type < other.m_type ? 1 : -1;

    // The types are the same, so handle actual value comparison.
    switch (m_type) {
    case KeyType::Invalid:
        // Invalid type should have been fully handled above
        ASSERT_NOT_REACHED();
        return 0;
    case KeyType::Array: {
        auto& array = WTF::get<Vector<IDBKeyData>>(m_value);
        auto& otherArray = WTF::get<Vector<IDBKeyData>>(other.m_value);
        for (size_t i = 0; i < array.size() && i < otherArray.size(); ++i) {
            if (int result = array[i].compare(otherArray[i]))
                return result;
        }
        if (array.size() < otherArray.size())
            return -1;
        if (array.size() > otherArray.size())
            return 1;
        return 0;
    }
    case KeyType::Binary:
        return compareBinaryKeyData(WTF::get<ThreadSafeDataBuffer>(m_value), WTF::get<ThreadSafeDataBuffer>(other.m_value));
    case KeyType::String:
        return codePointCompare(WTF::get<String>(m_value), WTF::get<String>(other.m_value));
    case KeyType::Date:
    case KeyType::Number: {
        auto number = WTF::get<double>(m_value);
        auto otherNumber = WTF::get<double>(other.m_value);

        if (number == otherNumber)
            return 0;
        return number > otherNumber ? 1 : -1;
    }
    case KeyType::Max:
    case KeyType::Min:
        return 0;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

#if !LOG_DISABLED
String IDBKeyData::loggingString() const
{
    if (m_isNull)
        return "<null>";

    String result;

    switch (m_type) {
    case KeyType::Invalid:
        return "<invalid>";
    case KeyType::Array: {
        StringBuilder builder;
        builder.appendLiteral("<array> - { ");
        auto& array = WTF::get<Vector<IDBKeyData>>(m_value);
        for (size_t i = 0; i < array.size(); ++i) {
            builder.append(array[i].loggingString());
            if (i < array.size() - 1)
                builder.appendLiteral(", ");
        }
        builder.appendLiteral(" }");
        result = builder.toString();
        break;
    }
    case KeyType::Binary: {
        StringBuilder builder;
        builder.append("<binary> - ");

        auto* data = WTF::get<ThreadSafeDataBuffer>(m_value).data();
        if (!data) {
            builder.append("(null)");
            result = builder.toString();
            break;
        }

        size_t i = 0;
        for (; i < 8 && i < data->size(); ++i)
            builder.append(String::format("%02x", data->at(i)));

        if (data->size() > 8)
            builder.append("...");

        result = builder.toString();
        break;
    }
    case KeyType::String:
        result = "<string> - " + WTF::get<String>(m_value);
        break;
    case KeyType::Date:
        return String::format("<date> - %f", WTF::get<double>(m_value));
    case KeyType::Number:
        return String::format("<number> - %f", WTF::get<double>(m_value));
    case KeyType::Max:
        return "<maximum>";
    case KeyType::Min:
        return "<minimum>";
    }

    if (result.length() > 150) {
        result.truncate(147);
        result.append("..."_s);
    }

    return result;
}
#endif

void IDBKeyData::setArrayValue(const Vector<IDBKeyData>& value)
{
    *this = IDBKeyData();
    m_value = value;
    m_type = KeyType::Array;
    m_isNull = false;
}

void IDBKeyData::setBinaryValue(const ThreadSafeDataBuffer& value)
{
    *this = IDBKeyData();
    m_value = value;
    m_type = KeyType::Binary;
    m_isNull = false;
}

void IDBKeyData::setStringValue(const String& value)
{
    *this = IDBKeyData();
    m_value = value;
    m_type = KeyType::String;
    m_isNull = false;
}

void IDBKeyData::setDateValue(double value)
{
    *this = IDBKeyData();
    m_value = value;
    m_type = KeyType::Date;
    m_isNull = false;
}

void IDBKeyData::setNumberValue(double value)
{
    *this = IDBKeyData();
    m_value = value;
    m_type = KeyType::Number;
    m_isNull = false;
}

IDBKeyData IDBKeyData::deletedValue()
{
    IDBKeyData result;
    result.m_isNull = false;
    result.m_isDeletedValue = true;
    return result;
}

bool IDBKeyData::isValid() const
{
    if (m_type == KeyType::Invalid)
        return false;

    if (m_type == KeyType::Array) {
        for (auto& key : array()) {
            if (!key.isValid())
                return false;
        }
    }

    return true;
}

bool IDBKeyData::operator<(const IDBKeyData& rhs) const
{
    return compare(rhs) < 0;
}

bool IDBKeyData::operator==(const IDBKeyData& other) const
{
    if (m_type != other.m_type || m_isNull != other.m_isNull || m_isDeletedValue != other.m_isDeletedValue)
        return false;
    switch (m_type) {
    case KeyType::Invalid:
    case KeyType::Max:
    case KeyType::Min:
        return true;
    case KeyType::Number:
    case KeyType::Date:
        return WTF::get<double>(m_value) == WTF::get<double>(other.m_value);
    case KeyType::String:
        return WTF::get<String>(m_value) == WTF::get<String>(other.m_value);
    case KeyType::Binary:
        return WTF::get<ThreadSafeDataBuffer>(m_value) == WTF::get<ThreadSafeDataBuffer>(other.m_value);
    case KeyType::Array:
        return WTF::get<Vector<IDBKeyData>>(m_value) == WTF::get<Vector<IDBKeyData>>(other.m_value);
    }
    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
