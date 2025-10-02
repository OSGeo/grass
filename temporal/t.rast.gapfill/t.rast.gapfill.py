#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.rast.gapfill
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Replace gaps in a space time raster dataset with interpolated raster maps.
# COPYRIGHT:    (C) 2012-2017 by the GRASS Development Team
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

# %module
# % description: Replaces gaps in a space time raster dataset with interpolated raster maps.
# % keyword: temporal
# % keyword: interpolation
# % keyword: raster
# % keyword: time
# % keyword: no-data filling
# %end

# %option G_OPT_STRDS_INPUT
# %end

# %option G_OPT_T_WHERE
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % required: yes
# % multiple: no
# %end

# %option
# % key: suffix
# % type: string
# % description: Suffix to add at basename: set 'gran' for granularity, 'time' for the full time format, 'num' for numerical suffix with a specific number of digits (default %05)
# % answer: gran
# % required: no
# % multiple: no
# %end

# %option
# % key: nprocs
# % type: integer
# % description: Number of interpolation processes to run in parallel
# % required: no
# % multiple: no
# % answer: 1
# %end

# %flag
# % key: t
# % description: Assign the space time raster dataset start and end time to the output map
# %end

import copy
import sys

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.pygrass.modules as pymod
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    base = options["basename"]
    where = options["where"]
    nprocs = options["nprocs"]
    tsuffix = options["suffix"]

    mapset = gs.gisenv()["MAPSET"]

    # Make sure the temporal database exists
    tgis.init()

    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = tgis.open_old_stds(input, "strds")

    maps = sp.get_registered_maps_as_objects_with_gaps(where, dbif)

    num = len(maps)

    # Configure the r.to.vect module
    gapfill_module = pymod.Module(
        "r.series.interp",
        overwrite=gs.overwrite(),
        quiet=True,
        run_=False,
        finish_=False,
    )

    process_queue = pymod.ParallelModuleQueue(int(nprocs))

    gap_list = []
    overwrite_flags = {}

    # Identify all gaps and create new names
    count = 0
    for map_ in maps:
        if map_.get_id() is None:
            count += 1
            if sp.get_temporal_type() == "absolute" and tsuffix in {"gran", "time"}:
                id_ = "{ba}@{ma}".format(ba=base, ma=mapset)
            else:
                map_name = tgis.create_numeric_suffix(base, num + count, tsuffix)
                id_ = "{name}@{ma}".format(name=map_name, ma=mapset)
            map_.set_id(id_)

            gap_list.append(map_)

    if len(gap_list) == 0:
        gs.message(_("No gaps found"))
        return

    # Build the temporal topology
    tb = tgis.SpatioTemporalTopologyBuilder()
    tb.build(maps)

    # Do some checks before computation
    for map_ in gap_list:
        if not map_.get_precedes() or not map_.get_follows():
            gs.fatal(_("Unable to determine successor and predecessor of a gap."))

        if len(map_.get_precedes()) > 1:
            gs.warning(
                _("More than one successor of the gap found. Using the first found.")
            )

        if len(map_.get_follows()) > 1:
            gs.warning(
                _("More than one predecessor of the gap found. Using the first found.")
            )

    # Interpolate the maps using parallel processing
    result_list = []

    for map_ in gap_list:
        predecessor = map_.get_follows()[0]
        successor = map_.get_precedes()[0]

        gran = sp.get_granularity()
        tmpval, start = predecessor.get_temporal_extent_as_tuple()
        end, tmpval = successor.get_temporal_extent_as_tuple()

        # Now resample the gap
        map_matrix = tgis.AbstractSpaceTimeDataset.resample_maplist_by_granularity(
            (map_,), start, end, gran
        )

        map_names = []
        map_positions = []

        increment = 1.0 / (len(map_matrix) + 1.0)
        position = increment
        count = 0
        for intp_list in map_matrix:
            new_map = intp_list[0]
            count += 1
            if sp.get_temporal_type() == "absolute" and tsuffix == "gran":
                suffix = tgis.create_suffix_from_datetime(
                    new_map.temporal_extent.get_start_time(), sp.get_granularity()
                )
                new_id = "{ba}_{su}@{ma}".format(
                    ba=new_map.get_name(), su=suffix, ma=mapset
                )
            elif sp.get_temporal_type() == "absolute" and tsuffix == "time":
                suffix = tgis.create_time_suffix(new_map)
                new_id = "{ba}_{su}@{ma}".format(
                    ba=new_map.get_name(), su=suffix, ma=mapset
                )
            else:
                map_name = tgis.create_numeric_suffix(
                    new_map.get_name(), count, tsuffix
                )
                new_id = "{name}@{ma}".format(name=map_name, ma=mapset)

            new_map.set_id(new_id)

            overwrite_flags[new_id] = False
            if new_map.map_exists() or new_map.is_in_db(dbif):
                if not gs.overwrite():
                    gs.fatal(
                        _(
                            "Map with name <%s> already exists. "
                            "Please use another base name."
                        )
                        % (id_)
                    )
                elif new_map.is_in_db(dbif):
                    overwrite_flags[new_id] = True

            map_names.append(new_map.get_name())
            map_positions.append(position)
            position += increment

            result_list.append(new_map)

        mod = copy.deepcopy(gapfill_module)
        mod(
            input=(predecessor.get_map_id(), successor.get_map_id()),
            datapos=(0, 1),
            output=map_names,
            samplingpos=map_positions,
        )
        sys.stderr.write(mod.get_bash() + "\n")
        process_queue.put(mod)

    # Wait for unfinished processes
    process_queue.wait()

    # Insert new interpolated maps in temporal database and dataset
    for map_ in result_list:
        id = map_.get_id()
        if overwrite_flags[id]:
            if map_.is_time_absolute():
                start, end = map_.get_absolute_time()
                if map_.is_in_db():
                    map_.delete(dbif)
                map_ = sp.get_new_map_instance(id)
                map_.set_absolute_time(start, end)
            else:
                start, end, unit = map_.get_relative_time()
                if map_.is_in_db():
                    map_.delete(dbif)
                map_ = sp.get_new_map_instance(id)
                map_.set_relative_time(start, end, unit)
        map_.load()
        map_.insert(dbif)
        sp.register_map(map_, dbif)

    sp.update_from_registered_maps(dbif)
    sp.update_command_string(dbif=dbif)
    dbif.close()


###############################################################################


def run_interp(inputs, dpos, output, outpos):
    """Helper function to run r.series.interp in parallel"""
    return gs.run_command(
        "r.series.interp",
        input=inputs,
        datapos=dpos,
        output=output,
        samplingpos=outpos,
        overwrite=gs.overwrite(),
        quiet=True,
    )


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
