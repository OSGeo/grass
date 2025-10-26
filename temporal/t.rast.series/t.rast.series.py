#!/usr/bin/env python3

############################################################################
#
# MODULE:    t.rast.series
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:    Perform different aggregation algorithms from r.series on all or a
#           selected subset of raster maps in a space time raster dataset
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
# % description: Performs different aggregation algorithms from r.series on all or a subset of raster maps in a space time raster dataset.
# % keyword: temporal
# % keyword: aggregation
# % keyword: series
# % keyword: raster
# % keyword: time
# %end

# %option G_OPT_STRDS_INPUT
# %end

# %option
# % key: method
# % type: string
# % description: Aggregate operation to be performed on the raster maps
# % required: yes
# % multiple: yes
# % options: average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
# % answer: average
# %end

# %option
# % key: quantile
# % type: double
# % description: Quantile to calculate for method=quantile
# % required: no
# % multiple: yes
# % options: 0.0-1.0
# %end

# %option
# % key: order
# % type: string
# % description: Sort the maps by category
# % required: no
# % multiple: yes
# % options: id, name, creator, mapset, creation_time, modification_time, start_time, end_time, north, south, west, east, min, max
# % answer: start_time
# %end

# %option G_OPT_M_NPROCS
# %end

# %option G_OPT_MEMORYMB
# %end

# %option G_OPT_T_WHERE
# %end

# %option G_OPT_R_OUTPUTS
# %end

# %option
# % key: file_limit
# % type: integer
# % description: The maximum number of open files allowed for each r.series process
# % required: no
# % answer: 1000
# %end

# %flag
# % key: t
# % description: Do not assign the space time raster dataset start and end time to the output map
# %end

# %flag
# % key: n
# % description: Propagate NULLs
# %end

import grass.script as gs
from grass.exceptions import CalledModuleError
from pathlib import Path
############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    method = options["method"]
    quantile = options["quantile"]
    order = options["order"]
    memory = options["memory"]
    nprocs = options["nprocs"]
    where = options["where"]
    max_files_open = int(options["file_limit"])
    add_time = flags["t"]
    nulls = flags["n"]

    # Check if number of methods and output maps matches
    if "quantile" in method:
        len_method = len(method.split(",")) - 1
    else:
        len_method = len(method.split(","))

    if (len(list(filter(None, quantile.split(",")))) + len_method) != len(
        output.split(",")
    ):
        gs.fatal(_("Number requested methods and output maps do not match."))

    # Make sure the temporal database exists
    tgis.init()

    sp = tgis.open_old_stds(input, "strds")

    rows = sp.get_registered_maps("id", where, order, None)

    if rows:
        # Create the r.series input file
        filename = gs.tempfile(True)
        Path(filename).write_text("\n".join(str(row["id"]) for row in rows))

        flag = ""
        if len(rows) > max_files_open:
            gs.warning(
                _(
                    "Processing over {} maps: activating -z flag of r.series which "
                    "slows down processing."
                ).format(max_files_open)
            )
            flag += "z"
        if nulls:
            flag += "n"

        try:
            gs.run_command(
                "r.series",
                flags=flag,
                file=filename,
                output=output,
                overwrite=gs.overwrite(),
                method=method,
                quantile=quantile,
                memory=memory,
                nprocs=nprocs,
            )
        except CalledModuleError:
            gs.fatal(_("%s failed. Check above error messages.") % "r.series")

        if not add_time:
            # We need to set the temporal extent from the subset of selected maps
            maps = sp.get_registered_maps_as_objects(
                where=where, order=order, dbif=None
            )
            first_map = maps[0]
            last_map = maps[-1]
            start_a, end_a = first_map.get_temporal_extent_as_tuple()
            start_b, end_b = last_map.get_temporal_extent_as_tuple()

            if end_b is None:
                end_b = start_b

            if first_map.is_time_absolute():
                extent = tgis.AbsoluteTemporalExtent(start_time=start_a, end_time=end_b)
            else:
                extent = tgis.RelativeTemporalExtent(
                    start_time=start_a,
                    end_time=end_b,
                    unit=first_map.get_relative_time_unit(),
                )

            for out_map in output.split(","):
                # Create the time range for the output map
                if out_map.find("@") >= 0:
                    id = out_map
                else:
                    mapset = gs.gisenv()["MAPSET"]
                    id = out_map + "@" + mapset

                map = sp.get_new_map_instance(id)
                map.load()

                map.set_temporal_extent(extent=extent)

                # Register the map in the temporal database
                if map.is_in_db():
                    map.update_all()
                else:
                    map.insert()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
