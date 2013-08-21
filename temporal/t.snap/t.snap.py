#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:   t.snap
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:  Temporally snap the maps of a space time dataset.
# COPYRIGHT:    (C) 2013 by the GRASS Development Team
#
#       This program is free software under the GNU General Public
#       License (version 2). Read the file COPYING that comes with GRASS
#       for details.
#
#############################################################################

#%module
#% description: Temporally snap the maps of a space time dataset.
#% keywords: temporal
#% keywords: snap
#%end

#%option G_OPT_STDS_INPUT
#% description: Name of an existing space time dataset
#%end

#%option
#% key: type
#% type: string
#% description: Type of the dataset, default is strds (space time raster dataset)
#% required: no
#% guidependency: input
#% guisection: Required
#% options: strds, str3ds, stvds
#% answer: strds
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

    stds = tgis.open_old_space_time_dataset(name, type, dbif)
    stds.snap(dbif=dbif)

    stds.update_command_string(dbif=dbif)
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
