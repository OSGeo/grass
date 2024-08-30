#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.iclass
# AUTHOR(S): Anna Kratochvilova, Vaclav Petras
# PURPOSE:   The Map Swipe is a wxGUI component which allows the user to
#            interactively compare two maps
# COPYRIGHT: (C) 2012-2013 by Anna Kratochvilova, and the GRASS Development Team
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
# % label: Tool for supervised classification of imagery data.
# % description: Generates spectral signatures for an image by allowing the user to outline regions of interest.
# % keyword: general
# % keyword: GUI
# % keyword: classification
# % keyword: supervised classification
# % keyword: signatures
# %end
# %flag
# % key: m
# % description: Maximize window
# %end
# %option G_OPT_I_GROUP
# % required: no
# %end
# %option G_OPT_I_SUBGROUP
# % required: no
# %end
# %option G_OPT_R_MAP
# % description: Name of raster map to load
# % required: no
# %end
# %option G_OPT_V_MAP
# % key: trainingmap
# % label: Ground truth training map to load
# % description:
# % required: no
# %end

import os
import grass.script as gs


def main():
    gs.set_raise_on_error(False)
    options, flags = gs.parser()

    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    from core.settings import UserSettings
    from core import globalvar
    from iclass.frame import IClassMapDisplay

    group_name = subgroup_name = map_name = trainingmap_name = None

    if options["group"]:
        if not options["subgroup"]:
            gs.fatal(_("Name of subgroup required"))
        group_name = gs.find_file(name=options["group"], element="group")["name"]
        if not group_name:
            gs.fatal(_("Group <%s> not found") % options["group"])
        subgroups = gs.read_command(
            "i.group", group=group_name, flags="sg"
        ).splitlines()
        if options["subgroup"] not in subgroups:
            gs.fatal(_("Subgroup <%s> not found") % options["subgroup"])
        subgroup_name = options["subgroup"]

    if options["map"]:
        map_name = gs.find_file(name=options["map"], element="cell")["fullname"]
        if not map_name:
            gs.fatal(_("Raster map <%s> not found") % options["map"])

    if options["trainingmap"]:
        trainingmap_name = gs.find_file(name=options["trainingmap"], element="vector")[
            "fullname"
        ]
        if not trainingmap_name:
            gs.fatal(_("Vector map <%s> not found") % options["trainingmap"])

    # define display driver
    driver = UserSettings.Get(group="display", key="driver", subkey="type")
    if driver == "png":
        os.environ["GRASS_RENDER_IMMEDIATE"] = "png"
    else:
        os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"

    # launch application
    app = wx.App()

    # show main frame
    frame = wx.Frame(
        parent=None,
        size=globalvar.MAP_WINDOW_SIZE,
        title=_("Supervised Classification Tool - GRASS GIS"),
    )
    frame = IClassMapDisplay(
        parent=frame,
        giface=None,
    )
    if not flags["m"]:
        frame.CenterOnScreen()
    if group_name:
        frame.SetGroup(group_name, subgroup_name)
    if map_name:
        frame.giface.WriteLog(_("Loading raster map <%s>...") % map_name)
        frame.trainingMapManager.AddLayer(map_name)
    if trainingmap_name:
        frame.giface.WriteLog(_("Loading training map <%s>...") % trainingmap_name)
        frame.ImportAreas(trainingmap_name)

    frame.Show()
    if flags["m"]:
        frame.Maximize()
    app.MainLoop()


if __name__ == "__main__":
    main()
