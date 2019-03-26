"""
Functions to register map layer in space time datasets and the temporal database

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.register_maps_in_space_time_dataset(type, name, maps)

(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from datetime import datetime
import grass.script as gscript
from .core import get_tgis_message_interface, init_dbif, get_current_mapset
from .open_stds import open_old_stds
from .abstract_map_dataset import AbstractMapDataset
from .factory import dataset_factory
from .datetime_math import check_datetime_string, increment_datetime_by_string, string_to_datetime

###############################################################################


def register_maps_in_space_time_dataset(
    type, name, maps=None, file=None, start=None,
    end=None, unit=None, increment=None, dbif=None,
    interval=False, fs="|", update_cmd_list=True):
    """Use this method to register maps in space time datasets.

       Additionally a start time string and an increment string can be
       specified to assign a time interval automatically to the maps.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       :param type: The type of the maps raster, raster_3d or vector
       :param name: The name of the space time dataset. Maps will be
                    registered in the temporal database if the name was set
                    to None
       :param maps: A comma separated list of map names
       :param file: Input file, one map per line map with start and optional
                   end time
       :param start: The start date and time of the first map
                    (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                    format relative is integer 5)
       :param end: The end date and time of the first map
                  (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                  format relative is integer 5)
       :param unit: The unit of the relative time: years, months, days,
                   hours, minutes, seconds
       :param increment: Time increment between maps for time stamp creation
                        (format absolute: NNN seconds, minutes, hours, days,
                        weeks, months, years; format relative: 1.0)
       :param dbif: The database interface to be used
       :param interval: If True, time intervals are created in case the start
                       time and an increment is provided
       :param fs: Field separator used in input file
       :param update_cmd_list: If is True, the command that was invoking this
                               process will be written to the process history
    """
    start_time_in_file = False
    end_time_in_file = False
    msgr = get_tgis_message_interface()

    # Make sure the arguments are of type string
    if start != "" and start is not None:
        start = str(start)
    if end != "" and end is not None:
        end = str(end)
    if increment != "" and increment is not None:
        increment = str(increment)

    if maps and file:
        msgr.fatal(_("%s= and %s= are mutually exclusive") % ("maps", "file"))

    if end and increment:
        msgr.fatal(_("%s= and %s= are mutually exclusive") % ("end",
                                                              "increment"))

    if end and interval:
        msgr.fatal(_("%s= and the %s flag are mutually exclusive") % ("end",
                                                                      "interval"))

    if increment and not start:
        msgr.fatal(_("The increment option requires the start option"))

    if interval and not start:
        msgr.fatal(_("The interval flag requires the start option"))

    if end and not start:
        msgr.fatal(_("Please specify %s= and %s=") % ("start_time",
                                                      "end_time"))

    if not maps and not file:
        msgr.fatal(_("Please specify %s= or %s=") % ("maps", "file"))
    # We may need the mapset
    mapset = get_current_mapset()
    dbif, connected = init_dbif(None)

    # The name of the space time dataset is optional
    if name:
        sp = open_old_stds(name, type, dbif)

        if sp.is_time_relative() and (start or end) and not unit:
            dbif.close()
            msgr.fatal(_("Space time %(sp)s dataset <%(name)s> with relative"
                         " time found, but no relative unit set for %(sp)s "
                         "maps") % {'name': name,
                       'sp': sp.get_new_map_instance(None).get_type()})

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
            mapid = AbstractMapDataset.build_id(maplist[count], mapset, None)

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

            row["id"] = AbstractMapDataset.build_id(mapname, mapset)

            maplist.append(row)

        if start_time_in_file is True and increment:
            increment = None
            msgr.warning(_("The increment option will be ignored because of time stamps in input file"))

        if start_time_in_file is True and interval:
            increment = None
            msgr.warning(_("The interval flag will be ignored because of time stamps in input file"))

    num_maps = len(maplist)
    map_object_list = []
    statement = ""
    # Store the ids of datasets that must be updated
    datatsets_to_modify = {}

    msgr.message(_("Gathering map information..."))

    for count in range(len(maplist)):
        if count % 50 == 0:
            msgr.percent(count, num_maps, 1)

        # Get a new instance of the map type
        map = dataset_factory(type, maplist[count]["id"])

        if map.map_exists() is not True:
            msgr.fatal(_("Unable to update %(t)s map <%(id)s>. "
                         "The map does not exist.") % {'t': map.get_type(),
                                                       'id': map.get_map_id()})

        # Use the time data from file
        if "start" in maplist[count]:
            start = maplist[count]["start"]
        if "end" in maplist[count]:
            end = maplist[count]["end"]

        is_in_db = False

        # Put the map into the database
        if not map.is_in_db(dbif):
            # Break in case no valid time is provided
            if (start == "" or start is None) and not map.has_grass_timestamp():
                dbif.close()
                if map.get_layer():
                    msgr.fatal(_("Unable to register %(t)s map <%(id)s> with "
                                 "layer %(l)s. The map has timestamp and "
                                 "the start time is not set.") % {
                               't': map.get_type(), 'id': map.get_map_id(),
                               'l': map.get_layer()})
                else:
                    msgr.fatal(_("Unable to register %(t)s map <%(id)s>. The"
                                 " map has no timestamp and the start time "
                                 "is not set.") % {'t': map.get_type(),
                                                   'id': map.get_map_id()})
            if start != "" and start is not None:
                # We need to check if the time is absolute and the unit was specified
                time_object = check_datetime_string(start)
                if isinstance(time_object, datetime) and unit:
                    msgr.fatal(_("%(u)s= can only be set for relative time") %
                               {'u': "unit"})
                if not isinstance(time_object, datetime) and not unit:
                    msgr.fatal(_("%(u)s= must be set in case of relative time"
                                 " stamps") % {'u': "unit"})

                if unit:
                    map.set_time_to_relative()
                else:
                    map.set_time_to_absolute()

        else:
            is_in_db = True
            # Check the overwrite flag
            if not gscript.overwrite():
                if map.get_layer():
                    msgr.warning(_("Map is already registered in temporal "
                                   "database. Unable to update %(t)s map "
                                   "<%(id)s> with layer %(l)s. Overwrite flag"
                                   " is not set.") % {'t': map.get_type(),
                                                      'id': map.get_map_id(),
                                                      'l': str(map.get_layer())})
                else:
                    msgr.warning(_("Map is already registered in temporal "
                                   "database. Unable to update %(t)s map "
                                   "<%(id)s>. Overwrite flag is not set.") %
                                 {'t': map.get_type(), 'id': map.get_map_id()})

                # Simple registration is allowed
                if name:
                    map_object_list.append(map)
                # Jump to next map
                continue

            # Select information from temporal database
            map.select(dbif)

            # Save the datasets that must be updated
            datasets = map.get_registered_stds(dbif)
            if datasets is not None:
                for dataset in datasets:
                    if dataset != "":
                        datatsets_to_modify[dataset] = dataset

                if name and map.get_temporal_type() != sp.get_temporal_type():
                    dbif.close()
                    if map.get_layer():
                        msgr.fatal(_("Unable to update %(t)s map <%(id)s> "
                                     "with layer %(l)s. The temporal types "
                                     "are different.") % {'t': map.get_type(),
                                                          'id': map.get_map_id(),
                                                          'l': map.get_layer()})
                    else:
                        msgr.fatal(_("Unable to update %(t)s map <%(id)s>. "
                                     "The temporal types are different.") %
                                   {'t': map.get_type(),
                                    'id': map.get_map_id()})

        # Load the data from the grass file database
        map.load()

        # Try to read an existing time stamp from the grass spatial database
        # in case this map wasn't already registered in the temporal database
        # Read the spatial database time stamp only, if no time stamp was provided for this map
        # as method argument or in the input file
        if not is_in_db and not start:
            map.read_timestamp_from_grass()

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

        # Sqlite3 performance is better for huge datasets when committing in
        # small chunks
        if dbif.get_dbmi().__name__ == "sqlite3":
            if count % 100 == 0:
                if statement is not None and statement != "":
                    dbif.execute_transaction(statement)
                    statement = ""

        # Store the maps in a list to register in a space time dataset
        if name:
            map_object_list.append(map)

    msgr.percent(num_maps, num_maps, 1)

    if statement is not None and statement != "":
        msgr.message(_("Registering maps in the temporal database..."))
        dbif.execute_transaction(statement)

    # Finally Register the maps in the space time dataset
    if name and map_object_list:
        count = 0
        num_maps = len(map_object_list)
        msgr.message(_("Registering maps in the space time dataset..."))
        for map in map_object_list:
            if count % 50 == 0:
                msgr.percent(count, num_maps, 1)
            sp.register_map(map=map, dbif=dbif)
            count += 1

    # Update the space time tables
    if name and map_object_list:
        msgr.message(_("Updating space time dataset..."))
        sp.update_from_registered_maps(dbif)
        if update_cmd_list is True:
            sp.update_command_string(dbif=dbif)

    # Update affected datasets
    if datatsets_to_modify:
        for dataset in datatsets_to_modify:
            if type == "rast" or type == "raster":
                ds = dataset_factory("strds", dataset)
            elif type == "raster_3d" or type == "rast3d" or type == "raster3d":
                ds = dataset_factory("str3ds", dataset)
            elif type == "vect" or type == "vector":
                ds = dataset_factory("stvds", dataset)
            ds.select(dbif)
            ds.update_from_registered_maps(dbif)

    if connected is True:
        dbif.close()

    msgr.percent(num_maps, num_maps, 1)


###############################################################################

def assign_valid_time_to_map(ttype, map, start, end, unit, increment=None,
                             mult=1, interval=False):
    """Assign the valid time to a map dataset

       :param ttype: The temporal type which should be assigned
                     and which the time format is of
       :param map: A map dataset object derived from abstract_map_dataset
       :param start: The start date and time of the first map
                     (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                     format relative is integer 5)
       :param end: The end date and time of the first map
                   (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd",
                   format relative is integer 5)
       :param unit: The unit of the relative time: years, months,
                    days, hours, minutes, seconds
       :param increment: Time increment between maps for time stamp creation
                        (format absolute: NNN seconds, minutes, hours, days,
                        weeks, months, years; format relative is integer 1)
       :param mult: A multiplier for the increment
       :param interval: If True, time intervals are created in case the start
                        time and an increment is provided
    """

    msgr = get_tgis_message_interface()

    if ttype == "absolute":
        start_time = string_to_datetime(start)
        if start_time is None:
            msgr.fatal(_("Unable to convert string \"%s\"into a "
                         "datetime object") % (start))
        end_time = None

        if end:
            end_time = string_to_datetime(end)
            if end_time is None:
                msgr.fatal(_("Unable to convert string \"%s\"into a "
                             "datetime object") % (end))

        # Add the increment
        if increment:
            start_time = increment_datetime_by_string(
                start_time, increment, mult)
            if start_time is None:
                msgr.fatal(_("Error occurred in increment computation"))
            if interval:
                end_time = increment_datetime_by_string(
                    start_time, increment, 1)
                if end_time is None:
                    msgr.fatal(_("Error occurred in increment computation"))

        if map.get_layer():
            msgr.debug(1, _("Set absolute valid time for map <%(id)s> with "
                            "layer %(layer)s to %(start)s - %(end)s") %
                       {'id': map.get_map_id(), 'layer': map.get_layer(),
                        'start': str(start_time), 'end': str(end_time)})
        else:
            msgr.debug(1, _("Set absolute valid time for map <%s> to %s - %s")
                       % (map.get_map_id(), str(start_time), str(end_time)))

        map.set_absolute_time(start_time, end_time)
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
            msgr.debug(1, _("Set relative valid time for map <%s> with layer"
                            " %s to %i - %s with unit %s") %
                       (map.get_map_id(), map.get_layer(), start_time,
                       str(end_time), unit))
        else:
            msgr.debug(1, _("Set relative valid time for map <%s> to %i - %s "
                            "with unit %s") % (map.get_map_id(), start_time,
                                               str(end_time), unit))

        map.set_relative_time(start_time, end_time, unit)


##############################################################################

def register_map_object_list(type,  map_list, output_stds,
                             delete_empty=False, unit=None, dbif=None):
    """Register a list of AbstractMapDataset objects in the temporal database
       and optional in a space time dataset.

       :param type: The type of the map layer (raster, raster_3d, vector)
       :param map_list: List of AbstractMapDataset objects
       :param output_stds: The output stds
       :param delete_empty: Set True to delete empty map layer found in the map_list
       :param unit: The temporal unit of the space time dataset
       :param dbif: The database interface to be used

    """
    import grass.pygrass.modules as pymod
    import copy

    dbif,  connected = init_dbif(dbif)

    filename = gscript.tempfile(True)
    file = open(filename, 'w')

    empty_maps = []
    for map_layer in map_list:
        # Read the map data
        map_layer.load()
        # In case of a empty map continue, do not register empty maps

        if delete_empty:
            if type in ["raster", "raster_3d", "rast", "rast3d"]:
                if map_layer.metadata.get_min() is None and \
                   map_layer.metadata.get_max() is None:
                    empty_maps.append(map_layer)
                    continue
            if type == "vector":
                if map_layer.metadata.get_number_of_primitives() == 0:
                    empty_maps.append(map_layer)
                    continue

        start,  end = map_layer.get_temporal_extent_as_tuple()
        id = map_layer.get_id()
        if not end:
            end = start
        string = "%s|%s|%s\n" % (id,  str(start),  str(end))
        file.write(string)
    file.close()

    if output_stds:
        output_stds_id = output_stds.get_id()
    else:
        output_stds_id = None

    register_maps_in_space_time_dataset(type, output_stds_id, unit=unit,
                                        file=filename, dbif=dbif)

    g_remove = pymod.Module("g.remove", flags='f', quiet=True,
                            run_=False, finish_=True)

    # Remove empty maps and unregister them from the temporal database
    if len(empty_maps) > 0:
        for map in empty_maps:
            mod = copy.deepcopy(g_remove)
            if map.get_name():
                if map.get_type() == "raster":
                    mod(type='raster', name=map.get_name())
                if map.get_type() == "raster3d":
                    mod(type='raster_3d', name=map.get_name())
                if map.get_type() == "vector":
                    mod(type='vector', name=map.get_name())
                mod.run()
            if map.is_in_db(dbif):
                map.delete(dbif)

    if connected:
        dbif.close()

if __name__ == "__main__":
    import doctest
    doctest.testmod()
