#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.extract
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Register raster maps in a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Extract a subset of a space time raster dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#% keywords: extract
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time raster dataset
#% required: yes
#% multiple: yes
#%end

#%option G_OPT_DB_WHERE
#%end

#%option
#% key: expression
#% type: string
#% description: The r.mapcalc expression assigned to all extracted raster maps
#% required: no
#% multiple: no
#%end

#%option
#% key: output
#% type: string
#% description: Name of the output space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: base
#% type: string
#% description: Base name  of the new created raster maps
#% required: yes
#% multiple: no
#%end


import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    expression = options["expression"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    if input.find("@") >= 0:
        id = input
    else:
        mapset =  grass.gisenv()["MAPSET"]
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)

    if sp.is_in_db() == False:
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select()

    rows = sp.get_registered_maps(None, where, sort)

    if rows:
        inputs = ""

        count = 0
        for row in rows:
            if count == 0:
                inputs += row["id"]
            else:
                inputs += "," + row["id"]
            count += 1

        print inputs

        if grass.overwrite() == True:
            grass.run_command("r.mapcalc", expression=expression, overwrite=True)
        else:
            grass.run_command("r.mapcalc", expression=expression, overwrite=False)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()

