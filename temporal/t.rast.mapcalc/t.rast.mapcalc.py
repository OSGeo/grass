#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.mapcalc
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Perform r.mapcalc expressions on maps of sampled space time raster datasets
# COPYRIGHT:	(C) 2012 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Perform r.mapcalc expressions on maps of sampled space time raster datasets
#% keywords: temporal
#% keywords: mapcalc
#%end

#%option G_OPT_STRDS_INPUTS
#%end

#%option
#% key: expression
#% type: string
#% description: The r.mapcalc expression applied to each time step of the sampled data
#% required: yes
#% multiple: no
#%end

#%option G_OPT_T_SAMPLE
#% key: method
#% answer: during,overlap,contain,equal
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option G_OPT_R_BASE
#% required: yes
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

#%flag
#% key: s
#% description: Check spatial overlap
#%end

from multiprocessing import Process
import copy
import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    inputs = options["inputs"]
    output = options["output"]
    expression = options["expression"]
    base = options["base"]
    method = options["method"]
    nprocs = int(options["nprocs"])
    register_null = flags["n"]
    spatial = flags["s"]
    
    # Create the method list
    method = method.split(",")
        
    # Make sure the temporal database exists
    tgis.create_temporal_database()
    
    # We need a database interface for fast computation
    dbif = tgis.sql_database_interface()
    dbif.connect()

    mapset =  grass.gisenv()["MAPSET"]
    
    input_name_list = inputs.split(",")
    
    # Process the first input
    if input_name_list[0].find("@") >= 0:
	id = input_name_list[0]
    else:
	id = input_name_list[0] + "@" + mapset
	
    first_input = tgis.space_time_raster_dataset(id)
    
    if first_input.is_in_db(dbif) == False:
	dbif.close()
        grass.fatal(_("Space time %s dataset <%s> not found") % (sp.get_new_map_instance(None).get_type(), id))

    # Fill the object with data from the temporal database
    first_input.select(dbif)
    
    # All additional inputs in reverse sorted order to avoid wrong name substitution
    input_name_list = input_name_list[1:]
    input_name_list.sort()
    input_name_list.reverse()
    input_list = []
        
    for input in input_name_list:

	if input.find("@") >= 0:
	    id = input
	else:
	    id = input + "@" + mapset
	    
	sp = tgis.space_time_raster_dataset(id)
	
	if sp.is_in_db(dbif) == False:
	    dbif.close()
	    grass.fatal(_("Space time raster dataset <%s> not found in temporal database") % (id))

	sp.select(dbif)
	
	input_list.append(copy.copy(sp))

    # Create the new space time raster dataset
    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset
        
    new_sp = tgis.space_time_raster_dataset(out_id)
    
    # Check if in database
    if new_sp.is_in_db(dbif):
        if grass.overwrite() == False:
	    dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is already in database, use overwrite flag to overwrite") % out_id)
 
    # Sample all inputs by the first input and create a sample matrix
    if spatial:
        grass.message(_("Start spatio-temporal sampling"))
    else:
        grass.message(_("Start temporal sampling"))
    map_matrix = []
    id_list = []
    sample_map_list = []
    # First entry is the first dataset id
    id_list.append(first_input.get_name())
    
    if len(input_list) > 0:
	has_samples = False
	for dataset in input_list:
	    list = dataset.sample_by_dataset(stds=first_input, method=method, spatial=spatial, dbif=dbif)
	    
	    # In case samples are not found
	    if not list and len(list) == 0:
		dbif.close()
		grass.message(_("No samples found for map calculation"))
		return 0
	    
	    # The fist entries are the samples
	    map_name_list = []
	    if has_samples == False:
		for entry in list:
		    granule = entry["granule"]
		    # Do not consider gaps
		    if granule.get_id() == None:
			continue
		    sample_map_list.append(granule)
		    map_name_list.append(granule.get_name())
		# Attach the map names
		map_matrix.append(copy.copy(map_name_list))
		has_samples = True
		
	    map_name_list = []
	    for entry in list:
		maplist = entry["samples"]
		granule = entry["granule"]
		
		# Do not consider gaps in the sampler
		if granule.get_id() == None:
		    continue
		
		if len(maplist) > 1:
		    grass.warning(_("Found more than a single map in a sample granule. "\
		    "Only the first map is used for computation. "\
		    "Use t.rast.aggregate.ds to create synchronous datasets."))
		
		# Store all maps! This includes non existent maps, identified by id == None 
		map_name_list.append(maplist[0].get_name())
	    
	    # Attach the map names
	    map_matrix.append(copy.copy(map_name_list))

	    id_list.append(dataset.get_name())
    else:
	list = first_input.get_registered_maps_as_objects(dbif=dbif)
	
	if list == None:
	    dbif.close()
            grass.message(_("No maps in input dataset"))
            return 0
	
	map_name_list = []
	for map in list:
	    map_name_list.append(map.get_name())
	    sample_map_list.append(map)
	
	# Attach the map names
	map_matrix.append(copy.copy(map_name_list))
    
   
    # Needed for map registration
    map_list = []
	
    if len(map_matrix) > 0:
	
	grass.message(_("Start r.mapcalc computation"))
	    
	count = 0
	# Get the number of samples
	num = len(map_matrix[0])
	
	# Parallel processing
        proc_list = []
        proc_count = 0
	
	# For all samples
        for i in range(num):
            
            count += 1
	    grass.percent(count, num, 1)

	    # Create the r.mapcalc statement for the current time step
	    map_name = "%s_%i" % (base, count)   
	    expr = "%s = %s" % (map_name, expression)
            
            # Check that all maps are in the sample
            valid_maps = True
            # Replace all dataset names with their map names of the current time step
            for j in range(len(map_matrix)):
		if map_matrix[j][i] == None:
		    valid_maps = False
		    break
		# Substitute the dataset name with the map name
		expr = expr.replace(id_list[j], map_matrix[j][i])

	    # Proceed with the next sample
	    if valid_maps == False:
		continue
		
	    # Create the new map id and check if the map is already in the database
	    map_id = map_name + "@" + mapset

	    new_map = first_input.get_new_map_instance(map_id)

	    # Check if new map is in the temporal database
	    if new_map.is_in_db(dbif):
		if grass.overwrite() == True:
		    # Remove the existing temporal database entry
		    new_map.delete(dbif)
		    new_map = first_input.get_new_map_instance(map_id)
		else:
		    grass.error(_("Raster map <%s> is already in temporal database, use overwrite flag to overwrite"))
		    continue

	    # Set the time stamp
	    if sample_map_list[i].is_time_absolute():
		start, end, tz = sample_map_list[i].get_absolute_time()
		new_map.set_absolute_time(start, end, tz)
	    else:
		start, end = sample_map_list[i].get_relative_time()
		new_map.set_relative_time(start, end)
	    
	    map_list.append(new_map)
	    
	    # Start the parallel r.mapcalc computation
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
		    dbif.close()
		    grass.fatal(_("Error while r.mapcalc computation"))
		    
		# Empty process list
		proc_list = []
		
	# Register the new maps in the output space time dataset
	grass.message(_("Start map registration in temporal database"))
	    
	# Overwrite an existing dataset if requested
	if new_sp.is_in_db(dbif):
	    if grass.overwrite() == True:
		new_sp.delete(dbif)
		new_sp = tgis.space_time_raster_dataset(out_id)
		
	# Copy the ids from the first input
	temporal_type, semantic_type, title, description = first_input.get_initial_values()
	new_sp.set_initial_values(temporal_type, semantic_type, title, description)
	# Insert the dataset in the temporal database
	new_sp.insert(dbif)
    
	count = 0
	
	for new_map in map_list:

            count += 1
	    grass.percent(count, num, 1)
	    
	    # Read the raster map data
	    new_map.load()
	    
	    # In case of a null map continue, do not register null maps
	    if new_map.metadata.get_min() == None and new_map.metadata.get_max() == None:
		if not register_null:
		    continue

	    # Insert map in temporal database
	    new_map.insert(dbif)

	    new_sp.register_map(new_map, dbif)

        # Update the spatio-temporal extent and the raster metadata table entries
        new_sp.update_from_registered_maps(dbif)
	
	grass.percent(1, 1, 1)
        
    dbif.close()

###############################################################################

def run_mapcalc(expr):
    """Helper function to run r.mapcalc in parallel"""
    return grass.run_command("r.mapcalc", expression=expr, overwrite=grass.overwrite(), quiet=True)

###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

