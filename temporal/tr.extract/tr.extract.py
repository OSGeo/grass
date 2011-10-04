#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.extract
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Extract a subset of a space time raster dataset
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
#% description: Name of a space time raster dataset
#% required: yes
#% multiple: no
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
#% description: Base name of the new created raster maps
#% required: no
#% multiple: no
#%end

#%flag
#% key: n
#% description: Register Null maps
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
    base = options["base"]
    register_null = flags["n"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    
    mapset =  grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)
    
    if sp.is_in_db() == False:
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    if expression and base == None:
        grass.fatal(_("A raster map base name must be provided"))

    dbif = tgis.sql_database_interface()
    dbif.connect()

    sp.select(dbif)

    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset

    # The new space time raster dataset
    new_sp = tgis.space_time_raster_dataset(out_id)
    if new_sp.is_in_db():
        if grass.overwrite() == True:
            new_sp.delete(dbif)
        else:
            grass.fatal(_("Space time raster dataset <%s> is already in database, use overwrite flag to overwrite") % out_id)

    granularity, temporal_type, semantic_type, title, description = sp.get_initial_values()
    new_sp.set_initial_values(granularity, temporal_type, semantic_type, title, description)
    new_sp.insert(dbif)

    rows = sp.get_registered_maps(dbif, where, "start_time")

    if rows:
        count = 0
        for row in rows:
            count += 1

            old_map = sp.get_new_map_instance(row["id"])
            old_map.select(dbif)
            
            if expression:

                map_name = "%s_%i" % (base, count)

                expr = "%s = %s" % (map_name, expression.replace(sp.get_id(), row["id"]))
                expr = expr.replace(sp.base.get_name(), row["id"])

                map_id = map_name + "@" + mapset

                new_map = sp.get_new_map_instance(map_id)

                # Check if new map is in the temporal database
                if new_map.is_in_db(dbif):
                    if grass.overwrite() == True:
                        # Remove the existing temporal database entry
                        new_map.delete(dbif)
                        new_map = sp.get_new_map_instance(map_id)
                    else:
                        grass.error(_("Raster map <%s> is already in temporal database, use overwrite flag to overwrite"))
                        continue

                grass.verbose(_("Apply r.mapcalc expression: \"%s\"") % expr)

                if grass.overwrite() == True:
                    ret = grass.run_command("r.mapcalc", expression=expr, overwrite=True)
                else:
                    ret = grass.run_command("r.mapcalc", expression=expr, overwrite=False)

                if ret != 0:
                    grass.error(_("Error while r.mapcalc computation, continue with next map"))
                    continue

                # Read the raster map data
                new_map.load()
                
                # In case of a null map continue, do not register null maps
                if new_map.metadata.get_min() == None and new_map.metadata.get_max() == None:
                    if not register_null:
                        continue

                # Set the time stamp
                if old_map.is_time_absolute():
                    start, end, tz = old_map.get_absolute_time()
                    new_map.set_absolute_time(start, end, tz)
                else:
                    start, end = old_map.get_relative_time()
                    new_map.set_relative_time(start, end)

                # Insert map in temporal database
                new_map.insert(dbif)

                new_sp.register_map(new_map, dbif)
            else:
                new_sp.register_map(old_map, dbif)

        # Update the spatio-temporal extent and the raster metadata table entries
        new_sp.update_from_registered_maps(dbif)
        
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

