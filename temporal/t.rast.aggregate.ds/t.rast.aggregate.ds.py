#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.rast.aggregate.ds
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Aggregates data of an existing space time raster dataset using the time intervals of a second space time dataset
# COPYRIGHT:	(C) 2011-2017 by the GRASS Development Team
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
# % description: Aggregates data of an existing space time raster dataset using the time intervals of a second space time dataset.
# % keyword: temporal
# % keyword: aggregation
# % keyword: raster
# % keyword: time
# %end

# %option G_OPT_STRDS_INPUT
# %end

# %option G_OPT_STDS_INPUT
# % key: sample
# % description: Time intervals from this space time dataset (raster, vector or raster3d) are used for aggregation computation
# %end

# %option G_OPT_STDS_TYPE
# % description: Type of the space time dataset from which aggregation will be copied
# %end

# %option G_OPT_STRDS_OUTPUT
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
# % description: Number of r.mapcalc processes to run in parallel
# % required: no
# % multiple: no
# % answer: 1
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
    sampler = options["sample"]
    where = options["where"]
    base = options["basename"]
    register_null = flags["n"]
    method = options["method"]
    sampling = options["sampling"]
    offset = options["offset"]
    nprocs = options["nprocs"]
    time_suffix = options["suffix"]
    type = options["type"]

    topo_list = sampling.split(",")

    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = tgis.open_old_stds(input, "strds", dbif)
    sampler_sp = tgis.open_old_stds(sampler, type, dbif)

    if sampler_sp.get_temporal_type() != sp.get_temporal_type():
        dbif.close()
        gs.fatal(_("Input and aggregation dataset must have the same temporal type"))

    # Check if intervals are present
    if sampler_sp.temporal_extent.get_map_time() != "interval":
        dbif.close()
        gs.fatal(
            _("All registered maps of the aggregation dataset must have time intervals")
        )

    # We will create the strds later, but need to check here
    tgis.check_new_stds(output, "strds", dbif, gs.overwrite())

    map_list = sp.get_registered_maps_as_objects(
        where=where, order="start_time", dbif=dbif
    )

    if not map_list:
        dbif.close()
        gs.fatal(_("Space time raster dataset <%s> is empty") % input)

    granularity_list = sampler_sp.get_registered_maps_as_objects(
        where=where, order="start_time", dbif=dbif
    )

    if not granularity_list:
        dbif.close()
        gs.fatal(_("Space time raster dataset <%s> is empty") % sampler)

    gran = sampler_sp.get_granularity()

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
