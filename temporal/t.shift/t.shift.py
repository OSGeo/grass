#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.shift
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally shift the maps of a space time dataset
# COPYRIGHT:    (C) 2013-2014 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Shifts temporally the maps of a space time dataset.
#% keywords: temporal
#% keywords: shift
#%end

#%option G_OPT_STDS_INPUT
#% description: Name of an existing space time dataset
#%end

#%option G_OPT_T_TYPE
#% guidependency: input
#% guisection: Required
#%end

#%option
#% key: granularity
#% type: string
#% label: Shift granularity
#% description: Format absolute time: "x years, x months, x weeks, x days, x hours, x minutes, x seconds", relative time is of type integer
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

    stds = tgis.open_old_stds(name, type, dbif)
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
