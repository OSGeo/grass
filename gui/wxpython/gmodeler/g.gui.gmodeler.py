#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.gmodeler
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   Graphical Modeler to create, edit, and manage models
# COPYRIGHT: (C) 2010-2012 by Martin Landa, and the GRASS Development Team
#
#  This program is free software; you can 1redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
############################################################################

#%module
#% label: Graphical Modeler.
#% description: Allows to interactively create, edit and manage models.
#% keywords: general
#% keywords: GUI
#% keywords: graphical modeler
#% keywords: workflow
#%end
#%option G_OPT_F_INPUT
#% key: file
#% description: Name of model file to be loaded
#% key_desc: name.gxm
#% required: no
#% guisection: Model
#%end

import os
import sys

import  wx
import gettext

import grass.script as grass

sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.giface import StandaloneGrassInterface
from core.globalvar import CheckWxVersion
from gmodeler.frame import ModelFrame

def main():
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
    app = wx.PySimpleApp()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()
    frame = ModelFrame(parent = None, giface = StandaloneGrassInterface())
    if options['file']:
        frame.LoadModelFile(options['file'])
    frame.Show()
    
    app.MainLoop()
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
