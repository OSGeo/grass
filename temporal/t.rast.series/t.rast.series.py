#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.series
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Perform different aggregation algorithms from r.series on all or a
#           selected subset of raster maps in a space time raster dataset
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

#%module
#% description: Performs different aggregation algorithms from r.series on all or a subset of raster maps in a space time raster dataset.
#% keyword: temporal
#% keyword: aggregation
#% keyword: series
#% keyword: raster
#% keyword: time
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option
#% key: method
#% type: string
#% description: Aggregate operation to be performed on the raster maps
#% required: yes
#% multiple: no
#% options: average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
#% answer: average
#%end

#%option
#% key: quantile
#% type: double
#% description: Quantile to calculate for method=quantile
#% required: no
#% multiple: no
#% options: 0.0-1.0
#%end

#%option
#% key: order
#% type: string
#% description: Sort the maps by category
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, creation_time, modification_time, start_time, end_time, north, south, west, east, min, max
#% answer: start_time
#%end

#%option G_OPT_T_WHERE
#%end

#%option G_OPT_R_OUTPUT
#%end

#%flag
#% key: t
#% description: Do not assign the space time raster dataset start and end time to the output map
#%end

#%flag
#% key: n
#% description: Propagate NULLs
#%end


import grass.script as grass
from grass.exceptions import CalledModuleError

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
    where = options["where"]
    add_time = flags["t"]
    nulls = flags["n"]

    # Make sure the temporal database exists
    tgis.init()

    sp = tgis.open_old_stds(input, "strds")

    rows = sp.get_registered_maps("id", where, order, None)

    if rows:
        # Create the r.series input file
        filename = grass.tempfile(True)
        file = open(filename, 'w')

        for row in rows:
            string = "%s\n" % (row["id"])
            file.write(string)

        file.close()

        flag = ""
        if len(rows) > 1000:
            grass.warning(_("Processing over 1000 maps: activating -z flag of r.series which slows down processing"))
            flag += "z"
        if nulls:
            flag += "n"

        try:
            grass.run_command("r.series", flags=flag, file=filename,
                              output=output, overwrite=grass.overwrite(),
                              method=method, quantile=quantile)
        except CalledModuleError:
            grass.fatal(_("%s failed. Check above error messages.") % 'r.series')

        if not add_time:
            # Create the time range for the output map
            if output.find("@") >= 0:
                id = output
            else:
                mapset = grass.encode(grass.gisenv()["MAPSET"])
                id = output + "@" + mapset

            map = sp.get_new_map_instance(id)
            map.load()
            map.set_temporal_extent(sp.get_temporal_extent())

            # Register the map in the temporal database
            if map.is_in_db():
                map.update_all()
            else:
                map.insert()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
