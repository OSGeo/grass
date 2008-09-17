"""
@package nviz.py

@brief Nviz extension for wxGUI

This module enables to visualize data in 2.5/3D space.

Map Display supports standard 2D mode ('mapdisp' module) and 2.5/3D
mode ('nviz_mapdisp' module).

(C) 2008 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
"""

errorMsg = ''

import os
import sys

import wx
try:
    from wx import glcanvas
    haveGLCanvas = True
except ImportError, e:
    haveGLCanvas = False
    errorMsg = e

import globalvar
try:
    sys.path.append(os.path.join(globalvar.ETCWXDIR, "nviz"))
    import grass7_wxnviz as wxnviz
    haveNviz = True
except ImportError, e:
    haveNviz = False
    errorMsg = e

import nviz_mapdisp
import nviz_tools

GLWindow = nviz_mapdisp.GLWindow
NvizToolWindow = nviz_tools.NvizToolWindow
