#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.accdetect
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Detect accumulation pattern in temporally accumulated space time raster datasets created by t.rast.accumulate.
# COPYRIGHT:    (C) 2013-2017  by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

#%module
#% description: Detects accumulation patterns in temporally accumulated space time raster datasets created by t.rast.accumulate.
#% keyword: temporal
#% keyword: accumulation
#% keyword: raster
#% keyword: time
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_STRDS_INPUT
#% key: minimum
#% description: Input space time raster dataset that specifies the minimum values to detect the accumulation pattern
#% required: no
#%end

#%option G_OPT_STRDS_INPUT
#% key: maximum
#% description: Input space time raster dataset that specifies the maximum values to detect the accumulation pattern
#% required: no
#%end

#%option G_OPT_STRDS_OUTPUT
#% key: occurrence
#% description: The output space time raster dataset that stores the occurrence of the the accumulation pattern using the provided data range
#% required: yes
#%end

#%option G_OPT_STRDS_OUTPUT
#% key: indicator
#% description: The output space time raster dataset that stores the indication of the start, intermediate and end of the specified data range
#% required: no
#%end

#%option
#% key: start
#% type: string
#% description: The temporal starting point to begin the accumulation, eg '2001-01-01'
#% required: yes
#% multiple: no
#%end

#%option
#% key: stop
#% type: string
#% description: The temporal date to stop the accumulation, eg '2009-01-01'
#% required: no
#% multiple: no
#%end

#%option
#% key: cycle
#% type: string
#% description: The temporal cycle to restart the accumulation, eg '12 months'
#% required: yes
#% multiple: no
#%end

#%option
#% key: offset
#% type: string
#% description: The temporal offset to the begin of the next cycle, eg '6 months'
#% required: no
#% multiple: no
#%end

#%option
#% key: basename
#% type: string
#% label: Basename of the new generated output maps
#% description: A numerical suffix separated by an underscore will be attached to create a unique identifier
#% required: yes
#% multiple: no
#% gisprompt:
#%end

#%option
#% key: suffix
#% type: string
#% description: Suffix to add at basename: set 'gran' for granularity, 'time' for the full time format, 'count' for numerical suffix with a specific number of digits (default %05)
#% answer: gran
#% required: no
#% multiple: no
#%end

#%option
#% key: range
#% type: double
#% key_desc: min,max
#% description: The minimum and maximum value of the occurrence of accumulated values, these values will be used if the min/max space time raster datasets are not specified
#% required: no
#% multiple: no
#%end

#%option
#% key: staend
#% type: integer
#% key_desc: start,intermediate,end
#% description: The user defined values that indicate start, intermediate and end status in the indicator output space time raster dataset
#% answer: 1,2,3
#% required: no
#% multiple: no
#%end

#%flag
#% key: n
#% description: Register empty maps in the output space time raster dataset, otherwise they will be deleted
#%end

#%flag
#% key: r
#% description: Reverse time direction in cyclic accumulation
#%end

import grass.script as grass


# lazy imports at the end of the file


############################################################################

range_relations = ["EQUALS", "DURING", "OVERLAPS", "OVERLAPPING", "CONTAINS"]

def main():
    # Get the options
    input = options["input"]
    start = options["start"]
    stop = options["stop"]
    base = options["basename"]
    cycle = options["cycle"]
    offset = options["offset"]
    minimum = options["minimum"]
    maximum = options["maximum"]
    occurrence = options["occurrence"]
    range_ = options["range"]
    indicator = options["indicator"]
    staend = options["staend"]
    register_null = flags["n"]
    reverse = flags["r"]
    time_suffix = options["suffix"]

    grass.set_raise_on_error(True)

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = tgis.get_current_mapset()

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    input_strds = tgis.SpaceTimeRasterDataset(id)

    if input_strds.is_in_db() == False:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> not found") % (
            input_strds.get_output_map_instance(None).get_type(), id))

    input_strds.select(dbif)
    dummy = input_strds.get_new_map_instance(None)

    # The occurrence space time raster dataset
    if occurrence:
        if not minimum or not maximum:
            if not range_:
                dbif.close()
                grass.fatal(_("You need to set the range to compute the occurrence"
                              " space time raster dataset"))

        if occurrence.find("@") >= 0:
            occurrence_id = occurrence
        else:
            occurrence_id = occurrence + "@" + mapset

        occurrence_strds = tgis.SpaceTimeRasterDataset(occurrence_id)
        if occurrence_strds.is_in_db(dbif):
            if not grass.overwrite():
                dbif.close()
                grass.fatal(_("Space time raster dataset <%s> is already in the "
                              "database, use overwrite flag to overwrite") % occurrence_id)

    # The indicator space time raster dataset
    if indicator:
        if not occurrence:
            dbif.close()
            grass.fatal(_("You need to set the occurrence to compute the indicator"
                          " space time raster dataset"))
        if not staend:
            dbif.close()
            grass.fatal(_("You need to set the staend options to compute the indicator"
                          " space time raster dataset"))
        if indicator.find("@") >= 0:
            indicator = indicator
        else:
            indicator_id = indicator + "@" + mapset

        indicator_strds = tgis.SpaceTimeRasterDataset(indicator_id)
        if indicator_strds.is_in_db(dbif):
            if not grass.overwrite():
                dbif.close()
                grass.fatal(_("Space time raster dataset <%s> is already in the "
                              "database, use overwrite flag to overwrite") % indicator_id)
        staend = staend.split(",")
        indicator_start = int(staend[0])
        indicator_mid = int(staend[1])
        indicator_end = int(staend[2])

    # The minimum threshold space time raster dataset
    minimum_strds = None
    if minimum:
        if minimum.find("@") >= 0:
            minimum_id = minimum
        else:
            minimum_id = minimum + "@" + mapset

        minimum_strds = tgis.SpaceTimeRasterDataset(minimum_id)
        if minimum_strds.is_in_db() == False:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> not found") % (minimum_strds.get_id()))

        if minimum_strds.get_temporal_type() != input_strds.get_temporal_type():
            dbif.close()
            grass.fatal(_("Temporal type of input strds and minimum strds must be equal"))

        minimum_strds.select(dbif)

    # The maximum threshold space time raster dataset
    maximum_strds = None
    if maximum:
        if maximum.find("@") >= 0:
            maximum_id = maximum
        else:
            maximum_id = maximum + "@" + mapset

        maximum_strds = tgis.SpaceTimeRasterDataset(maximum_id)
        if maximum_strds.is_in_db() == False:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> not found") % (maximum_strds.get_id()))

        if maximum_strds.get_temporal_type() != input_strds.get_temporal_type():
            dbif.close()
            grass.fatal(_("Temporal type of input strds and maximum strds must be equal"))

        maximum_strds.select(dbif)

    input_strds_start, input_strds_end = input_strds.get_temporal_extent_as_tuple()

    if input_strds.is_time_absolute():
        start = tgis.string_to_datetime(start)
        if stop:
            stop = tgis.string_to_datetime(stop)
        else:
            stop = input_strds_end
    else:
        start = int(start)
        if stop:
            stop = int(stop)
        else:
            stop = input_strds_end

    if input_strds.is_time_absolute():
        end = tgis.increment_datetime_by_string(start, cycle)
    else:
        end = start + cycle

    count = 1
    indi_count = 1
    occurrence_maps = {}
    indicator_maps = {}

    while input_strds_end > start and stop > start:

        # Make sure that the cyclic computation will stop at the correct time
        if stop and end > stop:
            end = stop

        where = "start_time >= \'%s\' AND start_time < \'%s\'"%(str(start),
                                                                str(end))
        input_maps = input_strds.get_registered_maps_as_objects(where=where,
                                                                dbif=dbif)

        grass.debug(len(input_maps))

        input_topo = tgis.SpatioTemporalTopologyBuilder()
        input_topo.build(input_maps, input_maps)

        if len(input_maps) == 0:
            continue

        grass.message(_("Processing cycle %s - %s"%(str(start), str(end))))

        count = compute_occurrence(occurrence_maps, input_strds, input_maps,
                                   start, base, count, time_suffix, mapset,
                                   where, reverse, range_, minimum_strds,
                                   maximum_strds, dbif)

        # Indicator computation is based on the occurrence so we need to start it after
        # the occurrence cycle
        if indicator:
            num_maps = len(input_maps)
            for i in range(num_maps):
                if reverse:
                    map = input_maps[num_maps - i - 1]
                else:
                    map = input_maps[i]

                if input_strds.get_temporal_type() == 'absolute' and time_suffix == 'gran':
                    suffix = tgis.create_suffix_from_datetime(map.temporal_extent.get_start_time(),
                                                              input_strds.get_granularity())
                    indicator_map_name = "{ba}_indicator_{su}".format(ba=base, su=suffix)
                elif input_strds.get_temporal_type() == 'absolute' and time_suffix == 'time':
                    suffix = tgis.create_time_suffix(map)
                    indicator_map_name = "{ba}_indicator_{su}".format(ba=base, su=suffix)
                else:
                    indicator_map_name = tgis.create_numeric_suffix(base + "_indicator",
                                                                    indi_count, time_suffix)
                indicator_map_id = dummy.build_id(indicator_map_name, mapset)
                indicator_map = input_strds.get_new_map_instance(indicator_map_id)

                # Check if new map is in the temporal database
                if indicator_map.is_in_db(dbif):
                    if grass.overwrite():
                        # Remove the existing temporal database entry
                        indicator_map.delete(dbif)
                        indicator_map = input_strds.get_new_map_instance(indicator_map_id)
                    else:
                        grass.fatal(_("Map <%s> is already registered in the temporal"
                                     " database, use overwrite flag to overwrite.") %
                                    (indicator_map.get_map_id()))

                curr_map = occurrence_maps[map.get_id()].get_name()

                # Reverse time
                if reverse:
                    if i ==  0:
                        prev_map = curr_map
                        subexpr1 = "null()"
                        subexpr3 = "%i"%(indicator_start)
                    elif i > 0 and i < num_maps - 1:
                        prev_map = occurrence_maps[map.next().get_id()].get_name()
                        next_map = occurrence_maps[map.prev().get_id()].get_name()
                        # In case the previous map is null() set null() or the start indicator
                        subexpr1 = "if(isnull(%s), null(), %i)"%(curr_map, indicator_start)
                        # In case the previous map was not null() if the current map is null() set null()
                        # if the current map is not null() and the next map is not null() set
                        # intermediate indicator, if the next map is null set the end indicator
                        subexpr2 = "if(isnull(%s), %i, %i)"%(next_map, indicator_end, indicator_mid)
                        subexpr3 = "if(isnull(%s), null(), %s)"%(curr_map, subexpr2)
                        expression = "%s = if(isnull(%s), %s, %s)"%(indicator_map_name,
                                                                    prev_map, subexpr1,
                                                                    subexpr3)
                    else:
                        prev_map = occurrence_maps[map.next().get_id()].get_name()
                        subexpr1 = "if(isnull(%s), null(), %i)"%(curr_map, indicator_start)
                        subexpr3 = "if(isnull(%s), null(), %i)"%(curr_map, indicator_mid)
                else:
                    if i == 0:
                        prev_map = curr_map
                        subexpr1 = "null()"
                        subexpr3 = "%i"%(indicator_start)
                    elif i > 0 and i < num_maps - 1:
                        prev_map = occurrence_maps[map.prev().get_id()].get_name()
                        next_map = occurrence_maps[map.next().get_id()].get_name()
                        # In case the previous map is null() set null() or the start indicator
                        subexpr1 = "if(isnull(%s), null(), %i)"%(curr_map, indicator_start)
                        # In case the previous map was not null() if the current map is null() set null()
                        # if the current map is not null() and the next map is not null() set
                        # intermediate indicator, if the next map is null set the end indicator
                        subexpr2 = "if(isnull(%s), %i, %i)"%(next_map, indicator_end, indicator_mid)
                        subexpr3 = "if(isnull(%s), null(), %s)"%(curr_map, subexpr2)
                        expression = "%s = if(isnull(%s), %s, %s)"%(indicator_map_name,
                                                                    prev_map, subexpr1,
                                                                    subexpr3)
                    else:
                        prev_map = occurrence_maps[map.prev().get_id()].get_name()
                        subexpr1 = "if(isnull(%s), null(), %i)"%(curr_map, indicator_start)
                        subexpr3 = "if(isnull(%s), null(), %i)"%(curr_map, indicator_mid)

                expression = "%s = if(isnull(%s), %s, %s)"%(indicator_map_name,
                                                            prev_map, subexpr1,
                                                            subexpr3)
                grass.debug(expression)
                grass.mapcalc(expression, overwrite=True)

                map_start, map_end = map.get_temporal_extent_as_tuple()

                if map.is_time_absolute():
                    indicator_map.set_absolute_time(map_start, map_end)
                else:
                    indicator_map.set_relative_time(map_start, map_end,
                                                 map.get_relative_time_unit())

                indicator_maps[map.get_id()] = indicator_map
                indi_count += 1

        # Increment the cycle
        start = end
        if input_strds.is_time_absolute():
            start = end
            if offset:
                start = tgis.increment_datetime_by_string(end, offset)
            end = tgis.increment_datetime_by_string(start, cycle)
        else:
            if offset:
                start = end + offset
            end = start + cycle

    empty_maps = []

    create_strds_register_maps(input_strds, occurrence_strds, occurrence_maps,
                               register_null, empty_maps, dbif)

    if indicator:
        create_strds_register_maps(input_strds, indicator_strds, indicator_maps,
                                   register_null, empty_maps, dbif)

    dbif.close()

    # Remove empty maps
    if len(empty_maps) > 0:
        for map in empty_maps:
            grass.run_command("g.remove", flags='f', type="raster",  name=map.get_name(), quiet=True)

############################################################################

def create_strds_register_maps(in_strds, out_strds, out_maps, register_null,
                    empty_maps, dbif):

    out_id = out_strds.get_id()

    if out_strds.is_in_db(dbif):
        if grass.overwrite():
            out_strds.delete(dbif)
            out_strds = in_strds.get_new_instance(out_id)

    temporal_type, semantic_type, title, description = in_strds.get_initial_values()
    out_strds.set_initial_values(temporal_type, semantic_type, title,
                                    description)
    out_strds.insert(dbif)

    # Register the maps in the database
    count = 0
    for map in out_maps.values():
        count += 1
        if count%10 == 0:
            grass.percent(count, len(out_maps), 1)
        # Read the raster map data
        map.load()
        # In case of a empty map continue, do not register empty maps
        if not register_null:
            if map.metadata.get_min() is None and \
                map.metadata.get_max() is None:
                empty_maps.append(map)
                continue

        # Insert map in temporal database
        map.insert(dbif)
        out_strds.register_map(map, dbif)

    out_strds.update_from_registered_maps(dbif)
    grass.percent(1, 1, 1)

############################################################################

def compute_occurrence(occurrence_maps, input_strds, input_maps, start, base,
                       count, tsuffix, mapset, where, reverse, range_,
                       minimum_strds, maximum_strds, dbif):

    if minimum_strds:
        input_maps_minimum = input_strds.get_registered_maps_as_objects(where=where,
                                                                      dbif=dbif)
        minimum_maps = minimum_strds.get_registered_maps_as_objects(dbif=dbif)
        minimum_topo = tgis.SpatioTemporalTopologyBuilder()
        minimum_topo.build(input_maps_minimum, minimum_maps)

    if maximum_strds:
        input_maps_maximum = input_strds.get_registered_maps_as_objects(where=where,
                                                                      dbif=dbif)
        maximum_maps = maximum_strds.get_registered_maps_as_objects(dbif=dbif)
        maximum_topo = tgis.SpatioTemporalTopologyBuilder()
        maximum_topo.build(input_maps_maximum, maximum_maps)

    # Aggregate
    num_maps = len(input_maps)
    for i in range(num_maps):
        if reverse:
            map = input_maps[num_maps - i - 1]
        else:
            map = input_maps[i]

        # Compute the days since start
        input_start, input_end = map.get_temporal_extent_as_tuple()

        td = input_start - start
        if map.is_time_absolute():
            days = tgis.time_delta_to_relative_time(td)
        else:
            days = td

        if input_strds.get_temporal_type() == 'absolute' and tsuffix == 'gran':
            suffix = tgis.create_suffix_from_datetime(map.temporal_extent.get_start_time(),
                                                      input_strds.get_granularity())
            occurrence_map_name = "{ba}_{su}".format(ba=base, su=suffix)
        elif input_strds.get_temporal_type() == 'absolute' and tsuffix == 'time':
            suffix = tgis.create_time_suffix(map)
            occurrence_map_name = "{ba}_{su}".format(ba=base, su=suffix)
        else:
            occurrence_map_name = tgis.create_numeric_suffix(base, count, tsuffix)

        occurrence_map_id = map.build_id(occurrence_map_name, mapset)
        occurrence_map = input_strds.get_new_map_instance(occurrence_map_id)

        # Check if new map is in the temporal database
        if occurrence_map.is_in_db(dbif):
            if grass.overwrite():
                # Remove the existing temporal database entry
                occurrence_map.delete(dbif)
                occurrence_map = input_strds.get_new_map_instance(occurrence_map_id)
            else:
                grass.fatal(_("Map <%s> is already registered in the temporal"
                             " database, use overwrite flag to overwrite.") %
                            (occurrence_map.get_map_id()))

        range_vals = range_.split(",")
        min = range_vals[0]
        max = range_vals[1]

        if minimum_strds:
            relations = input_maps_minimum[i].get_temporal_relations()
            for relation in range_relations:
                if relation in relations:
                    min = str(relations[relation][0].get_id())
                    break

        if maximum_strds:
            relations = input_maps_maximum[i].get_temporal_relations()
            for relation in range_relations:
                if relation in relations:
                    max = str(relations[relation][0].get_id())
                    break

        expression = "%s = if(%s > %s && %s < %s, %s, null())"%(occurrence_map_name,
                                                                map.get_name(),
                                                                min, map.get_name(),
                                                                max, days)
        grass.debug(expression)
        grass.mapcalc(expression, overwrite=True)

        map_start, map_end = map.get_temporal_extent_as_tuple()

        if map.is_time_absolute():
            occurrence_map.set_absolute_time(map_start, map_end)
        else:
            occurrence_map.set_relative_time(map_start, map_end,
                                         map.get_relative_time_unit())

        # Store the new maps
        occurrence_maps[map.get_id()] = occurrence_map

        count += 1

    return count

############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    # lazy imports
    import grass.temporal as tgis
    main()
