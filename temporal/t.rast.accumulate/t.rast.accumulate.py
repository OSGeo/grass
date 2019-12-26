#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.accumulate
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Compute cyclic accumulations of a space time raster dataset
# COPYRIGHT:    (C) 2013-2017 by the GRASS Development Team
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
#% description: Computes cyclic accumulations of a space time raster dataset.
#% keyword: temporal
#% keyword: accumulation
#% keyword: raster
#% keyword: time
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_STRDS_OUTPUT
#%end


#%option G_OPT_STRDS_INPUT
#% key: lower
#% description: Input space time raster dataset that defines the lower threshold, values lower than this threshold are excluded from accumulation
#% required: no
#%end

#%option G_OPT_STRDS_INPUT
#% key: upper
#% description: Input space time raster dataset that defines the upper threshold, values higher than this threshold are excluded from accumulation
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
#% description: The temporal offset to the beginning of the next cycle, eg '6 months'
#% required: no
#% multiple: no
#%end

#%option
#% key: granularity
#% type: string
#% description: The granularity for accumulation '1 day'
#% answer: 1 day
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
#% description: Suffix to add to the basename. Set 'gran' for granularity, 'time' for the full time format, 'num' for numerical suffix with a specific number of digits (default %05)
#% answer: gran
#% required: no
#% multiple: no
#%end

#%option
#% key: limits
#% type: double
#% key_desc: lower,upper
#% description: Use these limits in case lower and/or upper input space time raster datasets are not defined or contain NULL values
#% required: yes
#% multiple: no
#%end

#%option
#% key: scale
#% type: double
#% description: Scale factor for input space time raster dataset
#% required: no
#% multiple: no
#%end

#%option
#% key: shift
#% type: double
#% description: Shift factor for input space time raster dataset
#% required: no
#% multiple: no
#%end

#%option
#% key: method
#% type: string
#% label: This method will be applied to compute the accumulative values from the input maps in a single granule
#% description: Growing Degree Days or Winkler indices; Mean: sum(input maps)/(number of input maps); Biologically Effective Degree Days; Huglin Heliothermal index
#% options: mean,gdd,bedd,huglin
#% answer: mean
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
from __future__ import print_function

import grass.script as grass
from copy import copy

############################################################################

def main():
    # lazy imports
    import grass.temporal as tgis
    from grass.pygrass.modules import Module

    # Get the options
    input = options["input"]
    output = options["output"]
    start = options["start"]
    stop = options["stop"]
    base = options["basename"]
    cycle = options["cycle"]
    lower = options["lower"]
    upper = options["upper"]
    offset = options["offset"]
    limits = options["limits"]
    shift = options["shift"]
    scale = options["scale"]
    method = options["method"]
    granularity = options["granularity"]
    register_null = flags["n"]
    reverse = flags["r"]
    time_suffix = options["suffix"]

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
        grass.fatal(_("Space time raster dataset <%s> not found") % (id))

    input_strds.select(dbif)

    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset

    # The output space time raster dataset
    output_strds = tgis.SpaceTimeRasterDataset(out_id)
    if output_strds.is_in_db(dbif):
        if not grass.overwrite():
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is already in the "
                          "database, use overwrite flag to overwrite") % out_id)

    if tgis.check_granularity_string(granularity,
                                     input_strds.get_temporal_type()) == False:
            dbif.close()
            grass.fatal(_("Invalid granularity"))

    if tgis.check_granularity_string(cycle,
                                     input_strds.get_temporal_type()) == False:
            dbif.close()
            grass.fatal(_("Invalid cycle"))

    if offset:
        if tgis.check_granularity_string(offset,
                                         input_strds.get_temporal_type()) == False:
                dbif.close()
                grass.fatal(_("Invalid offset"))

    # The lower threshold space time raster dataset
    if lower:
        if not range:
            dbif.close()
            grass.fatal(_("You need to set the range to compute the occurrence"
                          " space time raster dataset"))

        if lower.find("@") >= 0:
            lower_id = lower
        else:
            lower_id = lower + "@" + mapset

        lower_strds = tgis.SpaceTimeRasterDataset(lower_id)
        if lower_strds.is_in_db() == False:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> not found") % (lower_strds.get_id()))

        if lower_strds.get_temporal_type() != input_strds.get_temporal_type():
            dbif.close()
            grass.fatal(_("Temporal type of input strds and lower strds must be equal"))

        lower_strds.select(dbif)

    # The upper threshold space time raster dataset
    if upper:
        if not lower:
            dbif.close()
            grass.fatal(_("The upper option works only in conjunction with the lower option"))

        if upper.find("@") >= 0:
            upper = upper
        else:
            upper_id = upper + "@" + mapset

        upper_strds = tgis.SpaceTimeRasterDataset(upper_id)
        if upper_strds.is_in_db() == False:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> not found") % (upper_strds.get_id()))

        if upper_strds.get_temporal_type() != input_strds.get_temporal_type():
            dbif.close()
            grass.fatal(_("Temporal type of input strds and upper strds must be equal"))

        upper_strds.select(dbif)

    input_strds_start, input_strds_end = input_strds.get_temporal_extent_as_tuple()

    if input_strds.is_time_absolute():
        start = tgis.string_to_datetime(start)
        if stop:
            stop = tgis.string_to_datetime(stop)
        else:
            stop = input_strds_end
        start = tgis.adjust_datetime_to_granularity(start, granularity)
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

    limit_relations = ["EQUALS", "DURING", "OVERLAPS", "OVERLAPPING", "CONTAINS"]

    count = 1
    output_maps = []


    while input_strds_end > start and stop > start:

        # Make sure that the cyclic computation will stop at the correct time
        if stop and end > stop:
            end = stop

        where = "start_time >= \'%s\' AND start_time < \'%s\'"%(str(start),
                                                                str(end))
        input_maps = input_strds.get_registered_maps_as_objects(where=where,
                                                                dbif=dbif)

        grass.message(_("Processing cycle %s - %s"%(str(start), str(end))))

        if len(input_maps) == 0:
            continue

        # Lets create a dummy list of maps with granularity conform intervals
        gran_list = []
        gran_list_low = []
        gran_list_up = []
        gran_start = start
        while gran_start < end:
            map = input_strds.get_new_map_instance("%i@%i"%(count, count))
            if input_strds.is_time_absolute():
                gran_end = tgis.increment_datetime_by_string(gran_start,
                                                             granularity)
                map.set_absolute_time(gran_start, gran_end)
                gran_start = tgis.increment_datetime_by_string(gran_start,
                                                               granularity)
            else:
                gran_end = gran_start + granularity
                map.set_relative_time(gran_start, gran_end,
                                      input_strds.get_relative_time_unit())
                gran_start = gran_start + granularity
            gran_list.append(copy(map))
            gran_list_low.append(copy(map))
            gran_list_up.append(copy(map))
        # Lists to compute the topology with upper and lower datasets

        # Create the topology between the granularity conform list and all maps
        # of the current cycle
        gran_topo = tgis.SpatioTemporalTopologyBuilder()
        gran_topo.build(gran_list, input_maps)

        if lower:
            lower_maps = lower_strds.get_registered_maps_as_objects(dbif=dbif)
            gran_lower_topo = tgis.SpatioTemporalTopologyBuilder()
            gran_lower_topo.build(gran_list_low, lower_maps)

        if upper:
            upper_maps = upper_strds.get_registered_maps_as_objects(dbif=dbif)
            gran_upper_topo = tgis.SpatioTemporalTopologyBuilder()
            gran_upper_topo.build(gran_list_up, upper_maps)

        old_map_name = None

        # Aggregate
        num_maps = len(gran_list)

        for i in range(num_maps):
            if reverse:
                map = gran_list[num_maps - i - 1]
            else:
                map = gran_list[i]
            # Select input maps based on temporal topology relations
            input_maps = []
            if map.get_equal():
                input_maps += map.get_equal()
            elif map.get_contains():
                input_maps += map.get_contains()
            elif map.get_overlaps():
                input_maps += map.get_overlaps()
            elif map.get_overlapped():
                input_maps += map.get_overlapped()
            elif map.get_during():
                input_maps += map.get_during()

            # Check input maps
            if len(input_maps) == 0:
                continue

            # New output map
            if input_strds.get_temporal_type() == 'absolute' and time_suffix == 'gran':
                suffix = tgis.create_suffix_from_datetime(map.temporal_extent.get_start_time(),
                                                          input_strds.get_granularity())
                output_map_name = "{ba}_{su}".format(ba=base, su=suffix)
            elif input_strds.get_temporal_type() == 'absolute' and time_suffix == 'time':
                suffix = tgis.create_time_suffix(map)
                output_map_name = "{ba}_{su}".format(ba=base, su=suffix)
            else:
                output_map_name = tgis.create_numeric_suffix(base, count, time_suffix)

            output_map_id = map.build_id(output_map_name, mapset)
            output_map = input_strds.get_new_map_instance(output_map_id)

            # Check if new map is in the temporal database
            if output_map.is_in_db(dbif):
                if grass.overwrite():
                    # Remove the existing temporal database entry
                    output_map.delete(dbif)
                    output_map = input_strds.get_new_map_instance(output_map_id)
                else:
                    grass.fatal(_("Map <%s> is already registered in the temporal"
                                 " database, use overwrite flag to overwrite.") %
                                (output_map.get_map_id()))

            map_start, map_end = map.get_temporal_extent_as_tuple()

            if map.is_time_absolute():
                output_map.set_absolute_time(map_start, map_end)
            else:
                output_map.set_relative_time(map_start, map_end,
                                             map.get_relative_time_unit())

            limits_vals = limits.split(",")
            limits_lower = float(limits_vals[0])
            limits_upper = float(limits_vals[1])

            lower_map_name = None
            if lower:
                relations = gran_list_low[i].get_temporal_relations()
                for relation in limit_relations:
                    if relation in relations:
                        lower_map_name = str(relations[relation][0].get_id())
                        break

            upper_map_name = None
            if upper:
                relations = gran_list_up[i].get_temporal_relations()
                for relation in limit_relations:
                    if relation in relations:
                        upper_map_name = str(relations[relation][0].get_id())
                        break

            input_map_names = []
            for input_map in input_maps:
                input_map_names.append(input_map.get_id())

            # Set up the module
            accmod = Module("r.series.accumulate", input=input_map_names,
                            output=output_map_name, run_=False)

            if old_map_name:
                accmod.inputs["basemap"].value = old_map_name
            if lower_map_name:
                accmod.inputs["lower"].value = lower_map_name
            if upper_map_name:
                accmod.inputs["upper"].value = upper_map_name

            accmod.inputs["limits"].value = (limits_lower, limits_upper)

            if shift:
                accmod.inputs["shift"].value = float(shift)

            if scale:
                accmod.inputs["scale"].value = float(scale)

            if method:
                accmod.inputs["method"].value = method

            print(accmod)
            accmod.run()

            if accmod.popen.returncode != 0:
                dbif.close()
                grass.fatal(_("Error running r.series.accumulate"))

            output_maps.append(output_map)
            old_map_name = output_map_name
            count += 1

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

    # Insert the maps into the output space time dataset
    if output_strds.is_in_db(dbif):
        if grass.overwrite():
            output_strds.delete(dbif)
            output_strds = input_strds.get_new_instance(out_id)

    temporal_type, semantic_type, title, description = input_strds.get_initial_values()
    output_strds.set_initial_values(temporal_type, semantic_type, title,
                                    description)
    output_strds.insert(dbif)

    empty_maps = []
    # Register the maps in the database
    count = 0
    for output_map in output_maps:
        count += 1
        if count%10 == 0:
            grass.percent(count, len(output_maps), 1)
        # Read the raster map data
        output_map.load()
        # In case of a empty map continue, do not register empty maps

        if not register_null:
            if output_map.metadata.get_min() is None and \
                output_map.metadata.get_max() is None:
                empty_maps.append(output_map)
                continue

        # Insert map in temporal database
        output_map.insert(dbif)
        output_strds.register_map(output_map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    output_strds.update_from_registered_maps(dbif)
    grass.percent(1, 1, 1)

    dbif.close()

    # Remove empty maps
    if len(empty_maps) > 0:
        for map in empty_maps:
            grass.run_command("g.remove", flags='f', type="raster", name=map.get_name(), quiet=True)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
