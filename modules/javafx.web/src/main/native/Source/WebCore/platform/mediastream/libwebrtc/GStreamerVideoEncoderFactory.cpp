/*
 * Copyright (C) 2018 Metrological Group B.V.
 * Copyright (C) 2018 Igalia S.L. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_STREAM) && USE(LIBWEBRTC) && USE(GSTREAMER)
#include "GStreamerVideoEncoderFactory.h"

#include "GStreamerVideoFrameLibWebRTC.h"
#include "webrtc/common_video/h264/h264_common.h"
#include "webrtc/common_video/h264/profile_level_id.h"
#include "webrtc/media/base/codec.h"
#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"
#include "webrtc/modules/video_coding/codecs/vp8/include/vp8.h"
#include "webrtc/modules/video_coding/include/video_codec_interface.h"

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#define GST_USE_UNSTABLE_API 1
#include <gst/codecparsers/gsth264parser.h>
#undef GST_USE_UNSTABLE_API
#include <gst/pbutils/encoding-profile.h>
#include <gst/video/video.h>

#include <mutex>

// Required for unified builds
#ifdef GST_CAT_DEFAULT
#undef GST_CAT_DEFAULT
#endif

GST_DEBUG_CATEGORY(webkit_webrtcenc_debug);
#define GST_CAT_DEFAULT webkit_webrtcenc_debug

#define KBIT_TO_BIT 1024

namespace WebCore {

typedef void (*BitrateSetter)(GstElement* encoder, uint32_t bitrate);
static GRefPtr<GRegex> targetBitrateBitPerSec;
static GRefPtr<GRegex> bitrateBitPerSec;
static GRefPtr<GRegex> bitrateKBitPerSec;

class GStreamerVideoEncoder : public webrtc::VideoEncoder {
public:
    GStreamerVideoEncoder(const webrtc::SdpVideoFormat&)
        : m_pictureId(0)
        , m_firstFramePts(GST_CLOCK_TIME_NONE)
        , m_restrictionCaps(adoptGRef(gst_caps_new_empty_simple("video/x-raw")))
        , m_bitrateSetter(nullptr)
    {
    }
    GStreamerVideoEncoder()
        : m_pictureId(0)
        , m_firstFramePts(GST_CLOCK_TIME_NONE)
        , m_restrictionCaps(adoptGRef(gst_caps_new_empty_simple("video/x-raw")))
        , m_bitrateSetter(nullptr)
    {
    }

    int SetRates(uint32_t newBitrate, uint32_t frameRate) override
    {
        GST_INFO_OBJECT(m_pipeline.get(), "New bitrate: %d - framerate is %d",
            newBitrate, frameRate);

        auto caps = gst_caps_make_writable(m_restrictionCaps.get());
        gst_caps_set_simple(caps, "framerate", GST_TYPE_FRACTION, frameRate, 1, nullptr);

        SetRestrictionCaps(caps);

        if (m_bitrateSetter && m_encoder)
            m_bitrateSetter(m_encoder, newBitrate);

        return WEBRTC_VIDEO_CODEC_OK;
    }

    GstElement* pipeline()
    {
        return m_pipeline.get();
    }

    GstElement* makeElement(const gchar* factoryName)
    {
        auto name = String::format("%s_enc_%s_%p", Name(), factoryName, this);
        auto elem = gst_element_factory_make(factoryName, name.utf8().data());

        return elem;
    }

    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, int32_t, size_t)
    {
        g_return_val_if_fail(codecSettings, WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
        g_return_val_if_fail(codecSettings->codecType == CodecType(), WEBRTC_VIDEO_CODEC_ERR_PARAMETER);

        m_pipeline = makeElement("pipeline");

        connectSimpleBusMessageCallback(m_pipeline.get());
        auto encodebin = createEncoder(&m_encoder).leakRef();
        ASSERT(m_encoder);
        m_bitrateSetter = getBitrateSetter(gst_element_get_factory(m_encoder));

        m_src = makeElement("appsrc");
        g_object_set(m_src, "is-live", true, "format", GST_FORMAT_TIME, nullptr);

        auto capsfilter = CreateFilter();
        auto sink = makeElement("appsink");
        gst_app_sink_set_emit_signals(GST_APP_SINK(sink), TRUE);
        g_signal_connect(sink, "new-sample", G_CALLBACK(newSampleCallbackTramp), this);

        gst_bin_add_many(GST_BIN(m_pipeline.get()), m_src, encodebin, capsfilter, sink, nullptr);
        if (!gst_element_link_many(m_src, encodebin, capsfilter, sink, nullptr))
            ASSERT_NOT_REACHED();

        gst_element_set_state(m_pipeline.get(), GST_STATE_PLAYING);

        return WEBRTC_VIDEO_CODEC_OK;
    }

    bool SupportsNativeHandle() const final
    {
        return true;
    }

    virtual GstElement* CreateFilter()
    {
        return makeElement("capsfilter");
    }

    int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) final
    {
        m_imageReadyCb = callback;

        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t Release() final
    {
        GRefPtr<GstBus> bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline.get())));
        gst_bus_set_sync_handler(bus.get(), nullptr, nullptr, nullptr);

        gst_element_set_state(m_pipeline.get(), GST_STATE_NULL);
        m_src = nullptr;
        m_pipeline = nullptr;

        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t SetChannelParameters(uint32_t, int64_t) final
    {
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t Encode(const webrtc::VideoFrame& frame,
        const webrtc::CodecSpecificInfo*,
        const std::vector<webrtc::FrameType>* frameTypes) final
    {
        if (!m_imageReadyCb) {
            GST_INFO_OBJECT(m_pipeline.get(), "No encoded callback set yet!");

            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }

        if (!m_src) {
            GST_INFO_OBJECT(m_pipeline.get(), "No source set yet!");

            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }

        auto sample = GStreamerSampleFromLibWebRTCVideoFrame(frame);
        auto buffer = gst_sample_get_buffer(sample.get());

        if (!GST_CLOCK_TIME_IS_VALID(m_firstFramePts)) {
            m_firstFramePts = GST_BUFFER_PTS(buffer);
            auto pad = adoptGRef(gst_element_get_static_pad(m_src, "src"));
            gst_pad_set_offset(pad.get(), -m_firstFramePts);
        }

        for (auto frame_type : *frameTypes) {
            if (frame_type == webrtc::kVideoFrameKey) {
                auto pad = adoptGRef(gst_element_get_static_pad(m_src, "src"));
                auto forceKeyUnit = gst_video_event_new_downstream_force_key_unit(GST_CLOCK_TIME_NONE,
                    GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, FALSE, 1);

                if (!gst_pad_push_event(pad.get(), forceKeyUnit))
                    GST_WARNING_OBJECT(pipeline(), "Could not send ForceKeyUnit event");

                break;
            }
        }

        switch (gst_app_src_push_sample(GST_APP_SRC(m_src), sample.get())) {
        case GST_FLOW_OK:
            return WEBRTC_VIDEO_CODEC_OK;
        case GST_FLOW_FLUSHING:
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        default:
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
    }

    GstFlowReturn newSampleCallback(GstElement* sink)
    {
        auto sample = adoptGRef(gst_app_sink_pull_sample(GST_APP_SINK(sink)));
        auto buffer = gst_sample_get_buffer(sample.get());
        auto caps = gst_sample_get_caps(sample.get());

        webrtc::RTPFragmentationHeader* fragmentationInfo;
        auto frame = Fragmentize(buffer, &fragmentationInfo);
        if (!frame._size)
            return GST_FLOW_OK;

        gst_structure_get(gst_caps_get_structure(caps, 0),
            "width", G_TYPE_INT, &frame._encodedWidth,
            "height", G_TYPE_INT, &frame._encodedHeight,
            nullptr);

        frame._frameType = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT) ? webrtc::kVideoFrameDelta : webrtc::kVideoFrameKey;
        frame._completeFrame = true;
        frame.capture_time_ms_ = GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer));
        frame._timeStamp = GST_TIME_AS_MSECONDS(GST_BUFFER_DTS(buffer));
        GST_LOG_OBJECT(m_pipeline.get(), "Got buffer TS: %" GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_PTS(buffer)));

        webrtc::CodecSpecificInfo codecSpecifiInfos;
        PopulateCodecSpecific(&codecSpecifiInfos, buffer);

        webrtc::EncodedImageCallback::Result result = m_imageReadyCb->OnEncodedImage(frame, &codecSpecifiInfos, fragmentationInfo);
        m_pictureId = (m_pictureId + 1) & 0x7FFF;
        if (result.error != webrtc::EncodedImageCallback::Result::OK) {
            GST_ELEMENT_ERROR(m_pipeline.get(), LIBRARY, FAILED, (nullptr),
                ("Encode callback failed: %d", result.error));

            return GST_FLOW_ERROR;
        }

        return GST_FLOW_OK;
    }

#define RETURN_BITRATE_SETTER_IF_MATCHES(regex, propertyName, bitrateMultiplier, unit)                 \
    if (g_regex_match(regex.get(), factoryName, static_cast<GRegexMatchFlags>(0), nullptr)) {          \
        GST_INFO_OBJECT(encoderFactory, "Detected as having a " #propertyName " property in " unit); \
        return [](GstElement* encoder, uint32_t bitrate)                                               \
            {                                                                                          \
                g_object_set(encoder, propertyName, bitrate * bitrateMultiplier, nullptr);            \
            };                                                                                         \
    }

    // GStreamer doesn't have a unified encoder API and the encoders have their
    // own semantics and naming to set the bitrate, this is a best effort to handle
    // setting bitrate for the well known encoders.
    // See https://bugzilla.gnome.org/show_bug.cgi?id=796716
    BitrateSetter getBitrateSetter(GstElementFactory* encoderFactory)
    {
        static std::once_flag regexRegisteredFlag;

        std::call_once(regexRegisteredFlag, [] {
            targetBitrateBitPerSec = g_regex_new("^vp.enc$|^omx.*enc$", static_cast<GRegexCompileFlags>(0), static_cast<GRegexMatchFlags>(0), nullptr);
            bitrateBitPerSec = g_regex_new("^openh264enc$", static_cast<GRegexCompileFlags>(0), static_cast<GRegexMatchFlags>(0), nullptr);
            bitrateKBitPerSec = g_regex_new("^x264enc$|vaapi.*enc$", static_cast<GRegexCompileFlags>(0), static_cast<GRegexMatchFlags>(0), nullptr);
            ASSERT(targetBitrateBitPerSec.get() && bitrateBitPerSec.get() && bitrateKBitPerSec.get());
        });

        auto factoryName = GST_OBJECT_NAME(encoderFactory);
        RETURN_BITRATE_SETTER_IF_MATCHES(targetBitrateBitPerSec, "target-bitrate", KBIT_TO_BIT, "Bits Per Second")
        RETURN_BITRATE_SETTER_IF_MATCHES(bitrateBitPerSec, "bitrate", KBIT_TO_BIT, "Bits Per Second")
        RETURN_BITRATE_SETTER_IF_MATCHES(bitrateKBitPerSec, "bitrate", 1, "KBits Per Second")

        GST_WARNING_OBJECT(encoderFactory, "unkonwn encoder, can't set bitrates on it");
        return nullptr;
    }
#undef RETURN_BITRATE_SETTER_IF_MATCHES

    GRefPtr<GstElement> createEncoder(GstElement** encoder)
    {
        GstElement* enc = nullptr;

        m_profile = GST_ENCODING_PROFILE(gst_encoding_video_profile_new(
            adoptGRef(gst_caps_from_string(Caps())).get(),
            ProfileName(),
            gst_caps_ref(m_restrictionCaps.get()),
            1));
        GRefPtr<GstElement> encodebin = makeElement("encodebin");

        if (!encodebin.get()) {
            GST_ERROR("No encodebin present... can't use GStreamer based encoders");
            return nullptr;
        }
        g_object_set(encodebin.get(), "profile", m_profile.get(), nullptr);

        for (GList* tmp = GST_BIN_CHILDREN(encodebin.get()); tmp; tmp = tmp->next) {
            GstElement* elem = GST_ELEMENT(tmp->data);
            GstElementFactory* factory = gst_element_get_factory((elem));

            if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_VIDEO_ENCODER))
                continue;

            enc = elem;
            break;
        }

        if (!enc)
            return nullptr;

        if (encoder)
            *encoder = enc;

        return encodebin;
    }

    void AddCodecIfSupported(std::vector<webrtc::SdpVideoFormat>* supportedFormats)
    {
        GstElement* encoder;

        if (createEncoder(&encoder).get() != nullptr) {
            webrtc::SdpVideoFormat format = ConfigureSupportedCodec(encoder);

            supportedFormats->push_back(format);
        }
    }

    virtual const gchar* ProfileName()
    {
        return nullptr;
    }

    virtual const gchar* Caps()
    {
        return nullptr;
    }

    virtual webrtc::VideoCodecType CodecType() = 0;
    virtual webrtc::SdpVideoFormat ConfigureSupportedCodec(GstElement*)
    {
        return webrtc::SdpVideoFormat(Name());
    }

    virtual void PopulateCodecSpecific(webrtc::CodecSpecificInfo*, GstBuffer*) = 0;

    virtual webrtc::EncodedImage Fragmentize(GstBuffer* buffer, webrtc::RTPFragmentationHeader** outFragmentationInfo)
    {
        GstMapInfo map;

        gst_buffer_map(buffer, &map, GST_MAP_READ);
        webrtc::EncodedImage frame(map.data, map.size, map.size);
        gst_buffer_unmap(buffer, &map);

        // No fragmentation by default.
        webrtc::RTPFragmentationHeader* fragmentationInfo = new webrtc::RTPFragmentationHeader();

        fragmentationInfo->VerifyAndAllocateFragmentationHeader(1);
        fragmentationInfo->fragmentationOffset[0] = 0;
        fragmentationInfo->fragmentationLength[0] = gst_buffer_get_size(buffer);
        fragmentationInfo->fragmentationPlType[0] = 0;
        fragmentationInfo->fragmentationTimeDiff[0] = 0;

        *outFragmentationInfo = fragmentationInfo;

        return frame;
    }

    const char* ImplementationName() const
    {
        g_return_val_if_fail(m_encoder, nullptr);

        return GST_OBJECT_NAME(gst_element_get_factory(m_encoder));
    }

    virtual const gchar* Name() = 0;

    void SetRestrictionCaps(GstCaps* caps)
    {
        if (caps && m_profile.get() && gst_caps_is_equal(m_restrictionCaps.get(), caps))
            g_object_set(m_profile.get(), "restriction-caps", caps, nullptr);

        m_restrictionCaps = caps;
    }

protected:
    int16_t m_pictureId;

private:
    static GstFlowReturn newSampleCallbackTramp(GstElement* sink, GStreamerVideoEncoder* enc)
    {
        return enc->newSampleCallback(sink);
    }

    GRefPtr<GstElement> m_pipeline;
    GstElement* m_src;
    GstElement* m_encoder;

    webrtc::EncodedImageCallback* m_imageReadyCb;
    GstClockTime m_firstFramePts;
    GRefPtr<GstCaps> m_restrictionCaps;
    GRefPtr<GstEncodingProfile> m_profile;
    BitrateSetter m_bitrateSetter;
};

class H264Encoder : public GStreamerVideoEncoder {
public:
    H264Encoder() { }

    H264Encoder(const webrtc::SdpVideoFormat& format)
        : m_parser(gst_h264_nal_parser_new())
        , packetizationMode(webrtc::H264PacketizationMode::NonInterleaved)
    {
        auto it = format.parameters.find(cricket::kH264FmtpPacketizationMode);

        if (it != format.parameters.end() && it->second == "1")
            packetizationMode = webrtc::H264PacketizationMode::NonInterleaved;
    }

    // FIXME - MT. safety!
    webrtc::EncodedImage Fragmentize(GstBuffer* gstbuffer, webrtc::RTPFragmentationHeader** outFragmentationInfo) final
    {
        GstMapInfo map;
        GstH264NalUnit nalu;
        auto parserResult = GST_H264_PARSER_OK;

        gsize offset = 0;
        size_t requiredSize = 0;

        std::vector<GstH264NalUnit> nals;
        webrtc::EncodedImage encodedImage;

        const uint8_t startCode[4] = { 0, 0, 0, 1 };
        gst_buffer_map(gstbuffer, &map, GST_MAP_READ);
        while (parserResult == GST_H264_PARSER_OK) {
            parserResult = gst_h264_parser_identify_nalu(m_parser, map.data, offset, map.size, &nalu);

            nalu.sc_offset = offset;
            nalu.offset = offset + sizeof(startCode);
            if (parserResult != GST_H264_PARSER_OK && parserResult != GST_H264_PARSER_NO_NAL_END)
                break;

            requiredSize += nalu.size + sizeof(startCode);
            nals.push_back(nalu);
            offset = nalu.offset + nalu.size;
        }

        encodedImage._size = requiredSize;
        encodedImage._buffer = new uint8_t[encodedImage._size];
        // Iterate nal units and fill the Fragmentation info.
        webrtc::RTPFragmentationHeader* fragmentationHeader = new webrtc::RTPFragmentationHeader();
        fragmentationHeader->VerifyAndAllocateFragmentationHeader(nals.size());
        size_t fragmentIndex = 0;
        encodedImage._length = 0;
        for (std::vector<GstH264NalUnit>::iterator nal = nals.begin(); nal != nals.end(); ++nal, fragmentIndex++) {

            ASSERT(map.data[nal->sc_offset + 0] == startCode[0]);
            ASSERT(map.data[nal->sc_offset + 1] == startCode[1]);
            ASSERT(map.data[nal->sc_offset + 2] == startCode[2]);
            ASSERT(map.data[nal->sc_offset + 3] == startCode[3]);

            fragmentationHeader->fragmentationOffset[fragmentIndex] = nal->offset;
            fragmentationHeader->fragmentationLength[fragmentIndex] = nal->size;

            memcpy(encodedImage._buffer + encodedImage._length, &map.data[nal->sc_offset],
                sizeof(startCode) + nal->size);
            encodedImage._length += nal->size + sizeof(startCode);
        }

        *outFragmentationInfo = fragmentationHeader;
        gst_buffer_unmap(gstbuffer, &map);
        return encodedImage;
    }

    GstElement* CreateFilter() final
    {
        GstElement* filter = makeElement("capsfilter");
        auto caps = adoptGRef(gst_caps_new_simple(Caps(),
            "alignment", G_TYPE_STRING, "au",
            "stream-format", G_TYPE_STRING, "byte-stream",
            nullptr));
        g_object_set(filter, "caps", caps.get(), nullptr);

        return filter;
    }

    webrtc::SdpVideoFormat ConfigureSupportedCodec(GstElement*) final
    {
        // TODO- Create from encoder src pad caps template
        return webrtc::SdpVideoFormat(cricket::kH264CodecName,
            { { cricket::kH264FmtpProfileLevelId, cricket::kH264ProfileLevelConstrainedBaseline },
                { cricket::kH264FmtpLevelAsymmetryAllowed, "1" },
                { cricket::kH264FmtpPacketizationMode, "1" } });
    }

    const gchar* Caps() final { return "video/x-h264"; }
    const gchar* Name() final { return cricket::kH264CodecName; }
    GstH264NalParser* m_parser;
    webrtc::VideoCodecType CodecType() final { return webrtc::kVideoCodecH264; }

    void PopulateCodecSpecific(webrtc::CodecSpecificInfo* codecSpecifiInfos, GstBuffer*) final
    {
        codecSpecifiInfos->codecType = CodecType();
        codecSpecifiInfos->codec_name = ImplementationName();
        webrtc::CodecSpecificInfoH264* h264Info = &(codecSpecifiInfos->codecSpecific.H264);
        h264Info->packetization_mode = packetizationMode;
    }

    webrtc::H264PacketizationMode packetizationMode;
};

class VP8Encoder : public GStreamerVideoEncoder {
public:
    VP8Encoder() { }
    VP8Encoder(const webrtc::SdpVideoFormat&) { }
    const gchar* Caps() final { return "video/x-vp8"; }
    const gchar* Name() final { return cricket::kVp8CodecName; }
    webrtc::VideoCodecType CodecType() final { return webrtc::kVideoCodecVP8; }
    virtual const gchar* ProfileName() { return "Profile Realtime"; }

    void PopulateCodecSpecific(webrtc::CodecSpecificInfo* codecSpecifiInfos, GstBuffer* buffer) final
    {
        codecSpecifiInfos->codecType = webrtc::kVideoCodecVP8;
        codecSpecifiInfos->codec_name = ImplementationName();
        webrtc::CodecSpecificInfoVP8* vp8Info = &(codecSpecifiInfos->codecSpecific.VP8);
        vp8Info->temporalIdx = 0;
        vp8Info->pictureId = m_pictureId;

        vp8Info->simulcastIdx = 0;
        vp8Info->keyIdx = webrtc::kNoKeyIdx;
        vp8Info->nonReference = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
        vp8Info->tl0PicIdx = webrtc::kNoTl0PicIdx;
    }
};

std::unique_ptr<webrtc::VideoEncoder> GStreamerVideoEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat& format)
{
    if (format.name == cricket::kVp8CodecName)
        return std::make_unique<VP8Encoder>(format);

    if (format.name == cricket::kH264CodecName)
        return std::make_unique<H264Encoder>(format);

    return nullptr;
}

GStreamerVideoEncoderFactory::GStreamerVideoEncoderFactory()
{
    static std::once_flag debugRegisteredFlag;

    std::call_once(debugRegisteredFlag, [] {
        GST_DEBUG_CATEGORY_INIT(webkit_webrtcenc_debug, "webkitlibwebrtcvideoencoder", 0, "WebKit WebRTC video encoder");
    });
}

std::vector<webrtc::SdpVideoFormat> GStreamerVideoEncoderFactory::GetSupportedFormats() const
{
    std::vector<webrtc::SdpVideoFormat> supportedCodecs;

    VP8Encoder().AddCodecIfSupported(&supportedCodecs);
    H264Encoder().AddCodecIfSupported(&supportedCodecs);

    return supportedCodecs;
}

} // namespace WebCore
#endif
