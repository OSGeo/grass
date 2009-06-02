#!/usr/bin/env python
 
# Setup script for wxGUI vdigit extension.

import os
import sys

sys.path.append('..')
from build_ext import update_opts

from distutils.core import setup, Extension

macros = [('PACKAGE', '"grasslibs"')]
inc_dirs = [os.path.join(os.path.normpath(os.getenv('ARCH_DISTDIR')), 'include'),
	    os.path.join(os.path.normpath(os.getenv('GISBASE')), 'include')]
lib_dirs = [os.path.join(os.path.normpath(os.getenv('ARCH_DISTDIR')), 'lib'),
	    os.path.join(os.path.normpath(os.getenv('GISBASE')), 'lib')]
libs = ['grass_gis',
        'grass_nviz',
        'grass_ogsf',
        'grass_g3d']
extras = []

for flag in ['GDALCFLAGS',
             'GDALLIBS',
             'WXWIDGETSCXXFLAGS',
             'OPENGLINC',
             'OPENGLLIB',
             'OPENGLULIB']:
    update_opts(os.getenv(flag), macros, inc_dirs, lib_dirs, libs, extras)
if sys.platform != 'darwin':
    update_opts(os.getenv('WXWIDGETSLIB'), macros, inc_dirs, lib_dirs, libs, extras)
if os.getenv('OPENGL_X11') == '1':
    update_opts(os.getenv('XCFLAGS'), macros, inc_dirs, lib_dirs, libs, extras)

setup(
    ext_modules= [
        Extension(
            name = '_grass7_wxnviz',
            sources=["change_view.cpp",
                     "draw.cpp",
                     "init.cpp",
                     "lights.cpp",
                     "load.cpp",
                     "surface.cpp",
                     "vector.cpp",
                     "volume.cpp",
                     "grass7_wxnviz.i"],
            swig_opts = ['-c++',
                         '-shadow'],
            define_macros = macros,
            include_dirs = inc_dirs,
            library_dirs = lib_dirs,
            libraries = libs,
            extra_link_args = extras,
            )
	]
    )

