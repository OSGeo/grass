#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.vect.what.strds
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Store raster map values at spatial and temporal positions of vector points as vector attributes.
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Store raster map values at spatial and temporal positions of vector points as vector attributes.
#% keywords: temporal
#% keywords: sampling
#%end

#%option G_OPT_STVDS_INPUT
#%end

#%option G_OPT_STRDS_INPUT
#% key: strds
#%end

#%option
#% key: column
#% type: string
#% label: Name of the vector column to be created and to store sampled raster values
#% description: The use of a column name forces t.vect.what.rast to sample only values from the first map found in an interval. Otherwise the raster map names are used as column names
#% required: no
#% multiple: no
#%end

#%option
#% key: method
#% type: string
#% description: Aggregate operation to be performed on the raster maps
#% required: yes
#% multiple: no
#% options: disabled,average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
#% answer: disabled
#%end

#%option G_OPT_DB_WHERE
#%end

#%option G_OPT_T_WHERE
#% key: t_where
#%end

#%option G_OPT_T_SAMPLE
#%end

import os
import grass.script as grass
import grass.temporal as tgis
import grass.script.raster as raster

############################################################################


def main():

    # Get the options
    input = options["input"]
    strds = options["strds"]
    where = options["where"]
    column = options["column"]
    method = options["method"]
    tempwhere = options["t_where"]
    sampling = options["sampling"]

    if where == "" or where == " " or where == "\n":
        where = None

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = tgis.open_old_space_time_dataset(input, "stvds", dbif)
    strds_sp = tgis.open_old_space_time_dataset(strds, "strds", dbif)

    if strds_sp.get_temporal_type() != sp.get_temporal_type():
        dbif.close()
        grass.fatal(_("Input and aggregation dataset must "
                      "have the same temporal type"))

    # Check if intervals are present in the sample dataset
    if sp.get_temporal_type() == "absolute":
        map_time = sp.absolute_time.get_map_time()
    else:
        map_time = sp.relative_time.get_map_time()

    if map_time != "interval":
        dbif.close()
        grass.fatal(_("All registered maps of the space time vector "
                      "dataset must have time intervals"))

    rows = sp.get_registered_maps("name,layer,mapset,start_time,end_time",
                                  tempwhere, "start_time", dbif)

    if not rows:
        dbif.close()
        grass.fatal(_("Space time vector dataset <%s> is empty") % sp.get_id())

    # Sample the raster dataset with the vector dataset and run v.what.rast
    for row in rows:
        start = row["start_time"]
        end = row["end_time"]
        vectmap = row["name"] + "@" + row["mapset"]
        layer = row["layer"]

        raster_maps = tgis.collect_map_names(
            strds_sp, dbif, start, end, sampling)

        aggreagated_map_name = None

        if raster_maps:
            # Aggregation
            if method != "disabled" and len(raster_maps) > 1:
                # Generate the temporary map name
                aggreagated_map_name = "aggreagated_map_name_" + \
                    str(os.getpid())
                new_map = tgis.aggregate_raster_maps(raster_maps,
                                                     aggreagated_map_name,
                                                     start, end, 0, method,
                                                     False, dbif)
                aggreagated_map_name = aggreagated_map_name + "_0"
                if new_map is None:
                    continue
                # We overwrite the raster_maps list
                raster_maps = (new_map.get_id(), )

            for rastermap in raster_maps:

                if column:
                    col_name = column
                else:
                    # Create a new column with the SQL compliant
                    # name of the sampled raster map
                    col_name = rastermap.split("@")[0].replace(".", "_")

                coltype = "DOUBLE PRECISION"
                # Get raster type
                rasterinfo = raster.raster_info(rastermap)
                if rasterinfo["datatype"] == "CELL":
                    coltype = "INT"

                if layer:
                    ret = grass.run_command("v.db.addcolumn",
                                            map=vectmap, layer=layer,
                                            column="%s %s" % (col_name, coltype),
                                            overwrite=grass.overwrite())
                else:
                    ret = grass.run_command("v.db.addcolumn", map=vectmap,
                                            column="%s %s" % (col_name, coltype),
                                            overwrite=grass.overwrite())

                if ret != 0:
                    dbif.close()
                    grass.fatal(_("Unable to add column %s to vector map <%s>")
                                % (col_name, vectmap))

                # Call v.what.rast
                if layer:
                    ret = grass.run_command("v.what.rast", map=vectmap,
                                            layer=layer, raster=rastermap,
                                            column=col_name, where=where)
                else:
                    ret = grass.run_command("v.what.rast", map=vectmap,
                                            raster=rastermap, column=col_name,
                                            where=where)
                if ret != 0:
                    dbif.close()
                    grass.fatal(_("Unable to run v.what.rast for vector map "
                                  "<%s> and raster map <%s>") % (vectmap,
                                                                 rastermap))

                if aggreagated_map_name:
                    ret = grass.run_command("g.remove",
                                            rast=aggreagated_map_name)
                    if ret != 0:
                        dbif.close()
                        grass.fatal(_("Unable to remove raster map <%s>")
                                    % (aggreagated_map_name))

                # Use the first map in case a column names was provided
                if column:
                    break

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
