#!/usr/bin/env python3

############################################################################
#
# MODULE:    Create 3-Dimensional GCPs from elevation and target image
# AUTHOR(S): Yann modified the code (was Markus Metz for the GCP manager)
# PURPOSE:   Georectification and Ground Control Points management for 3D correction.
# SPDX-FileCopyrightText: 2012-2020 Markus Metz
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
############################################################################

# %module
# % description: Georectifies a map and allows managing Ground Control Points for 3D correction.
# % keyword: imagery
# % keyword: GUI
# % keyword: aerial
# % keyword: photo
# % keyword: georectification
# % keyword: geometry
# % keyword: GCP
# %end


"""
Module to run GCP management tool as standalone application.
"""

import os

import grass.script as gs


def main():
    """
    Sets the GRASS display driver
    """
    options, flags = gs.parser()

    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    from core.settings import UserSettings
    from core.giface import StandaloneGrassInterface
    from image2target.ii2t_manager import GCPWizard

    driver = UserSettings.Get(group="display", key="driver", subkey="type")
    if driver == "png":
        os.environ["GRASS_RENDER_IMMEDIATE"] = "png"
    else:
        os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"

    app = wx.App()

    GCPWizard(parent=None, giface=StandaloneGrassInterface())

    app.MainLoop()


if __name__ == "__main__":
    main()
