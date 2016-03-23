#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.tplot.py
# AUTHOR(S): Luca Delucchi
# PURPOSE:   Temporal Plot Tool is a wxGUI component (based on matplotlib)
#            the user to see in a plot the values of one or more temporal
#            datasets for a queried point defined by a coordinate pair.
# COPYRIGHT: (C) 2014-2015 by Luca Delucchi, and the GRASS Development Team
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
#% description: Plots the values of temporal datasets.
#% keywords: general
#% keywords: GUI
#% keywords: temporal
#%end

#%option G_OPT_STVDS_INPUTS
#% key: stvds
#% required: no
#%end

#%option G_OPT_STRDS_INPUTS
#% key: strds
#% required: no
#%end

#%option G_OPT_M_COORDS
#% required: no
#%end

#TODO use option G_OPT_V_CATS
#%option
#% key: cats
#% label: Categories of vectores features
#% description: To use only with stvds
#% required: no
#%end


#%option
#% key: attr
#% label: Name of attribute
#% description: Name of attribute which represent data for plotting
#% required: no
#%end

#%option G_OPT_F_OUTPUT
#% required: no
#% label: Name for output file
#% description: Add extension to specify format (.png, .pdf, .svg)
#%end

#%option
#% key: size
#% type: string
#% label: The size for output image
#% description: It works only with output parameter
#% required: no
#%end

import grass.script as gscript


def main():
    options, flags = gscript.parser()

    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.utils import _
    from core.giface import StandaloneGrassInterface
    try:
        from tplot.frame import TplotFrame
    except ImportError as e:
        gscript.fatal(e.message)
    rasters = None
    if options['strds']:
        rasters = options['strds'].strip().split(',')
    coords = None
    if options['coordinates']:
        coords = options['coordinates'].strip().split(',')
    cats = None
    if options['cats']:
        cats = options['cats']
    output = options['output']
    vectors = None
    attr = None
    if options['stvds']:
        vectors = options['stvds'].strip().split(',')
        if not options['attr']:
            gscript.fatal(_("With stvds you have to set 'attr' option"))
        else:
            attr = options['attr']
        if coords and cats:
            gscript.fatal(_("With stvds it is not possible to use 'coordinates' "
                            "and 'cats' options together"))
        elif not coords and not cats:
            gscript.warning(_("With stvds you have to use 'coordinates' or "
                              "'cats' option"))
    app = wx.App()
    frame = TplotFrame(parent=None, giface=StandaloneGrassInterface())
    frame.SetDatasets(rasters, vectors, coords, cats, attr)
    if output:
        frame.OnRedraw()
        if options['size']:
            sizes = options['size'].strip().split(',')
            sizes = [int(s) for s in sizes]
            frame.canvas.SetSize(sizes)
        frame.canvas.figure.savefig(output)
    else:
        frame.Show()
        app.MainLoop()

if __name__ == '__main__':
    main()
