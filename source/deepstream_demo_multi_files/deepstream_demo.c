/*
 * Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <glib.h>
#include "gstnvdsmeta.h"

#define MAX_DISPLAY_LEN 64

#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2

gint frame_number = 0;
gchar pgie_classes_str[4][32] = { "Vehicle", "TwoWheeler", "Person",
  "Roadsign"
};

/* osd_sink_pad_buffer_probe  will extract metadata received on OSD sink pad
 * and update params for drawing rectangle, object information etc. */

static GstPadProbeReturn
osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer u_data)
{

  GstMeta *gst_meta = NULL;
  NvDsMeta *nvdsmeta = NULL;
  gpointer state = NULL;
  static GQuark _nvdsmeta_quark = 0;
  GstBuffer *buf = (GstBuffer *) info->data;
  NvDsFrameMeta *frame_meta = NULL;
  guint num_rects = 0, rect_index = 0, l_index = 0;
  NvDsObjectParams *obj_meta = NULL;
  guint i = 0;
  NvOSD_TextParams *txt_params = NULL;
  guint vehicle_count = 0;
  guint person_count = 0;

  if (!_nvdsmeta_quark)
    _nvdsmeta_quark = g_quark_from_static_string (NVDS_META_STRING);

  while ((gst_meta = gst_buffer_iterate_meta (buf, &state))) {
    if (gst_meta_api_type_has_tag (gst_meta->info->api, _nvdsmeta_quark)) {

      nvdsmeta = (NvDsMeta *) gst_meta;

      /* We are interested only in intercepting Meta of type
       * "NVDS_META_FRAME_INFO" as they are from our infer elements. */
      if (nvdsmeta->meta_type == NVDS_META_FRAME_INFO) {
        frame_meta = (NvDsFrameMeta *) nvdsmeta->meta_data;
        if (frame_meta == NULL) {
          g_print ("NvDS Meta contained NULL meta \n");
          return GST_PAD_PROBE_OK;
        }

        /* We reset the num_strings here as we plan to iterate through the
         *  the detected objects and form our own strings.GST_ELEMENT_GET_CLASS
         *  The pipeline generated strings shall be discarded.
         */
        frame_meta->num_strings = 0;

        num_rects = frame_meta->num_rects;

        /* This means we have num_rects in frame_meta->obj_params,
         * now lets iterate through them */

        for (rect_index = 0; rect_index < num_rects; rect_index++) {
          /* Now using above information we need to form a text that should
           * be displayed on top of the bounding box, so lets form it here. */

          obj_meta = (NvDsObjectParams *) & frame_meta->obj_params[rect_index];

          txt_params = &(obj_meta->text_params);
          if (txt_params->display_text)
            g_free (txt_params->display_text);

          txt_params->display_text = g_malloc0 (MAX_DISPLAY_LEN);

          g_snprintf (txt_params->display_text, MAX_DISPLAY_LEN, "%s ",
              pgie_classes_str[obj_meta->class_id]);

          if (obj_meta->class_id == PGIE_CLASS_ID_VEHICLE)
            vehicle_count++;
          if (obj_meta->class_id == PGIE_CLASS_ID_PERSON)
            person_count++;

          /* Now set the offsets where the string should appear */
          txt_params->x_offset = obj_meta->rect_params.left;
          txt_params->y_offset = obj_meta->rect_params.top - 25;

          /* Font , font-color andGST_ELEMENT_GET_CLASS font-size */
          txt_params->font_params.font_name = "Arial";
          txt_params->font_params.font_size = 10;
          txt_params->font_params.font_color.red = 1.0;
          txt_params->font_params.font_color.green = 1.0;
          txt_params->font_params.font_color.blue = 1.0;
          txt_params->font_params.font_color.alpha = 1.0;

          /* Text background color */
          txt_params->set_bg_clr = 1;
          txt_params->text_bg_clr.red = 0.0;
          txt_params->text_bg_clr.green = 0.0;
          txt_params->text_bg_clr.blue = 0.0;
          txt_params->text_bg_clr.alpha = 1.0;

          frame_meta->num_strings++;
        }
      }
    }
  }
  g_print ("Frame Number = %d Number of objects = %d "
      "Vehicle Count = %d Person Count = %d\n",
      frame_number, num_rects, vehicle_count, person_count);
  frame_number++;

  return GST_PAD_PROBE_OK;
}

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      g_free (debug);
      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop = NULL;
  GstElement *pipeline = NULL,
		  *source1 = NULL, *source2 = NULL, *source3 = NULL, *source0 = NULL,
		  *h264parser1 =NULL, *h264parser2=NULL, *h264parser3 =NULL, *h264parser0=NULL,
      *decoder1 = NULL, * decoder2 = NULL, *decoder3 = NULL, * decoder0 = NULL,
      *streammux = NULL,
      *sink = NULL, *pgie = NULL, *nvmultistreamtiler = NULL, *nvvidconv = NULL, *nvosd = NULL, *filter1 = NULL,
      *filter2 = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id;
  GstCaps *caps1 = NULL, *caps2 = NULL;
  GstCaps *caps1_1 = NULL, *caps2_1 = NULL;
  GstCaps *parsecaps = NULL;
  gulong osd_probe_id = 0;
  GstPad *osd_sink_pad = NULL;

  GstPad * streammux_sink_pad1 = NULL;
  GstPad * streammux_sink_pad2 = NULL;
  GstPad * streammux_sink_pad3 = NULL;
  GstPad * streammux_sink_pad0 = NULL;
  GstPad * decoder1_src_pad = NULL;
  GstPad * decoder2_src_pad = NULL;
  GstPad * decoder3_src_pad = NULL;
  GstPad * decoder0_src_pad = NULL;

  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <H264 filename>\n", argv[0]);
    return -1;
  }
/*
  GST_LEVEL_NONE = 0,
    GST_LEVEL_ERROR = 1,
    GST_LEVEL_WARNING = 2,
    GST_LEVEL_FIXME = 3,
    GST_LEVEL_INFO = 4,
    GST_LEVEL_DEBUG = 5,
    GST_LEVEL_LOG = 6,
    GST_LEVEL_TRACE = 7,
    GST_LEVEL_MEMDUMP = 9,
    GST_LEVEL_COUNT*/
  //gst_debug_set_default_threshold(GST_LEVEL_INFO);

  /* Standard GStreamer initialization */
  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  /* Create gstreamer elements */
  /* Create Pipeline element that will form a connection of other elements */
  pipeline = gst_pipeline_new ("dstest1-pipeline");

  /* Source element for reading from the file */
  source1 = gst_element_factory_make ("filesrc", "file-source1");
  source2 = gst_element_factory_make ("filesrc", "file-source2");
  source3 = gst_element_factory_make ("filesrc", "file-source3");
  source0 = gst_element_factory_make ("filesrc", "file-source0");

  /* Since the data format in the input file is elementary h264 stream,
   * we need a h264parser */
  h264parser1 = gst_element_factory_make ("h264parse", "h264-parser1");
  h264parser2 = gst_element_factory_make ("h264parse", "h264-parser2");
  h264parser3 = gst_element_factory_make ("h264parse", "h264-parser3");
  h264parser0 = gst_element_factory_make ("h264parse", "h264-parser0");

  /* Use nvdec_h264 for hardware accelerated decode on GPU */
  decoder1 = gst_element_factory_make ("nvdec_h264", "nvh264-decoder1");
  decoder2 = gst_element_factory_make ("nvdec_h264", "nvh264-decoder2");
  decoder3 = gst_element_factory_make ("nvdec_h264", "nvh264-decoder3");
  decoder0 = gst_element_factory_make ("nvdec_h264", "nvh264-decoder0");


  streammux = gst_element_factory_make("nvstreammux","nvstream-mux");

  /* Use nvinfer to run inferencing on decoder's output,
   * behaviour of inferencing is set through config file */
  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  nvmultistreamtiler = gst_element_factory_make("nvmultistreamtiler", "nvmultisourcetile");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv = gst_element_factory_make ("nvvidconv", "nvvideo-converter");

  /* Create OSD to draw on the converted RGBA buffer */
  nvosd = gst_element_factory_make ("nvosd", "nv-onscreendisplay");

  /* Finally render the osd output */
  sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");

  /* caps filter for nvvidconv to convert NV12 to RGBA as nvosd expects input
   * in RGBA format */
  filter1 = gst_element_factory_make ("capsfilter", "filter1");

  filter2 = gst_element_factory_make ("capsfilter", "filter2");


  if (!pipeline || !source1 || !source2 || !h264parser1 || !h264parser2
	    || !decoder1 || !decoder2 || !streammux || !pgie || !nvmultistreamtiler
	    || !filter1 || !nvvidconv || !filter2 || !nvosd || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* we set the input filename to the source element */
  g_object_set (G_OBJECT (source1), "location", argv[1], NULL);
  g_object_set (G_OBJECT (source2), "location", argv[1], NULL);
  g_object_set (G_OBJECT (source3), "location", argv[1], NULL);
  g_object_set (G_OBJECT (source0), "location", argv[1], NULL);

  g_object_set (G_OBJECT (streammux), "batch-size", 4, NULL);

  g_object_set (G_OBJECT (nvmultistreamtiler), "columns", 2, NULL);
  g_object_set (G_OBJECT (nvmultistreamtiler), "rows", 2, NULL);
  g_object_set (G_OBJECT (nvmultistreamtiler), "show-source", -1, NULL);
  //g_object_set (G_OBJECT (nvmultistreamtiler), "width", 1280, NULL);
  //g_object_set (G_OBJECT (nvmultistreamtiler), "height", 720, NULL);

  //g_object_set (G_OBJECT (nvvidconv), "num-buffers-in-batch", 4, NULL);

  /* Set all the necessary properties of the nvinfer element,
   * the necessary ones are : */
  g_object_set (G_OBJECT (pgie),
      "config-file-path", "demo_pgie_config.txt", NULL);

  g_object_set (G_OBJECT (pgie),"batch-size", 1, NULL);
  g_object_set (G_OBJECT (pgie),"num-buffers-in-batch", 2, NULL);

  /* we set the osd properties here */
  g_object_set (G_OBJECT (nvosd), "font-size", 15, NULL);




  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  caps1 = gst_caps_from_string ("video/x-raw(memory:NVMM), format=NV12");
  g_object_set (G_OBJECT (filter1), "caps", caps1, NULL);
  gst_caps_unref (caps1);
  caps2 = gst_caps_from_string ("video/x-raw(memory:NVMM), format=RGBA");
  g_object_set (G_OBJECT (filter2), "caps", caps2, NULL);
  gst_caps_unref (caps2);


  /* Set up the pipeline */
  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline),
      source1, source2, source3, source0, h264parser1, h264parser2, h264parser3, h264parser0, decoder1, decoder2, decoder3, decoder0, streammux, pgie,
      nvmultistreamtiler, filter1, nvvidconv, filter2, nvosd, sink, NULL);


  /* we link the elements together */
  /* file-source -> h264-parser -> nvh264-decoder ->
   * nvinfer -> filter1 -> nvvidconv -> filter2 -> nvosd -> video-renderer */
  if(!gst_element_link_many(source1, h264parser1, decoder1, NULL)
       || !gst_element_link_many(source2, h264parser2, decoder2, NULL)
       || !gst_element_link_many(source3, h264parser3, decoder3, NULL)
       || !gst_element_link_many(source0, h264parser0, decoder0, NULL)
       || !gst_element_link_many(streammux, pgie, nvmultistreamtiler, filter1, nvvidconv,filter2, nvosd, sink, NULL))
  {
	  g_printerr("Elements can't be linked!");
	  return -1;
  }
  streammux_sink_pad1 = gst_element_get_request_pad(streammux,"sink_1");
  g_print("Obtained request pad %s for streammux.\n",gst_pad_get_name(streammux_sink_pad1));
  streammux_sink_pad2 = gst_element_get_request_pad(streammux,"sink_2");
  g_print("Obtained request pad %s for streammux.\n",gst_pad_get_name(streammux_sink_pad2));
  streammux_sink_pad3 = gst_element_get_request_pad(streammux,"sink_3");
  g_print("Obtained request pad %s for streammux.\n",gst_pad_get_name(streammux_sink_pad3));
  streammux_sink_pad0 = gst_element_get_request_pad(streammux,"sink_0");
  g_print("Obtained request pad %s for streammux.\n",gst_pad_get_name(streammux_sink_pad0));

  decoder1_src_pad = gst_element_get_static_pad(decoder1,"src");
  decoder2_src_pad = gst_element_get_static_pad(decoder2,"src");
  decoder3_src_pad = gst_element_get_static_pad(decoder3,"src");
  decoder0_src_pad = gst_element_get_static_pad(decoder0,"src");
  if( gst_pad_link(decoder1_src_pad,streammux_sink_pad1) != GST_PAD_LINK_OK
		|| gst_pad_link(decoder2_src_pad,streammux_sink_pad2) != GST_PAD_LINK_OK
		||gst_pad_link(decoder3_src_pad,streammux_sink_pad3) != GST_PAD_LINK_OK
		|| gst_pad_link(decoder0_src_pad,streammux_sink_pad0) != GST_PAD_LINK_OK
		  		  )
  {
	  g_printerr("streammux can't be linked!");
	  return -1;
  }
  gst_object_unref(decoder1_src_pad);
  gst_object_unref(decoder2_src_pad);
  gst_object_unref(decoder3_src_pad);
  gst_object_unref(decoder0_src_pad);

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the osd element, since by that time, the buffer would have
   * had got all the metadata. */
  osd_sink_pad = gst_element_get_static_pad (nvosd, "sink");
  if (!osd_sink_pad)
    g_print ("Unable to get sink pad\n");
  else
    osd_probe_id = gst_pad_add_probe (osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
        osd_sink_pad_buffer_probe, NULL, NULL);

  /* Set the pipeline to "playing" state */
  g_print ("Now playing: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait till pipeline encounters an error or EOS */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  return 0;


}
