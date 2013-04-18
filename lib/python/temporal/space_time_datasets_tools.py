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


def register_maps_in_space_time_dataset(
    type, name, maps=None, file=None, start=None,
    end=None, unit=None, increment=None, dbif=None,
        interval=False, fs="|"):
    """!Use this method to register maps in space time datasets. 

       Additionally a start time string and an increment string can be specified
       to assign a time interval automatically to the maps.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       @param type The type of the maps rast, rast3d or vect
       @param name The name of the space time dataset
       @param maps A comma separated list of map names
       @param file Input file one map with start and optional end time, 
                    one per line
       @param start The start date and time of the first raster map
                    (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                    format relative is integer 5)
       @param end The end date and time of the first raster map
                   (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                   format relative is integer 5)
       @param unit The unit of the relative time: years, months, days,
                    hours, minutes, seconds
       @param increment Time increment between maps for time stamp creation
                         (format absolute: NNN seconds, minutes, hours, days,
                         weeks, months, years; format relative: 1.0)
       @param dbif The database interface to be used
       @param interval If True, time intervals are created in case the start
                        time and an increment is provided
       @param fs Field separator used in input file
    """

    start_time_in_file = False
    end_time_in_file = False

    if maps and file:
        core.fatal(_("%s= and %s= are mutually exclusive") % ("maps", "file"))

    if end and increment:
        core.fatal(_("%s= and %s= are mutually exclusive") % (
            "end", "increment"))

    if end and not start:
        core.fatal(_("Please specify %s= and %s=") % ("start_time",
                                                      "end_time"))

    if not maps and not file:
        core.fatal(_("Please specify %s= or %s=") % ("maps", "file"))

    # We may need the mapset
    mapset = core.gisenv()["MAPSET"]

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
            core.fatal(_("Unkown map type: %s") % (type))

    dbif, connected = init_dbif(None)

    if name:
        # Read content from temporal database
        sp.select(dbif)

        if not sp.is_in_db(dbif):
            dbif.close()
            core.fatal(_("Space time %s dataset <%s> no found") %
                       (sp.get_new_map_instance(None).get_type(), name))

        if sp.is_time_relative() and not unit:
            dbif.close()
            core.fatal(_("Space time %s dataset <%s> with relative time found, "
                         "but no relative unit set for %s maps") %
                       (sp.get_new_map_instance(None).get_type(),
                        name, sp.get_new_map_instance(None).get_type()))

    # We need a dummy map object to build the map ids
    dummy = dataset_factory(type, None)

    maplist = []

    # Map names as comma separated string
    if maps:
        if maps.find(",") < 0:
            maplist = [maps, ]
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

            if start_time_in_file and end_time_in_file:
                row["start"] = line_list[1].strip()
                row["end"] = line_list[2].strip()

            if start_time_in_file and not end_time_in_file:
                row["start"] = line_list[1].strip()

            row["id"] = dummy.build_id(mapname, mapset)

            maplist.append(row)

    num_maps = len(maplist)
    map_object_list = []
    statement = ""
    # Store the ids of datasets that must be updated
    datatsets_to_modify = {}

    core.message(_("Gathering map informations"))

    for count in range(len(maplist)):
        if count%50 == 0:
            core.percent(count, num_maps, 1)

        # Get a new instance of the map type
        map = dataset_factory(type, maplist[count]["id"])

        # Use the time data from file
        if "start" in maplist[count]:
            start = maplist[count]["start"]
        if "end" in maplist[count]:
            end = maplist[count]["end"]

        is_in_db = False

        # Put the map into the database
        if not map.is_in_db(dbif):
            is_in_db = False
            # Break in case no valid time is provided
            if start == "" or start is None:
                dbif.close()
                if map.get_layer():
                    core.fatal(_("Unable to register %s map <%s> with layer %s. "
                                 "The map has no valid time and the start time is not set.") %
                               (map.get_type(), map.get_map_id(), map.get_layer()))
                else:
                    core.fatal(_("Unable to register %s map <%s>. The map has no valid"
                                 " time and the start time is not set.") %
                               (map.get_type(), map.get_map_id()))

            if unit:
                map.set_time_to_relative()
            else:
                map.set_time_to_absolute()

        else:
            is_in_db = True
            
            # Check the overwrite flag
            if not core.overwrite():                        
                if map.get_layer():
                    core.warning(_("Map is already registered in temporal database. "
                                   "Unable to update %s map <%s> with layer %s. "
                                   "Overwrite flag is not set.") %
                               (map.get_type(), map.get_map_id(), str(map.get_layer())))
                else:
                    core.warning(_("Map is already registered in temporal database. "
                                   "Unable to update %s map <%s>. "
                                   "Overwrite flag is not set.") %
                               (map.get_type(), map.get_map_id()))
                
                # Simple registration is allowed
                if name:
                    map_object_list.append(map)
                # Jump to next map
                continue
            
            # Select information from temporal database
            map.select(dbif)
            
            # Save the datasets that must be updated
            datasets = map.get_registered_datasets(dbif)
            if datasets:
                for dataset in datasets:
                    datatsets_to_modify[dataset["id"]] = dataset["id"]
                
                if name and map.get_temporal_type() != sp.get_temporal_type():
                    dbif.close()
                    if map.get_layer():
                        core.fatal(_("Unable to update %s map <%s> with layer. "
                                     "The temporal types are different.") %
                                   (map.get_type(), map.get_map_id(), map.get_layer()))
                    else:
                        core.fatal(_("Unable to update %s map <%s>. "
                                     "The temporal types are different.") %
                                   (map.get_type(), map.get_map_id()))

        # Load the data from the grass file database
        map.load()

        # Set the valid time
        if start:
            # In case the time is in the input file we ignore the increment counter
            if start_time_in_file:
                count = 1
            assign_valid_time_to_map(ttype=map.get_temporal_type(),
                                     map=map, start=start, end=end, unit=unit,
                                     increment=increment, mult=count,
                                     interval=interval)

        if is_in_db:
            #  Gather the SQL update statement
            statement += map.update_all(dbif=dbif, execute=False)
        else:
            #  Gather the SQL insert statement
            statement += map.insert(dbif=dbif, execute=False)

        # Sqlite3 performace better for huge datasets when committing in small chunks
        if dbif.dbmi.__name__ == "sqlite3":
            if count % 100 == 0:
                if statement is not None and statement != "":
                    core.message(_("Registering maps in the temporal database")
                                 )
                    dbif.execute_transaction(statement)
                    statement = ""

        # Store the maps in a list to register in a space time dataset
        if name:
            map_object_list.append(map)

    core.percent(num_maps, num_maps, 1)

    if statement is not None and statement != "":
        core.message(_("Register maps in the temporal database"))
        dbif.execute_transaction(statement)

    # Finally Register the maps in the space time dataset
    if name and map_object_list:
        statement = ""
        count = 0
        num_maps = len(map_object_list)
        core.message(_("Register maps in the space time raster dataset"))
        for map in map_object_list:
            if count%50 == 0:
                core.percent(count, num_maps, 1)
            sp.register_map(map=map, dbif=dbif)
            count += 1

    # Update the space time tables
    if name and map_object_list:
        core.message(_("Update space time raster dataset"))
        sp.update_from_registered_maps(dbif)
        sp.update_command_string(dbif=dbif)
    
    # Update affected datasets
    if datatsets_to_modify:
        for dataset in datatsets_to_modify:
            if type == "rast" or type == "raster":
                ds = dataset_factory("strds", dataset)
            elif type == "rast3d":
                ds = dataset_factory("str3ds", dataset)
            elif type == "vect" or type == "vector":
                ds = dataset_factory("stvds", dataset)
            ds.select(dbif)
            ds.update_from_registered_maps(dbif)

    if connected == True:
        dbif.close()

    core.percent(num_maps, num_maps, 1)


###############################################################################

def assign_valid_time_to_map(ttype, map, start, end, unit, increment=None, mult=1, interval=False):
    """!Assign the valid time to a map dataset

       @param ttype The temporal type which should be assigned
                     and which the time format is of
       @param map A map dataset object derived from abstract_map_dataset
       @param start The start date and time of the first raster map
                     (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                     format relative is integer 5)
       @param end The end date and time of the first raster map
                   (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                   format relative is integer 5)
       @param unit The unit of the relative time: years, months,
                    days, hours, minutes, seconds
       @param increment Time increment between maps for time stamp creation
                        (format absolute: NNN seconds, minutes, hours, days,
                        weeks, months, years; format relative is integer 1)
       @param multi A multiplier for the increment
       @param interval If True, time intervals are created in case the start
                        time and an increment is provided
    """

    if ttype == "absolute":
        start_time = string_to_datetime(start)
        if start_time is None:
            core.fatal(_("Unable to convert string \"%s\"into a "
                         "datetime object") % (start))
        end_time = None

        if end:
            end_time = string_to_datetime(end)
            if end_time is None:
                dbif.close()
                core.fatal(_("Unable to convert string \"%s\"into a "
                             "datetime object") % (end))

        # Add the increment
        if increment:
            start_time = increment_datetime_by_string(
                start_time, increment, mult)
            if start_time is None:
                core.fatal(_("Error in increment computation"))
            if interval:
                end_time = increment_datetime_by_string(
                    start_time, increment, 1)
                if end_time is None:
                    core.fatal(_("Error in increment computation"))
        # Commented because of performance issue calling g.message thousend times
        #if map.get_layer():
        #    core.verbose(_("Set absolute valid time for map <%(id)s> with "
        #                   "layer %(layer)s to %(start)s - %(end)s") %
        #                 {'id': map.get_map_id(), 'layer': map.get_layer(),
        #                  'start': str(start_time), 'end': str(end_time)})
        #else:
        #    core.verbose(_("Set absolute valid time for map <%s> to %s - %s") %
        #                 (map.get_map_id(), str(start_time), str(end_time)))

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

        # Commented because of performance issue calling g.message thousend times
        #if map.get_layer():
        #    core.verbose(_("Set relative valid time for map <%s> with layer %s "
        #                   "to %i - %s with unit %s") %
        #                 (map.get_map_id(), map.get_layer(), start_time,
        #                  str(end_time), unit))
        #else:
        #    core.verbose(_("Set relative valid time for map <%s> to %i - %s "
        #                   "with unit %s") % (map.get_map_id(), start_time,
        #                                      str(end_time), unit))

        map.set_relative_time(start_time, end_time, unit)

###############################################################################


def dataset_factory(type, id):
    """!A factory functions to create space time or map datasets

       @param type the dataset type: rast or raster, rast3d,
                    vect or vector, strds, str3ds, stvds
       @param id The id of the dataset ("name@mapset")
    """
    if type == "strds":
        sp = SpaceTimeRasterDataset(id)
    elif type == "str3ds":
        sp = SpaceTimeRaster3DDataset(id)
    elif type == "stvds":
        sp = SpaceTimeVectorDataset(id)
    elif type == "rast" or type == "raster":
        sp = RasterDataset(id)
    elif type == "rast3d":
        sp = Raster3DDataset(id)
    elif type == "vect" or type == "vector":
        sp = VectorDataset(id)
    else:
        core.error(_("Unknown dataset type: %s") % type)
        return None

    return sp

###############################################################################


def list_maps_of_stds(type, input, columns, order, where, separator, method, header, gran=None):
    """! List the maps of a space time dataset using diffetent methods

        @param type The type of the maps raster, raster3d or vector
        @param input Name of a space time raster dataset
        @param columns A comma separated list of columns to be printed to stdout
        @param order A comma separated list of columns to order the
                      space time dataset by category
        @param where A where statement for selected listing without "WHERE"
                      e.g: start_time < "2001-01-01" and end_time > "2001-01-01"
        @param separator The field separator character between the columns
        @param method String identifier to select a method out of cols,
                       comma,delta or deltagaps
            - "cols" Print preselected columns specified by columns
            - "comma" Print the map ids (name@mapset) as comma separated string
            - "delta" Print the map ids (name@mapset) with start time,
                       end time, relative length of intervals and the relative
                       distance to the begin
            - "deltagaps" Same as "delta" with additional listing of gaps.
                           Gaps can be simply identified as the id is "None"
            - "gran" List map using the granularity of the space time dataset,
                      columns are identical to deltagaps
        @param header Set True to print column names
        @param gran The user defined granule to be used if method=gran is set, in case gran=None the 
            granule of the space time dataset is used
    """
    mapset = core.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    dbif, connected = init_dbif(None)
    
    sp = dataset_factory(type, id)

    if not sp.is_in_db(dbif=dbif):
        core.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select(dbif=dbif)

    if separator is None or separator == "":
        separator = "\t"

    # This method expects a list of objects for gap detection
    if method == "delta" or method == "deltagaps" or method == "gran":
        if type == "stvds":
            columns = "id,name,layer,mapset,start_time,end_time"
        else:
            columns = "id,name,mapset,start_time,end_time"
        if method == "deltagaps":
            maps = sp.get_registered_maps_as_objects_with_gaps(where=where, dbif=dbif)
        elif method == "delta":
            maps = sp.get_registered_maps_as_objects(where=where, order="start_time", dbif=dbif)
        elif method == "gran":
            if gran is not None and gran != "":
                maps = sp.get_registered_maps_as_objects_by_granularity(gran=gran, dbif=dbif)
            else:
                maps = sp.get_registered_maps_as_objects_by_granularity(dbif=dbif)
            
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
            string += "%s" % ("distance_from_begin")
            print string

        if maps and len(maps) > 0:

            if isinstance(maps[0], list):
                if len(maps[0]) > 0:
                    first_time, dummy = maps[0][0].get_valid_time()
                else:
                    core.warning(_("Empty map list."))
                    return
            else:
                first_time, dummy = maps[0].get_valid_time()

            for mymap in maps:

                if isinstance(mymap, list):
                    if len(mymap) > 0:
                        map = mymap[0]
                    else:
                        core.fatal(_("Empty entry in map list, this should not happen."))
                else:
                    map = mymap

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
                string += "%s%s" % (map.get_name(), separator)
                if type == "stvds":
                    string += "%s%s" % (map.get_layer(), separator)
                string += "%s%s" % (map.get_mapset(), separator)
                string += "%s%s" % (start, separator)
                string += "%s%s" % (end, separator)
                string += "%s%s" % (delta, separator)
                string += "%s" % (delta_first)
                print string

    else:
        # In comma separated mode only map ids are needed
        if method == "comma":
            columns = "id"

        rows = sp.get_registered_maps(columns, where, order, dbif)

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
    if connected:
        dbif.close()
###############################################################################


def sample_stds_by_stds_topology(intype, sampletype, inputs, sampler, header, 
                                 separator, method, spatial=False, 
                                 print_only=True):
    """!Sample the input space time datasets with a sample 
       space time dataset, return the created map matrix and optionally 
       print the result to stdout

        In case multiple maps are located in the current granule, 
        the map names are separated by comma.

        In case a layer is present, the names map ids are extended 
        in this form: name:layer@mapset

        Attention: Do not use the comma as separator for printing

        @param intype  Type of the input space time dataset (strds, stvds or str3ds)
        @param samtype Type of the sample space time dataset (strds, stvds or str3ds)
        @param inputs Name or comma separated names of space time datasets
        @param sampler Name of a space time dataset used for temporal sampling
        @param header Set True to print column names
        @param separator The field separator character between the columns
        @param method The method to be used for temporal sampling 
                       (start,during,contain,overlap,equal)
        @param spatial Perform spatial overlapping check
        @param print_only If set True (default) then the result of the sampling will be 
                    printed to stdout, if set to False the resulting map matrix 
                    will be returned. 
                    
        @return The map matrix or None if nothing found
    """
    mapset = core.gisenv()["MAPSET"]

    # Make a method list
    method = method.split(",")

    # Split the inputs
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

    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    for st in sts:
        if st.is_in_db(dbif) == False:
            core.fatal(_("Dataset <%s> not found in temporal database") % (st.get_id()))
        st.select(dbif)

    if sst.is_in_db(dbif) == False:
        core.fatal(_("Dataset <%s> not found in temporal database") % (sid))

    sst.select(dbif)

    if separator is None or separator == "" or separator.find(",") >= 0:
        separator = " | "

    mapmatrizes = []
    for st in sts:
        mapmatrix = st.sample_by_dataset(sst, method, spatial, dbif)
        if mapmatrix and len(mapmatrix) > 0:
            mapmatrizes.append(mapmatrix)

    if len(mapmatrizes) > 0:
        
        # Simply return the map matrix
        if not print_only:
            dbif.close()
            return mapmatrizes

        if header:
            string = ""
            string += "%s%s" % (sst.get_id(), separator)
            for st in sts:
                string += "%s%s" % (st.get_id(), separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s" % ("distance_from_begin")
            print string

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
            string += "%s" % (delta_first)
            print string

    dbif.close()
    if len(mapmatrizes) > 0:
        return mapmatrizes
    
    return None

###############################################################################

def tlist_grouped(type, group_type = False):
    """!List of temporal elements grouped by mapsets.

    Returns a dictionary where the keys are mapset 
    names and the values are lists of space time datasets in that
    mapset. Example:

    @code
    >>> tgis.tlist_grouped('strds')['PERMANENT']
    ['precipitation', 'temperature']
    @endcode
    
    @param type element type (strds, str3ds, stvds)

    @return directory of mapsets/elements
    """
    result = {}
    
    mapset = None
    if type == 'stds':
        types = ['strds', 'str3ds', 'stvds']
    else:
        types = [type]
    for type in types:
        try:
            tlist_result = tlist(type)
        except core.ScriptError, e:
            warning(e)
            continue

        for line in tlist_result:
            try:
                name, mapset = line.split('@')
            except ValueError:
                warning(_("Invalid element '%s'") % line)
                continue

            if mapset not in result:
                if group_type:
                    result[mapset] = {}
                else:
                    result[mapset] = []

            if group_type:
                if type in result[mapset]:
                    result[mapset][type].append(name)
                else:        
                    result[mapset][type] = [name, ]
            else:
                result[mapset].append(name)

    return result

###############################################################################

def tlist(type):
    """!Return a list of space time datasets of absolute and relative time
     
    @param type element type (strds, str3ds, stvds)

    @return a list of space time dataset ids
    """
    id = None
    sp = dataset_factory(type, id)

    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    output = []
    temporal_type = ["absolute", 'relative']
    for type in temporal_type:
        # Table name
        if type == "absolute":
            table = sp.get_type() + "_view_abs_time"
        else:
            table = sp.get_type() + "_view_rel_time"

        # Create the sql selection statement
        sql = "SELECT id FROM " + table
        sql += " ORDER BY id"

        dbif.cursor.execute(sql)
        rows = dbif.cursor.fetchall()

        # Append the ids of the space time datasets
        for row in rows:
            for col in row:
                output.append(str(col))
    dbif.close()

    return output

###############################################################################

def create_space_time_dataset(name, type, temporaltype, title, descr, semantic,
                              dbif=None, overwrite=False):
    """!Create a new space time dataset
    
       This function is sensitive to the settings in grass.core.overwrite to
       overwrute existing space time datasets.
    
       @param name The name of the new space time dataset
       @param type The type (strds, stvds, str3ds) of the new space time dataset
       @param temporaltype The temporal type (relative or absolute)
       @param title The title
       @param descr The dataset description
       @param semantic Semantical information
       @param dbif The temporal database interface to be used
       
       @return The new created space time dataset
       
       This function will raise a ScriptError in case of an error.
    """
    
    #Get the current mapset to create the id of the space time dataset

    mapset = core.gisenv()["MAPSET"]
    id = name + "@" + mapset
    
    print id
    print overwrite

    sp = dataset_factory(type, id)

    dbif, connected = init_dbif(dbif)

    if sp.is_in_db(dbif) and overwrite == False:
        if connected:
            dbif.close()
        core.fatal(_("Space time %s dataset <%s> is already in the database. "
                      "Use the overwrite flag.") %
                    (sp.get_new_map_instance(None).get_type(), name))
        return None

    if sp.is_in_db(dbif) and overwrite == True:
        core.warning(_("Overwrite space time %s dataset <%s> "
                     "and unregister all maps.") %
                   (sp.get_new_map_instance(None).get_type(), name))
        sp.delete(dbif)
        sp = sp.get_new_instance(id)

    core.verbose(_("Create new space time %s dataset.") %
                  sp.get_new_map_instance(None).get_type())

    sp.set_initial_values(temporal_type=temporaltype, semantic_type=semantic,
                          title=title, description=descr)
    sp.insert(dbif)

    if connected:
        dbif.close()
        
    return sp
