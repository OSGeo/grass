#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.remove
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Remove space time or map dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Remove space time or map datasets from temporal database
#% keywords: spacetime
#% keywords: dataset
#% keywords: remove
#%end

#%option
#% key: input
#% type: string
#% description: Name(s) of the space time or map dataset to be removed from the temporal database
#% required: no
#% multiple: yes
#%end

#%option
#% key: file
#% type: string
#% description: Input file with raster map names, one per line
#% required: no
#% multiple: no
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is strds
#% required: no
#% options: strds, str3ds, stvds, rast, rast3d, vect
#% answer: strds
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():
    
    # Get the options
    maps = options["input"]
    file = options["file"]
    type = options["type"]

    if maps and file:
        core.fata(_("%s= and %s= are mutually exclusive") % ("input","file"))

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    dbif = tgis.sql_database_interface()
    dbif.connect()

    maplist = []

    # Map names as comma separated string
    if maps:
        if maps.find(",") == -1:
            maplist = (maps,)
        else:
            maplist = tuple(maps.split(","))

    # Read the map list from file
    if file:
        fd = open(file, "r")

        line = True
        while True:
            line = fd.readline()
            if not line:
                break

            line_list = line.split("\n")
            mapname = line_list[0]
            maplist.append(mapname)
    
    mapset =  grass.gisenv()["MAPSET"]

    for name in maplist:
        name = name.strip()
        # Check for the mapset in name
        if name.find("@") < 0:
            id = name + "@" + mapset
        else:
            id = name

        if type == "strds":
            ds = tgis.space_time_raster_dataset(id)
        if type == "str3ds":
            ds = tgis.space_time_raster3d_dataset(id)
        if type == "stvds":
            ds = tgis.space_time_vector_dataset(id)
        if type == "rast":
            ds = tgis.raster_dataset(id)
        if type == "rast3d":
            ds = tgis.raster3d_dataset(id)
        if type == "vect":
            ds = tgis.vector_dataset(id)

        if ds.is_in_db(dbif) == False:
            dbif.close()
            grass.fatal(_("%s dataset <%s> not found in temporal database") % (ds.get_type(), name))

        # We need to read some data from the temporal database
        ds.delete(dbif)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

