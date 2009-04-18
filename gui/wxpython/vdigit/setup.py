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
libs = ['grass_dbmibase',
        'grass_dbmiclient',
        'grass_vect',
        'grass_gis',
        'grass_vedit']

for flag in ('GDALCFLAGS',
             'GDALLIBS',
             'WXWIDGETSCXXFLAGS',
             'WXWIDGETSLIB'):
    update_opts(flag, macros, inc_dirs, lib_dirs, libs)

setup(
    ext_modules= [
        Extension(
            name = '_grass7_wxvdigit',
            sources = ["cats.cpp",
                       "driver.cpp",
                       "driver_draw.cpp",
                       "driver_select.cpp",
                       "line.cpp",
                       "message.cpp",
                       "select.cpp",
                       "undo.cpp",
                       "vertex.cpp",
                       "pseudodc.cpp",
                       "digit.cpp",
                       "grass7_wxvdigit.i"],
            swig_opts = ['-c++',
                         '-shadow'],
            define_macros = macros,
            include_dirs = inc_dirs,
            library_dirs = lib_dirs,
            libraries = libs,
            )
	]
    )
