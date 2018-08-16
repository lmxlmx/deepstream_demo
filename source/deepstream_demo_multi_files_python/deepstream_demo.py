#!/usr/bin/python
# coding=utf-8

import sys
import random
import  gi
import deepstream_demopy

gi.require_version('Gst', '1.0')
gi.require_version('GstBase', '1.0')
gi.require_version('GstVideo', '1.0')

#from gi.repository import Gst, Glib ,GObject, GstBase, GstVideo
from gi.repository import Gst, GLib, GObject, GstBase, GstVideo


frame_number = 0
nvdsmeta_quark = 0





def osd_sink_pad_buffer_probe(pad, info):
    return deepstream_demopy.callCfunc(info.data)




def bus_call( bus, message, loop, pipline):
    messagetype = message.type
    if messagetype == Gst.MessageType.EOS:
        sys.stdout.write("End of stream\n")
        loop.quit()
    elif messagetype == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        sys.stderr.write("Error: %s: %s\n" %(err, debug))
        loop.quit()
    # elif messagetype == Gst.MessageType.STATE_CHANGED:
    #     sys.stdout.write("Stream get state_change message %s\n" % message.type)
    #     if message.src == pipline:
    #         sys.stdout.write("Pipline state changed\n")
    #         old_state, new_state, pending_state = message.parse_state_changed()
    #         sys.stdout.write("Pipline state changed from {0:s} to {1:s}\n".format(
    #             Gst.Element.state_get_name(old_state),
    #             Gst.Element.state_get_name(new_state)))
    # elif messagetype == Gst.MessageType.DURATION_CHANGED:
    #     sys.stdout.write("Stream get duration_change message %s\n" % message.type)
    # else:
    #     sys.stdout.write("Stream get some message %s\n" % message.type)
    return True


def add_many(pipline, *args):
    if pipline and args:
        for e in args:
            pipline.add(e)


def link_many(*args):
    if args:
        length = len(args)
        for i in range(length):
            if i == length - 1:
                return
            args[i].link(args[i+1])


def main(args):
    if len(args) !=2:
        sys.stderr.write("usage: %s <media file or uri>\n" % args[0])
        sys.exit(1)

    GObject.threads_init()
    Gst.init(args)
    # Gst.debug_set_default_threshold(Gst.DebugLevel.INFO)
    loop = GObject.MainLoop()

    columns = 2
    rows = 2
    filesrcnum = columns * rows

    pipeline = Gst.Pipeline.new("dstest1-pipeline")
    streammux = Gst.ElementFactory.make("nvstreammux", "nvstream-mux")

    pgie = Gst.ElementFactory.make("nvinfer", "primary-nvinference-engine")
    multitiler = Gst.ElementFactory.make("nvmultistreamtiler", "nvmultisourcetile")

    nvvidconv = Gst.ElementFactory.make("nvvidconv", "nvvideo-converter")

    nvosd = Gst.ElementFactory.make("nvosd", "nv-onscreendisplay")
    sink = Gst.ElementFactory.make("nveglglessink", "nvvideo-renderer")

    filter1 = Gst.ElementFactory.make("capsfilter", "filter1")
    filter2 = Gst.ElementFactory.make("capsfilter", "filter2")

    if (not pipeline) or (not streammux) or (not pgie) \
            or (not multitiler) or (not nvvidconv) or (not nvosd) \
            or (not sink) or (not filter1) or (not filter2):
        sys.stderr.write("One element could not be created. Exiting.\n")
        sys.exit(1)

    pgie.set_property("config-file-path", "demo_pgie_config.txt")
    nvosd.set_property("font-size", 15)

    cap1 = Gst.Caps("video/x-raw(memory:NVMM), format=NV12")
    filter1.set_property("caps", cap1)
    cap2 = Gst.Caps.from_string("video/x-raw(memory:NVMM), format=RGBA")
    filter2.set_property("caps", cap2)

    streammux.set_property("batch-size", filesrcnum)
    multitiler.set_property("columns", columns)
    multitiler.set_property("rows", rows)

    add_many(pipeline, streammux, pgie, multitiler, filter1, nvvidconv, filter2, nvosd, sink)
    link_many(streammux, pgie, multitiler, filter1, nvvidconv, filter2, nvosd, sink)

    for index in range(filesrcnum):
        exec('source{0} = Gst.ElementFactory.make("filesrc","file-source{0}")\n'
             'h264parser{0} = Gst.ElementFactory.make("h264parse", "h264-parser{0}")\n'
             'decoder{0} = Gst.ElementFactory.make("nvdec_h264", "nvh264-decoder{0}")\n'
             'if (not source{0}) or (not h264parser{0}) or (not decoder{0}):\n'
             '    sys.stderr.write("One element could not be created. Exiting.\\n")\n'
             '    sys.exit(1)\n'
             'source{0}.set_property("location", args[1])\n'
             'add_many(pipeline, source{0}, h264parser{0}, decoder{0})\n'
             'link_many(source{0}, h264parser{0}, decoder{0})\n'
             'decoder{0}_src_pad = decoder{0}.get_static_pad("src")\n'
             'streammux_sink_pad{0} = streammux.get_request_pad("sink_{0}")\n'
             'if decoder{0}_src_pad.link(streammux_sink_pad{0}) is not Gst.PadLinkReturn.OK:\n'
             '    sys.stderr.write("streammux can\'t be linked!\\n")\n'
             '    sys.exit(1)\n'.format(index))

    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.enable_sync_message_emission()
    bus.connect("message", bus_call, loop, pipeline)

    osd_sink_pad = nvosd.get_static_pad("sink")
    if not osd_sink_pad:
        sys.stdout.write("Unable to get sink nvosd pad!\n")
    else:
        sys.stdout.write("Get sink nvosd pad!\n")
        osd_probe_id = osd_sink_pad.add_probe(Gst.PadProbeType.BUFFER, osd_sink_pad_buffer_probe)

    sys.stdout.write("Now playing: %s\n" % args[1])
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        sys.stdout.write("ERROR: Unable to set the pipeline to the playing state\n")
        sys.exit(1)
    try:
        sys.stdout.write("Running...\n")
        loop.run()
    except:
        sys.stdout.write("something happened\n")
        pass
    sys.stdout.write("Returned, stopping playback\n")
    pipeline.set_state(Gst.State.NULL)


if __name__ == '__main__':
    sys.exit(main(sys.argv))