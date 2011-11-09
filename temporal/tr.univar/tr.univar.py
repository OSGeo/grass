#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.univar
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Calculates univariate statistics from the non-null cells of a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Calculates univariate statistics from the non-null cells for each registred raster map of a space time raster dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#% keywords: statistics
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option G_OPT_DB_WHERE
#%end

#%option
#% key: fs
#% type: string
#% description: The field separator character between the columns
#% required: no
#% answer: |
#%end

#%flag
#% key: e
#% description: Calculate extended statistics
#%end

#%flag
#% key: h
#% description: Print column names 
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    input = options["input"]
    where = options["where"]
    extended = flags["e"]
    header = flags["h"]
    fs = options["fs"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # We need a database interface
    dbif = tgis.sql_database_interface()
    dbif.connect()
   
    mapset =  grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)
    
    if sp.is_in_db() == False:
        dbif.close()
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select(dbif)

    rows = sp.get_registered_maps("id,start_time,end_time", where, "start_time", dbif)

    if not rows:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is empty") % out_id)

    if header == True:
        print "id" + fs + "start" + fs + "end" + fs + "mean" + fs + "min" + fs + "max" + fs,
        print "mean_of_abs" + fs + "stddev" + fs + "variance" + fs,
        if extended == True:
            print "coeff_var" + fs + "sum" + fs + "null_cells" + fs + "cells" + fs,
            print "first_quartile" + fs + "median" + fs + "third_quartile" + fs + "percentile_90" 
        else:
            print "coeff_var" + fs + "sum" + fs + "null_cells" + fs + "cells" 

    for row in rows:
        id = row["id"]
        start = row["start_time"]
        end = row["end_time"]

        flag="g"

        if extended == True:
            flag += "e"

        stats = grass.parse_command("r.univar", map=id, flags=flag)

        print str(id) + fs + str(start) + fs + str(end),
        print fs + str(stats["mean"]) + fs + str(stats["min"]) + fs + str(stats["max"]) + fs + str(stats["mean_of_abs"]),
        print fs + str(stats["stddev"]) + fs + str(stats["variance"]) + fs + str(stats["coeff_var"]) + fs + str(stats["sum"]),

        if extended == True:
            print fs + str(stats["null_cells"]) + fs + str(stats["cells"]) + fs,
            print str(stats["first_quartile"]) + fs + str(stats["median"]) + fs + str(stats["third_quartile"]) + fs + str(stats["percentile_90"]) 
        else:
            print fs + str(stats["null_cells"]) + fs + str(stats["cells"])
        
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

