from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
from Cython.Distutils import build_ext
ext_modules = [
        Extension("deepstream_demopy", 
                   ["deepstream_demopy.pyx","deepstream_demo.c"],
                   libraries=["gstreamer-1.0","gobject-2.0","glib-2.0"],
                   include_dirs=["../includes",
                                 "/usr/include/gstreamer-1.0", 
                                 "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/include",
                                 "/usr/include/glib-2.0",
                                 "/usr/lib/x86_64-linux-gnu/glib-2.0/include"],
                   extra_compile_args=[],
                   extra_link_args=[])
]
setup(
    name = 'deepstream_demopy',
    cmdclass = {"build_ext": build_ext},
    ext_modules = cythonize(ext_modules),
)
