/*
 * Copyright (c) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#ifndef HarfBuzzFace_h
#define HarfBuzzFace_h

#include <hb.h>

#include <memory>
#include <wtf/FastMalloc.h>
#include <wtf/HashMap.h>
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class FontPlatformData;

class HarfBuzzFace {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static const hb_tag_t vertTag;
    static const hb_tag_t vrt2Tag;
    static const hb_tag_t kernTag;

    HarfBuzzFace(FontPlatformData&, uint64_t);
    ~HarfBuzzFace();

    hb_font_t* createFont();

    void setScriptForVerticalGlyphSubstitution(hb_buffer_t*);

    using GlyphCache = HashMap<uint32_t, uint32_t, DefaultHash<uint32_t>::Hash, WTF::UnsignedWithZeroKeyHashTraits<uint32_t>>;
private:
    class CacheEntry : public RefCounted<CacheEntry> {
    public:
        static Ref<CacheEntry> create(hb_face_t* face)
        {
            return adoptRef(*new CacheEntry(face));
        }
        ~CacheEntry();

        hb_face_t* face() { return m_face; }
        GlyphCache& glyphCache() { return m_glyphCache; }

    private:
        CacheEntry(hb_face_t*);

        hb_face_t* m_face;
        GlyphCache m_glyphCache;
    };

    using Cache = HashMap<uint64_t, RefPtr<CacheEntry>, WTF::IntHash<uint64_t>, WTF::UnsignedWithZeroKeyHashTraits<uint64_t>>;
    static Cache& cache();

    hb_face_t* createFace();

    FontPlatformData& m_platformData;
    uint64_t m_uniqueID;
    RefPtr<CacheEntry> m_cacheEntry;

    hb_script_t m_scriptForVerticalText;
};

}

#endif // HarfBuzzFace_h
