"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

tgis.register_maps_in_space_time_dataset(type, name, maps)

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *
 
###############################################################################

def register_maps_in_space_time_dataset(type, name, maps=None, file=None, start=None, \
                                        end=None, unit=None, increment=None, dbif = None, \
                                        interval=False, fs="|"):
    """!Use this method to register maps in space time datasets. This function is generic and

       Additionally a start time string and an increment string can be specified
       to assign a time interval automatically to the maps.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       @param type: The type of the maps rast, rast3d or vect
       @param name: The name of the space time dataset
       @param maps: A comma separated list of map names
       @param file: Input file one map with start and optional end time, one per line
       @param start: The start date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative is integer 5)
       @param end: The end date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative is integer 5)
       @param unit: The unit of the relative time: years, months, days, hours, minutes, seconds
       @param increment: Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
       @param dbif: The database interface to be used
       @param interval: If True, time intervals are created in case the start time and an increment is provided
       @param fs: Field separator used in input file
    """

    start_time_in_file = False
    end_time_in_file = False
    
    if maps and file:
        core.fatal(_("%s= and %s= are mutually exclusive") % ("input","file"))

    if end and increment:
        core.fatal(_("%s= and %s= are mutually exclusive") % ("end","increment"))

    if end and not start:
        core.fatal(_("Please specify %s= and %s=") % ("start_time","end_time"))

    if not maps and not file:
        core.fatal(_("Please specify %s= or %s=") % ("input","file"))

    # We may need the mapset
    mapset =  core.gisenv()["MAPSET"]
    
    # The name of the space time dataset is optional
    if name:
	# Check if the dataset name contains the mapset as well
	if name.find("@") < 0:
	    id = name + "@" + mapset
	else:
	    id = name

	if type == "rast" or type == "raster":
	    sp = dataset_factory("strds", id)
	elif type == "rast3d":
	    sp = dataset_factory("str3ds", id)
	elif type == "vect" or type == "vector":
	    sp = dataset_factory("stvds", id)
	else:
	    core.fatal(_("Unkown map type: %s")%(type))

        
    dbif, connect = init_dbif(None)

    if name:
	# Read content from temporal database
	sp.select(dbif)

	if sp.is_in_db(dbif) == False:
	    dbif.close()
	    core.fatal(_("Space time %s dataset <%s> no found") % (sp.get_new_map_instance(None).get_type(), name))

	if sp.is_time_relative() and not unit:
	    dbif.close()
	    core.fatal(_("Space time %s dataset <%s> with relative time found, but no relative unit set for %s maps") % (sp.get_new_map_instance(None).get_type(), name, sp.get_new_map_instance(None).get_type()))
    
    # We need a dummy map object to build the map ids
    dummy = dataset_factory(type, None)
        
    maplist = []
    
    # Map names as comma separated string
    if maps:
        if maps.find(",") < 0:
            maplist = [maps,]
        else:
            maplist = maps.split(",")

	# Build the map list again with the ids
	for count in range(len(maplist)):
	    row = {}
	    mapid = dummy.build_id(maplist[count], mapset, None)
		
	    row["id"] = mapid
            maplist[count] = row
            
    # Read the map list from file
    if file:
        fd = open(file, "r")

        line = True
        while True:
            line = fd.readline()
            if not line:
                break

            line_list = line.split(fs)

            # Detect start and end time
            if len(line_list) == 2:
                start_time_in_file = True
                end_time_in_file = False
            elif len(line_list) == 3:
                start_time_in_file = True
                end_time_in_file = True
            else:
                start_time_in_file = False
                end_time_in_file = False

            mapname = line_list[0].strip()
            row = {}
            
	    if start_time_in_file and  end_time_in_file:
	        row["start"] = line_list[1].strip()
	        row["end"] = line_list[2].strip()

	    if start_time_in_file and  not end_time_in_file:
	        row["start"] = line_list[1].strip()
	    
	    row["id"] = dummy.build_id(mapname, mapset)

            maplist.append(row)
    
    num_maps = len(maplist)
    map_object_list = []
    statement = ""
    
    core.message(_("Gathering map informations"))
    
    for count in range(len(maplist)):
	core.percent(count, num_maps, 1)

        # Get a new instance of the map type
        map = dataset_factory(type, maplist[count]["id"])

        # Use the time data from file
        if maplist[count].has_key("start"):
            start = maplist[count]["start"]
        if maplist[count].has_key("end"):
            end = maplist[count]["end"]
            
        is_in_db = False

        # Put the map into the database
        if map.is_in_db(dbif) == False:
            is_in_db = False
            # Break in case no valid time is provided
            if start == "" or start == None:
                dbif.close()
                if map.get_layer():
		    core.fatal(_("Unable to register %s map <%s> with layer %s. The map has no valid time and the start time is not set.") % \
				(map.get_type(), map.get_map_id(), map.get_layer() ))
		else:
		    core.fatal(_("Unable to register %s map <%s>. The map has no valid time and the start time is not set.") % \
				(map.get_type(), map.get_map_id() ))
	    
	    if unit:
                map.set_time_to_relative()
            else:
                map.set_time_to_absolute()
 
        else:
            is_in_db = True
            if core.overwrite == False:
		continue
            map.select(dbif)
            if name and map.get_temporal_type() != sp.get_temporal_type():
                dbif.close()
                if map.get_layer():
		    core.fatal(_("Unable to register %s map <%s> with layer. The temporal types are different.") %  \
		                 (map.get_type(), map.get_map_id(), map.get_layer()))
		else:
		    core.fatal(_("Unable to register %s map <%s>. The temporal types are different.") %  \
		                 (map.get_type(), map.get_map_id()))

        # Load the data from the grass file database
        map.load()

        # Set the valid time
        if start:
            # In case the time is in the input file we ignore the increment counter
            if start_time_in_file:
                count = 1
            assign_valid_time_to_map(ttype=map.get_temporal_type(), map=map, start=start, end=end, unit=unit, increment=increment, mult=count, interval=interval)

        if is_in_db:
           #  Gather the SQL update statement
           statement += map.update_all(dbif=dbif, execute=False)
        else:
           #  Gather the SQL insert statement
           statement += map.insert(dbif=dbif, execute=False)

        # Sqlite3 performace better for huge datasets when committing in small chunks
        if dbmi.__name__ == "sqlite3":
            if count % 100 == 0:
                if statement != None and statement != "":
                    core.message(_("Registering maps in the temporal database"))
		    dbif.execute_transaction(statement)
                    statement = ""

        # Store the maps in a list to register in a space time dataset
        if name:
            map_object_list.append(map)

    core.percent(num_maps, num_maps, 1)

    if statement != None and statement != "":
        core.message(_("Register maps in the temporal database"))
        dbif.execute_transaction(statement)

    # Finally Register the maps in the space time dataset
    if name:
        statement = ""
        count = 0
        num_maps = len(map_object_list)
        core.message(_("Register maps in the space time raster dataset"))
        for map in map_object_list:
	    core.percent(count, num_maps, 1)
	    sp.register_map(map=map, dbif=dbif)
            count += 1
        
    # Update the space time tables
    if name:
        core.message(_("Update space time raster dataset"))
	sp.update_from_registered_maps(dbif)

    if connect == True:
        dbif.close()

    core.percent(num_maps, num_maps, 1)
        

###############################################################################

def assign_valid_time_to_map(ttype, map, start, end, unit, increment=None, mult=1, interval=False):
    """!Assign the valid time to a map dataset

       @param ttype: The temporal type which should be assigned and which the time format is of
       @param map: A map dataset object derived from abstract_map_dataset
       @param start: The start date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative is integer 5)
       @param end: The end date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative is integer 5)
       @param unit: The unit of the relative time: years, months, days, hours, minutes, seconds
       @param increment: Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative is integer 1)
       @param multi: A multiplier for the increment
       @param interval: If True, time intervals are created in case the start time and an increment is provided
    """

    if ttype == "absolute":
        start_time = string_to_datetime(start)
        if start_time == None:
            dbif.close()
            core.fatal(_("Unable to convert string \"%s\"into a datetime object")%(start))
        end_time = None

        if end:
            end_time = string_to_datetime(end)
            if end_time == None:
                dbif.close()
                core.fatal(_("Unable to convert string \"%s\"into a datetime object")%(end))

        # Add the increment
        if increment:
            start_time = increment_datetime_by_string(start_time, increment, mult)
            if start_time == None:
		core.fatal(_("Error in increment computation"))
            if interval:
                end_time = increment_datetime_by_string(start_time, increment, 1)
		if end_time == None:
		    core.fatal(_("Error in increment computation"))
	if map.get_layer():
	    core.verbose(_("Set absolute valid time for map <%s> with layer %s to %s - %s") % (map.get_map_id(), map.get_layer(), str(start_time), str(end_time)))
        else:
	    core.verbose(_("Set absolute valid time for map <%s> to %s - %s") % (map.get_map_id(), str(start_time), str(end_time)))
        
        map.set_absolute_time(start_time, end_time, None)
    else:
        start_time = int(start)
        end_time = None

        if end:
            end_time = int(end)

        if increment:
            start_time = start_time + mult * int(increment)
            if interval:
                end_time = start_time + int(increment)

	if map.get_layer():
	    core.verbose(_("Set relative valid time for map <%s> with layer %s to %i - %s with unit %s") % (map.get_map_id(), map.get_layer(), start_time,  str(end_time), unit))
        else:
	    core.verbose(_("Set relative valid time for map <%s> to %i - %s with unit %s") % (map.get_map_id(), start_time,  str(end_time), unit))
	    
        map.set_relative_time(start_time, end_time, unit)

###############################################################################

def dataset_factory(type, id):
    """!A factory functions to create space time or map datasets
    
       @param type: the dataset type: rast or raster, rast3d, vect or vector, strds, str3ds, stvds
       @param id: The id of the dataset ("name@mapset")
    """
    if type == "strds":
        sp = space_time_raster_dataset(id)
    elif type == "str3ds":
        sp = space_time_raster3d_dataset(id)
    elif type == "stvds":
        sp = space_time_vector_dataset(id)
    elif type == "rast" or type == "raster":
        sp = raster_dataset(id)
    elif type == "rast3d":
        sp = raster3d_dataset(id)
    elif type == "vect" or  type == "vector":
        sp = vector_dataset(id)
    else:
        core.error(_("Unknown dataset type: %s") % type)
        return None

    return sp

###############################################################################

def list_maps_of_stds(type, input, columns, order, where, separator, method, header):
    """! List the maps of a space time dataset using diffetent methods

        @param type: The type of the maps raster, raster3d or vector
        @param input: Name of a space time raster dataset
        @param columns: A comma separated list of columns to be printed to stdout 
        @param order: A comma separated list of columns to order the space time dataset by category 
        @param where: A where statement for selected listing without "WHERE" e.g: start_time < "2001-01-01" and end_time > "2001-01-01"
        @param separator: The field separator character between the columns
        @param method: String identifier to select a method out of cols,comma,delta or deltagaps
            * "cols": Print preselected columns specified by columns
            * "comma": Print the map ids (name@mapset) as comma separated string
            * "delta": Print the map ids (name@mapset) with start time, end time, relative length of intervals and the relative distance to the begin
            * "deltagaps": Same as "delta" with additional listing of gaps. Gaps can be simply identified as the id is "None"
            * "gran": List map using the granularity of the space time dataset, columns are identical to deltagaps 
        @param header: Set True to print column names 
    """
    mapset =  core.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = dataset_factory(type, id)
    
    if sp.is_in_db() == False:
        core.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select()

    if separator == None or separator == "":
        separator = "\t"
           
    # This method expects a list of objects for gap detection
    if method == "delta" or method == "deltagaps" or method == "gran":
	if type == "stvds":
	    columns = "id,name,layer,mapset,start_time,end_time"
	else:
	    columns = "id,name,mapset,start_time,end_time"
        if method == "deltagaps":
            maps = sp.get_registered_maps_as_objects_with_gaps(where, None)
        elif method == "delta":
            maps = sp.get_registered_maps_as_objects(where, "start_time", None)
        elif method == "gran":
            maps = sp.get_registered_maps_as_objects_by_granularity(None)

        if header:
            string = ""
	    string += "%s%s" % ("id", separator)
	    string += "%s%s" % ("name", separator)
            if type == "stvds":
		string += "%s%s" % ("layer", separator)
	    string += "%s%s" % ("mapset", separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s"   % ("distance_from_begin")

        if maps and len(maps) > 0:

            if isinstance(maps[0], list):
                first_time, dummy = maps[0][0].get_valid_time()
            else:
                first_time, dummy = maps[0].get_valid_time()

            for mymap in maps:

                if isinstance(mymap, list):
                    map = mymap[0]
                else:
                    map = mymap

                start, end = map.get_valid_time()
                if end:
                    delta = end -start
                else:
                    delta = None
                delta_first = start - first_time

                if map.is_time_absolute():
                    if end:
                        delta = time_delta_to_relative_time(delta)
                    delta_first = time_delta_to_relative_time(delta_first)

                string = ""
                string += "%s%s" % (map.get_id(), separator)
                string += "%s%s" % (map.get_name(), separator)
		if type == "stvds":
		    string += "%s%s" % (map.get_layer(), separator)
                string += "%s%s" % (map.get_mapset(), separator)
                string += "%s%s" % (start, separator)
                string += "%s%s" % (end, separator)
                string += "%s%s" % (delta, separator)
                string += "%s"   % (delta_first)
                print string

    else:
        # In comma separated mode only map ids are needed
        if method == "comma":
            columns = "id"

        rows = sp.get_registered_maps(columns, where, order, None)

        if rows:
            if method == "comma":
                string = ""
                count = 0
                for row in rows:
                    if count == 0:
                        string += row["id"]
                    else:
                        string += ",%s" % row["id"]
                    count += 1
                print string

            elif method == "cols":
                # Print the column names if requested
                if header:
                    output = ""
                    count = 0

                    collist = columns.split(",")

                    for key in collist:
                        if count > 0:
                            output += separator + str(key)
                        else:
                            output += str(key)
                        count += 1
                    print output

                for row in rows:
                    output = ""
                    count = 0
                    for col in row:
                        if count > 0:
                            output += separator + str(col)
                        else:
                            output += str(col)
                        count += 1
                        
                    print output

###############################################################################

def sample_stds_by_stds_topology(intype, sampletype, inputs, sampler, header, separator, method, spatial=False):
    """! Sample the input space time datasets with a sample space time dataset and print the result to stdout

        In case multiple maps are located in the current granule, the map names are separated by comma.
        
        In case a layer is present, the names map ids are extended in this form: name:layer@mapset 

        Attention: Do not use the comma as separator

        @param intype:  Type of the input space time dataset (strds, stvds or str3ds)
        @param samtype: Type of the sample space time dataset (strds, stvds or str3ds)
        @param input: Name of a space time dataset
        @param sampler: Name of a space time dataset used for temporal sampling
        @param header: Set True to print column names 
        @param separator: The field separator character between the columns
        @param method: The method to be used for temporal sampling (start,during,contain,overlap,equal)
        @param spatial: Perform spatial overlapping check
    """
    mapset =  core.gisenv()["MAPSET"]

    input_list = inputs.split(",")
    sts = []

    for input in input_list:
        if input.find("@") >= 0:
            id = input
        else:
            id = input + "@" + mapset

        st = dataset_factory(intype, id)
        sts.append(st)

    if sampler.find("@") >= 0:
        sid = sampler
    else:
        sid = sampler + "@" + mapset

    sst = dataset_factory(sampletype, sid)

    dbif = sql_database_interface_connection()
    dbif.connect()

    for st in sts:
        if st.is_in_db(dbif) == False:
            core.fatal(_("Dataset <%s> not found in temporal database") % (id))
        st.select(dbif)

    if sst.is_in_db(dbif) == False:
        core.fatal(_("Dataset <%s> not found in temporal database") % (sid))

    sst.select(dbif)

    if separator == None or separator == "" or separator.find(",") >= 0:
        separator = " | "
       
    mapmatrizes = []
    for st in sts:
        mapmatrix = st.sample_by_dataset(sst, method, spatial, dbif)
        if mapmatrix and len(mapmatrix) > 0:
            mapmatrizes.append(mapmatrix)

    if len(mapmatrizes) > 0:

        if header:
            string = ""
            string += "%s%s" % (sst.get_id(), separator)
            for st in sts:
                string += "%s%s" % (st.get_id(), separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s"   % ("distance_from_begin")

        first_time, dummy = mapmatrizes[0][0]["granule"].get_valid_time()

        for i in range(len(mapmatrizes[0])):
            mapname_list = []
            for mapmatrix in mapmatrizes:
                mapnames = ""
                count = 0
                entry = mapmatrix[i]
                for sample in entry["samples"]:
                    if count == 0:
                        mapnames += str(sample.get_id())
                    else:
                        mapnames += ",%s" % str(sample.get_id())
                    count += 1
                mapname_list.append(mapnames)
                
            entry = mapmatrizes[0][i]
            map = entry["granule"]

            start, end = map.get_valid_time()
            if end:
                delta = end - start
            else:
                delta = None
            delta_first = start - first_time

            if map.is_time_absolute():
                if end:
                    delta = time_delta_to_relative_time(delta)
                delta_first = time_delta_to_relative_time(delta_first)

            string = ""
            string += "%s%s" % (map.get_id(), separator)
            for mapnames in mapname_list:
                string += "%s%s" % (mapnames, separator)
            string += "%s%s" % (start, separator)
            string += "%s%s" % (end, separator)
            string += "%s%s" % (delta, separator)
            string += "%s"   % (delta_first)
            print string

    dbif.close()

