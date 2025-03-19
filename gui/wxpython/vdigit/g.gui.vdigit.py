#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.vdigit
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   wxGUI Vector Digitizer
# COPYRIGHT: (C) 2007-2013 by Martin Landa, and the GRASS Development Team
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

# %module
# % description: Interactive editing and digitization of vector maps.
# % keyword: general
# % keyword: GUI
# % keyword: vector
# % keyword: editing
# % keyword: digitizer
# %end
# %flag
# % key: c
# % description: Create new vector map if doesn't exist
# %end
# %option G_OPT_V_MAP
# % label: Name of vector map to edit
# %end

import os
import grass.script as gs


def main():
    gs.set_raise_on_error(False)

    options, flags = gs.parser()

    # import wx only after running parser
    # to avoid issues with complex imports when only interface is needed
    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    from core.render import Map
    from core.globalvar import ICONDIR
    from mapdisp.frame import MapPanel
    from gui_core.mapdisp import FrameMixin
    from mapdisp.main import DMonGrassInterface
    from core.settings import UserSettings
    from vdigit.main import haveVDigit, errorMsg
    from grass.exceptions import CalledModuleError

    # define classes which needs imports as local
    # for longer definitions, a separate file would be a better option
    class VDigitMapDisplay(FrameMixin, MapPanel):
        """Map display for wrapping map panel with v.digit methods and frame methods"""

        def __init__(self, parent, vectorMap):
            MapPanel.__init__(
                self, parent=parent, Map=Map(), giface=DMonGrassInterface(None)
            )

            # set system icon
            parent.SetIcon(
                wx.Icon(os.path.join(ICONDIR, "grass_map.ico"), wx.BITMAP_TYPE_ICO)
            )

            # bindings
            parent.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

            # extend shortcuts and create frame accelerator table
            self.shortcuts_table.append(
                (self.OnFullScreen, wx.ACCEL_NORMAL, wx.WXK_F11)
            )
            self._initShortcuts()

            # this giface issue not solved yet, we must set mapframe afterwards
            self._giface._mapframe = self
            # load vector map
            mapLayer = self.GetMap().AddLayer(
                ltype="vector",
                name=vectorMap,
                command=["d.vect", "map=%s" % vectorMap],
                active=True,
                hidden=False,
                opacity=1.0,
                render=True,
            )

            # switch toolbar
            self.AddToolbar("vdigit", fixed=True)

            # start editing
            self.toolbars["vdigit"].StartEditing(mapLayer)
            # use Close instead of QuitVDigit for standalone tool
            self.toolbars["vdigit"].quitDigitizer.disconnect(self.QuitVDigit)
            self.toolbars["vdigit"].quitDigitizer.connect(lambda: self.Close())

            # add Map Display panel to Map Display frame
            sizer = wx.BoxSizer(wx.VERTICAL)
            sizer.Add(self, proportion=1, flag=wx.EXPAND)
            parent.SetSizer(sizer)
            parent.Layout()

    if not haveVDigit:
        gs.fatal(_("Vector digitizer not available. %s") % errorMsg)

    if not gs.find_file(
        name=options["map"], element="vector", mapset=gs.gisenv()["MAPSET"]
    )["fullname"]:
        if not flags["c"]:
            gs.fatal(
                _(
                    "Vector map <%s> not found in current mapset. "
                    "New vector map can be created by providing '-c' flag."
                )
                % options["map"]
            )
        else:
            gs.verbose(_("New vector map <%s> created") % options["map"])
            try:
                gs.run_command("v.edit", map=options["map"], tool="create", quiet=True)
            except CalledModuleError:
                gs.fatal(_("Unable to create new vector map <%s>") % options["map"])

    # allow immediate rendering
    driver = UserSettings.Get(group="display", key="driver", subkey="type")
    if driver == "png":
        os.environ["GRASS_RENDER_IMMEDIATE"] = "png"
    else:
        os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"

    app = wx.App()
    frame = wx.Frame(
        None,
        id=wx.ID_ANY,
        size=(850, 600),
        style=wx.DEFAULT_FRAME_STYLE,
        title=_("Vector Digitizer - GRASS GIS"),
    )
    frame = VDigitMapDisplay(parent=frame, vectorMap=options["map"])
    frame.Show()

    app.MainLoop()


if __name__ == "__main__":
    main()
