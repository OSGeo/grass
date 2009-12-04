#!/usr/bin/env python
############################################################################
#
# MODULE:       p.db
# AUTHOR(S):    Jachym Cepicky, Markus Neteler, Hamish Bowman
#               Converted to Python by Huidae Cho
# PURPOSE:      Start stand-alone attribute table manager
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
#% description: Start stand-alone attribute table manager
#% keywords: database
#%End
#%Option
#% key: table
#% type: string
#% required: yes
#% multiple: no
#% description: Table name
#%End

import os
import grass.script as grass

def main():
    table = options["table"]

    os.spawnlp(os.P_NOWAIT, os.environ["GRASS_PYTHON"], os.environ["GRASS_PYTHON"], "%s/etc/wxpython/gui_modules/dbm.py" % os.environ["GISBASE"], table)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
