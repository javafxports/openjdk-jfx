/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include "config.h"
#include "TextCodecJava.h"

#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

using std::pair;

namespace WebCore {

typedef pair<CString, CString> AliasNamePair;

static JGClass textCodecClass;
static jmethodID ctorMID;
static jmethodID getEncodingsMID;
static jmethodID encodeMID;
static jmethodID decodeMID;

static std::unique_ptr<TextCodec> newTextCodecJava(const TextEncoding& encoding, const void*)
{
    return adoptPtr(new TextCodecJava(encoding));
}

static JNIEnv* setUpCodec() {
    JNIEnv* env = WTF::GetJavaEnv();

    if (!textCodecClass) {
        textCodecClass =  JLClass(env->FindClass("com/sun/webkit/text/TextCodec"));
        ASSERT(textCodecClass);

        ctorMID = env->GetMethodID
            (textCodecClass, "<init>", "(Ljava/lang/String;)V");
        ASSERT(ctorMID);
        encodeMID = env->GetMethodID(textCodecClass, "encode", "([C)[B");
        ASSERT(encodeMID);
        decodeMID = env->GetMethodID(
                textCodecClass, "decode", "([B)Ljava/lang/String;");
        ASSERT(decodeMID);
        getEncodingsMID = env->GetStaticMethodID(
                textCodecClass, "getEncodings", "()[Ljava/lang/String;");
        ASSERT(getEncodingsMID);
    }

    return env;
}

static Vector<AliasNamePair>* buildPairs()
{
    JNIEnv* env = setUpCodec();

    jobjectArray arr = static_cast<jobjectArray>
            (env->CallStaticObjectMethod(textCodecClass, getEncodingsMID));
    WTF::CheckAndClearException(env);
    ASSERT(arr);
    jsize length = env->GetArrayLength(arr);

    Vector<AliasNamePair>* pairs = new Vector<AliasNamePair>;
    for (int i = 0; i < length; i += 2) {
        jstring s0 = static_cast<jstring>(env->GetObjectArrayElement(arr, i));
        ASSERT(s0);
        const char* u0 = env->GetStringUTFChars(s0, NULL);
        ASSERT(u0);
        CString alias(u0);
        env->ReleaseStringUTFChars(s0, u0);
        env->DeleteLocalRef(s0);

        jstring s1 = static_cast<jstring>(env->GetObjectArrayElement(arr, i + 1));
        ASSERT(s1);
        const char* u1 = env->GetStringUTFChars(s1, NULL);
        ASSERT(u1);
        CString name(u1);
        env->ReleaseStringUTFChars(s1, u1);
        env->DeleteLocalRef(s1);

        pairs->append(pair<CString, CString>(alias, name));
    }
    env->DeleteLocalRef(arr);

    return pairs;
}

static Vector<AliasNamePair>* getEncodingPairs()
{
    static Vector<AliasNamePair>* pairs;

    if (! pairs) {
        pairs = buildPairs();
    }
    return pairs;
}

void TextCodecJava::registerEncodingNames(EncodingNameRegistrar registrar)
{
    Vector<AliasNamePair>* pairs = getEncodingPairs();
    for (int size = pairs->size(), i = 0; i < size; i++) {
        AliasNamePair p = pairs->at(i);
        registrar(p.first.data(), p.second.data());
    }
}

void TextCodecJava::registerCodecs(TextCodecRegistrar registrar)
{
    Vector<AliasNamePair>* pairs = getEncodingPairs();
    for (int size = pairs->size(), i = 0; i < size; i++) {
        AliasNamePair p = pairs->at(i);
        registrar(p.first.data(), newTextCodecJava, 0);
    }
}

TextCodecJava::TextCodecJava(const TextEncoding& encoding)
    : m_encoding(encoding)
{
    JNIEnv* env = setUpCodec();

    jstring s = env->NewStringUTF(encoding.name());
    WTF::CheckAndClearException(env); // OOME
    ASSERT(s);
    jobject codec = env->NewObject(textCodecClass, ctorMID, s);
    WTF::CheckAndClearException(env); // OOME
    ASSERT(codec);
    env->DeleteLocalRef(s);
    m_codec = env->NewGlobalRef(codec);
    ASSERT(m_codec);
    env->DeleteLocalRef(codec);
}

TextCodecJava::~TextCodecJava()
{
    WC_GETJAVAENV_CHKRET(env);

    if (m_codec) {
        env->DeleteGlobalRef(m_codec);
    }
}

String TextCodecJava::decode(const char* bytes, size_t length, bool flush,
                             bool stopOnError, bool& sawError)
{
    JNIEnv* env = setUpCodec();

    JLocalRef<jbyteArray> barr(env->NewByteArray(length));
    WTF::CheckAndClearException(env); // OOME
    if (!barr) {
        return String();
    }

    const jbyte* elements = reinterpret_cast<const jbyte*>(bytes);
    env->SetByteArrayRegion((jbyteArray)barr, 0, length, elements);

    JLString s(static_cast<jstring>(env->CallObjectMethod(m_codec, decodeMID, (jbyteArray)barr)));
    if (env->ExceptionOccurred()) {
        sawError = true;
    }
    WTF::CheckAndClearException(env); // OOME

    return s ? String(env, s) : String();
}

CString TextCodecJava::encode(const UChar* characters, size_t length, UnencodableHandling)
{
    JNIEnv* env = setUpCodec();

    JLocalRef<jcharArray> carr(env->NewCharArray(length));
    WTF::CheckAndClearException(env); // OOME
    if (!carr) {
        return CString();
    }

    env->SetCharArrayRegion((jcharArray)carr, 0, length, reinterpret_cast<const jchar*>(characters));
    JLocalRef<jbyteArray> barr(
        static_cast<jbyteArray>(env->CallObjectMethod(m_codec, encodeMID, (jcharArray)carr)));
    WTF::CheckAndClearException(env); // OOME
    if (!barr) {
        return CString();
    }

    int nbytes = env->GetArrayLength((jbyteArray)barr);
    jbyte* bytes = (jbyte*)env->GetPrimitiveArrayCritical((jbyteArray)barr, NULL);
    CString encoded(reinterpret_cast<const char*>(bytes), nbytes);
    env->ReleasePrimitiveArrayCritical((jbyteArray)barr, bytes, JNI_ABORT);

    return encoded;
}

}
