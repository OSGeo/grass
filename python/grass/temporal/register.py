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

import grass.script as gs

from .abstract_map_dataset import AbstractMapDataset
from .core import get_current_mapset, get_tgis_message_interface, init_dbif
from .datetime_math import (
    check_datetime_string,
    increment_datetime_by_string,
    string_to_datetime,
)
from .factory import dataset_factory
from .open_stds import open_old_stds

###############################################################################


def register_maps_in_space_time_dataset(
    type,
    name,
    maps=None,
    file=None,
    start=None,
    end=None,
    unit=None,
    increment=None,
    dbif=None,
    interval: bool = False,
    fs: str = "|",
    update_cmd_list: bool = True,
) -> None:
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
                end time, or the same as io object (with readline capability)
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
    semantic_label_in_file = False
    overwrite = gs.overwrite()

    msgr = get_tgis_message_interface()

    msgr.debug(1, "register_maps_in_space_time_dataset()")

    # Make sure the arguments are of type string
    if start != "" and start is not None:
        start = str(start)
    if end != "" and end is not None:
        end = str(end)
    if increment != "" and increment is not None:
        increment = str(increment)

    if maps and file:
        msgr.fatal(_("maps and file are mutually exclusive"))

    if end and increment:
        msgr.fatal(_("end and increment are mutually exclusive"))

    if end and interval:
        msgr.fatal(_("end and the interval flag are mutually exclusive"))

    if increment and not start:
        msgr.fatal(_("The increment option requires the start option"))

    if interval and not start:
        msgr.fatal(_("The interval flag requires the start option"))

    if end and not start:
        msgr.fatal(_("Please specify start_time and end_time"))

    if not maps and not file:
        msgr.fatal(_("Please specify maps or file"))

    # We may need the mapset
    mapset = get_current_mapset()
    dbif, connection_state_changed = init_dbif(dbif)

    # create new stds only in the current mapset
    # remove all connections to any other mapsets
    # ugly hack !
    currcon = {}
    currcon[mapset] = dbif.connections[mapset]
    dbif.connections = currcon

    # The name of the space time dataset is optional
    if name:
        sp = open_old_stds(name, type, dbif)

        if sp.is_time_relative() and (start or end) and not unit:
            dbif.close()
            msgr.fatal(
                _(
                    "Space time {sp} dataset <{name}> with relative"
                    " time found, but no relative unit set for {sp} "
                    "maps"
                ).format(name=name, sp=sp.get_new_map_instance(None).get_type())
            )

    maplist = []

    # Map names as comma separated string
    if maps:
        maplist = maps.split(",")

        # Build the map list again with the ids
        for idx, maplist_item in enumerate(maplist):
            maplist[idx] = {
                "id": AbstractMapDataset.build_id_from_search_path(maplist_item, type)
            }

    # Read the map list from file
    if file:
        fd = file if hasattr(file, "readline") else open(file)

        line = True
        while True:
            line = fd.readline().strip()
            if not line:
                break

            line_list = line.split(fs)

            # Detect start and end time (and semantic label)
            if len(line_list) == 2:
                start_time_in_file = True
                end_time_in_file = False
                semantic_label_in_file = False
            elif len(line_list) == 3:
                start_time_in_file = True
                # Check if last column is an end time or a semantic label
                time_object = check_datetime_string(line_list[2])
                if not sp.is_time_relative() and isinstance(time_object, datetime):
                    end_time_in_file = True
                    semantic_label_in_file = False
                else:
                    end_time_in_file = False
                    semantic_label_in_file = True
            elif len(line_list) == 4:
                start_time_in_file = True
                end_time_in_file = True
                semantic_label_in_file = True
            else:
                start_time_in_file = False
                end_time_in_file = False
                semantic_label_in_file = False

            mapname = line_list[0].strip()
            row = {}

            if start_time_in_file:
                row["start"] = line_list[1].strip()

            if end_time_in_file:
                row["end"] = line_list[2].strip()

            if semantic_label_in_file:
                idx = 3 if end_time_in_file else 2
                # case-sensitive, the user decides on the band name
                row["semantic_label"] = line_list[idx].strip()

            row["id"] = AbstractMapDataset.build_id_from_search_path(mapname, type)

            maplist.append(row)

        if start_time_in_file is True and increment:
            increment = None
            msgr.warning(
                _(
                    "The increment option will be ignored because of time stamps in "
                    "input file"
                )
            )

        if start_time_in_file is True and interval:
            increment = None
            msgr.warning(
                _(
                    "The interval flag will be ignored because of time stamps in input "
                    "file"
                )
            )
        fd.close()

    num_maps = len(maplist)
    map_object_list = []
    statement = ""
    # Store the ids of datasets that must be updated
    datatsets_to_modify = {}

    msgr.debug(2, "Gathering map information...")

    for count, row in enumerate(maplist):
        if count % 50 == 0:
            msgr.percent(count, num_maps, 1)

        # Get a new instance of the map type
        map_object = dataset_factory(type, row["id"])

        map_object_id = map_object.get_map_id()
        map_object_layer = map_object.get_layer()
        map_object_type = map_object.get_type()
        if not map_object.map_exists():
            msgr.fatal(
                _("Unable to update {t} map <{mid}>. The map does not exist.").format(
                    t=map_object_type, mid=map_object_id
                )
            )

        # Use the time data from file
        if "start" in row:
            start = row["start"]
        if "end" in row:
            end = row["end"]

        # Use the semantic label from file
        semantic_label = row.get("semantic_label", None)

        is_in_db = map_object.is_in_db(dbif, mapset)

        # Put the map into the database of the current mapset
        if not is_in_db:
            # Break in case no valid time is provided
            if (start == "" or start is None) and not map_object.has_grass_timestamp():
                dbif.close()
                if map_object_layer:
                    msgr.fatal(
                        _(
                            "Unable to register {t} map <{mid}> with "
                            "layer {l}. The map has timestamp and "
                            "the start time is not set."
                        ).format(
                            t=map_object_type,
                            mid=map_object_id,
                            l=map_object_layer,
                        )
                    )
                else:
                    msgr.fatal(
                        _(
                            "Unable to register {t} map <{mid}>. The"
                            " map has no timestamp and the start time "
                            "is not set."
                        ).format(t=map_object_type, mid=map_object_id)
                    )
            if start != "" and start is not None:
                # We need to check if the time is absolute and the unit was specified
                time_object = check_datetime_string(start)
                if isinstance(time_object, datetime) and unit:
                    msgr.fatal(_("unit can only be set for relative time"))
                if not isinstance(time_object, datetime) and not unit:
                    msgr.fatal(_("unit must be set in case of relative time stamps"))

                if unit:
                    map_object.set_time_to_relative()
                else:
                    map_object.set_time_to_absolute()

        else:
            # Check the overwrite flag
            if not overwrite:
                if map_object_layer:
                    msgr.warning(
                        _(
                            "Map is already registered in temporal "
                            "database. Unable to update {t} map "
                            "<{mid}> with layer {l}. Overwrite flag"
                            " is not set."
                        ).format(
                            t=map_object_type,
                            mid=map_object_id,
                            l=str(map_object_layer),
                        )
                    )
                else:
                    msgr.warning(
                        _(
                            "Map is already registered in temporal "
                            "database. Unable to update {t} map "
                            "<{mid}>. Overwrite flag is not set."
                        ).format(t=map_object_type, mid=map_object_id)
                    )

                # Simple registration is allowed
                if name:
                    map_object_list.append(map_object)
                # Jump to next map
                continue

            # Reload properties from database
            map_object.select(dbif)

            # Save the datasets that must be updated
            datasets = map_object.get_registered_stds(dbif)
            if datasets is not None:
                for dataset in datasets:
                    if dataset != "":
                        datatsets_to_modify[dataset] = dataset

                if name and map_object.get_temporal_type() != sp.get_temporal_type():
                    dbif.close()
                    if map_object_layer:
                        msgr.fatal(
                            _(
                                "Unable to update {t} map <{id}> "
                                "with layer {l}. The temporal types "
                                "are different."
                            ).format(
                                t=map_object_type,
                                mid=map_object_id,
                                l=map_object_layer,
                            )
                        )
                    else:
                        msgr.fatal(
                            _(
                                "Unable to update {t} map <{mid}>. "
                                "The temporal types are different."
                            ).format(t=map_object_type, mid=map_object_id)
                        )

        # Load the data from the grass file database
        map_object.load()

        # Try to read an existing time stamp from the grass spatial database
        # in case this map wasn't already registered in the temporal database
        # Read the spatial database time stamp only, if no time stamp was provided for
        # this map
        # as method argument or in the input file
        if not is_in_db and not start:
            map_object.read_timestamp_from_grass()

        # Set the valid time
        if start:
            # In case the time is in the input file we ignore the increment
            # counter
            if start_time_in_file:
                count = 1
            assign_valid_time_to_map(
                ttype=map_object.get_temporal_type(),
                map_object=map_object,
                start=start,
                end=end,
                unit=unit,
                increment=increment,
                mult=count,
                interval=interval,
            )

        # Set the semantic label (only raster type supported)
        if semantic_label:
            # semantic label defined in input file
            # -> update raster metadata
            # -> write band identifier to GRASS data base
            map_object.set_semantic_label(semantic_label)
        else:
            # Try to read semantic label from GRASS data base if defined
            map_object.read_semantic_label_from_grass()

        if is_in_db:
            #  Gather the SQL update statement
            statement += map_object.update_all(dbif=dbif, execute=False)
        else:
            #  Gather the SQL insert statement
            statement += map_object.insert(dbif=dbif, execute=False)

        # Sqlite3 performance is better for huge datasets when committing in
        # small chunks
        if dbif.get_dbmi().__name__ == "sqlite3":
            if count % 100 == 0:
                if statement is not None and statement != "":
                    dbif.execute_transaction(statement)
                    statement = ""

        # Store the maps in a list to register in a space time dataset
        if name:
            map_object_list.append(map_object)

    msgr.percent(num_maps, num_maps, 1)

    if statement is not None and statement != "":
        dbif.execute_transaction(statement)

    # Finally Register the maps in the space time dataset
    if name and map_object_list:
        num_maps = len(map_object_list)
        for count, map_object in enumerate(map_object_list):
            if count % 50 == 0:
                msgr.percent(count, num_maps, 1)
            sp.register_map(map=map_object, dbif=dbif)

    # Update the space time tables
    if name and map_object_list:
        sp.update_from_registered_maps(dbif)
        if update_cmd_list is True:
            sp.update_command_string(dbif=dbif)

    # Update affected datasets
    if datatsets_to_modify:
        for dataset in datatsets_to_modify:
            if type in {"rast", "raster"}:
                ds = dataset_factory("strds", dataset)
            elif type in {"raster_3d", "rast3d", "raster3d"}:
                ds = dataset_factory("str3ds", dataset)
            elif type in {"vect", "vector"}:
                ds = dataset_factory("stvds", dataset)
            ds.select(dbif)
            ds.update_from_registered_maps(dbif)

    if connection_state_changed is True:
        dbif.close()

    msgr.percent(num_maps, num_maps, 1)


###############################################################################


def assign_valid_time_to_map(
    ttype, map_object, start, end, unit, increment=None, mult=1, interval: bool = False
) -> None:
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
            msgr.fatal(
                _('Unable to convert string "{}" into a datetime object').format(start)
            )
        end_time = None

        if end:
            end_time = string_to_datetime(end)
            if end_time is None:
                msgr.fatal(
                    _('Unable to convert string "{}" into a datetime object').format(
                        end
                    )
                )

        # Add the increment
        if increment:
            start_time = increment_datetime_by_string(start_time, increment, mult)
            if start_time is None:
                msgr.fatal(_("Error occurred in increment computation"))
            if interval:
                end_time = increment_datetime_by_string(start_time, increment, 1)
                if end_time is None:
                    msgr.fatal(_("Error occurred in increment computation"))

        if map_object.get_layer():
            msgr.debug(
                1,
                _(
                    "Set absolute valid time for map <{id}> with "
                    "layer {layer} to {start} - {end}"
                ).format(
                    id=map_object.get_map_id(),
                    layer=map_object.get_layer(),
                    start=str(start_time),
                    end=str(end_time),
                ),
            )
        else:
            msgr.debug(
                1,
                _(
                    "Set absolute valid time for map <{mid}> to "
                    "{start_time} - {end_time}"
                ).format(
                    mid=map_object.get_map_id(),
                    start_time=str(start_time),
                    end_time=str(end_time),
                ),
            )

        map_object.set_absolute_time(start_time, end_time)
    else:
        start_time = int(start)
        end_time = None

        if end:
            end_time = int(end)

        if increment:
            start_time += mult * int(increment)
            if interval:
                end_time = start_time + int(increment)

        if map_object.get_layer():
            msgr.debug(
                1,
                _(
                    "Set relative valid time for map <{mid}> with layer"
                    " {layer} to {start} - {end} with unit {unit}"
                ).format(
                    mid=map_object.get_map_id(),
                    layer=map_object.get_layer(),
                    start=start_time,
                    end=str(end_time),
                    unit=unit,
                ),
            )
        else:
            msgr.debug(
                1,
                _(
                    "Set relative valid time for map <{mid}> to "
                    "{start} - {end} with unit {unit}"
                ).format(
                    mid=map_object.get_map_id(),
                    start=start_time,
                    end=str(end_time),
                    unit=unit,
                ),
            )

        map_object.set_relative_time(start_time, end_time, unit)


##############################################################################


def register_map_object_list(
    type, map_list, output_stds, delete_empty: bool = False, unit=None, dbif=None
) -> None:
    """Register a list of AbstractMapDataset objects in the temporal database
    and optional in a space time dataset.

    :param type: The type of the map layer (raster, raster_3d, vector)
    :param map_list: List of AbstractMapDataset objects
    :param output_stds: The output stds
    :param delete_empty: Set True to delete empty map layer found in the map_list
    :param unit: The temporal unit of the space time dataset
    :param dbif: The database interface to be used

    """
    import copy

    import grass.pygrass.modules as pymod

    dbif, connection_state_changed = init_dbif(None)

    filename = gs.tempfile(True)
    with open(filename, "w") as register_file:
        empty_maps = []
        for map_layer in map_list:
            # Read the map data
            map_layer.load()
            # In case of a empty map continue, do not register empty maps
            if delete_empty:
                if type in {"raster", "raster_3d", "rast", "rast3d"}:
                    if (
                        map_layer.metadata.get_min() is None
                        and map_layer.metadata.get_max() is None
                    ):
                        empty_maps.append(map_layer)
                        continue
                if type == "vector":
                    if map_layer.metadata.get_number_of_primitives() == 0:
                        empty_maps.append(map_layer)
                        continue

            start, end = map_layer.get_temporal_extent_as_tuple()
            id = map_layer.get_id()
            if not end:
                end = start
            string = f"{id}|{start}|{end}\n"
            register_file.write(string)

    output_stds_id = output_stds.get_id() if output_stds else None

    register_maps_in_space_time_dataset(
        type, output_stds_id, unit=unit, file=filename, dbif=dbif
    )

    # Remove empty maps and unregister them from the temporal database
    g_remove = pymod.Module("g.remove", flags="f", quiet=True, run_=False, finish_=True)
    if len(empty_maps) > 0:
        for map_object in empty_maps:
            mod = copy.deepcopy(g_remove)
            if map_object.get_name():
                if map_object.get_type() == "raster":
                    mod(type="raster", name=map_object.get_name())
                if map_object.get_type() == "raster3d":
                    mod(type="raster_3d", name=map_object.get_name())
                if map_object.get_type() == "vector":
                    mod(type="vector", name=map_object.get_name())
                mod.run()
            if map_object.is_in_db(dbif):
                map_object.delete(dbif)

    if connection_state_changed:
        dbif.close()


if __name__ == "__main__":
    import doctest

    doctest.testmod()
