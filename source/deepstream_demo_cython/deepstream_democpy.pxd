cdef extern from "deepstream_demo.h":
    ctypedef struct GType:
        pass
    ctypedef struct GstMetaInfo:
        pass
    ctypedef struct GstBuffer:
        pass
    ctypedef struct GstPad:
        pass
    ctypedef struct GstPadProbeInfo:
        pass
    ctypedef struct GDestroyNotify:
        pass
    ctypedef enum NvDsMetaType:
        pass
    ctypedef enum GstPadProbeReturn:
        pass
    ctypedef struct NvDsAttr:
        pass
    ctypedef struct NvDsAttrInfo:
        pass
    ctypedef struct NvDsObjectParams:
        pass
    ctypedef struct NvDsFrameMeta:
        pass
    ctypedef struct NvDsMeta:
        pass
    ctypedef void * gpointer 

    cdef GstPadProbeReturn osd_sink_pad_buffer_probe(gpointer data)
