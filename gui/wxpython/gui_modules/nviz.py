"""!
@package nviz.py

@brief Nviz (3D view) module

This module implements 3D visualization mode for map display.

Map Display supports standard 2D view mode ('mapdisp' module) and
2.5/3D mode ('nviz_mapdisp' module).

(C) 2008, 2010-2011 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Anna Kratochvilova <KratochAnna seznam.cz> (Google SoC 2011)
"""

errorMsg = ''

import os
import sys

import wx
import globalvar
try:
    from wx import glcanvas
    import nviz_mapdisp
    import nviz_tools
    import wxnviz
    haveNviz = True
except ImportError, err:
    haveNviz = False
    errorMsg = err

if haveNviz:
    GLWindow = nviz_mapdisp.GLWindow
    NvizToolWindow = nviz_tools.NvizToolWindow
else:
    GLWindow = None
    NvizToolWindow = None
