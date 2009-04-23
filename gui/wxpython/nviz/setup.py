#!/usr/bin/env python
 
# Setup script for wxGUI vdigit extension.

import os
import sys

sys.path.append('..')
from build_ext import variables
from build_ext import update_opts

from distutils.core import setup, Extension

macros = [('PACKAGE', '"grasslibs"')]
inc_dirs = [os.path.join(variables['GRASS_HOME'],
                         'dist.' + variables['ARCH'],
                         'include')]
lib_dirs = [os.path.join(variables['GRASS_HOME'],
                         'dist.' + variables['ARCH'],
                         'lib')]


libs = ['grass_gis',
        'grass_nviz',
        'grass_ogsf',
        'grass_g3d']

for flag in ('GDALCFLAGS',
             'GDALLIBS',
             'WXWIDGETSCXXFLAGS',
             'WXWIDGETSLIB',
             'XCFLAGS',
             'OPENGLLIB',
             'OPENGLULIB'):
    update_opts(flag, macros, inc_dirs, lib_dirs, libs)

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
            )
	]
    )

