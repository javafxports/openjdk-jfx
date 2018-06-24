


#ifndef __GST_ENUM_TYPES_H__
#define __GST_ENUM_TYPES_H__

#include <glib-object.h>
#include <gst/gstconfig.h>

G_BEGIN_DECLS

/* enumerations from "gstallocator.h" */
GST_API GType gst_allocator_flags_get_type (void);
#define GST_TYPE_ALLOCATOR_FLAGS (gst_allocator_flags_get_type())

/* enumerations from "gstbin.h" */
GST_API GType gst_bin_flags_get_type (void);
#define GST_TYPE_BIN_FLAGS (gst_bin_flags_get_type())

/* enumerations from "gstbuffer.h" */
GST_API GType gst_buffer_flags_get_type (void);
#define GST_TYPE_BUFFER_FLAGS (gst_buffer_flags_get_type())
GST_API GType gst_buffer_copy_flags_get_type (void);
#define GST_TYPE_BUFFER_COPY_FLAGS (gst_buffer_copy_flags_get_type())

/* enumerations from "gstbufferpool.h" */
GST_API GType gst_buffer_pool_acquire_flags_get_type (void);
#define GST_TYPE_BUFFER_POOL_ACQUIRE_FLAGS (gst_buffer_pool_acquire_flags_get_type())

/* enumerations from "gstbus.h" */
GST_API GType gst_bus_flags_get_type (void);
#define GST_TYPE_BUS_FLAGS (gst_bus_flags_get_type())
GST_API GType gst_bus_sync_reply_get_type (void);
#define GST_TYPE_BUS_SYNC_REPLY (gst_bus_sync_reply_get_type())

/* enumerations from "gstcaps.h" */
GST_API GType gst_caps_flags_get_type (void);
#define GST_TYPE_CAPS_FLAGS (gst_caps_flags_get_type())
GST_API GType gst_caps_intersect_mode_get_type (void);
#define GST_TYPE_CAPS_INTERSECT_MODE (gst_caps_intersect_mode_get_type())

/* enumerations from "gstclock.h" */
GST_API GType gst_clock_return_get_type (void);
#define GST_TYPE_CLOCK_RETURN (gst_clock_return_get_type())
GST_API GType gst_clock_entry_type_get_type (void);
#define GST_TYPE_CLOCK_ENTRY_TYPE (gst_clock_entry_type_get_type())
GST_API GType gst_clock_flags_get_type (void);
#define GST_TYPE_CLOCK_FLAGS (gst_clock_flags_get_type())

/* enumerations from "gstdebugutils.h" */
GST_API GType gst_debug_graph_details_get_type (void);
#define GST_TYPE_DEBUG_GRAPH_DETAILS (gst_debug_graph_details_get_type())

/* enumerations from "gstelement.h" */
GST_API GType gst_state_get_type (void);
#define GST_TYPE_STATE (gst_state_get_type())
GST_API GType gst_state_change_return_get_type (void);
#define GST_TYPE_STATE_CHANGE_RETURN (gst_state_change_return_get_type())
GST_API GType gst_state_change_get_type (void);
#define GST_TYPE_STATE_CHANGE (gst_state_change_get_type())
GST_API GType gst_element_flags_get_type (void);
#define GST_TYPE_ELEMENT_FLAGS (gst_element_flags_get_type())

/* enumerations from "gsterror.h" */
GST_API GType gst_core_error_get_type (void);
#define GST_TYPE_CORE_ERROR (gst_core_error_get_type())
GST_API GType gst_library_error_get_type (void);
#define GST_TYPE_LIBRARY_ERROR (gst_library_error_get_type())
GST_API GType gst_resource_error_get_type (void);
#define GST_TYPE_RESOURCE_ERROR (gst_resource_error_get_type())
GST_API GType gst_stream_error_get_type (void);
#define GST_TYPE_STREAM_ERROR (gst_stream_error_get_type())

/* enumerations from "gstevent.h" */
GST_API GType gst_event_type_flags_get_type (void);
#define GST_TYPE_EVENT_TYPE_FLAGS (gst_event_type_flags_get_type())
GST_API GType gst_event_type_get_type (void);
#define GST_TYPE_EVENT_TYPE (gst_event_type_get_type())
GST_API GType gst_stream_flags_get_type (void);
#define GST_TYPE_STREAM_FLAGS (gst_stream_flags_get_type())
GST_API GType gst_qos_type_get_type (void);
#define GST_TYPE_QOS_TYPE (gst_qos_type_get_type())

/* enumerations from "gstformat.h" */
GST_API GType gst_format_get_type (void);
#define GST_TYPE_FORMAT (gst_format_get_type())

/* enumerations from "gstinfo.h" */
GST_API GType gst_debug_level_get_type (void);
#define GST_TYPE_DEBUG_LEVEL (gst_debug_level_get_type())
GST_API GType gst_debug_color_flags_get_type (void);
#define GST_TYPE_DEBUG_COLOR_FLAGS (gst_debug_color_flags_get_type())
GST_API GType gst_stack_trace_flags_get_type (void);
#define GST_TYPE_STACK_TRACE_FLAGS (gst_stack_trace_flags_get_type())
GST_API GType gst_debug_color_mode_get_type (void);
#define GST_TYPE_DEBUG_COLOR_MODE (gst_debug_color_mode_get_type())

/* enumerations from "gstiterator.h" */
GST_API GType gst_iterator_result_get_type (void);
#define GST_TYPE_ITERATOR_RESULT (gst_iterator_result_get_type())
GST_API GType gst_iterator_item_get_type (void);
#define GST_TYPE_ITERATOR_ITEM (gst_iterator_item_get_type())

/* enumerations from "gstmemory.h" */
GST_API GType gst_memory_flags_get_type (void);
#define GST_TYPE_MEMORY_FLAGS (gst_memory_flags_get_type())
GST_API GType gst_map_flags_get_type (void);
#define GST_TYPE_MAP_FLAGS (gst_map_flags_get_type())

/* enumerations from "gstmessage.h" */
GST_API GType gst_message_type_get_type (void);
#define GST_TYPE_MESSAGE_TYPE (gst_message_type_get_type())
GST_API GType gst_structure_change_type_get_type (void);
#define GST_TYPE_STRUCTURE_CHANGE_TYPE (gst_structure_change_type_get_type())
GST_API GType gst_stream_status_type_get_type (void);
#define GST_TYPE_STREAM_STATUS_TYPE (gst_stream_status_type_get_type())
GST_API GType gst_progress_type_get_type (void);
#define GST_TYPE_PROGRESS_TYPE (gst_progress_type_get_type())

/* enumerations from "gstmeta.h" */
GST_API GType gst_meta_flags_get_type (void);
#define GST_TYPE_META_FLAGS (gst_meta_flags_get_type())

/* enumerations from "gstminiobject.h" */
GST_API GType gst_mini_object_flags_get_type (void);
#define GST_TYPE_MINI_OBJECT_FLAGS (gst_mini_object_flags_get_type())
GST_API GType gst_lock_flags_get_type (void);
#define GST_TYPE_LOCK_FLAGS (gst_lock_flags_get_type())

/* enumerations from "gstobject.h" */
GST_API GType gst_object_flags_get_type (void);
#define GST_TYPE_OBJECT_FLAGS (gst_object_flags_get_type())

/* enumerations from "gstpad.h" */
GST_API GType gst_pad_direction_get_type (void);
#define GST_TYPE_PAD_DIRECTION (gst_pad_direction_get_type())
GST_API GType gst_pad_mode_get_type (void);
#define GST_TYPE_PAD_MODE (gst_pad_mode_get_type())
GST_API GType gst_pad_link_return_get_type (void);
#define GST_TYPE_PAD_LINK_RETURN (gst_pad_link_return_get_type())
GST_API GType gst_flow_return_get_type (void);
#define GST_TYPE_FLOW_RETURN (gst_flow_return_get_type())
GST_API GType gst_pad_link_check_get_type (void);
#define GST_TYPE_PAD_LINK_CHECK (gst_pad_link_check_get_type())
GST_API GType gst_pad_probe_type_get_type (void);
#define GST_TYPE_PAD_PROBE_TYPE (gst_pad_probe_type_get_type())
GST_API GType gst_pad_probe_return_get_type (void);
#define GST_TYPE_PAD_PROBE_RETURN (gst_pad_probe_return_get_type())
GST_API GType gst_pad_flags_get_type (void);
#define GST_TYPE_PAD_FLAGS (gst_pad_flags_get_type())

/* enumerations from "gstpadtemplate.h" */
GST_API GType gst_pad_presence_get_type (void);
#define GST_TYPE_PAD_PRESENCE (gst_pad_presence_get_type())
GST_API GType gst_pad_template_flags_get_type (void);
#define GST_TYPE_PAD_TEMPLATE_FLAGS (gst_pad_template_flags_get_type())

/* enumerations from "gstparse.h" */
GST_API GType gst_parse_error_get_type (void);
#define GST_TYPE_PARSE_ERROR (gst_parse_error_get_type())
GST_API GType gst_parse_flags_get_type (void);
#define GST_TYPE_PARSE_FLAGS (gst_parse_flags_get_type())

/* enumerations from "gstpipeline.h" */
GST_API GType gst_pipeline_flags_get_type (void);
#define GST_TYPE_PIPELINE_FLAGS (gst_pipeline_flags_get_type())

/* enumerations from "gstplugin.h" */
GST_API GType gst_plugin_error_get_type (void);
#define GST_TYPE_PLUGIN_ERROR (gst_plugin_error_get_type())
GST_API GType gst_plugin_flags_get_type (void);
#define GST_TYPE_PLUGIN_FLAGS (gst_plugin_flags_get_type())
GST_API GType gst_plugin_dependency_flags_get_type (void);
#define GST_TYPE_PLUGIN_DEPENDENCY_FLAGS (gst_plugin_dependency_flags_get_type())

/* enumerations from "gstpluginfeature.h" */
GST_API GType gst_rank_get_type (void);
#define GST_TYPE_RANK (gst_rank_get_type())

/* enumerations from "gstpromise.h" */
GST_API GType gst_promise_result_get_type (void);
#define GST_TYPE_PROMISE_RESULT (gst_promise_result_get_type())

/* enumerations from "gstquery.h" */
GST_API GType gst_query_type_flags_get_type (void);
#define GST_TYPE_QUERY_TYPE_FLAGS (gst_query_type_flags_get_type())
GST_API GType gst_query_type_get_type (void);
#define GST_TYPE_QUERY_TYPE (gst_query_type_get_type())
GST_API GType gst_buffering_mode_get_type (void);
#define GST_TYPE_BUFFERING_MODE (gst_buffering_mode_get_type())
GST_API GType gst_scheduling_flags_get_type (void);
#define GST_TYPE_SCHEDULING_FLAGS (gst_scheduling_flags_get_type())

/* enumerations from "gstsegment.h" */
GST_API GType gst_seek_type_get_type (void);
#define GST_TYPE_SEEK_TYPE (gst_seek_type_get_type())
GST_API GType gst_seek_flags_get_type (void);
#define GST_TYPE_SEEK_FLAGS (gst_seek_flags_get_type())
GST_API GType gst_segment_flags_get_type (void);
#define GST_TYPE_SEGMENT_FLAGS (gst_segment_flags_get_type())

/* enumerations from "gststreams.h" */
GST_API GType gst_stream_type_get_type (void);
#define GST_TYPE_STREAM_TYPE (gst_stream_type_get_type())

/* enumerations from "gstsystemclock.h" */
GST_API GType gst_clock_type_get_type (void);
#define GST_TYPE_CLOCK_TYPE (gst_clock_type_get_type())

/* enumerations from "gsttaglist.h" */
GST_API GType gst_tag_merge_mode_get_type (void);
#define GST_TYPE_TAG_MERGE_MODE (gst_tag_merge_mode_get_type())
GST_API GType gst_tag_flag_get_type (void);
#define GST_TYPE_TAG_FLAG (gst_tag_flag_get_type())
GST_API GType gst_tag_scope_get_type (void);
#define GST_TYPE_TAG_SCOPE (gst_tag_scope_get_type())

/* enumerations from "gsttask.h" */
GST_API GType gst_task_state_get_type (void);
#define GST_TYPE_TASK_STATE (gst_task_state_get_type())

/* enumerations from "gsttoc.h" */
GST_API GType gst_toc_scope_get_type (void);
#define GST_TYPE_TOC_SCOPE (gst_toc_scope_get_type())
GST_API GType gst_toc_entry_type_get_type (void);
#define GST_TYPE_TOC_ENTRY_TYPE (gst_toc_entry_type_get_type())
GST_API GType gst_toc_loop_type_get_type (void);
#define GST_TYPE_TOC_LOOP_TYPE (gst_toc_loop_type_get_type())

/* enumerations from "gsttracerrecord.h" */
GST_API GType gst_tracer_value_scope_get_type (void);
#define GST_TYPE_TRACER_VALUE_SCOPE (gst_tracer_value_scope_get_type())
GST_API GType gst_tracer_value_flags_get_type (void);
#define GST_TYPE_TRACER_VALUE_FLAGS (gst_tracer_value_flags_get_type())

/* enumerations from "gsttypefind.h" */
GST_API GType gst_type_find_probability_get_type (void);
#define GST_TYPE_TYPE_FIND_PROBABILITY (gst_type_find_probability_get_type())

/* enumerations from "gsturi.h" */
GST_API GType gst_uri_error_get_type (void);
#define GST_TYPE_URI_ERROR (gst_uri_error_get_type())
GST_API GType gst_uri_type_get_type (void);
#define GST_TYPE_URI_TYPE (gst_uri_type_get_type())

/* enumerations from "gstutils.h" */
GST_API GType gst_search_mode_get_type (void);
#define GST_TYPE_SEARCH_MODE (gst_search_mode_get_type())
G_END_DECLS

#endif /* __GST_ENUM_TYPES_H__ */



