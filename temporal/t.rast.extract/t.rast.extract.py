#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.extract
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
#% keywords: temporal
#% keywords: extract
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option
#% key: expression
#% type: string
#% description: The r.mapcalc expression assigned to all extracted raster maps
#% required: no
#% multiple: no
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option G_OPT_R_BASE
#%end

#%option
#% key: nprocs
#% type: integer
#% description: The number of r.mapcalc processes to run in parallel
#% required: no
#% multiple: no
#% answer: 1
#%end

#%flag
#% key: n
#% description: Register Null maps
#%end

import grass.script as grass
import grass.temporal as tgis
from multiprocessing import Process

############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    expression = options["expression"]
    base = options["base"]
    nprocs = int(options["nprocs"])
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
        grass.fatal(_("Space time %s dataset <%s> not found") % (sp.get_new_map_instance(None).get_type(), id))

    if expression and not base:
        grass.fatal(_("Please specify base="))

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
        if grass.overwrite() == False:
            grass.fatal(_("Space time raster dataset <%s> is already in database, use overwrite flag to overwrite") % out_id)
            
    rows = sp.get_registered_maps("id", where, "start_time", dbif)

    new_maps = {}
    if rows:
	num_rows = len(rows)
	
	grass.percent(0, num_rows, 1)
	
	# Run the r.mapcalc expression
        if expression:
	    count = 0
	    proc_count = 0
	    proc_list = []
	    for row in rows:
		count += 1
		
		grass.percent(count, num_rows, 1)
		
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
		
		proc_list.append(Process(target=run_mapcalc, args=(expr,)))
		proc_list[proc_count].start()
		proc_count += 1
		
		if proc_count == nprocs:
		    proc_count = 0
		    exitcodes = 0
		    for proc in proc_list:
			proc.join()
			exitcodes += proc.exitcode
			
		    if exitcodes != 0:
			grass.fatal(_("Error while r.mapcalc computation"))
			
		    # Empty proc list
		    proc_list = []
		new_maps[row["id"]] = new_map
	
	grass.percent(0, num_rows, 1)
	
	# Insert the new space time raster dataset
	if new_sp.is_in_db():
	    if grass.overwrite() == True:
		new_sp.delete(dbif)
		new_sp = tgis.space_time_raster_dataset(out_id)
		
	temporal_type, semantic_type, title, description = sp.get_initial_values()
	new_sp.set_initial_values(temporal_type, semantic_type, title, description)
	new_sp.insert(dbif)
	
	# Register the maps in the database
        count = 0
        for row in rows:
            count += 1
	    
	    grass.percent(count, num_rows, 1)

            old_map = sp.get_new_map_instance(row["id"])
            old_map.select(dbif)
            
            if expression:
		
		if new_maps.has_key(row["id"]):
		    new_map = new_maps[row["id"]]

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
	
	grass.percent(num_rows, num_rows, 1)
        
    dbif.close()

###############################################################################

def run_mapcalc(expr):
    """Helper function to run r.mapcalc in parallel"""
    return grass.run_command("r.mapcalc", expression=expr, overwrite=grass.overwrite(), quiet=True)
    
###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

