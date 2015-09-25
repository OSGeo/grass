#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.snap
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally snap the maps of a space time dataset.
# COPYRIGHT:    (C) 2013 by the GRASS Development Team
#
#       This program is free software under the GNU General Public
#       License (version 2). Read the file COPYING that comes with GRASS
#       for details.
#
#############################################################################

#%module
#% description: Snaps temporally the maps of a space time dataset.
#% keyword: temporal
#% keyword: snapping
#% keyword: time
#%end

#%option G_OPT_STDS_INPUT
#% description: Name of an existing space time dataset
#%end

#%option G_OPT_STDS_TYPE
#% guidependency: input
#% guisection: Required
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    name = options["input"]
    type = options["type"]

    # Make sure the temporal database exists
    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds = tgis.open_old_stds(name, type, dbif)
    stds.snap(dbif=dbif)

    stds.update_command_string(dbif=dbif)
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
