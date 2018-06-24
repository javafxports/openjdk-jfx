


#ifndef __GST_AUDIO_ENUM_TYPES_H__
#define __GST_AUDIO_ENUM_TYPES_H__

#include <gst/gst.h>
#include <gst/audio/audio-prelude.h>
G_BEGIN_DECLS

/* enumerations from "audio-channel-mixer.h" */
GST_AUDIO_API
GType gst_audio_channel_mixer_flags_get_type (void);
#define GST_TYPE_AUDIO_CHANNEL_MIXER_FLAGS (gst_audio_channel_mixer_flags_get_type())

/* enumerations from "audio-channels.h" */
GST_AUDIO_API
GType gst_audio_channel_position_get_type (void);
#define GST_TYPE_AUDIO_CHANNEL_POSITION (gst_audio_channel_position_get_type())

/* enumerations from "audio-converter.h" */
GST_AUDIO_API
GType gst_audio_converter_flags_get_type (void);
#define GST_TYPE_AUDIO_CONVERTER_FLAGS (gst_audio_converter_flags_get_type())

/* enumerations from "audio-format.h" */
GST_AUDIO_API
GType gst_audio_format_get_type (void);
#define GST_TYPE_AUDIO_FORMAT (gst_audio_format_get_type())
GST_AUDIO_API
GType gst_audio_format_flags_get_type (void);
#define GST_TYPE_AUDIO_FORMAT_FLAGS (gst_audio_format_flags_get_type())
GST_AUDIO_API
GType gst_audio_pack_flags_get_type (void);
#define GST_TYPE_AUDIO_PACK_FLAGS (gst_audio_pack_flags_get_type())

/* enumerations from "audio-info.h" */
GST_AUDIO_API
GType gst_audio_flags_get_type (void);
#define GST_TYPE_AUDIO_FLAGS (gst_audio_flags_get_type())
GST_AUDIO_API
GType gst_audio_layout_get_type (void);
#define GST_TYPE_AUDIO_LAYOUT (gst_audio_layout_get_type())

/* enumerations from "audio-quantize.h" */
GST_AUDIO_API
GType gst_audio_dither_method_get_type (void);
#define GST_TYPE_AUDIO_DITHER_METHOD (gst_audio_dither_method_get_type())
GST_AUDIO_API
GType gst_audio_noise_shaping_method_get_type (void);
#define GST_TYPE_AUDIO_NOISE_SHAPING_METHOD (gst_audio_noise_shaping_method_get_type())
GST_AUDIO_API
GType gst_audio_quantize_flags_get_type (void);
#define GST_TYPE_AUDIO_QUANTIZE_FLAGS (gst_audio_quantize_flags_get_type())

/* enumerations from "audio-resampler.h" */
GST_AUDIO_API
GType gst_audio_resampler_filter_mode_get_type (void);
#define GST_TYPE_AUDIO_RESAMPLER_FILTER_MODE (gst_audio_resampler_filter_mode_get_type())
GST_AUDIO_API
GType gst_audio_resampler_filter_interpolation_get_type (void);
#define GST_TYPE_AUDIO_RESAMPLER_FILTER_INTERPOLATION (gst_audio_resampler_filter_interpolation_get_type())
GST_AUDIO_API
GType gst_audio_resampler_method_get_type (void);
#define GST_TYPE_AUDIO_RESAMPLER_METHOD (gst_audio_resampler_method_get_type())
GST_AUDIO_API
GType gst_audio_resampler_flags_get_type (void);
#define GST_TYPE_AUDIO_RESAMPLER_FLAGS (gst_audio_resampler_flags_get_type())

/* enumerations from "gstaudiobasesink.h" */
GST_AUDIO_API
GType gst_audio_base_sink_slave_method_get_type (void);
#define GST_TYPE_AUDIO_BASE_SINK_SLAVE_METHOD (gst_audio_base_sink_slave_method_get_type())
GST_AUDIO_API
GType gst_audio_base_sink_discont_reason_get_type (void);
#define GST_TYPE_AUDIO_BASE_SINK_DISCONT_REASON (gst_audio_base_sink_discont_reason_get_type())

/* enumerations from "gstaudiobasesrc.h" */
GST_AUDIO_API
GType gst_audio_base_src_slave_method_get_type (void);
#define GST_TYPE_AUDIO_BASE_SRC_SLAVE_METHOD (gst_audio_base_src_slave_method_get_type())

/* enumerations from "gstaudiocdsrc.h" */
#ifndef GSTREAMER_LITE
GST_AUDIO_API
GType gst_audio_cd_src_mode_get_type (void);
#define GST_TYPE_AUDIO_CD_SRC_MODE (gst_audio_cd_src_mode_get_type())
#endif // GSTREAMER_LITE

/* enumerations from "gstaudioringbuffer.h" */
GST_AUDIO_API
GType gst_audio_ring_buffer_state_get_type (void);
#define GST_TYPE_AUDIO_RING_BUFFER_STATE (gst_audio_ring_buffer_state_get_type())
GST_AUDIO_API
GType gst_audio_ring_buffer_format_type_get_type (void);
#define GST_TYPE_AUDIO_RING_BUFFER_FORMAT_TYPE (gst_audio_ring_buffer_format_type_get_type())
G_END_DECLS

#endif /* __GST_AUDIO_ENUM_TYPES_H__ */



