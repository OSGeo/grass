"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *
from multiprocessing import Process

############################################################################


def dataset_mapcalculator(inputs, output, type, expression, base, method, 
                          nprocs=1, register_null=False, spatial=False):
    """!Perform map-calculations of maps from different space time 
       raster/raster3d datasets, using a specific sampling method 
       to select temporal related maps.

       A mapcalc expression must be provided to process the temporal 
       selected maps. Temporal operators are available in addition to 
       the r.mapcalc operators:
       
       Supported operators for relative and absolute time:
       * td() - the time delta of the current interval in days 
         and fractions of days or the unit in case of relative time
       * start_time() - The start time of the interval from the begin of the time 
         series in days and fractions of days or the unit in case of relative time
       * end_time() - The end time of the current interval from the begin of the time 
         series in days and fractions of days or the unit in case of relative time
         
       Supported operators for absolute time:
       * start_doy() - Day of year (doy) from the start time [1 - 366]
       * start_dow() - Day of week (dow) from the start time [1 - 7], 
         the start of the week is monday == 1
       * start_year() - The year of the start time [0 - 9999]
       * start_month() - The month of the start time [1 - 12]
       * start_week() - Week of year of the start time [1 - 54]
       * start_day() - Day of month from the start time [1 - 31]
       * start_hour() - The hour of the start time [0 - 23]
       * start_minute() - The minute of the start time [0 - 59]
       * start_second() - The second of the start time [0 - 59]
       
       * end_doy() - Day of year (doy) from the start time [1 - 366]
       * end_dow() - Day of week (dow) from the end time [1 - 7], 
         end is monday == 1
       * end_year() - The year of the end time [0 - 9999]
       * end_month() - The month of the end time [1 - 12]
       * end_woy() - Week of year (woy) of the end time [1 - 54]
       * end_day() - Day of month from the start time [1 - 31]
       * end_hour() - The hour of the end time [0 - 23]
       * end_minute() - The minute of the end time [0 - 59]
       * end_second() - The second of the end time [0 - 59]
       

       @param input The name of the input space time raster/raster3d dataset
       @param output The name of the extracted new space time raster(3d) dataset
       @param type The type of the dataset: "raster" or "raster3d"
       @param method The method to be used for temporal sampling
       @param expression The r(3).mapcalc expression
       @param base The base name of the new created maps in case a 
              mapclac expression is provided
       @param nprocs The number of parallel processes to be used for 
              mapcalc processing
       @param register_null Set this number True to register empty maps
       @param spatial Check spatial overlap
    """

    # We need a database interface for fast computation
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = core.gisenv()["MAPSET"]

    input_name_list = inputs.split(",")

    # Process the first input
    if input_name_list[0].find("@") >= 0:
        id = input_name_list[0]
    else:
        id = input_name_list[0] + "@" + mapset

    if type == "raster":
        first_input = SpaceTimeRasterDataset(id)
    else:
        first_input = SpaceTimeRaster3DDataset(id)

    if not first_input.is_in_db(dbif):
        dbif.close()
        core.fatal(_("Space time %s dataset <%s> not found") % (type, id))

    # Fill the object with data from the temporal database
    first_input.select(dbif)

    # All additional inputs in reverse sorted order to avoid 
    # wrong name substitution
    input_name_list = input_name_list[1:]
    input_name_list.sort()
    input_name_list.reverse()
    input_list = []

    for input in input_name_list:

        if input.find("@") >= 0:
            id = input
        else:
            id = input + "@" + mapset

        sp = first_input.get_new_instance(id)

        if not sp.is_in_db(dbif):
            dbif.close()
            core.fatal(_("Space time %s dataset <%s> not "
                         "found in temporal database") % (type, id))

        sp.select(dbif)

        input_list.append(copy.copy(sp))

    # Create the new space time dataset
    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset

    new_sp = first_input.get_new_instance(out_id)

    # Check if in database
    if new_sp.is_in_db(dbif):
        if not core.overwrite():
            dbif.close()
            core.fatal(_("Space time %s dataset <%s> is already in database, "
                         "use overwrite flag to overwrite") % (type, out_id))

    # Sample all inputs by the first input and create a sample matrix
    if spatial:
        core.message(_("Start spatio-temporal sampling"))
    else:
        core.message(_("Start temporal sampling"))
    map_matrix = []
    id_list = []
    sample_map_list = []
    # First entry is the first dataset id
    id_list.append(first_input.get_name())

    if len(input_list) > 0:
        has_samples = False
        for dataset in input_list:
            list = dataset.sample_by_dataset(stds=first_input,
                                             method=method, spatial=spatial, 
                                             dbif=dbif)

            # In case samples are not found
            if not list and len(list) == 0:
                dbif.close()
                core.message(_("No samples found for map calculation"))
                return 0

            # The fist entries are the samples
            map_name_list = []
            if not has_samples:
                for entry in list:
                    granule = entry["granule"]
                    # Do not consider gaps
                    if granule.get_id() is None:
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
                if granule.get_id() is None:
                    continue

                if len(maplist) > 1:
                    core.warning(_("Found more than a single map in a sample "
                                   "granule. Only the first map is used for "
                                   "computation. Use t.rast.aggregate.ds to "
                                   "create synchronous raster datasets."))

                # Store all maps! This includes non existent maps, 
                # identified by id == None
                map_name_list.append(maplist[0].get_name())

            # Attach the map names
            map_matrix.append(copy.copy(map_name_list))

            id_list.append(dataset.get_name())
    else:
        list = first_input.get_registered_maps_as_objects(dbif=dbif)

        if list is None:
            dbif.close()
            core.message(_("No maps in input dataset"))
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

        core.message(_("Start mapcalc computation"))

        count = 0
        # Get the number of samples
        num = len(map_matrix[0])

        # Parallel processing
        proc_list = []
        proc_count = 0

        # For all samples
        for i in range(num):

            count += 1
            core.percent(count, num, 1)

            # Create the r.mapcalc statement for the current time step
            map_name = "%s_%i" % (base, count)
            expr = "%s=%s" % (map_name, expression)
            # Remove spaces and new lines
            expr = expr.replace(" ", "")
            
            # Check that all maps are in the sample
            valid_maps = True
            # Replace all dataset names with their map names of the 
            # current time step
            for j in range(len(map_matrix)):
                if map_matrix[j][i] is None:
                    valid_maps = False
                    break
                # Substitute the dataset name with the map name
                expr = expr.replace(id_list[j], map_matrix[j][i])

            # Proceed with the next sample
            if not valid_maps:
                continue

            # Create the new map id and check if the map is already 
            # in the database
            map_id = map_name + "@" + mapset

            new_map = first_input.get_new_map_instance(map_id)

            # Check if new map is in the temporal database
            if new_map.is_in_db(dbif):
                if core.overwrite():
                    # Remove the existing temporal database entry
                    new_map.delete(dbif)
                    new_map = first_input.get_new_map_instance(map_id)
                else:
                    core.error(_("Map <%s> is already in temporal database, "
                                 "use overwrite flag to overwrite"))
                    continue

            # Set the time stamp
            if sample_map_list[i].is_time_absolute():
                start, end, tz = sample_map_list[i].get_absolute_time()
                new_map.set_absolute_time(start, end, tz)
            else:
                start, end, unit = sample_map_list[i].get_relative_time()
                new_map.set_relative_time(start, end, unit)
            
            # Parse the temporal expressions
            expr = _operator_parser(expr,  sample_map_list[0],  sample_map_list[i])

            map_list.append(new_map)

            # Start the parallel r.mapcalc computation
            core.verbose(_("Apply mapcalc expression: \"%s\"") % expr)

            if type == "raster":
                proc_list.append(Process(target=_run_mapcalc2d, args=(expr,)))
            else:
                proc_list.append(Process(target=_run_mapcalc3d, args=(expr,)))
            proc_list[proc_count].start()
            proc_count += 1

            if proc_count == nprocs or proc_count == num:
                proc_count = 0
                exitcodes = 0
                for proc in proc_list:
                    proc.join()
                    exitcodes += proc.exitcode

                if exitcodes != 0:
                    dbif.close()
                    core.fatal(_("Error while mapcalc computation"))

                # Empty process list
                proc_list = []

        # Register the new maps in the output space time dataset
        core.message(_("Start map registration in temporal database"))

        # Overwrite an existing dataset if requested
        if new_sp.is_in_db(dbif):
            if core.overwrite():
                new_sp.delete(dbif)
                new_sp = first_input.get_new_instance(out_id)

        # Copy the ids from the first input
        temporal_type, semantic_type, title, description = first_input.get_initial_values()
        new_sp.set_initial_values(
            temporal_type, semantic_type, title, description)
        # Insert the dataset in the temporal database
        new_sp.insert(dbif)

        count = 0

        # collect empty maps to remove them
        empty_maps = []

        # Insert maps in the temporal database and in the new space time dataset
        for new_map in map_list:

            count += 1
            core.percent(count, num, 1)

            # Read the map data
            new_map.load()

            # In case of a null map continue, do not register null maps
            if new_map.metadata.get_min() is None and \
               new_map.metadata.get_max() is None:
                if not register_null:
                    empty_maps.append(new_map)
                    continue

            # Insert map in temporal database
            new_map.insert(dbif)

            new_sp.register_map(new_map, dbif)

        # Update the spatio-temporal extent and the metadata table entries
        new_sp.update_from_registered_maps(dbif)

        core.percent(1, 1, 1)

        # Remove empty maps
        if len(empty_maps) > 0:
            names = ""
            count = 0
            for map in empty_maps:
                if count == 0:
                    names += "%s" % (map.get_name())
                else:
                    names += ",%s" % (map.get_name())
                count += 1
            if type == "raster":
                core.run_command("g.remove", rast=names, quiet=True)
            elif type == "raster3d":
                core.run_command("g.remove", rast3d=names, quiet=True)

    dbif.close()


###############################################################################

def _run_mapcalc2d(expr):
    """Helper function to run r.mapcalc in parallel"""
    return core.run_command("r.mapcalc", expression=expr, 
                            overwrite=core.overwrite(), quiet=True)

###############################################################################

def _run_mapcalc3d(expr):
    """Helper function to run r3.mapcalc in parallel"""
    return core.run_command("r3.mapcalc", expression=expr, 
                            overwrite=core.overwrite(), quiet=True)

###############################################################################

def _operator_parser(expr, first, current):
    """This method parses the expression string and substitutes 
       the temporal operators with numerical values.
       
       Supported operators for relative and absolute time are:
       * td() - the time delta of the current interval in days 
         and fractions of days or the unit in case of relative time
       * start_time() - The start time of the interval from the begin of the time 
         series in days and fractions of days or the unit in case of relative time
       * end_time() - The end time of the current interval from the begin of the time 
         series in days and fractions of days or the unit in case of relative time
         
       Supported operators for absolute time:
       * start_doy() - Day of year (doy) from the start time [1 - 366]
       * start_dow() - Day of week (dow) from the start time [1 - 7], 
         the start of the week is monday == 1
       * start_year() - The year of the start time [0 - 9999]
       * start_month() - The month of the start time [1 - 12]
       * start_week() - Week of year of the start time [1 - 54]
       * start_day() - Day of month from the start time [1 - 31]
       * start_hour() - The hour of the start time [0 - 23]
       * start_minute() - The minute of the start time [0 - 59]
       * start_second() - The second of the start time [0 - 59]
       
       * end_doy() - Day of year (doy) from the start time [1 - 366]
       * end_dow() - Day of week (dow) from the end time [1 - 7], 
         end is monday == 1
       * end_year() - The year of the end time [0 - 9999]
       * end_month() - The month of the end time [1 - 12]
       * end_woy() - Week of year (woy) of the end time [1 - 54]
       * end_day() - Day of month from the start time [1 - 31]
       * end_hour() - The hour of the end time [0 - 23]
       * end_minute() - The minute of the end time [0 - 59]
       * end_second() - The second of the end time [0 - 59]
       
       The modified expression is returned.
       
    """
    is_time_absolute = first.is_time_absolute()
    
    expr = _parse_td_operator(expr, is_time_absolute, first, current)
    expr = _parse_start_time_operator(expr, is_time_absolute, first, current)
    expr = _parse_end_time_operator(expr, is_time_absolute, first, current)
    expr = _parse_start_operators(expr, is_time_absolute, current)
    expr = _parse_end_operators(expr, is_time_absolute, current)
    
    return expr

###############################################################################

def _parse_start_operators(expr, is_time_absolute, current):
    """
       Supported operators for absolute time:
       * start_doy() - Day of year (doy) from the start time [1 - 366]
       * start_dow() - Day of week (dow) from the start time [1 - 7], 
         the start of the week is monday == 1
       * start_year() - The year of the start time [0 - 9999]
       * start_month() - The month of the start time [1 - 12]
       * start_week() - Week of year of the start time [1 - 54]
       * start_day() - Day of month from the start time [1 - 31]
       * start_hour() - The hour of the start time [0 - 23]
       * start_minute() - The minute of the start time [0 - 59]
       * start_second() - The second of the start time [0 - 59]
    """
    
    if not is_time_absolute:
        core.fatal(_("The temporal operators <%s> supports only absolute time."%("start_*")))
    
    start, end, tz = current.get_absolute_time()
        
    if expr.find("start_year()") >= 0:
        expr = expr.replace("start_year()", str(start.year))
        
    if expr.find("start_month()") >= 0:
        expr = expr.replace("start_month()", str(start.month))
        
    if expr.find("start_week()") >= 0:
        expr = expr.replace("start_week()", str(start.isocalendar()[1]))
        
    if expr.find("start_day()") >= 0:
        expr = expr.replace("start_day()", str(start.day))
        
    if expr.find("start_hour()") >= 0:
        expr = expr.replace("start_hour()", str(start.hour))
        
    if expr.find("start_minute()") >= 0:
        expr = expr.replace("start_minute()", str(start.minute))
        
    if expr.find("start_second()") >= 0:
        expr = expr.replace("start_second()", str(start.second))
        
    if expr.find("start_dow()") >= 0:
        expr = expr.replace("start_dow()", str(start.isoweekday()))
        
    if expr.find("start_doy()") >= 0:        
        year = datetime(start.year, 1, 1)
        delta = start - year
        
        expr = expr.replace("start_doy()", str(delta.days + 1))
        
    return expr

###############################################################################

def _parse_end_operators(expr, is_time_absolute, current):
    """
       Supported operators for absolute time:
       * end_doy() - Day of year (doy) from the end time [1 - 366]
       * end_dow() - Day of week (dow) from the end time [1 - 7], 
         the end of the week is monday == 1
       * end_year() - The year of the end time [0 - 9999]
       * end_month() - The month of the end time [1 - 12]
       * end_week() - Week of year of the end time [1 - 54]
       * end_day() - Day of month from the end time [1 - 31]
       * end_hour() - The hour of the end time [0 - 23]
       * end_minute() - The minute of the end time [0 - 59]
       * end_second() - The minute of the end time [0 - 59]
       
       In case of time instances the end_* expression will be replaced by null()
    """
    
    if not is_time_absolute:
        core.fatal(_("The temporal operators <%s> supports only absolute time."%("end_*")))
    
    start, end, tz = current.get_absolute_time()
        
    if expr.find("end_year()") >= 0:
        if not end:
            expr = expr.replace("end_year()", "null()")
        else:
            expr = expr.replace("end_year()", str(end.year))
        
    if expr.find("end_month()") >= 0:
        if not end:
            expr = expr.replace("end_month()", "null()")
        else:
            expr = expr.replace("end_month()", str(end.month))
        
    if expr.find("end_week()") >= 0:
        if not end:
            expr = expr.replace("end_week()", "null()")
        else:
            expr = expr.replace("end_week()", str(end.isocalendar()[1]))
        
    if expr.find("end_day()") >= 0:
        if not end:
            expr = expr.replace("end_day()", "null()")
        else:
            expr = expr.replace("end_day()", str(end.day))
        
    if expr.find("end_hour()") >= 0:
        if not end:
            expr = expr.replace("end_hour()", "null()")
        else:
            expr = expr.replace("end_hour()", str(end.hour))
        
    if expr.find("end_minute()") >= 0:
        if not end:
            expr = expr.replace("end_minute()", "null()")
        else:
            expr = expr.replace("end_minute()", str(end.minute))
        
    if expr.find("end_second()") >= 0:
        if not end:
            expr = expr.replace("end_second()", "null()")
        else:
            expr = expr.replace("end_second()", str(end.second))
        
    if expr.find("end_dow()") >= 0:
        if not end:
            expr = expr.replace("end_dow()", "null()")
        else:
            expr = expr.replace("end_dow()", str(end.isoweekday()))
        
    if expr.find("end_doy()") >= 0:
        if not end:
            expr = expr.replace("end_doy()", "null()")
        else:
            year = datetime(end.year, 1, 1)
            delta = end - year
            
            expr = expr.replace("end_doy()", str(delta.days + 1))
        
    return expr

###############################################################################

def _parse_td_operator(expr, is_time_absolute, first, current):
    """Parse the time delta operator td(). This operator
       represents the size of the current time interval
       in days and fraction of days for absolute time,
       and in relative units in case of relative time.
       
       In case of time instances, the td() operator will be of type null(). 
    """
    if expr.find("td()") >= 0:
        td = "null()"
        if is_time_absolute:
            start, end, tz = current.get_absolute_time()
            if end != None:
                td = time_delta_to_relative_time(end - start)
        else:
            start, end, unit = current.get_relative_time()
            if end != None:
                td = end - start
        expr = expr.replace("td()", str(td))
        
    return expr

###############################################################################

def _parse_start_time_operator(expr, is_time_absolute, first, current):
    """Parse the start_time() operator. This operator represent 
    the time difference between the start time of the sample space time
    raster dataset and the start time of the current interval or instance. The time
    is measured  in days and fraction of days for absolute time,
    and in relative units in case of relative time."""
    if expr.find("start_time()") >= 0:
        if is_time_absolute:
            start1, end, tz = first.get_absolute_time()
            start, end, tz = current.get_absolute_time()
            x = time_delta_to_relative_time(start - start1)
        else:
            start1, end, unit = first.get_relative_time()
            start, end, unit = current.get_relative_time()
            x = start - start1
        expr = expr.replace("start_time()", str(x))
        
    return expr

###############################################################################

def _parse_end_time_operator(expr, is_time_absolute, first, current):
    """Parse the end_time() operator. This operator represent 
    the time difference between the start time of the sample space time
    raster dataset and the start time of the current interval. The time
    is measured  in days and fraction of days for absolute time,
    and in relative units in case of relative time.
    
    The end_time() will be represented by null() in case of a time instance.
    """
    if expr.find("end_time()") >= 0:
        x = "null()"
        if is_time_absolute:
            start1, end, tz = first.get_absolute_time()
            start, end, tz = current.get_absolute_time()
            if end:
                x = time_delta_to_relative_time(end - start1)
        else:
            start1, end, unit = first.get_relative_time()
            start, end, unit = current.get_relative_time()
            if end:
                x = end - start1
        expr = expr.replace("end_time()", str(x))
        
    return expr

