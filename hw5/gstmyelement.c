/* GStreamer
 * Copyright (C) 2023 FIXME <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstmyelement
 *
 * The myelement element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! myelement ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define UNUSED __attribute__((unused))

#include "gstmyelement.h"
#include <gst/base/gstbasesrc.h>
#include <gst/gst.h>


#define BUFFER_SIZE 1024
#define WAV_HEADER_SIZE 44

GST_DEBUG_CATEGORY_STATIC(gst_myelement_debug_category);
#define GST_CAT_DEFAULT gst_myelement_debug_category

/* prototypes */
enum {
  PROP_0,
  PROP_LOCATON
  /* FILL ME */
};

static void gst_myelement_set_property(GObject *object, guint property_id, const GValue *value,
                                       GParamSpec *pspec);
static void gst_myelement_get_property(GObject *object, guint property_id, GValue *value,
                                       GParamSpec *pspec);
static void gst_myelement_dispose(GObject *object);
static void gst_myelement_finalize(GObject *object);

static GstCaps *gst_myelement_get_caps(GstBaseSrc *src, GstCaps *filter);
static gboolean gst_myelement_negotiate(GstBaseSrc *src);
static GstCaps *gst_myelement_fixate(GstBaseSrc *src, GstCaps *caps);
static gboolean gst_myelement_set_caps(GstBaseSrc *src, GstCaps *caps);
static gboolean gst_myelement_decide_allocation(GstBaseSrc *src, GstQuery *query);
static gboolean gst_myelement_start(GstBaseSrc *src);
static gboolean gst_myelement_stop(GstBaseSrc *src);
static void gst_myelement_get_times(GstBaseSrc *src, GstBuffer *buffer, GstClockTime *start,
                                    GstClockTime *end);
static gboolean gst_myelement_get_size(GstBaseSrc *src, guint64 *size);
static gboolean gst_myelement_is_seekable(GstBaseSrc *src);
static gboolean gst_myelement_prepare_seek_segment(GstBaseSrc *src, GstEvent *seek, GstSegment *segment);
static gboolean gst_myelement_do_seek(GstBaseSrc *src, GstSegment *segment);
static gboolean gst_myelement_unlock(GstBaseSrc *src);
static gboolean gst_myelement_unlock_stop(GstBaseSrc *src);
static gboolean gst_myelement_query(GstBaseSrc *src, GstQuery *query);
static gboolean gst_myelement_event(GstBaseSrc *src, GstEvent *event);
static GstFlowReturn gst_myelement_create(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buf);
static GstFlowReturn gst_myelement_alloc(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buf);
static GstFlowReturn gst_myelement_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);


/* pad templates */

static GstStaticPadTemplate gst_myelement_src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("audio/x-raw, "
                                            "channels = (int) [ 1, 6 ]"));


/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstMyelement, gst_myelement, GST_TYPE_BASE_SRC,
                        GST_DEBUG_CATEGORY_INIT(gst_myelement_debug_category, "myelement", 5,
                                                "debug category for myelement "))

static void gst_myelement_class_init(GstMyelementClass *klass) {

  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS(klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass), &gst_myelement_src_template);

  gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), "Wave source", "Generic",
                                        "Extract data from wave-file to src",
                                        "Sergey Simonov<sb.simonov@gmail.com>");


  gobject_class->set_property = gst_myelement_set_property;
  gobject_class->get_property = gst_myelement_get_property;
  gobject_class->dispose = gst_myelement_dispose;
  gobject_class->finalize = gst_myelement_finalize;
  base_src_class->get_caps = GST_DEBUG_FUNCPTR(gst_myelement_get_caps);
  base_src_class->negotiate = GST_DEBUG_FUNCPTR(gst_myelement_negotiate);
  base_src_class->fixate = GST_DEBUG_FUNCPTR(gst_myelement_fixate);
  base_src_class->set_caps = GST_DEBUG_FUNCPTR(gst_myelement_set_caps);
  base_src_class->decide_allocation = GST_DEBUG_FUNCPTR(gst_myelement_decide_allocation);
  base_src_class->start = GST_DEBUG_FUNCPTR(gst_myelement_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR(gst_myelement_stop);
  base_src_class->get_times = GST_DEBUG_FUNCPTR(gst_myelement_get_times);
  base_src_class->get_size = GST_DEBUG_FUNCPTR(gst_myelement_get_size);
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR(gst_myelement_is_seekable);
  base_src_class->prepare_seek_segment = GST_DEBUG_FUNCPTR(gst_myelement_prepare_seek_segment);
  base_src_class->do_seek = GST_DEBUG_FUNCPTR(gst_myelement_do_seek);
  base_src_class->unlock = GST_DEBUG_FUNCPTR(gst_myelement_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR(gst_myelement_unlock_stop);
  base_src_class->query = GST_DEBUG_FUNCPTR(gst_myelement_query);
  base_src_class->event = GST_DEBUG_FUNCPTR(gst_myelement_event);
  base_src_class->create = GST_DEBUG_FUNCPTR(gst_myelement_create);
  base_src_class->alloc = GST_DEBUG_FUNCPTR(gst_myelement_alloc);
  base_src_class->fill = GST_DEBUG_FUNCPTR(gst_myelement_fill);

  g_object_class_install_property(gobject_class, PROP_LOCATON,
                                  g_param_spec_string("location", "location", "Path to wave file", "",
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gst_myelement_init(UNUSED GstMyelement *myelement) {
}


void gst_myelement_set_property(GObject *object, guint property_id, const GValue *value,
                                GParamSpec *pspec) {
  GstMyelement *myelement = GST_MYELEMENT(object);

  // TODO "location" set
  GST_DEBUG_OBJECT(myelement, "set_property");
  switch (property_id) {
  case PROP_LOCATON:
    if (myelement->location != NULL) {
      g_free(myelement->location);
    }
    myelement->location = g_strdup(g_value_get_string(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_myelement_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  GstMyelement *myelement = GST_MYELEMENT(object);

  // TODO "location" get
  GST_DEBUG_OBJECT(myelement, "get_property");

  switch (property_id) {
  case PROP_LOCATON:
    g_value_set_string(value, myelement->location);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_myelement_dispose(GObject *object) {
  GstMyelement *myelement = GST_MYELEMENT(object);

  GST_DEBUG_OBJECT(myelement, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS(gst_myelement_parent_class)->dispose(object);
}

void gst_myelement_finalize(GObject *object) {
  GstMyelement *myelement = GST_MYELEMENT(object);

  GST_DEBUG_OBJECT(myelement, "finalize");

  /* clean up object here */
  if (myelement->location != NULL) {
    g_free(myelement->location);
  }

  G_OBJECT_CLASS(gst_myelement_parent_class)->finalize(object);
}

/* get caps from subclass */
static GstCaps *gst_myelement_get_caps(GstBaseSrc *src, UNUSED GstCaps *filter) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "get_caps");

  return NULL;
}

/* decide on caps */
static gboolean gst_myelement_negotiate(GstBaseSrc *src) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "negotiate");
  return TRUE;
}

/* called if, in negotiation, caps need fixating */
static GstCaps *gst_myelement_fixate(GstBaseSrc *src, UNUSED GstCaps *caps) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "fixate");

  return NULL;
}

/* notify the subclass of new caps */
static gboolean gst_myelement_set_caps(GstBaseSrc *src, UNUSED GstCaps *caps) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "set_caps");

  return TRUE;
}

/* setup allocation query */
static gboolean gst_myelement_decide_allocation(GstBaseSrc *src, UNUSED GstQuery *query) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "decide_allocation");

  return TRUE;
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean gst_myelement_start(GstBaseSrc *src) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "start");
  GST_DEBUG_OBJECT(myelement, "open file %s", myelement->location);
  myelement->fd = fopen(myelement->location, "r");
  if (!myelement->fd) {
    return FALSE;
  }
  return TRUE;
}

static gboolean gst_myelement_stop(GstBaseSrc *src) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "stop");
  fclose(myelement->fd);
  return TRUE;
}

/* given a buffer, return start and stop time when it should be pushed
 * out. The base class will sync on the clock using these times. */
static void gst_myelement_get_times(GstBaseSrc *src, UNUSED GstBuffer *buffer, UNUSED GstClockTime *start,
                                    UNUSED GstClockTime *end) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "get_times");
}

/* get the total size of the resource in bytes */
static gboolean gst_myelement_get_size(GstBaseSrc *src, UNUSED guint64 *size) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "get_size");
  return FALSE;
}

/* check if the resource is seekable */
static gboolean gst_myelement_is_seekable(GstBaseSrc *src) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "is_seekable");

  return FALSE;
}

/* Prepare the segment on which to perform do_seek(), converting to the
 * current basesrc format. */
static gboolean gst_myelement_prepare_seek_segment(GstBaseSrc *src, UNUSED GstEvent *seek, UNUSED GstSegment *segment) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "prepare_seek_segment");

  return FALSE;
}

/* notify subclasses of a seek */
static gboolean gst_myelement_do_seek(GstBaseSrc *src, UNUSED GstSegment *segment) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "do_seek");

  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean gst_myelement_unlock(GstBaseSrc *src) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "unlock");

  return FALSE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean gst_myelement_unlock_stop(GstBaseSrc *src) {
  GstMyelement *myelement = GST_MYELEMENT(src);

  GST_DEBUG_OBJECT(myelement, "unlock_stop");

  return FALSE;
}

/* notify subclasses of a query */
static gboolean gst_myelement_query(GstBaseSrc *src, GstQuery *query) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  // sfl_debug("query '%s'", gst_query_type_get_name(GST_QUERY_TYPE(query)));
  GST_DEBUG_OBJECT(myelement, "myelement_query '%s'", gst_query_type_get_name(GST_QUERY_TYPE(query)));
  return FALSE;
  // return TRUE;
}

/* notify subclasses of an event */
static gboolean gst_myelement_event(GstBaseSrc *src, UNUSED GstEvent *event) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "event");
  return TRUE;
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
static GstFlowReturn gst_myelement_create(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buf) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "create offset = %ld, size=%d", offset, size);

  GstBuffer *buffer = gst_buffer_new();
  gchar *data = g_malloc(size);
  g_assert(data);
  if (!data) {
    *buf = NULL;
    return GST_FLOW_ERROR;
  }
  if (fseek(myelement->fd, offset + WAV_HEADER_SIZE, SEEK_SET) != 0) {
    *buf = NULL;
    return GST_FLOW_ERROR;
  }

  int rd_sz = fread(data, 1, size, myelement->fd);
  if (rd_sz <= 0) {
    g_free(data);
    *buf = NULL;
    return GST_FLOW_OK;
  }

  GstMemory *memory = gst_memory_new_wrapped(0, data, rd_sz, 0, rd_sz, data, g_free);
  g_assert(memory);
  if (!memory) {
    g_free(data);
    *buf = NULL;
    return GST_FLOW_ERROR;
  }
  gst_buffer_insert_memory(buffer, -1, memory);

  *buf = buffer;
  return GST_FLOW_OK;
}

/* ask the subclass to allocate an output buffer. The default implementation
 * will use the negotiated allocator. */
static GstFlowReturn gst_myelement_alloc(GstBaseSrc *src, UNUSED guint64 offset, UNUSED guint size, UNUSED GstBuffer **buf) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "alloc");
  return GST_FLOW_OK;
}

/* ask the subclass to fill the buffer with data from offset and size */
static GstFlowReturn gst_myelement_fill(GstBaseSrc *src, UNUSED guint64 offset, UNUSED guint size, UNUSED GstBuffer *buf) {
  GstMyelement *myelement = GST_MYELEMENT(src);
  GST_DEBUG_OBJECT(myelement, "fill");
  return GST_FLOW_OK;
}

static gboolean plugin_init(GstPlugin *plugin) {

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register(plugin, "myelement", GST_RANK_NONE, GST_TYPE_MYELEMENT);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "my_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "my_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://github.com/Ampermetr123"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, myelement,
                  "Exercise plugin for OTUS C Developper course", plugin_init, VERSION, "LGPL",
                  PACKAGE_NAME, GST_PACKAGE_ORIGIN)
