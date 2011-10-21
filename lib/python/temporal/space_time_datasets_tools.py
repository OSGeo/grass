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

def register_maps_in_space_time_dataset(type, name, maps=None, file=None, start=None, end=None, increment=None, dbif = None, interval=False, fs="|"):
    """Use this method to register maps in space time datasets. This function is generic and
       can handle raster, vector and raster3d maps as well as there space time datasets.

       Additionally a start time string and an increment string can be specified
       to assign a time interval automatically to the maps.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       @param type: The type of the maps raster, raster3d or vector
       @param name: The name of the space time dataset
       @param maps: A comma separated list of map names
       @param file: Input file one map with optional start and end time, one per line
       @param start: The start date and time of the first raster map, in case the map has no date (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative 5.0)
       @param increment: Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
       @param dbif: The database interface to be used
       @param interval: If True, time intervals are created in case the start time and an increment is provided
       @param fs: Field separator used in input file
    """

    start_time_in_file = False
    end_time_in_file = False

    if maps and file:
        core.fata(_("%s= and %s= are mutually exclusive") % ("input","file"))

    if end and increment:
        core.fata(_("%s= and %s= are mutually exclusive") % ("end","increment"))

    if end and not start:
        core.fata(_("Please specify %s= and %s=") % ("start_time","end_time"))

    if not maps and not file:
        core.fata(_("Please specify %s= or %s=") % ("input","file"))

    if start and start == "file":
        start_time_in_file = True

    if end and end == "file":
        end_time_in_file = True

    # We may need the mapset
    mapset =  core.gisenv()["MAPSET"]

    # Check if the dataset name contains the mapset as well
    if name.find("@") < 0:
        id = name + "@" + mapset
    else:
        id = name

    sp = dataset_factory(type, id)

    connect = False

    if dbif == None:
        dbif = sql_database_interface()
        dbif.connect()
        connect = True

    # Read content from temporal database
    sp.select(dbif)

    if sp.is_in_db(dbif) == False:
        dbif.close()
        core.fatal(_("Space time %s dataset <%s> no found") % (sp.get_new_map_instance(None).get_type(), name))

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

            line_list = line.split(fs)

            mapname = line_list[0].strip()

            if mapname.find("@") < 0:
                mapid = mapname + "@" + mapset
            else:
                mapid = mapname

            row = {}
            row["id"] = mapid

            if start_time_in_file and  end_time_in_file:
                row["start"] = line_list[1].strip()
                row["end"] = line_list[2].strip()

            if start_time_in_file and  not end_time_in_file:
                row["start"] = line_list[1].strip()

            maplist.append(row)
    
    num_maps = len(maplist)
    count = 0
    for entry in maplist:
	core.percent(count, num_maps, 1)

        # Get a new instance of the space time dataset map type
        if file:
            map = sp.get_new_map_instance(entry["id"])
        else:
            if entry.find("@") < 0:
                mapid = entry + "@" + mapset
            else:
                mapid = entry

            map = sp.get_new_map_instance(mapid)

        # Use the time data from file
        if start_time_in_file:
            start = entry["start"]
        if end_time_in_file:
            end = entry["end"]

        # Put the map into the database
        if map.is_in_db(dbif) == False:
            # Break in case no valid time is provided
            if start == "" or start == None:
                dbif.close()
                core.fatal(_("Unable to register %s map <%s>. The map has no valid time and the start time is not set.") % \
                            (map.get_type(), map.get_id() ))
            # Load the data from the grass file database
            map.load()

            if sp.get_temporal_type() == "absolute":
                map.set_time_to_absolute()
            else:
                map.set_time_to_relative()
            #  Put it into the temporal database
            map.insert(dbif)
        else:
            map.select(dbif)
            if map.get_temporal_type() != sp.get_temporal_type():
                dbif.close()
                core.fatal(_("Unable to register %s map <%s>. The temporal types are different.") %  (map.get_type(), map.get_id()))

        # In case the time is in the input file we ignore the increment counter
        if start_time_in_file:
            count = 1

        # Set the valid time
        if start:
            assign_valid_time_to_map(ttype=sp.get_temporal_type(), map=map, start=start, end=end, increment=increment, mult=count, dbif=dbif, interval=interval)

        # Finally Register map in the space time dataset
        sp.register_map(map, dbif)
        count += 1

    # Update the space time tables
    sp.update_from_registered_maps(dbif)

    if connect == True:
        dbif.close()

    core.percent(num_maps, num_maps, 1)
        
###############################################################################

def unregister_maps_from_space_time_datasets(type, name, maps, file=None, dbif = None):
    """Unregister maps from a single space time dataset or, in case no dataset name is provided,
       unregister from all datasets within the maps are registered.

       @param type: The type of the maps raster, vector or raster3d
       @param name: Name of an existing space time raster dataset. If no name is provided the raster map(s) are unregistered from all space time datasets in which they are registered.
       @param maps: A comma separated list of map names
       @param dbif: The database interface to be used
    """

    if maps and file:
        core.fata(_("%s= and %s= are mutually exclusive") % ("input","file"))

    mapset =  core.gisenv()["MAPSET"]

    if dbif == None:
        dbif = sql_database_interface()
        dbif.connect()
        connect = True

    # In case a space time dataset is specified
    if name:
        # Check if the dataset name contains the mapset as well
        if name.find("@") < 0:
            id = name + "@" + mapset
        else:
            id = name

        if type == "rast":
            sp = dataset_factory("strds", id)
        if type == "rast3d":
            sp = dataset_factory("str3ds", id)
        if type == "vect":
            sp = dataset_factory("stvds", id)

        if sp.is_in_db(dbif) == False:
            dbif.close()
            core.fatal("Space time " + sp.get_new_map_instance(None).get_type() + " dataset <" + name + "> not found")

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

            line_list = line.split(fs)
            mapname = line_list[0].strip()
            maplist.append(mapname)
    
    num_maps = len(maplist)
    count = 0
    for mapname in maplist:
	core.percent(count, num_maps, 1)
        mapname = mapname.strip()
        # Check if the map name contains the mapset as well
        if mapname.find("@") < 0:
            mapid = mapname + "@" + mapset
        else:
            mapid = mapname
            
        map = dataset_factory(type, mapid)

        # Unregister map if in database
        if map.is_in_db(dbif) == True:
            if name:
                sp.select(dbif)
                sp.unregister_map(map, dbif)
            else:
                map.select(dbif)
                map.unregister(dbif)
		
	count += 1

    if name:
        sp.update_from_registered_maps(dbif)

    if connect == True:
        dbif.close()
	
    core.percent(num_maps, num_maps, 1)

###############################################################################

def assign_valid_time_to_maps(type, maps, ttype, start, end=None, file=file, increment=None, dbif = None, interval=False, fs="|"):
    """Use this method to assign valid time (absolute or relative) to raster,
       raster3d and vector datasets.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       Valid end time and increment are mutual exclusive.

       @param type: The type of the maps raster, raster3d or vector
       @param maps: A comma separated list of map names
       @param start: The start date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative 5.0)
       @param end: The end date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative 5.0)
       @param increment: Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
       @param file: Input file one map with optional start and end time, one per line
       @param dbif: The database interface to be used
       @param interval: If True, time intervals are created in case the start time and an increment is provided
       @param fs: Field separator used in input file
    """

    start_time_in_file = False
    end_time_in_file = False

    if end and increment:
        if dbif:
            dbif.close()
        core.fatal(_("Valid end time and increment are mutual exclusive"))

    # List of space time datasets to be updated
    splist = {}

    if maps and file:
        core.fata(_("%s= and %s= are mutually exclusive") % ("input","file"))

    if end and increment:
        core.fata(_("%s= and %s= are mutually exclusive") % ("end","increment"))

    if end and not start:
        core.fata(_("Please specify %s= and %s=") % ("start_time","end_time"))

    if not maps and not file:
        core.fata(_("Please specify %s= or %s=") % ("input","file"))

    if start and start == "file":
        start_time_in_file = True

    if end and end == "file":
        end_time_in_file = True

    # We may need the mapset
    mapset =  core.gisenv()["MAPSET"]

    connect = False

    if dbif == None:
        dbif = sql_database_interface()
        dbif.connect()
        connect = True

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

            line_list = line.split(fs)

            mapname = line_list[0].strip()

            if mapname.find("@") < 0:
                mapid = mapname + "@" + mapset
            else:
                mapid = mapname

            row = {}
            row["id"] = mapid

            if start_time_in_file and  end_time_in_file:
                row["start"] = line_list[1].strip()
                row["end"] = line_list[2].strip()

            if start_time_in_file and  not end_time_in_file:
                row["start"] = line_list[1].strip()

            maplist.append(row)
    
    num_maps = len(maplist)
    count = 0

    for entry in maplist:
	core.percent(count, num_maps, 1)
        if file:
            mapid = entry["id"]
        else:
            if entry.find("@") < 0:
                mapid = entry + "@" + mapset
            else:
                mapid = entry

        sp = dataset_factory(type, id)

        # Use the time data from file
        if start_time_in_file:
            start = entry["start"]
        if end_time_in_file:
            end = entry["end"]

        if map.is_in_db(dbif) == False:
            # Load the data from the grass file database
            map.load()
            if ttype == "absolute":
                map.set_time_to_absolute()
            else:
                map.set_time_to_relative()
            #  Put it into the temporal database
            map.insert(dbif)
        else:
            map.select(dbif)
            sprows = map.get_registered_datasets(dbif)
            # Make an entry in the dataset list, using a dict make sure that
            # each dataset is listed only once
            if sprows != None:
                for dataset in sprows:
                    splist[dataset["id"]] = True
            
        # In case the time is in the input file we ignore the increment counter
        if start_time_in_file:
            count = 1

        # Set the valid time
        assign_valid_time_to_map(ttype=ttype, map=map, start=start, end=end, increment=increment, mult=count, dbif=dbif, interval=interval)

        count += 1

    # Update all the space time datasets in which registered maps are changed there valid time
    for name in splist.keys():
        sp = map.get_new_stds_instance(name)
        sp.select(dbif)
        sp.update_from_registered_maps(dbif)

    if connect == True:
        dbif.close()

    core.percent(num_maps, num_maps, 1)
    

###############################################################################

def assign_valid_time_to_map(ttype, map, start, end, increment=None, mult=1, dbif = None, interval=False):
    """Assign the valid time to a map dataset

       @param ttype: The temporal type which should be assigned and which the time format is of
       @param map: A map dataset object derived from abstract_map_dataset
       @param start: The start date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative 5.0)
       @param end: The end date and time of the first raster map (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative 5.0)
       @param increment: Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
       @param multi: A multiplier for the increment
       @param dbif: The database interface to use for sql queries
       @param interval: If True, time intervals are created in case the start time and an increment is provided
    """
    
    connect = False

    if dbif == None:
        dbif = sql_database_interface()
        dbif.connect()
        connect = True

    if ttype == "absolute":
        # Create the start time object
        if start.find(":") > 0:
            time_format = "%Y-%m-%d %H:%M:%S"
        else:
            time_format = "%Y-%m-%d"

        start_time = datetime.strptime(start, time_format)
        end_time = None
        
        if end:
            end_time = datetime.strptime(end, time_format)

        # Add the increment
        if increment:
            start_time = increment_datetime_by_string(start_time, increment, mult)
            if interval:
                end_time = increment_datetime_by_string(start_time, increment, 1)

        core.verbose(_("Set absolute valid time for map <%s> to %s - %s") % (map.get_id(), str(start_time), str(end_time)))
        map.update_absolute_time(start_time, end_time, None, dbif)
    else:
        start_time = float(start)
        end_time = None

        if end:
            end_time = float(end)

        if increment:
            start_time = start_time + mult * float(increment)
            if interval:
                end_time = start_time + float(increment)

        core.verbose(_("Set relative valid time for map <%s> to %f - %s") % (map.get_id(), start_time,  str(end_time)))
        map.update_relative_time(start_time, end_time, dbif)

    if connect == True:
        dbif.close()

###############################################################################

def dataset_factory(type, id):
    """A factory functions to create space time or map datasets
    
       @param type: the dataset type: rast, rast3d, vect, strds, str3ds, stvds
       @param id: The id of the dataset ("name@mapset")
    """
    if type == "strds":
        sp = space_time_raster_dataset(id)
    elif type == "str3ds":
        sp = space_time_raster3d_dataset(id)
    elif type == "stvds":
        sp = space_time_vector_dataset(id)
    elif type == "rast":
        sp = raster_dataset(id)
    elif type == "rast3d":
        sp = raster3d_dataset(id)
    elif type == "vect":
        sp = vector_dataset(id)
    else:
        core.error(_("Unknown dataset type: %s") % type)
        return None

    return sp

###############################################################################

def list_maps_of_stds(type, input, columns, order, where, separator, method, header):
    """ List the maps of a space time dataset using diffetent methods

        @param type: The type of the maps raster, raster3d or vector
        @param input: Name of a space time raster dataset
        @param columns: A comma separated list of columns to be printed to stdout 
        @param order: A comma seoarated list of columns to order the space time dataset by category 
        @param where: A where statement for selected listing without "WHERE" e.g: start_time < "2001-01-01" and end_time > "2001-01-01"
        @param separator: The field separator character between the columns
        @param method: String identifier to select a method out of cols,comma,delta or deltagaps
            * "cols": Print preselected columns specified by columns
            * "comma": Print the map ids (name@mapset) as comma separated string
            * "delta": Print the map ids (name@mapset) with start time, end time, relative length of intervals and the relative distance to the begin
            * "deltagaps": Same as "delta" with addtitionakl listing of gaps. Gaps can be simply identified as the id is "None"
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
    if method == "delta" or method == "deltagaps":
        columns = "id,start_time,end_time"
        if method == "deltagaps":
            maps = sp.get_registered_maps_as_objects_with_gaps(where, None)
        else:
            maps = sp.get_registered_maps_as_objects(where, "start_time", None)

        if header:
            string = ""
            string += "%s%s" % ("id", separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s"   % ("distance_from_begin")
            print string

        if maps and len(maps) > 0:

            first_time, dummy = maps[0].get_valid_time()

            for map in maps:
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


