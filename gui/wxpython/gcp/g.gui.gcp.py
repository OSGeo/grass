#!/usr/bin/env python

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
#% description: Georectifies a map and allows to manage Ground Control Points.
#% keywords: general
#% keywords: GUI
#% keywords: georectification
#%end

"""
Module to run GCP management tool as stadalone application.

@author Vaclav Petras  <wenzeslaus gmail.com> (standalone module)
"""

import os
import sys
import  wx

import grass.script as grass

# adding a path to wxGUI modules
if __name__ == '__main__':
    WXGUIBASE = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if WXGUIBASE not in sys.path:
        sys.path.append(WXGUIBASE)

from core.settings import UserSettings
from core.globalvar import CheckWxVersion
from core.giface import StandaloneGrassInterface
from gcp.manager import GCPWizard


def main():
    """!Sets the GRASS display driver

    @todo use command line options as an alternative to wizard
    """
    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    options, flags = grass.parser()

    app = wx.PySimpleApp()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()

    wizard = GCPWizard(parent=None, giface=StandaloneGrassInterface())

    app.MainLoop()


if __name__ == '__main__':
    # launch GUI in the background
    child_pid = os.fork()
    if child_pid == 0:
        main()
    os._exit(0)
