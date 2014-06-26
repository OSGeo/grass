#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.tplot.py
# AUTHOR(S): Luca Delucchi
# PURPOSE:   Temporal Plot Tool is a wxGUI component (based on matplotlib)
#            the user to see in a plot the values of one or more temporal
#            datasets for a queried point defined by a coordinate pair.
# COPYRIGHT: (C) 2014 by Luca Delucchi, and the GRASS Development Team
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
#% description: Allows the user to see in a plot the values of one or more temporal datasets for a queried point defined by a coordinate pair.
#% keywords: general
#% keywords: GUI
#% keywords: temporal
#%end
#%option G_OPT_STDS_INPUTS
#% required: no
#%end
#%option G_OPT_M_COORDS
#% required: no
#%end
#%option G_OPT_F_OUTPUT
#% required: no
#%end
#%option
#% key: dpi
#% type: integer
#% label: The DPI for output image
#% description: To use only with output parameters
#% required: no
#%end

## #%flag
## #% key: 3
## #% description: Show also 3D plot of spatio-temporal extents
## #%end

import  wx

import grass.script as grass
from core.utils import GuiModuleMain


def main():
    try:
        from tplot.frame import TplotFrame
    except ImportError as e:
        grass.fatal(e.message)

    datasets = options['inputs'].strip().split(',')
    datasets = [data for data in datasets if data]
    coords = options['coordinates'].strip().split(',')
    output = options['output']
    dpi = options['dpi']
#    view3d = flags['3']
    if dpi and not output:
        grass.warning(_("No output filename set, so DPI option will not used"))

    app = wx.App()
    frame = TplotFrame(None)
    frame.SetDatasets(datasets, coords, output, dpi)
    if output:
        return 
#    frame.Show3D(view3d)
    frame.Show()
    app.MainLoop()


if __name__ == '__main__':
    options, flags = grass.parser()

    GuiModuleMain(main)
