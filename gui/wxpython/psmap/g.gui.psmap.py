#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.psmap
# AUTHOR(S): Anna Kratochvilova <kratochanna gmail.com>
# PURPOSE:   Cartographic Composer
# COPYRIGHT: (C) 2011-2012 by Anna Kratochvilova, and the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or
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
#% description: Tool for creating hardcopy map outputs.
#% keywords: general
#% keywords: GUI
#% keywords: printing
#%end
#%option G_OPT_F_INPUT
#% key: file
#% label: File containing mapping instructions to load
#% description: See ps.map manual for details
#% key_desc: name
#% required: no
#%end

import os
import sys

import  wx

import grass.script as grass

wxbase = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
if wxbase not in sys.path:
    sys.path.append(wxbase)

from core.globalvar import CheckWxVersion
from core.utils     import _, GuiModuleMain
from psmap.frame        import PsMapFrame
from psmap.instructions import Instruction

def main():
    app = wx.PySimpleApp()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()
    frame = PsMapFrame(parent = None)
    frame.Show()

    if options['file']:
        frame.LoadFile(options['file'])

    app.MainLoop()

if __name__ == "__main__":
    options, flags = grass.parser()
    
    GuiModuleMain(main)
