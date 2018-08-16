from cpython.pycapsule cimport *
from libc.stdlib cimport malloc, free
#cimport deepstream_democpy
from deepstream_democpy cimport *
#cdef class GstPadProbeInfo_wrap(object): 
#    cdef deepstream_democpy.GstPadProbeInfo *ptr 
#    def __init__(self): 
#        self.ptr = <deepstream_democpy.GstPadProbeInfo *>malloc(sizeof(deepstream_democpy.GstPadProbeInfo)) 
#    def __del__(self): 
#        free(self.ptr)
#    def set_data(self, value):
#        self.ptr.data = value
   

cpdef callCfunc(long data):
    #print(PyCapsule_GetName(pad))
    #print(PyCapsule_GetName(info))
    #cpad = <deepstream_democpy.GstPad *>PyCapsule_GetPointer(pad,"GstPad")
    #cinfo = <deepstream_democpy.GstPadProbeInfo *>PyCapsule_GetPointer(info,"GstPadProbeInfo")
    #return deepstream_democpy.osd_sink_pad_buffer_probe(cpad, cinfo)
    cdef gpointer cdata = <gpointer> data
    return osd_sink_pad_buffer_probe(cdata)

