#!/usr/bin/env python3

############################################################################
#
# MODULE:    GCP Manager
# AUTHOR(S): Markus Metz
# PURPOSE:   Georectification and Ground Control Points management.
# COPYRIGHT: (C) 2012 by Markus Metz, and the GRASS Development Team
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
#% description: Georectifies a map and allows managing Ground Control Points.
#% keyword: general
#% keyword: GUI
#% keyword: georectification
#% keyword: GCP
#%end

"""
Module to run GCP management tool as stadalone application.

@author Vaclav Petras  <wenzeslaus gmail.com> (standalone module)
"""

import os

import grass.script as gscript


def main():
    """Sets the GRASS display driver

    .. todo::
        use command line options as an alternative to wizard
    """
    options, flags = gscript.parser()

    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.settings import UserSettings
    from core.giface import StandaloneGrassInterface
    from gcp.manager import GCPWizard

    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    app = wx.App()

    wizard = GCPWizard(parent=None, giface=StandaloneGrassInterface())

    app.MainLoop()

if __name__ == '__main__':
    main()
