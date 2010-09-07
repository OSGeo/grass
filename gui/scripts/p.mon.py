#!/usr/bin/env python
############################################################################
#
# MODULE:       p.mon
# AUTHOR(S):    Jachym Cepicky, Michael Barton, Martin Landa, Markus Neteler,
#               Hamish Bowman
#               Converted to Python by Huidae Cho
# PURPOSE:      To establish and control use of a graphics display monitor.
# COPYRIGHT:    (C) 2009 by The GRASS Development Team
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

#%Module
#% description: To establish and control use of a graphics display monitor.
#% keywords: display
#%End

##%Flag
##% key: l
##% description: List all monitors
##%End

##%Flag
##% key: L
##% description: List all monitors (with current status)
##%End

##%Flag
##% key: p
##% description: Print name of currently selected monitor
##%End

##%Flag
##% key: r
##% description: Release currently selected monitor
##%End

##%Flag
##% key: s
##% description: Do not automatically select when starting
##%End

#%Option
#% key: start
#% type: string
#% required: no
#% multiple: no
#% description: Name of graphics monitor to start (p0-p9)
#%End

##%Option
##% key: stop
##% type: string
##% required: no
##% multiple: no
##% description: Name of graphics monitor to stop
##%End

##%Option
##% key: select
##% type: string
##% required: no
##% multiple: no
##% description: Name of graphics monitor to select
##%End

##%Option
##% key: unlock
##% type: string
##% required: no
##% multiple: no
##% description: Name of graphics monitor to unlock
##%End

import os
import grass.script as grass

def main():
    start = options["start"]
#    select = options["select"]
#    stop = options["stop"]
#    unlock = options["unlock"]

    # create the command file
    command_file = grass.tempfile()
    os.system("g.gisenv set=GRASS_PYCMDFILE=%s" % command_file)

    if start != "":
        os.spawnlp(os.P_NOWAIT, os.environ["GRASS_PYTHON"], os.environ["GRASS_PYTHON"], "%s/etc/gui/wxpython/gui_modules/mapdisp.py" % os.environ["GISBASE"], start, command_file)
        return

#    if stop != "" or select != "" or unlock != "":
#        grass.message(_("Not implemented yet"), "w")

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
