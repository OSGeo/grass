#!/usr/bin/env python3

############################################################################
#
# MODULE:    Correcting distortions of a scanned photo (modified from GCP Manager)
# AUTHOR(S): Yann modified the code (was Markus Metz for the GCP Manager)
# PURPOSE:   Takes a scanned photo and fits fiducial points to known geometry
# COPYRIGHT: (C) 2012-2017 by Markus Metz, and the GRASS Development Team
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
#% description: Corrects scanning distortions of a paper photo.
#% keyword: imagery
#% keyword: GUI
#% keyword: aerial
#% keyword: photo
#% keyword: georectification
#% keyword: geometry
#% keyword: GCP
#%end

#%option G_OPT_I_GROUP
#% key: group
#% required: yes
#%end

#%option G_OPT_R_INPUT
#% key: raster
#% required: yes
#%end

#%option 
#% key: camera
#% type: string
#% label: The name of the camera (generated in i.ortho.camera)
#% description: The name of the camera (generated in i.ortho.camera)
#% required: yes
#%end

#%option 
#% key: order
#% type: string
#% label: The rectification order (no of Fiducial=4 -> order=1, no of Fiducial=8 -> order=2)
#% description: The rectification order (no of Fiducial=4 -> order=1, no of Fiducial=8 -> order=2)
#% required: yes
#% answer: 1
#%end

#%option 
#% key: extension
#% type: string
#% label: The name of the output files extension (used in i.rectify)
#% description: The name of the output files extension (used in i.rectify)
#% required: yes
#% answer: _ip2i_out
#%end

"""
Module to run GCP management tool as stadalone application.
@author Vaclav Petras  <wenzeslaus gmail.com> (standalone module)
"""
import os
import grass.script as gscript

def main():
    """Sets the GRASS display driver
    """
    options, flags = gscript.parser()

    import wx
    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.settings import UserSettings
    from core.giface import StandaloneGrassInterface
    from photo2image.ip2i_manager import GCPWizard

    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    if options['group']:
        group = options['group']
    else:
        gscript.fatal(_("Please provide a group name to process"))

    if options['raster']:
        raster = options['raster']
    else:
        gscript.fatal(_("Please provide a raster map name to process"))

    if options['camera']:
        camera = options['camera']
    else:
        gscript.fatal(_("Please provide a camera name (generated by i.ortho.camera)"))

    if options['order']:
        order = options['order']
    else:
        gscript.fatal(_("Please provive an order value (1 if 4 Fiducials, 2 if 8 Fiducials)"))

    if options['extension']:
        extension = options['extension']
    else:
        gscript.fatal(_("Please provive an output files extension (used by i.rectify)"))

    app = wx.App()

    wizard = GCPWizard(parent=None, giface=StandaloneGrassInterface(), group=group, 
            raster=raster, raster1=raster, camera=camera, order=order, extension=extension)
    app.MainLoop()

if __name__ == '__main__':
    main()
