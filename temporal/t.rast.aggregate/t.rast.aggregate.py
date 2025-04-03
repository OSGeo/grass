#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.rast.aggregate
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally aggregates the maps of a space time raster dataset by a user defined granularity.
# COPYRIGHT:    (C) 2011-2017 by the GRASS Development Team
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
# % description: Aggregates temporally the maps of a space time raster dataset by a user defined granularity.
# % keyword: temporal
# % keyword: aggregation
# % keyword: raster
# % keyword: time
# %end

# %option G_OPT_STRDS_INPUT
# %end

# %option G_OPT_STRDS_OUTPUT
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: Either a numerical suffix or the start time (s-flag) separated by an underscore will be attached to create a unique identifier
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
# % key: granularity
# % type: string
# % description: Aggregation granularity, format absolute time "x years, x months, x weeks, x days, x hours, x minutes, x seconds" or an integer value for relative time
# % required: yes
# % multiple: no
# %end

# %option
# % key: method
# % type: string
# % description: Aggregate operation to be performed on the raster maps
# % required: yes
# % multiple: no
# % options: average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
# % answer: average
# %end

# %option
# % key: offset
# % type: integer
# % description: Offset that is used to create the output map ids, output map id is generated as: basename_ (count + offset)
# % required: no
# % multiple: no
# % answer: 0
# %end

# %option
# % key: nprocs
# % type: integer
# % description: Number of r.series processes to run in parallel
# % required: no
# % multiple: no
# % answer: 1
# %end

# %option
# % key: file_limit
# % type: integer
# % description: The maximum number of open files allowed for each r.series process
# % required: no
# % multiple: no
# % answer: 1000
# %end

# %option G_OPT_T_SAMPLE
# % options: equal,overlaps,overlapped,starts,started,finishes,finished,during,contains
# % answer: contains
# %end

# %option G_OPT_T_WHERE
# %end

# %flag
# % key: n
# % description: Register Null maps
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    gran = options["granularity"]
    base = options["basename"]
    register_null = flags["n"]
    method = options["method"]
    sampling = options["sampling"]
    offset = options["offset"]
    nprocs = options["nprocs"]
    file_limit = options["file_limit"]
    time_suffix = options["suffix"]

    topo_list = sampling.split(",")

    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = tgis.open_old_stds(input, "strds", dbif)

    map_list = sp.get_registered_maps_as_objects(
        where=where, order="start_time", dbif=dbif
    )

    if not map_list:
        dbif.close()
        gs.fatal(_("Space time raster dataset <%s> is empty") % input)

    # We will create the strds later, but need to check here
    tgis.check_new_stds(output, "strds", dbif, gs.overwrite())

    start_time = map_list[0].temporal_extent.get_start_time()

    if sp.is_time_absolute():
        start_time = tgis.adjust_datetime_to_granularity(start_time, gran)

    # We use the end time first
    end_time = map_list[-1].temporal_extent.get_end_time()
    has_end_time = True

    # In case no end time is available, then we use the start time of the last map layer
    if end_time is None:
        end_time = map_list[-1].temporal_extent.get_start_time()
        has_end_time = False

    granularity_list = []

    # Build the granularity list
    while True:
        if has_end_time is True:
            if start_time >= end_time:
                break
        else:  # noqa: PLR5501
            if start_time > end_time:
                break

        granule = tgis.RasterDataset(None)
        start = start_time
        if sp.is_time_absolute():
            end = tgis.increment_datetime_by_string(start_time, gran)
            granule.set_absolute_time(start, end)
        else:
            end = start_time + int(gran)
            granule.set_relative_time(start, end, sp.get_relative_time_unit())
        start_time = end

        granularity_list.append(granule)

    output_list = tgis.aggregate_by_topology(
        granularity_list=granularity_list,
        granularity=gran,
        map_list=map_list,
        topo_list=topo_list,
        basename=base,
        time_suffix=time_suffix,
        offset=offset,
        method=method,
        nprocs=nprocs,
        spatial=None,
        overwrite=gs.overwrite(),
        file_limit=file_limit,
    )

    if output_list:
        temporal_type, semantic_type, title, description = sp.get_initial_values()
        output_strds = tgis.open_new_stds(
            output,
            "strds",
            temporal_type,
            title,
            description,
            semantic_type,
            dbif,
            gs.overwrite(),
        )
        register_null = not register_null

        tgis.register_map_object_list(
            "rast",
            output_list,
            output_strds,
            register_null,
            sp.get_relative_time_unit(),
            dbif,
        )

        # Update the raster metadata table entries with aggregation type
        output_strds.set_aggregation_type(method)
        output_strds.metadata.update(dbif)

    dbif.close()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
