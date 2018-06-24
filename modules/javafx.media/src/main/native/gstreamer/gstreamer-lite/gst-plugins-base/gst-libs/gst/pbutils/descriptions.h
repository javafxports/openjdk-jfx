/* GStreamer base utils library source/sink/codec description support
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_PB_UTILS_DESCRIPTIONS_H__
#define __GST_PB_UTILS_DESCRIPTIONS_H__

#include <gst/gsttaglist.h>
#include <gst/gstcaps.h>
#include <gst/pbutils/pbutils-prelude.h>

G_BEGIN_DECLS

/*
 * functions for use by demuxers or decoders to add CODEC tags to tag lists
 * from caps
 */

GST_PBUTILS_API
gboolean   gst_pb_utils_add_codec_description_to_tag_list (GstTagList    * taglist,
                                                             const gchar   * codec_tag,
                                                             const GstCaps * caps);

GST_PBUTILS_API
gchar    * gst_pb_utils_get_codec_description (const GstCaps * caps);

/*
 * functions mainly used by the missing plugins message creation functions to
 * find descriptions of what exactly is missing
 */

GST_PBUTILS_API
gchar    * gst_pb_utils_get_source_description (const gchar * protocol);

GST_PBUTILS_API
gchar    * gst_pb_utils_get_sink_description (const gchar * protocol);

GST_PBUTILS_API
gchar    * gst_pb_utils_get_decoder_description (const GstCaps * caps);

GST_PBUTILS_API
gchar    * gst_pb_utils_get_encoder_description (const GstCaps * caps);

GST_PBUTILS_API
gchar    * gst_pb_utils_get_element_description (const gchar * factory_name);


G_END_DECLS

#endif /* __GST_PB_UTILS_DESCRIPTIONS_H__ */

