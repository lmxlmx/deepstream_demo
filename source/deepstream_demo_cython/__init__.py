cdef extern from "../includes/gstnvdsmeta.h":
    ctypedef struct GType:
        pass
    ctypedef struct GstMetaInfo:
        pass
    ctypedef struct GstBuffer:
        pass
    ctypedef void* gpointer
    ctypedef struct GDestroyNotify:
        pass
    ctypedef enum NvDsMetaType:
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

    GType nvds_meta_api_get_type ()
    const GstMetaInfo *nvds_meta_get_info ()
    NvDsMeta* gst_buffer_add_nvds_meta (GstBuffer *buffer, gpointer meta_data,GDestroyNotify destroy)
    NvDsMeta* gst_buffer_get_nvds_meta (GstBuffer *buffer)
    NvDsFrameMeta* nvds_copy_frame_meta (NvDsFrameMeta *src)
    void nvds_free_frame_meta (NvDsFrameMeta *params)
