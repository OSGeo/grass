#!/usr/bin/env python3

############################################################################
#
# MODULE: t.list
# AUTHOR(S): Soeren Gebbert
#
# PURPOSE: List space time datasets and maps registered in the temporal database
# COPYRIGHT: (C) 2011-2017, Soeren Gebbert and the GRASS Development Team
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
# % description: Lists space time datasets and maps registered in the temporal database.
# % keyword: temporal
# % keyword: map management
# % keyword: list
# % keyword: time
# %end

# %option
# % key: type
# % type: string
# % description: Type of the space time dataset or map, default is strds
# % guisection: Selection
# % required: no
# % options: strds, str3ds, stvds, raster, raster_3d, vector
# % answer: strds
# %end

# %option G_OPT_T_TYPE
# % multiple: yes
# % answer: absolute,relative
# % guisection: Selection
# %end

# %option
# % key: order
# % type: string
# % description: Columns number_of_maps and granularity only available for space time datasets
# % label: Sort the space time dataset by category
# % guisection: Formatting
# % required: no
# % multiple: yes
# % options: id,name,semantic_label,creator,mapset,number_of_maps,creation_time,start_time,end_time,interval,north,south,west,east,granularity
# % answer: id
# %end

# %option
# % key: columns
# % type: string
# % description: Columns number_of_maps and granularity only available for space time datasets
# % label: Columns to be printed to stdout
# % guisection: Selection
# % required: no
# % multiple: yes
# % options: id,name,semantic_label,creator,mapset,number_of_maps,creation_time,start_time,end_time,north,south,west,east,granularity,all
# % answer: id
# %end

# %option G_OPT_T_WHERE
# % guisection: Selection
# %end

# %option G_OPT_F_SEP
# % label: Field separator character between the output columns
# % guisection: Formatting
# %end

# %option G_OPT_F_OUTPUT
# % required: no
# %end

# %flag
# % key: c
# % description: Print the column names as first row
# % guisection: Formatting
# %end

import sys

import grass.script as gs
from contextlib import nullcontext

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    type = options["type"]
    temporal_type = options["temporaltype"]
    columns = options["columns"]
    order = options["order"]
    where = options["where"]
    separator = gs.separator(options["separator"])
    outpath = options["output"]
    colhead = flags["c"]

    # Make sure the temporal database exists
    tgis.init()

    sp = tgis.dataset_factory(type, None)
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()
    first = True

    if gs.verbosity() > 0 and not outpath:
        sys.stderr.write("----------------------------------------------\n")

    # Replace separate "if outpath" and "else" blocks with a unified context manager:
    with (
        open(outpath, "w")
        if (outpath and outpath != "-")
        else nullcontext(sys.stdout) as out_file
    ):
        for ttype in temporal_type.split(","):
            time = "absolute time" if ttype == "absolute" else "relative time"

            stds_list = tgis.get_dataset_list(
                type, ttype, columns, where, order, dbif=dbif
            )

            mapsets = tgis.get_tgis_c_library_interface().available_mapsets()

            for key in mapsets:
                if key in stds_list.keys():
                    rows = stds_list[key]

                    if rows:
                        if gs.verbosity() > 0 and (not outpath or outpath == "-"):
                            if issubclass(sp.__class__, tgis.AbstractMapDataset):
                                sys.stderr.write(
                                    _(
                                        "Time stamped %s maps with %s available in mapset "
                                        "<%s>:\n"
                                    )
                                    % (sp.get_type(), time, key)
                                )
                            else:
                                sys.stderr.write(
                                    _(
                                        "Space time %s datasets with %s available in "
                                        "mapset <%s>:\n"
                                    )
                                    % (
                                        sp.get_new_map_instance(None).get_type(),
                                        time,
                                        key,
                                    )
                                )

                        if colhead and first:
                            output = ""
                            count = 0
                            for col_key in rows[0].keys():
                                output += (separator if count > 0 else "") + str(
                                    col_key
                                )
                                count += 1
                            out_file.write("{st}\n".format(st=output))
                            first = False

                        for row in rows:
                            output = ""
                            count = 0
                            for col in row:
                                output += (separator if count > 0 else "") + str(col)
                                count += 1
                            out_file.write("{st}\n".format(st=output))
    dbif.close()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
