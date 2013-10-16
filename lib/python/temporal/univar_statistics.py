"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

tgis.print_gridded_dataset_univar_statistics(
    type, input, where, extended, header, fs)

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *
from factory import *
from open import *

###############################################################################


def print_gridded_dataset_univar_statistics(type, input, where, extended,
                                            header, fs):
    """!Print univariate statistics for a space time raster or raster3d dataset

       @param type Must be "strds" or "str3ds"
       @param input The name of the space time dataset
       @param where A temporal database where statement
       @param extended If True compute extended statistics
       @param header   If True print column names as header
       @param fs Field separator
    """

    # We need a database interface
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = open_old_space_time_dataset(input, type, dbif)

    rows = sp.get_registered_maps(
        "id,start_time,end_time", where, "start_time", dbif)

    if not rows:
        dbif.close()
        core.fatal(_("Space time %(sp)s dataset <%(i)s> is empty") % {
                     'sp': sp.get_new_map_instance(None).get_type(), 'i': id})

    if header == True:
        print "id" + fs + "start" + fs + "end" + fs + "mean" + fs + \
            "min" + fs + "max" + fs,
        print "mean_of_abs" + fs + "stddev" + fs + "variance" + fs,
        if extended == True:
            print "coeff_var" + fs + "sum" + fs + \
                "null_cells" + fs + "cells" + fs,
            print "first_quartile" + fs + "median" + fs + \
                "third_quartile" + fs + "percentile_90"
        else:
            print "coeff_var" + fs + "sum" + fs + "null_cells" + fs + "cells"

    for row in rows:
        id = row["id"]
        start = row["start_time"]
        end = row["end_time"]

        flag = "g"

        if extended == True:
            flag += "e"

        if type == "strds":
            stats = core.parse_command("r.univar", map=id, flags=flag)
        elif type == "str3ds":
            stats = core.parse_command("r3.univar", map=id, flags=flag)

        print str(id) + fs + str(start) + fs + str(end),
        print fs + str(stats["mean"]) + fs + str(stats["min"]) + \
            fs + str(stats["max"]) + fs + str(stats["mean_of_abs"]),
        print fs + str(stats["stddev"]) + fs + str(stats["variance"]) + \
            fs + str(stats["coeff_var"]) + fs + str(stats["sum"]),

        if extended == True:
            print fs + str(stats["null_cells"]) + fs + str(
                stats["cells"]) + fs,
            print str(stats["first_quartile"]) + fs + str(stats["median"]) + \
                  fs + str(stats["third_quartile"]) + \
                  fs + str(stats["percentile_90"])
        else:
            print fs + str(stats["null_cells"]) + fs + str(stats["cells"])

    dbif.close()

###############################################################################


def print_vector_dataset_univar_statistics(input, twhere, layer, type, column,
                                           where, extended, header, fs):
    """!Print univariate statistics for a space time vector dataset

       @param input The name of the space time dataset
       @param twhere A temporal database where statement
       @param layer The layer number used in case no layer is present
              in the temporal dataset
       @param type options: point,line,boundary,centroid,area
       @param column The name of the attribute column
       @param where A temporal database where statement
       @param extended If True compute extended statistics
       @param header   If True print column names as header
       @param fs Field separator
    """

    # We need a database interface
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = get_current_mapset()

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = dataset_factory("stvds", id)

    if sp.is_in_db(dbif) == False:
        dbif.close()
        core.fatal(_("Space time %(sp)s dataset <%(i)s> not found") % {
                     'sp': sp.get_new_map_instance(None).get_type(), 'i': id})

    sp.select(dbif)

    rows = sp.get_registered_maps("id,name,mapset,start_time,end_time,layer",
                                  twhere, "start_time", dbif)

    if not rows:
        dbif.close()
        core.fatal(_("Space time %(sp)s dataset <%(i)s> is empty") % {
                     'sp': sp.get_new_map_instance(None).get_type(), 'i': id})

    string = ""
    if header == True:
        string += "id" + fs + "start" + fs + "end" + fs + "n" + \
            fs + "nmissing" + fs + "nnull" + fs
        string += "min" + fs + "max" + fs + "range"
        if type == "point" or type == "centroid":
            string += fs + "mean" + fs + "mean_abs" + fs + "population_stddev" +\
                      fs + "population_variance" + fs
            string += "population_coeff_variation" + fs + \
                "sample_stddev" + fs + "sample_variance" + fs
            string += "kurtosis" + fs + "skewness"
            if extended == True:
                string += fs + "first_quartile" + fs + "median" + fs + \
                    "third_quartile" + fs + "percentile_90"

        print string

    for row in rows:
        id = row["name"] + "@" + row["mapset"]
        start = row["start_time"]
        end = row["end_time"]
        mylayer = row["layer"]

        flags = "g"

        if extended == True:
            flags += "e"

        if not mylayer:
            mylayer = layer

        stats = core.parse_command("v.univar", map=id, where=where,
                                   column=column, layer=mylayer,
                                   type=type, flags=flags)

        string = ""
        if stats:
            string += str(id) + fs + str(start) + fs + str(end)
            string += fs + str(stats["n"]) + fs + str(stats[
                "nmissing"]) + fs + str(stats["nnull"])
            if "min" in stats:
                string += fs + str(stats["min"]) + fs + str(
                    stats["max"]) + fs + str(stats["range"])
            else:
                string += fs + fs + fs

            if type == "point" or type == "centroid":
                if "mean" in stats:
                    string += fs + str(stats["mean"]) + fs + \
                    str(stats["mean_abs"]) + fs + \
                    str(stats["population_stddev"]) + fs + \
                    str(stats["population_variance"])

                    string += fs + str(stats["population_coeff_variation"]) + \
                    fs + str(stats["sample_stddev"]) + fs + \
                    str(stats["sample_variance"])

                    string += fs + str(stats["kurtosis"]) + fs + \
                    str(stats["skewness"])
                else:
                    string += fs + fs + fs + fs + fs + fs + fs + fs + fs
                if extended == True:
                    if "first_quartile" in stats:
                        string += fs + str(stats["first_quartile"]) + fs + \
                        str(stats["median"]) + fs + \
                        str(stats["third_quartile"]) + fs + \
                        str(stats["percentile_90"])
                    else:
                        string += fs + fs + fs + fs

            print string

    dbif.close()
