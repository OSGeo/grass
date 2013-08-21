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
from factory import *
from open import *

###############################################################################


def register_maps_in_space_time_dataset(
    type, name, maps=None, file=None, start=None,
    end=None, unit=None, increment=None, dbif=None,
        interval=False, fs="|"):
    """!Use this method to register maps in space time datasets.

       Additionally a start time string and an increment string can be
       specified to assign a time interval automatically to the maps.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       @param type The type of the maps rast, rast3d or vect
       @param name The name of the space time dataset
       @param maps A comma separated list of map names
       @param file Input file, one map per line map with start and optional
                   end time
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
        core.fatal(_("%(m)s= and %(f)s= are mutually exclusive") % {'m': "maps",
                                                                    'f': "file"})

    if end and increment:
        core.fatal(_("%(e)s= and %(i)s= are mutually exclusive") % {'e': "end",
                   'i': "increment"})

    if end and not start:
        core.fatal(_("Please specify %(st)s= and %(e)s=") % {'st': "start_time",
                                                             'e': "end_time"})

    if not maps and not file:
        core.fatal(_("Please specify %(m)s= or %(f)s=") % {'m': "maps",
                                                           'f': "file"})

    # We may need the mapset
    mapset = core.gisenv()["MAPSET"]
    dbif, connected = init_dbif(None)

    # The name of the space time dataset is optional
    if name:
        sp = open_old_space_time_dataset(name, type, dbif)

        if sp.is_time_relative() and not unit:
            dbif.close()
            core.fatal(_("Space time %(sp)s dataset <%(name)s> with relative"
                         " time found, but no relative unit set for %(sp)s "
                         "maps") % {
                         'sp': sp.get_new_map_instance(None).get_type(),
                         'name': name})

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
        if count % 50 == 0:
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
                    core.fatal(_("Unable to register %(t)s map <%(id)s> with "
                                 "layer %(l)s. The map has no valid time and "
                                 "the start time is not set.") % {
                                 't': map.get_type(), 'id': map.get_map_id(),
                                 'l': map.get_layer()})
                else:
                    core.fatal(_("Unable to register %(t)s map <%(id)s>. The"
                                 " map has no valid time and the start time "
                                 "is not set.") % {'t': map.get_type(),
                                                   'id': map.get_map_id()})

            if unit:
                map.set_time_to_relative()
            else:
                map.set_time_to_absolute()

        else:
            is_in_db = True
            # Check the overwrite flag
            if not core.overwrite():
                if map.get_layer():
                    core.warning(_("Map is already registered in temporal "
                                   "database. Unable to update %(t)s map "
                                   "<%(id)s> with layer %(l)s. Overwrite flag"
                                   " is not set.") % {'t': map.get_type(),
                                                      'id': map.get_map_id(),
                                                      'l': str(map.get_layer())})
                else:
                    core.warning(_("Map is already registered in temporal "
                                   "database. Unable to update %(t)s map "
                                   "<%(id)s>. Overwrite flag is not set.") % {
                                   't': map.get_type(), 'id': map.get_map_id()})

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
                        core.fatal(_("Unable to update %(t)s map <%(id)s> "
                                     "with layer %(l)s. The temporal types "
                                     "are different.") % {'t': map.get_type(),
                                                        'id': map.get_map_id(),
                                                        'l': map.get_layer()})
                    else:
                        core.fatal(_("Unable to update %(t)s map <%(id)s>. "
                                     "The temporal types are different.") %
                                     {'t': map.get_type(),
                                      'id': map.get_map_id()})

        # Load the data from the grass file database
        map.load()

        # Set the valid time
        if start:
            # In case the time is in the input file we ignore the increment
            # counter
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

        # Sqlite3 performace better for huge datasets when committing in
        # small chunks
        if dbif.dbmi.__name__ == "sqlite3":
            if count % 100 == 0:
                if statement is not None and statement != "":
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
        count = 0
        num_maps = len(map_object_list)
        core.message(_("Register maps in the space time raster dataset"))
        for map in map_object_list:
            if count % 50 == 0:
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

def assign_valid_time_to_map(ttype, map, start, end, unit, increment=None,
                             mult=1, interval=False):
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
       @param mult A multiplier for the increment
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
