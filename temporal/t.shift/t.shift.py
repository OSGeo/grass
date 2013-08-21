#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.shift
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Temporally shift the maps of a space time dataset.
# COPYRIGHT:	(C) 2013 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Temporally shift the maps of a space time dataset.
#% keywords: temporal
#% keywords: shift
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

#%option
#% key: granularity
#% type: string
#% description: Shift granularity, format absolute time "x years, x months, x weeks, x days, x hours, x minutes, x seconds" or an integer value for relative time
#% required: yes
#% multiple: no
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    name = options["input"]
    type = options["type"]
    gran = options["granularity"]

    # Make sure the temporal database exists
    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds = tgis.open_old_space_time_dataset(name, type, dbif)
    check = stds.shift(gran=gran, dbif=dbif)

    if check == False:
        dbif.close()
        grass.fatal(_("Unable to temporally shift the space time %s dataset <%s>") % \
                     (stds.get_new_map_instance(None).get_type(), id))

    stds.update_command_string(dbif=dbif)
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
