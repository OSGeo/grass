#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.timeline.py
# AUTHOR(S): Anna Kratochvilova
# PURPOSE:   Timeline Tool is a wxGUI component (based on matplotlib)
#            which allows the user to compare temporal datasets' extents.
# COPYRIGHT: (C) 2012-13 by Anna Kratochvilova, and the GRASS Development Team
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

# %module
# % description: Allows comparing temporal datasets by displaying their temporal extents in a plot.
# % keyword: general
# % keyword: GUI
# % keyword: temporal
# % keyword: plot
# %end
# %option G_OPT_STDS_INPUTS
# % required: no
# %end
# %flag
# % key: 3
# % description: Show also 3D plot of spatio-temporal extents
# %end

import grass.script as gs


def main():
    options, flags = gs.parser()

    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    try:
        from timeline.frame import TimelineFrame
    except ImportError as e:
        # TODO: why do we need this special check here, the reason of error
        # is wrong installation or something, no need to report this to the
        # user in a nice way
        gs.fatal(str(e))

    datasets = options["inputs"].strip().split(",")
    datasets = [data for data in datasets if data]
    view3d = flags["3"]

    app = wx.App()
    frame = TimelineFrame(
        parent=None,
        title=_("Timeline Tool - GRASS GIS"),
    )
    frame.SetDatasets(datasets)
    frame.Show3D(view3d)
    frame.Show()
    app.MainLoop()


if __name__ == "__main__":
    main()
