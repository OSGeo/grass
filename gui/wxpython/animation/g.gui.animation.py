#!/usr/bin/env python
############################################################################
#
# MODULE:    Animation
# AUTHOR(S): Anna Kratochvilova
# PURPOSE:   Tool for animating a series of GRASS raster and vector maps
#            or a space time raster dataset
# COPYRIGHT: (C) 2012 by Anna Kratochvilova, and the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
############################################################################

#%module
#% description: Tool for animating a series of raster and vector maps or a space time raster or vector dataset.
#% keywords: general
#% keywords: GUI
#% keywords: display
#%end
#%option G_OPT_R_INPUTS
#% key: rast
#% description: Raster maps to animate
#% required: no
#% guisection: Input
#%end
#%option G_OPT_V_INPUTS
#% key: vect
#% label: Vector maps to animate
#% required: no
#% guisection: Input
#%end
#%option G_OPT_STRDS_INPUT
#% key: strds
#% description: Space time raster dataset to animate
#% required: no
#% guisection: Input
#%end
#%option G_OPT_STVDS_INPUT
#% key: stvds
#% description: Space time vector dataset to animate
#% required: no
#% guisection: Input
#%end

import os
import sys

import wx

import grass.script as grass

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.settings import UserSettings
from core.globalvar import CheckWxVersion
from core.giface import StandaloneGrassInterface
from core.utils import _, GuiModuleMain
from animation.frame import AnimationFrame, MAX_COUNT

def main():
    rast = options['rast']
    vect = options['vect']
    strds = options['strds']
    stvds = options['stvds']
    
    dataType=None
    inputs=None
    numInputs=0
    
    if rast:
        numInputs += 1
    if vect:
        numInputs += 1
    if strds:
        numInputs += 1
    if stvds:
        numInputs += 1

    if  numInputs > 1:
        grass.fatal(_("Options 'rast', 'vect', 'strds' and 'stvds' are mutually exclusive."))

    if rast:
        inputs = [rast.split(',')] + [None] * (MAX_COUNT - 1)
        dataType='rast'
    if vect:
        inputs = [vect.split(',')] + [None] * (MAX_COUNT - 1)
        dataType='vect'
    if strds:
        inputs = [strds] + [None] * (MAX_COUNT - 1)
        dataType='strds'
    if stvds:
        inputs = [stvds] + [None] * (MAX_COUNT - 1)
        dataType='stvds'

    app = wx.PySimpleApp()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()

    frame = AnimationFrame(parent = None)
    frame.CentreOnScreen()
    frame.Show()
    frame.SetAnimations(inputs = inputs, dataType = dataType)
    app.MainLoop()

if __name__ == '__main__':
    options, flags = grass.parser()
    
    GuiModuleMain(main)
