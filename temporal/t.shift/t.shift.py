#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.shift
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally shift the maps of a space time dataset
# COPYRIGHT:    (C) 2013-2017 by the GRASS Development Team
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
#############################################################################

#%module
#% description: Shifts temporally the maps of a space time dataset.
#% keyword: temporal
#% keyword: time management
#% keyword: shift
#% keyword: time
#%end

#%option G_OPT_STDS_INPUT
#% description: Name of an existing space time dataset
#%end

#%option G_OPT_STDS_TYPE
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


############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

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
