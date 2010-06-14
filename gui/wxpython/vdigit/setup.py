#!/usr/bin/env python
 
# Setup script for wxGUI vdigit extension.

import os
import sys

sys.path.append('..')
from build_ext import update_opts

from distutils.core import setup, Extension

if sys.platform == "win32":
    package = '\\"grasslibs\\"'
else:
    package = '"grasslibs"'
macros = [('PACKAGE', package)]

inc_dirs = [os.path.join(os.path.normpath(os.getenv('ARCH_DISTDIR')), 'include'),
	    os.path.join(os.path.normpath(os.getenv('GISBASE')), 'include')]
lib_dirs = [os.path.join(os.path.normpath(os.getenv('ARCH_DISTDIR')), 'lib'),
	    os.path.join(os.path.normpath(os.getenv('GISBASE')), 'lib')]

gversion = os.getenv('GRASS_VERSION_NUMBER')
libs = ['grass_%s.%s' % (name, gversion)
        for name in ['dbmibase',
                     'dbmiclient',
                     'vector',
                     'gis',
                     'vedit']]

extras = []

for flag in ['CXXFLAGS',
	     'GDALCFLAGS',
             'GDALLIBS',
             'GEOSCFLAGS',
             'WXWIDGETSLIB',
             'WXWIDGETSCXXFLAGS']:
    update_opts(os.getenv(flag), macros, inc_dirs, lib_dirs, libs, extras)

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
            extra_link_args = extras,
            )
	]
    )

