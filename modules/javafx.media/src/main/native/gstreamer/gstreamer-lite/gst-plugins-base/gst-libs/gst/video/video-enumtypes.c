


#include "video-enumtypes.h"

#include "video.h"
#include "video-format.h"
#include "video-color.h"
#include "video-info.h"
#include "video-dither.h"
#include "colorbalance.h"
#include "navigation.h"
#include "video-chroma.h"
#include "video-tile.h"
#include "video-converter.h"
#ifndef GSTREAMER_LITE
#include "video-resampler.h"
#endif // GSTREAMER_LITE
#include "video-frame.h"
#ifndef GSTREAMER_LITE
#include "video-scaler.h"
#endif // GSTREAMER_LITE

/* enumerations from "colorbalance.h" */
GType
gst_color_balance_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_COLOR_BALANCE_HARDWARE, "GST_COLOR_BALANCE_HARDWARE", "hardware" },
      { GST_COLOR_BALANCE_SOFTWARE, "GST_COLOR_BALANCE_SOFTWARE", "software" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstColorBalanceType", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "navigation.h" */
GType
gst_navigation_command_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_NAVIGATION_COMMAND_INVALID, "GST_NAVIGATION_COMMAND_INVALID", "invalid" },
      { GST_NAVIGATION_COMMAND_MENU1, "GST_NAVIGATION_COMMAND_MENU1", "menu1" },
      { GST_NAVIGATION_COMMAND_MENU2, "GST_NAVIGATION_COMMAND_MENU2", "menu2" },
      { GST_NAVIGATION_COMMAND_MENU3, "GST_NAVIGATION_COMMAND_MENU3", "menu3" },
      { GST_NAVIGATION_COMMAND_MENU4, "GST_NAVIGATION_COMMAND_MENU4", "menu4" },
      { GST_NAVIGATION_COMMAND_MENU5, "GST_NAVIGATION_COMMAND_MENU5", "menu5" },
      { GST_NAVIGATION_COMMAND_MENU6, "GST_NAVIGATION_COMMAND_MENU6", "menu6" },
      { GST_NAVIGATION_COMMAND_MENU7, "GST_NAVIGATION_COMMAND_MENU7", "menu7" },
      { GST_NAVIGATION_COMMAND_LEFT, "GST_NAVIGATION_COMMAND_LEFT", "left" },
      { GST_NAVIGATION_COMMAND_RIGHT, "GST_NAVIGATION_COMMAND_RIGHT", "right" },
      { GST_NAVIGATION_COMMAND_UP, "GST_NAVIGATION_COMMAND_UP", "up" },
      { GST_NAVIGATION_COMMAND_DOWN, "GST_NAVIGATION_COMMAND_DOWN", "down" },
      { GST_NAVIGATION_COMMAND_ACTIVATE, "GST_NAVIGATION_COMMAND_ACTIVATE", "activate" },
      { GST_NAVIGATION_COMMAND_PREV_ANGLE, "GST_NAVIGATION_COMMAND_PREV_ANGLE", "prev-angle" },
      { GST_NAVIGATION_COMMAND_NEXT_ANGLE, "GST_NAVIGATION_COMMAND_NEXT_ANGLE", "next-angle" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstNavigationCommand", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_navigation_query_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_NAVIGATION_QUERY_INVALID, "GST_NAVIGATION_QUERY_INVALID", "invalid" },
      { GST_NAVIGATION_QUERY_COMMANDS, "GST_NAVIGATION_QUERY_COMMANDS", "commands" },
      { GST_NAVIGATION_QUERY_ANGLES, "GST_NAVIGATION_QUERY_ANGLES", "angles" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstNavigationQueryType", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_navigation_message_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_NAVIGATION_MESSAGE_INVALID, "GST_NAVIGATION_MESSAGE_INVALID", "invalid" },
      { GST_NAVIGATION_MESSAGE_MOUSE_OVER, "GST_NAVIGATION_MESSAGE_MOUSE_OVER", "mouse-over" },
      { GST_NAVIGATION_MESSAGE_COMMANDS_CHANGED, "GST_NAVIGATION_MESSAGE_COMMANDS_CHANGED", "commands-changed" },
      { GST_NAVIGATION_MESSAGE_ANGLES_CHANGED, "GST_NAVIGATION_MESSAGE_ANGLES_CHANGED", "angles-changed" },
      { GST_NAVIGATION_MESSAGE_EVENT, "GST_NAVIGATION_MESSAGE_EVENT", "event" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstNavigationMessageType", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_navigation_event_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_NAVIGATION_EVENT_INVALID, "GST_NAVIGATION_EVENT_INVALID", "invalid" },
      { GST_NAVIGATION_EVENT_KEY_PRESS, "GST_NAVIGATION_EVENT_KEY_PRESS", "key-press" },
      { GST_NAVIGATION_EVENT_KEY_RELEASE, "GST_NAVIGATION_EVENT_KEY_RELEASE", "key-release" },
      { GST_NAVIGATION_EVENT_MOUSE_BUTTON_PRESS, "GST_NAVIGATION_EVENT_MOUSE_BUTTON_PRESS", "mouse-button-press" },
      { GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE, "GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE", "mouse-button-release" },
      { GST_NAVIGATION_EVENT_MOUSE_MOVE, "GST_NAVIGATION_EVENT_MOUSE_MOVE", "mouse-move" },
      { GST_NAVIGATION_EVENT_COMMAND, "GST_NAVIGATION_EVENT_COMMAND", "command" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstNavigationEventType", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-chroma.h" */
GType
gst_video_chroma_site_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_CHROMA_SITE_UNKNOWN, "GST_VIDEO_CHROMA_SITE_UNKNOWN", "unknown" },
      { GST_VIDEO_CHROMA_SITE_NONE, "GST_VIDEO_CHROMA_SITE_NONE", "none" },
      { GST_VIDEO_CHROMA_SITE_H_COSITED, "GST_VIDEO_CHROMA_SITE_H_COSITED", "h-cosited" },
      { GST_VIDEO_CHROMA_SITE_V_COSITED, "GST_VIDEO_CHROMA_SITE_V_COSITED", "v-cosited" },
      { GST_VIDEO_CHROMA_SITE_ALT_LINE, "GST_VIDEO_CHROMA_SITE_ALT_LINE", "alt-line" },
      { GST_VIDEO_CHROMA_SITE_COSITED, "GST_VIDEO_CHROMA_SITE_COSITED", "cosited" },
      { GST_VIDEO_CHROMA_SITE_JPEG, "GST_VIDEO_CHROMA_SITE_JPEG", "jpeg" },
      { GST_VIDEO_CHROMA_SITE_MPEG2, "GST_VIDEO_CHROMA_SITE_MPEG2", "mpeg2" },
      { GST_VIDEO_CHROMA_SITE_DV, "GST_VIDEO_CHROMA_SITE_DV", "dv" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoChromaSite", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_chroma_method_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_CHROMA_METHOD_NEAREST, "GST_VIDEO_CHROMA_METHOD_NEAREST", "nearest" },
      { GST_VIDEO_CHROMA_METHOD_LINEAR, "GST_VIDEO_CHROMA_METHOD_LINEAR", "linear" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoChromaMethod", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_chroma_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_CHROMA_FLAG_NONE, "GST_VIDEO_CHROMA_FLAG_NONE", "none" },
      { GST_VIDEO_CHROMA_FLAG_INTERLACED, "GST_VIDEO_CHROMA_FLAG_INTERLACED", "interlaced" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoChromaFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-color.h" */
GType
gst_video_color_range_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_COLOR_RANGE_UNKNOWN, "GST_VIDEO_COLOR_RANGE_UNKNOWN", "unknown" },
      { GST_VIDEO_COLOR_RANGE_0_255, "GST_VIDEO_COLOR_RANGE_0_255", "0-255" },
      { GST_VIDEO_COLOR_RANGE_16_235, "GST_VIDEO_COLOR_RANGE_16_235", "16-235" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoColorRange", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_color_matrix_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_COLOR_MATRIX_UNKNOWN, "GST_VIDEO_COLOR_MATRIX_UNKNOWN", "unknown" },
      { GST_VIDEO_COLOR_MATRIX_RGB, "GST_VIDEO_COLOR_MATRIX_RGB", "rgb" },
      { GST_VIDEO_COLOR_MATRIX_FCC, "GST_VIDEO_COLOR_MATRIX_FCC", "fcc" },
      { GST_VIDEO_COLOR_MATRIX_BT709, "GST_VIDEO_COLOR_MATRIX_BT709", "bt709" },
      { GST_VIDEO_COLOR_MATRIX_BT601, "GST_VIDEO_COLOR_MATRIX_BT601", "bt601" },
      { GST_VIDEO_COLOR_MATRIX_SMPTE240M, "GST_VIDEO_COLOR_MATRIX_SMPTE240M", "smpte240m" },
      { GST_VIDEO_COLOR_MATRIX_BT2020, "GST_VIDEO_COLOR_MATRIX_BT2020", "bt2020" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoColorMatrix", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_transfer_function_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_TRANSFER_UNKNOWN, "GST_VIDEO_TRANSFER_UNKNOWN", "unknown" },
      { GST_VIDEO_TRANSFER_GAMMA10, "GST_VIDEO_TRANSFER_GAMMA10", "gamma10" },
      { GST_VIDEO_TRANSFER_GAMMA18, "GST_VIDEO_TRANSFER_GAMMA18", "gamma18" },
      { GST_VIDEO_TRANSFER_GAMMA20, "GST_VIDEO_TRANSFER_GAMMA20", "gamma20" },
      { GST_VIDEO_TRANSFER_GAMMA22, "GST_VIDEO_TRANSFER_GAMMA22", "gamma22" },
      { GST_VIDEO_TRANSFER_BT709, "GST_VIDEO_TRANSFER_BT709", "bt709" },
      { GST_VIDEO_TRANSFER_SMPTE240M, "GST_VIDEO_TRANSFER_SMPTE240M", "smpte240m" },
      { GST_VIDEO_TRANSFER_SRGB, "GST_VIDEO_TRANSFER_SRGB", "srgb" },
      { GST_VIDEO_TRANSFER_GAMMA28, "GST_VIDEO_TRANSFER_GAMMA28", "gamma28" },
      { GST_VIDEO_TRANSFER_LOG100, "GST_VIDEO_TRANSFER_LOG100", "log100" },
      { GST_VIDEO_TRANSFER_LOG316, "GST_VIDEO_TRANSFER_LOG316", "log316" },
      { GST_VIDEO_TRANSFER_BT2020_12, "GST_VIDEO_TRANSFER_BT2020_12", "bt2020-12" },
      { GST_VIDEO_TRANSFER_ADOBERGB, "GST_VIDEO_TRANSFER_ADOBERGB", "adobergb" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoTransferFunction", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_color_primaries_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_COLOR_PRIMARIES_UNKNOWN, "GST_VIDEO_COLOR_PRIMARIES_UNKNOWN", "unknown" },
      { GST_VIDEO_COLOR_PRIMARIES_BT709, "GST_VIDEO_COLOR_PRIMARIES_BT709", "bt709" },
      { GST_VIDEO_COLOR_PRIMARIES_BT470M, "GST_VIDEO_COLOR_PRIMARIES_BT470M", "bt470m" },
      { GST_VIDEO_COLOR_PRIMARIES_BT470BG, "GST_VIDEO_COLOR_PRIMARIES_BT470BG", "bt470bg" },
      { GST_VIDEO_COLOR_PRIMARIES_SMPTE170M, "GST_VIDEO_COLOR_PRIMARIES_SMPTE170M", "smpte170m" },
      { GST_VIDEO_COLOR_PRIMARIES_SMPTE240M, "GST_VIDEO_COLOR_PRIMARIES_SMPTE240M", "smpte240m" },
      { GST_VIDEO_COLOR_PRIMARIES_FILM, "GST_VIDEO_COLOR_PRIMARIES_FILM", "film" },
      { GST_VIDEO_COLOR_PRIMARIES_BT2020, "GST_VIDEO_COLOR_PRIMARIES_BT2020", "bt2020" },
      { GST_VIDEO_COLOR_PRIMARIES_ADOBERGB, "GST_VIDEO_COLOR_PRIMARIES_ADOBERGB", "adobergb" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoColorPrimaries", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-converter.h" */
GType
gst_video_alpha_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_ALPHA_MODE_COPY, "GST_VIDEO_ALPHA_MODE_COPY", "copy" },
      { GST_VIDEO_ALPHA_MODE_SET, "GST_VIDEO_ALPHA_MODE_SET", "set" },
      { GST_VIDEO_ALPHA_MODE_MULT, "GST_VIDEO_ALPHA_MODE_MULT", "mult" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoAlphaMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_chroma_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_CHROMA_MODE_FULL, "GST_VIDEO_CHROMA_MODE_FULL", "full" },
      { GST_VIDEO_CHROMA_MODE_UPSAMPLE_ONLY, "GST_VIDEO_CHROMA_MODE_UPSAMPLE_ONLY", "upsample-only" },
      { GST_VIDEO_CHROMA_MODE_DOWNSAMPLE_ONLY, "GST_VIDEO_CHROMA_MODE_DOWNSAMPLE_ONLY", "downsample-only" },
      { GST_VIDEO_CHROMA_MODE_NONE, "GST_VIDEO_CHROMA_MODE_NONE", "none" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoChromaMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_matrix_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_MATRIX_MODE_FULL, "GST_VIDEO_MATRIX_MODE_FULL", "full" },
      { GST_VIDEO_MATRIX_MODE_INPUT_ONLY, "GST_VIDEO_MATRIX_MODE_INPUT_ONLY", "input-only" },
      { GST_VIDEO_MATRIX_MODE_OUTPUT_ONLY, "GST_VIDEO_MATRIX_MODE_OUTPUT_ONLY", "output-only" },
      { GST_VIDEO_MATRIX_MODE_NONE, "GST_VIDEO_MATRIX_MODE_NONE", "none" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoMatrixMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_gamma_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_GAMMA_MODE_NONE, "GST_VIDEO_GAMMA_MODE_NONE", "none" },
      { GST_VIDEO_GAMMA_MODE_REMAP, "GST_VIDEO_GAMMA_MODE_REMAP", "remap" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoGammaMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_primaries_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_PRIMARIES_MODE_NONE, "GST_VIDEO_PRIMARIES_MODE_NONE", "none" },
      { GST_VIDEO_PRIMARIES_MODE_MERGE_ONLY, "GST_VIDEO_PRIMARIES_MODE_MERGE_ONLY", "merge-only" },
      { GST_VIDEO_PRIMARIES_MODE_FAST, "GST_VIDEO_PRIMARIES_MODE_FAST", "fast" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoPrimariesMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-dither.h" */
GType
gst_video_dither_method_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_DITHER_NONE, "GST_VIDEO_DITHER_NONE", "none" },
      { GST_VIDEO_DITHER_VERTERR, "GST_VIDEO_DITHER_VERTERR", "verterr" },
      { GST_VIDEO_DITHER_FLOYD_STEINBERG, "GST_VIDEO_DITHER_FLOYD_STEINBERG", "floyd-steinberg" },
      { GST_VIDEO_DITHER_SIERRA_LITE, "GST_VIDEO_DITHER_SIERRA_LITE", "sierra-lite" },
      { GST_VIDEO_DITHER_BAYER, "GST_VIDEO_DITHER_BAYER", "bayer" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoDitherMethod", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_dither_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_DITHER_FLAG_NONE, "GST_VIDEO_DITHER_FLAG_NONE", "none" },
      { GST_VIDEO_DITHER_FLAG_INTERLACED, "GST_VIDEO_DITHER_FLAG_INTERLACED", "interlaced" },
      { GST_VIDEO_DITHER_FLAG_QUANTIZE, "GST_VIDEO_DITHER_FLAG_QUANTIZE", "quantize" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoDitherFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-format.h" */
GType
gst_video_format_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_FORMAT_UNKNOWN, "GST_VIDEO_FORMAT_UNKNOWN", "unknown" },
      { GST_VIDEO_FORMAT_ENCODED, "GST_VIDEO_FORMAT_ENCODED", "encoded" },
      { GST_VIDEO_FORMAT_I420, "GST_VIDEO_FORMAT_I420", "i420" },
      { GST_VIDEO_FORMAT_YV12, "GST_VIDEO_FORMAT_YV12", "yv12" },
      { GST_VIDEO_FORMAT_YUY2, "GST_VIDEO_FORMAT_YUY2", "yuy2" },
      { GST_VIDEO_FORMAT_UYVY, "GST_VIDEO_FORMAT_UYVY", "uyvy" },
      { GST_VIDEO_FORMAT_AYUV, "GST_VIDEO_FORMAT_AYUV", "ayuv" },
      { GST_VIDEO_FORMAT_RGBx, "GST_VIDEO_FORMAT_RGBx", "rgbx" },
      { GST_VIDEO_FORMAT_BGRx, "GST_VIDEO_FORMAT_BGRx", "bgrx" },
      { GST_VIDEO_FORMAT_xRGB, "GST_VIDEO_FORMAT_xRGB", "xrgb" },
      { GST_VIDEO_FORMAT_xBGR, "GST_VIDEO_FORMAT_xBGR", "xbgr" },
      { GST_VIDEO_FORMAT_RGBA, "GST_VIDEO_FORMAT_RGBA", "rgba" },
      { GST_VIDEO_FORMAT_BGRA, "GST_VIDEO_FORMAT_BGRA", "bgra" },
      { GST_VIDEO_FORMAT_ARGB, "GST_VIDEO_FORMAT_ARGB", "argb" },
      { GST_VIDEO_FORMAT_ABGR, "GST_VIDEO_FORMAT_ABGR", "abgr" },
      { GST_VIDEO_FORMAT_RGB, "GST_VIDEO_FORMAT_RGB", "rgb" },
      { GST_VIDEO_FORMAT_BGR, "GST_VIDEO_FORMAT_BGR", "bgr" },
      { GST_VIDEO_FORMAT_Y41B, "GST_VIDEO_FORMAT_Y41B", "y41b" },
      { GST_VIDEO_FORMAT_Y42B, "GST_VIDEO_FORMAT_Y42B", "y42b" },
      { GST_VIDEO_FORMAT_YVYU, "GST_VIDEO_FORMAT_YVYU", "yvyu" },
      { GST_VIDEO_FORMAT_Y444, "GST_VIDEO_FORMAT_Y444", "y444" },
      { GST_VIDEO_FORMAT_v210, "GST_VIDEO_FORMAT_v210", "v210" },
      { GST_VIDEO_FORMAT_v216, "GST_VIDEO_FORMAT_v216", "v216" },
      { GST_VIDEO_FORMAT_NV12, "GST_VIDEO_FORMAT_NV12", "nv12" },
      { GST_VIDEO_FORMAT_NV21, "GST_VIDEO_FORMAT_NV21", "nv21" },
      { GST_VIDEO_FORMAT_GRAY8, "GST_VIDEO_FORMAT_GRAY8", "gray8" },
      { GST_VIDEO_FORMAT_GRAY16_BE, "GST_VIDEO_FORMAT_GRAY16_BE", "gray16-be" },
      { GST_VIDEO_FORMAT_GRAY16_LE, "GST_VIDEO_FORMAT_GRAY16_LE", "gray16-le" },
      { GST_VIDEO_FORMAT_v308, "GST_VIDEO_FORMAT_v308", "v308" },
      { GST_VIDEO_FORMAT_RGB16, "GST_VIDEO_FORMAT_RGB16", "rgb16" },
      { GST_VIDEO_FORMAT_BGR16, "GST_VIDEO_FORMAT_BGR16", "bgr16" },
      { GST_VIDEO_FORMAT_RGB15, "GST_VIDEO_FORMAT_RGB15", "rgb15" },
      { GST_VIDEO_FORMAT_BGR15, "GST_VIDEO_FORMAT_BGR15", "bgr15" },
      { GST_VIDEO_FORMAT_UYVP, "GST_VIDEO_FORMAT_UYVP", "uyvp" },
      { GST_VIDEO_FORMAT_A420, "GST_VIDEO_FORMAT_A420", "a420" },
      { GST_VIDEO_FORMAT_RGB8P, "GST_VIDEO_FORMAT_RGB8P", "rgb8p" },
      { GST_VIDEO_FORMAT_YUV9, "GST_VIDEO_FORMAT_YUV9", "yuv9" },
      { GST_VIDEO_FORMAT_YVU9, "GST_VIDEO_FORMAT_YVU9", "yvu9" },
      { GST_VIDEO_FORMAT_IYU1, "GST_VIDEO_FORMAT_IYU1", "iyu1" },
      { GST_VIDEO_FORMAT_ARGB64, "GST_VIDEO_FORMAT_ARGB64", "argb64" },
      { GST_VIDEO_FORMAT_AYUV64, "GST_VIDEO_FORMAT_AYUV64", "ayuv64" },
      { GST_VIDEO_FORMAT_r210, "GST_VIDEO_FORMAT_r210", "r210" },
      { GST_VIDEO_FORMAT_I420_10BE, "GST_VIDEO_FORMAT_I420_10BE", "i420-10be" },
      { GST_VIDEO_FORMAT_I420_10LE, "GST_VIDEO_FORMAT_I420_10LE", "i420-10le" },
      { GST_VIDEO_FORMAT_I422_10BE, "GST_VIDEO_FORMAT_I422_10BE", "i422-10be" },
      { GST_VIDEO_FORMAT_I422_10LE, "GST_VIDEO_FORMAT_I422_10LE", "i422-10le" },
      { GST_VIDEO_FORMAT_Y444_10BE, "GST_VIDEO_FORMAT_Y444_10BE", "y444-10be" },
      { GST_VIDEO_FORMAT_Y444_10LE, "GST_VIDEO_FORMAT_Y444_10LE", "y444-10le" },
      { GST_VIDEO_FORMAT_GBR, "GST_VIDEO_FORMAT_GBR", "gbr" },
      { GST_VIDEO_FORMAT_GBR_10BE, "GST_VIDEO_FORMAT_GBR_10BE", "gbr-10be" },
      { GST_VIDEO_FORMAT_GBR_10LE, "GST_VIDEO_FORMAT_GBR_10LE", "gbr-10le" },
      { GST_VIDEO_FORMAT_NV16, "GST_VIDEO_FORMAT_NV16", "nv16" },
      { GST_VIDEO_FORMAT_NV24, "GST_VIDEO_FORMAT_NV24", "nv24" },
      { GST_VIDEO_FORMAT_NV12_64Z32, "GST_VIDEO_FORMAT_NV12_64Z32", "nv12-64z32" },
      { GST_VIDEO_FORMAT_A420_10BE, "GST_VIDEO_FORMAT_A420_10BE", "a420-10be" },
      { GST_VIDEO_FORMAT_A420_10LE, "GST_VIDEO_FORMAT_A420_10LE", "a420-10le" },
      { GST_VIDEO_FORMAT_A422_10BE, "GST_VIDEO_FORMAT_A422_10BE", "a422-10be" },
      { GST_VIDEO_FORMAT_A422_10LE, "GST_VIDEO_FORMAT_A422_10LE", "a422-10le" },
      { GST_VIDEO_FORMAT_A444_10BE, "GST_VIDEO_FORMAT_A444_10BE", "a444-10be" },
      { GST_VIDEO_FORMAT_A444_10LE, "GST_VIDEO_FORMAT_A444_10LE", "a444-10le" },
      { GST_VIDEO_FORMAT_NV61, "GST_VIDEO_FORMAT_NV61", "nv61" },
      { GST_VIDEO_FORMAT_P010_10BE, "GST_VIDEO_FORMAT_P010_10BE", "p010-10be" },
      { GST_VIDEO_FORMAT_P010_10LE, "GST_VIDEO_FORMAT_P010_10LE", "p010-10le" },
      { GST_VIDEO_FORMAT_IYU2, "GST_VIDEO_FORMAT_IYU2", "iyu2" },
      { GST_VIDEO_FORMAT_VYUY, "GST_VIDEO_FORMAT_VYUY", "vyuy" },
      { GST_VIDEO_FORMAT_GBRA, "GST_VIDEO_FORMAT_GBRA", "gbra" },
      { GST_VIDEO_FORMAT_GBRA_10BE, "GST_VIDEO_FORMAT_GBRA_10BE", "gbra-10be" },
      { GST_VIDEO_FORMAT_GBRA_10LE, "GST_VIDEO_FORMAT_GBRA_10LE", "gbra-10le" },
      { GST_VIDEO_FORMAT_GBR_12BE, "GST_VIDEO_FORMAT_GBR_12BE", "gbr-12be" },
      { GST_VIDEO_FORMAT_GBR_12LE, "GST_VIDEO_FORMAT_GBR_12LE", "gbr-12le" },
      { GST_VIDEO_FORMAT_GBRA_12BE, "GST_VIDEO_FORMAT_GBRA_12BE", "gbra-12be" },
      { GST_VIDEO_FORMAT_GBRA_12LE, "GST_VIDEO_FORMAT_GBRA_12LE", "gbra-12le" },
      { GST_VIDEO_FORMAT_I420_12BE, "GST_VIDEO_FORMAT_I420_12BE", "i420-12be" },
      { GST_VIDEO_FORMAT_I420_12LE, "GST_VIDEO_FORMAT_I420_12LE", "i420-12le" },
      { GST_VIDEO_FORMAT_I422_12BE, "GST_VIDEO_FORMAT_I422_12BE", "i422-12be" },
      { GST_VIDEO_FORMAT_I422_12LE, "GST_VIDEO_FORMAT_I422_12LE", "i422-12le" },
      { GST_VIDEO_FORMAT_Y444_12BE, "GST_VIDEO_FORMAT_Y444_12BE", "y444-12be" },
      { GST_VIDEO_FORMAT_Y444_12LE, "GST_VIDEO_FORMAT_Y444_12LE", "y444-12le" },
      { GST_VIDEO_FORMAT_GRAY10_LE32, "GST_VIDEO_FORMAT_GRAY10_LE32", "gray10-le32" },
      { GST_VIDEO_FORMAT_NV12_10LE32, "GST_VIDEO_FORMAT_NV12_10LE32", "nv12-10le32" },
      { GST_VIDEO_FORMAT_NV16_10LE32, "GST_VIDEO_FORMAT_NV16_10LE32", "nv16-10le32" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoFormat", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_format_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_FORMAT_FLAG_YUV, "GST_VIDEO_FORMAT_FLAG_YUV", "yuv" },
      { GST_VIDEO_FORMAT_FLAG_RGB, "GST_VIDEO_FORMAT_FLAG_RGB", "rgb" },
      { GST_VIDEO_FORMAT_FLAG_GRAY, "GST_VIDEO_FORMAT_FLAG_GRAY", "gray" },
      { GST_VIDEO_FORMAT_FLAG_ALPHA, "GST_VIDEO_FORMAT_FLAG_ALPHA", "alpha" },
      { GST_VIDEO_FORMAT_FLAG_LE, "GST_VIDEO_FORMAT_FLAG_LE", "le" },
      { GST_VIDEO_FORMAT_FLAG_PALETTE, "GST_VIDEO_FORMAT_FLAG_PALETTE", "palette" },
      { GST_VIDEO_FORMAT_FLAG_COMPLEX, "GST_VIDEO_FORMAT_FLAG_COMPLEX", "complex" },
      { GST_VIDEO_FORMAT_FLAG_UNPACK, "GST_VIDEO_FORMAT_FLAG_UNPACK", "unpack" },
      { GST_VIDEO_FORMAT_FLAG_TILED, "GST_VIDEO_FORMAT_FLAG_TILED", "tiled" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoFormatFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_pack_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_PACK_FLAG_NONE, "GST_VIDEO_PACK_FLAG_NONE", "none" },
      { GST_VIDEO_PACK_FLAG_TRUNCATE_RANGE, "GST_VIDEO_PACK_FLAG_TRUNCATE_RANGE", "truncate-range" },
      { GST_VIDEO_PACK_FLAG_INTERLACED, "GST_VIDEO_PACK_FLAG_INTERLACED", "interlaced" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoPackFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-frame.h" */
GType
gst_video_frame_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_FRAME_FLAG_NONE, "GST_VIDEO_FRAME_FLAG_NONE", "none" },
      { GST_VIDEO_FRAME_FLAG_INTERLACED, "GST_VIDEO_FRAME_FLAG_INTERLACED", "interlaced" },
      { GST_VIDEO_FRAME_FLAG_TFF, "GST_VIDEO_FRAME_FLAG_TFF", "tff" },
      { GST_VIDEO_FRAME_FLAG_RFF, "GST_VIDEO_FRAME_FLAG_RFF", "rff" },
      { GST_VIDEO_FRAME_FLAG_ONEFIELD, "GST_VIDEO_FRAME_FLAG_ONEFIELD", "onefield" },
      { GST_VIDEO_FRAME_FLAG_MULTIPLE_VIEW, "GST_VIDEO_FRAME_FLAG_MULTIPLE_VIEW", "multiple-view" },
      { GST_VIDEO_FRAME_FLAG_FIRST_IN_BUNDLE, "GST_VIDEO_FRAME_FLAG_FIRST_IN_BUNDLE", "first-in-bundle" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoFrameFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_buffer_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_BUFFER_FLAG_INTERLACED, "GST_VIDEO_BUFFER_FLAG_INTERLACED", "interlaced" },
      { GST_VIDEO_BUFFER_FLAG_TFF, "GST_VIDEO_BUFFER_FLAG_TFF", "tff" },
      { GST_VIDEO_BUFFER_FLAG_RFF, "GST_VIDEO_BUFFER_FLAG_RFF", "rff" },
      { GST_VIDEO_BUFFER_FLAG_ONEFIELD, "GST_VIDEO_BUFFER_FLAG_ONEFIELD", "onefield" },
      { GST_VIDEO_BUFFER_FLAG_MULTIPLE_VIEW, "GST_VIDEO_BUFFER_FLAG_MULTIPLE_VIEW", "multiple-view" },
      { GST_VIDEO_BUFFER_FLAG_FIRST_IN_BUNDLE, "GST_VIDEO_BUFFER_FLAG_FIRST_IN_BUNDLE", "first-in-bundle" },
      { GST_VIDEO_BUFFER_FLAG_LAST, "GST_VIDEO_BUFFER_FLAG_LAST", "last" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoBufferFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_frame_map_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_FRAME_MAP_FLAG_NO_REF, "GST_VIDEO_FRAME_MAP_FLAG_NO_REF", "no-ref" },
      { GST_VIDEO_FRAME_MAP_FLAG_LAST, "GST_VIDEO_FRAME_MAP_FLAG_LAST", "last" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoFrameMapFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-info.h" */
GType
gst_video_interlace_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_INTERLACE_MODE_PROGRESSIVE, "GST_VIDEO_INTERLACE_MODE_PROGRESSIVE", "progressive" },
      { GST_VIDEO_INTERLACE_MODE_INTERLEAVED, "GST_VIDEO_INTERLACE_MODE_INTERLEAVED", "interleaved" },
      { GST_VIDEO_INTERLACE_MODE_MIXED, "GST_VIDEO_INTERLACE_MODE_MIXED", "mixed" },
      { GST_VIDEO_INTERLACE_MODE_FIELDS, "GST_VIDEO_INTERLACE_MODE_FIELDS", "fields" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoInterlaceMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_multiview_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_MULTIVIEW_MODE_NONE, "GST_VIDEO_MULTIVIEW_MODE_NONE", "none" },
      { GST_VIDEO_MULTIVIEW_MODE_MONO, "GST_VIDEO_MULTIVIEW_MODE_MONO", "mono" },
      { GST_VIDEO_MULTIVIEW_MODE_LEFT, "GST_VIDEO_MULTIVIEW_MODE_LEFT", "left" },
      { GST_VIDEO_MULTIVIEW_MODE_RIGHT, "GST_VIDEO_MULTIVIEW_MODE_RIGHT", "right" },
      { GST_VIDEO_MULTIVIEW_MODE_SIDE_BY_SIDE, "GST_VIDEO_MULTIVIEW_MODE_SIDE_BY_SIDE", "side-by-side" },
      { GST_VIDEO_MULTIVIEW_MODE_SIDE_BY_SIDE_QUINCUNX, "GST_VIDEO_MULTIVIEW_MODE_SIDE_BY_SIDE_QUINCUNX", "side-by-side-quincunx" },
      { GST_VIDEO_MULTIVIEW_MODE_COLUMN_INTERLEAVED, "GST_VIDEO_MULTIVIEW_MODE_COLUMN_INTERLEAVED", "column-interleaved" },
      { GST_VIDEO_MULTIVIEW_MODE_ROW_INTERLEAVED, "GST_VIDEO_MULTIVIEW_MODE_ROW_INTERLEAVED", "row-interleaved" },
      { GST_VIDEO_MULTIVIEW_MODE_TOP_BOTTOM, "GST_VIDEO_MULTIVIEW_MODE_TOP_BOTTOM", "top-bottom" },
      { GST_VIDEO_MULTIVIEW_MODE_CHECKERBOARD, "GST_VIDEO_MULTIVIEW_MODE_CHECKERBOARD", "checkerboard" },
      { GST_VIDEO_MULTIVIEW_MODE_FRAME_BY_FRAME, "GST_VIDEO_MULTIVIEW_MODE_FRAME_BY_FRAME", "frame-by-frame" },
      { GST_VIDEO_MULTIVIEW_MODE_MULTIVIEW_FRAME_BY_FRAME, "GST_VIDEO_MULTIVIEW_MODE_MULTIVIEW_FRAME_BY_FRAME", "multiview-frame-by-frame" },
      { GST_VIDEO_MULTIVIEW_MODE_SEPARATED, "GST_VIDEO_MULTIVIEW_MODE_SEPARATED", "separated" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoMultiviewMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_multiview_frame_packing_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_NONE, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_NONE", "none" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_MONO, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_MONO", "mono" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_LEFT, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_LEFT", "left" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_RIGHT, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_RIGHT", "right" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_SIDE_BY_SIDE, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_SIDE_BY_SIDE", "side-by-side" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_SIDE_BY_SIDE_QUINCUNX, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_SIDE_BY_SIDE_QUINCUNX", "side-by-side-quincunx" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_COLUMN_INTERLEAVED, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_COLUMN_INTERLEAVED", "column-interleaved" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_ROW_INTERLEAVED, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_ROW_INTERLEAVED", "row-interleaved" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_TOP_BOTTOM, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_TOP_BOTTOM", "top-bottom" },
      { GST_VIDEO_MULTIVIEW_FRAME_PACKING_CHECKERBOARD, "GST_VIDEO_MULTIVIEW_FRAME_PACKING_CHECKERBOARD", "checkerboard" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoMultiviewFramePacking", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_multiview_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_MULTIVIEW_FLAGS_NONE, "GST_VIDEO_MULTIVIEW_FLAGS_NONE", "none" },
      { GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST, "GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_VIEW_FIRST", "right-view-first" },
      { GST_VIDEO_MULTIVIEW_FLAGS_LEFT_FLIPPED, "GST_VIDEO_MULTIVIEW_FLAGS_LEFT_FLIPPED", "left-flipped" },
      { GST_VIDEO_MULTIVIEW_FLAGS_LEFT_FLOPPED, "GST_VIDEO_MULTIVIEW_FLAGS_LEFT_FLOPPED", "left-flopped" },
      { GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_FLIPPED, "GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_FLIPPED", "right-flipped" },
      { GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_FLOPPED, "GST_VIDEO_MULTIVIEW_FLAGS_RIGHT_FLOPPED", "right-flopped" },
      { GST_VIDEO_MULTIVIEW_FLAGS_HALF_ASPECT, "GST_VIDEO_MULTIVIEW_FLAGS_HALF_ASPECT", "half-aspect" },
      { GST_VIDEO_MULTIVIEW_FLAGS_MIXED_MONO, "GST_VIDEO_MULTIVIEW_FLAGS_MIXED_MONO", "mixed-mono" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoMultiviewFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_FLAG_NONE, "GST_VIDEO_FLAG_NONE", "none" },
      { GST_VIDEO_FLAG_VARIABLE_FPS, "GST_VIDEO_FLAG_VARIABLE_FPS", "variable-fps" },
      { GST_VIDEO_FLAG_PREMULTIPLIED_ALPHA, "GST_VIDEO_FLAG_PREMULTIPLIED_ALPHA", "premultiplied-alpha" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_field_order_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_FIELD_ORDER_UNKNOWN, "GST_VIDEO_FIELD_ORDER_UNKNOWN", "unknown" },
      { GST_VIDEO_FIELD_ORDER_TOP_FIELD_FIRST, "GST_VIDEO_FIELD_ORDER_TOP_FIELD_FIRST", "top-field-first" },
      { GST_VIDEO_FIELD_ORDER_BOTTOM_FIELD_FIRST, "GST_VIDEO_FIELD_ORDER_BOTTOM_FIELD_FIRST", "bottom-field-first" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoFieldOrder", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

#ifndef GSTREAMER_LITE
/* enumerations from "video-resampler.h" */
GType
gst_video_resampler_method_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_RESAMPLER_METHOD_NEAREST, "GST_VIDEO_RESAMPLER_METHOD_NEAREST", "nearest" },
      { GST_VIDEO_RESAMPLER_METHOD_LINEAR, "GST_VIDEO_RESAMPLER_METHOD_LINEAR", "linear" },
      { GST_VIDEO_RESAMPLER_METHOD_CUBIC, "GST_VIDEO_RESAMPLER_METHOD_CUBIC", "cubic" },
      { GST_VIDEO_RESAMPLER_METHOD_SINC, "GST_VIDEO_RESAMPLER_METHOD_SINC", "sinc" },
      { GST_VIDEO_RESAMPLER_METHOD_LANCZOS, "GST_VIDEO_RESAMPLER_METHOD_LANCZOS", "lanczos" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoResamplerMethod", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_resampler_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_RESAMPLER_FLAG_NONE, "GST_VIDEO_RESAMPLER_FLAG_NONE", "none" },
      { GST_VIDEO_RESAMPLER_FLAG_HALF_TAPS, "GST_VIDEO_RESAMPLER_FLAG_HALF_TAPS", "half-taps" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoResamplerFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video-scaler.h" */
GType
gst_video_scaler_flags_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { GST_VIDEO_SCALER_FLAG_NONE, "GST_VIDEO_SCALER_FLAG_NONE", "none" },
      { GST_VIDEO_SCALER_FLAG_INTERLACED, "GST_VIDEO_SCALER_FLAG_INTERLACED", "interlaced" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_flags_register_static ("GstVideoScalerFlags", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
#endif // GSTREAMER_LITE

/* enumerations from "video-tile.h" */
GType
gst_video_tile_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_TILE_TYPE_INDEXED, "GST_VIDEO_TILE_TYPE_INDEXED", "indexed" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoTileType", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}
GType
gst_video_tile_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_TILE_MODE_UNKNOWN, "GST_VIDEO_TILE_MODE_UNKNOWN", "unknown" },
      { GST_VIDEO_TILE_MODE_ZFLIPZ_2X2, "GST_VIDEO_TILE_MODE_ZFLIPZ_2X2", "zflipz-2x2" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoTileMode", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}

/* enumerations from "video.h" */
GType
gst_video_orientation_method_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { GST_VIDEO_ORIENTATION_IDENTITY, "GST_VIDEO_ORIENTATION_IDENTITY", "identity" },
      { GST_VIDEO_ORIENTATION_90R, "GST_VIDEO_ORIENTATION_90R", "90r" },
      { GST_VIDEO_ORIENTATION_180, "GST_VIDEO_ORIENTATION_180", "180" },
      { GST_VIDEO_ORIENTATION_90L, "GST_VIDEO_ORIENTATION_90L", "90l" },
      { GST_VIDEO_ORIENTATION_HORIZ, "GST_VIDEO_ORIENTATION_HORIZ", "horiz" },
      { GST_VIDEO_ORIENTATION_VERT, "GST_VIDEO_ORIENTATION_VERT", "vert" },
      { GST_VIDEO_ORIENTATION_UL_LR, "GST_VIDEO_ORIENTATION_UL_LR", "ul-lr" },
      { GST_VIDEO_ORIENTATION_UR_LL, "GST_VIDEO_ORIENTATION_UR_LL", "ur-ll" },
      { GST_VIDEO_ORIENTATION_AUTO, "GST_VIDEO_ORIENTATION_AUTO", "auto" },
      { GST_VIDEO_ORIENTATION_CUSTOM, "GST_VIDEO_ORIENTATION_CUSTOM", "custom" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = g_enum_register_static ("GstVideoOrientationMethod", values);
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}



