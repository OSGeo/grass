"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *
from multiprocessing import Process

############################################################################

def extract_dataset(input, output, type, where, expression, base, nprocs, register_null=False):
    """!Extract a subset of a space time raster or raster3d dataset
    
       A mapcalc expression can be provided to process the temporal extracted maps.
       Mapcalc expressions are supported for raster and raster3d maps.
       
       @param input The name of the input space time raster/raster3d dataset 
       @param output The name of the extracted new space time raster/raster3d dataset
       @param type The type of the dataset: "raster" or "raster3d"
       @param where The SQL WHERE statement for subset extraction
       @param expression The r(3).mapcalc expression
       @param base The base name of the new created maps in case a mapclac expression is provided 
       @param nprocs The number of parallel processes to be used for mapcalc processing
       @param register_null Set this number True to register empty maps
    """
    
    mapset =  core.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    if type == "raster":
	sp = space_time_raster_dataset(id)
    else:
	sp = space_time_raster3d_dataset(id)
	
    
    dbif = sql_database_interface_connection()
    dbif.connect()
    
    if sp.is_in_db(dbif) == False:
	dbif.close()
        core.fatal(_("Space time %s dataset <%s> not found") % (type, id))

    if expression and not base:
	dbif.close()
        core.fatal(_("Please specify base="))

    sp.select(dbif)

    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset

    # The new space time dataset
    new_sp = sp.get_new_instance(out_id)
	
    if new_sp.is_in_db():
        if core.overwrite() == False:
	    dbif.close()
            core.fatal(_("Space time %s dataset <%s> is already in database, use overwrite flag to overwrite") % (type, out_id))
            
    rows = sp.get_registered_maps("id", where, "start_time", dbif)

    new_maps = {}
    if rows:
	num_rows = len(rows)
	
	core.percent(0, num_rows, 1)
	
	# Run the mapcalc expression
        if expression:
	    count = 0
	    proc_count = 0
	    proc_list = []
	    for row in rows:
		count += 1
		
		core.percent(count, num_rows, 1)
		
		map_name = "%s_%i" % (base, count)

		expr = "%s = %s" % (map_name, expression)
		
		expr = expr.replace(sp.base.get_map_id(), row["id"])
		expr = expr.replace(sp.base.get_name(), row["id"])
		    
		map_id = map_name + "@" + mapset

		new_map = sp.get_new_map_instance(map_id)

		# Check if new map is in the temporal database
		if new_map.is_in_db(dbif):
		    if core.overwrite() == True:
			# Remove the existing temporal database entry
			new_map.delete(dbif)
			new_map = sp.get_new_map_instance(map_id)
		    else:
			core.error(_("Map <%s> is already in temporal database, use overwrite flag to overwrite")%(new_map.get_map_id()))
			continue

		core.verbose(_("Apply mapcalc expression: \"%s\"") % expr)
		
		if type == "raster":
		    proc_list.append(Process(target=run_mapcalc2d, args=(expr,)))
		else:
		    proc_list.append(Process(target=run_mapcalc3d, args=(expr,)))
		proc_list[proc_count].start()
		proc_count += 1
		
		if proc_count == nprocs:
		    proc_count = 0
		    exitcodes = 0
		    for proc in proc_list:
			proc.join()
			exitcodes += proc.exitcode
			
		    if exitcodes != 0:
			dbif.close()
			core.fatal(_("Error while mapcalc computation"))
			
		    # Empty proc list
		    proc_list = []
		    
		# Store the new maps
		new_maps[row["id"]] = new_map
	
	core.percent(0, num_rows, 1)
	
	# Insert the new space time dataset
	if new_sp.is_in_db(dbif):
	    if core.overwrite() == True:
		new_sp.delete(dbif)
		new_sp = sp.get_new_instance(out_id)

	temporal_type, semantic_type, title, description = sp.get_initial_values()
	new_sp.set_initial_values(temporal_type, semantic_type, title, description)
	new_sp.insert(dbif)
	
	# Register the maps in the database
        count = 0
        for row in rows:
            count += 1
	    
	    core.percent(count, num_rows, 1)

            old_map = sp.get_new_map_instance(row["id"])
            old_map.select(dbif)
            
            if expression:
		# Register the new maps
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
                
        # Update the spatio-temporal extent and the metadata table entries
        new_sp.update_from_registered_maps(dbif)
	
	core.percent(num_rows, num_rows, 1)
        
    dbif.close()

###############################################################################

def run_mapcalc2d(expr):
    """Helper function to run r.mapcalc in parallel"""
    return core.run_command("r.mapcalc", expression=expr, overwrite=core.overwrite(), quiet=True)


def run_mapcalc3d(expr):
    """Helper function to run r3.mapcalc in parallel"""
    return core.run_command("r3.mapcalc", expression=expr, overwrite=core.overwrite(), quiet=True)