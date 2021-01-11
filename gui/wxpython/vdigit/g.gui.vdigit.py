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

#%module
#% description: Interactive editing and digitization of vector maps.
#% keyword: general
#% keyword: GUI
#% keyword: vector
#% keyword: editing
#% keyword: digitizer
#%end
#%flag
#% key: c
#% description: Create new vector map if doesn't exist
#%end
#%option G_OPT_V_MAP
#% label: Name of vector map to edit
#%end

import os
import grass.script as grass


def main():
    grass.set_raise_on_error(False)

    options, flags = grass.parser()

    # import wx only after running parser
    # to avoid issues with complex imports when only interface is needed
    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.render import Map
    from mapdisp.frame import MapFrame
    from mapdisp.main import DMonGrassInterface
    from core.settings import UserSettings
    from vdigit.main import haveVDigit, errorMsg
    from grass.exceptions import CalledModuleError

    # define classes which needs imports as local
    # for longer definitions, a separate file would be a better option
    class VDigitMapFrame(MapFrame):

        def __init__(self, vectorMap):
            MapFrame.__init__(
                self, parent=None, Map=Map(), giface=DMonGrassInterface(None),
                title=_("Vector Digitizer - GRASS GIS"), size=(850, 600))
            # this giface issue not solved yet, we must set mapframe aferwards
            self._giface._mapframe = self
            # load vector map
            mapLayer = self.GetMap().AddLayer(
                ltype='vector', name=vectorMap,
                command=['d.vect', 'map=%s' % vectorMap],
                active=True, hidden=False, opacity=1.0, render=True)

            # switch toolbar
            self.AddToolbar('vdigit', fixed=True)

            # start editing
            self.toolbars['vdigit'].StartEditing(mapLayer)

    if not haveVDigit:
        grass.fatal(_("Vector digitizer not available. %s") % errorMsg)

    if not grass.find_file(name=options['map'], element='vector',
                           mapset=grass.gisenv()['MAPSET'])['fullname']:
        if not flags['c']:
            grass.fatal(_("Vector map <%s> not found in current mapset. "
                          "New vector map can be created by providing '-c' flag.") %
                        options['map'])
        else:
            grass.verbose(_("New vector map <%s> created") % options['map'])
            try:
                grass.run_command(
                    'v.edit', map=options['map'],
                    tool='create', quiet=True)
            except CalledModuleError:
                grass.fatal(
                    _("Unable to create new vector map <%s>") %
                    options['map'])

    # allow immediate rendering
    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    app = wx.App()
    frame = VDigitMapFrame(options['map'])
    frame.Show()

    app.MainLoop()

if __name__ == "__main__":
    main()
