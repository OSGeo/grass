#!/usr/bin/env python3
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
#% keywords: plot
#%end

#%flag
#% key: h
#% description: Set the header of CSV file, to be used with csv option
#%end

#%flag
#% key: l
#% description: Show simple linear regression model line
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

# TODO use option G_OPT_V_CATS
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
#% label: Name for output graphical file
#% description: Full path for output file containing the plot, ddd extension to specify format (.png, .pdf, .svg)
#%end

#%option G_OPT_F_OUTPUT
#% key: csv
#% required: no
#% label: Name for output CSV file
#% description: Full path for the CSV file containing the plotted data
#%end

#%option
#% key: title
#% label: Title for plot
#% description: The title for the output plot
#% required: no
#%end

#%option
#% key: xlabel
#% label: Label for x axis
#% description: The x axis label for the output plot
#% required: no
#%end

#%option
#% key: ylabel
#% label: Label for y axis
#% description: The y axis label for the output plot
#% required: no
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
    title = None
    if options['title']:
        title = options['title']
    xlabel = None
    if options['xlabel']:
        xlabel = options['xlabel']
    ylabel = None
    if options['ylabel']:
        ylabel = options['ylabel']
    csvfile = None
    if options['csv']:
        csvfile = options['csv']
    app = wx.App()
    frame = TplotFrame(
        parent=None,
        giface=StandaloneGrassInterface(),
        title=_("Temporal Plot Tool - GRASS GIS"),
    )
    if flags['l']:
        frame.linRegRaster.SetValue(state=True)
        frame.linRegVector.SetValue(state=True)
    frame.SetDatasets(rasters, vectors, coords, cats, attr, title, xlabel,
                      ylabel, csvfile, flags['h'], gscript .overwrite)
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
