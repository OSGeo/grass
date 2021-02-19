"""
Univariate statistic function for space time datasets

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.print_gridded_dataset_univar_statistics(type, input, output, where, extended, no_header, fs, rast_region)

..

(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from __future__ import print_function

from .core import SQLDatabaseInterfaceConnection, get_current_mapset
from .factory import dataset_factory
from .open_stds import open_old_stds
import grass.script as gscript

###############################################################################


def print_gridded_dataset_univar_statistics(type, input, output, where, extended,
                                            no_header=False, fs="|",
                                            rast_region=False):
    """Print univariate statistics for a space time raster or raster3d dataset

       :param type: Must be "strds" or "str3ds"
       :param input: The name of the space time dataset
       :param output: Name of the optional output file, if None stdout is used
       :param where: A temporal database where statement
       :param extended: If True compute extended statistics
       :param no_header: Suppress the printing of column names
       :param fs: Field separator
       :param rast_region: If set True ignore the current region settings
              and use the raster map regions for univar statistical calculation.
              Only available for strds.
    """

    # We need a database interface
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = open_old_stds(input, type, dbif)

    if output is not None:
        out_file = open(output, "w")

    rows = sp.get_registered_maps(
        "id,start_time,end_time", where, "start_time", dbif)

    if not rows:
        dbif.close()
        err = "Space time %(sp)s dataset <%(i)s> is empty"
        if where:
            err += " or where condition is wrong"
        gscript.fatal(_(err) % {'sp': sp.get_new_map_instance(None).get_type(),
                                'i': sp.get_id()})

    if no_header is False:
        string = ""
        string += "id" + fs + "start" + fs + "end" + fs + "mean" + fs
        string += "min" + fs + "max" + fs
        string += "mean_of_abs" + fs + "stddev" + fs + "variance" + fs
        string += "coeff_var" + fs + "sum" + fs + "null_cells" + fs + "cells"
        string += fs + "non_null_cells"
        if extended is True:
            string += fs + "first_quartile" + fs + "median" + fs
            string += "third_quartile" + fs + "percentile_90"

        if output is None:
            print(string)
        else:
            out_file.write(string + "\n")

    for row in rows:
        string = ""
        id = row["id"]
        start = row["start_time"]
        end = row["end_time"]

        flag = "g"

        if extended is True:
            flag += "e"
        if type == "strds" and rast_region is True:
            flag += "r"

        if type == "strds":
            stats = gscript.parse_command("r.univar", map=id, flags=flag)
        elif type == "str3ds":
            stats = gscript.parse_command("r3.univar", map=id, flags=flag)

        if not stats:
            if type == "strds":
                gscript.warning(_("Unable to get statistics for raster map "
                                  "<%s>") % id)
            elif type == "str3ds":
                gscript.warning(_("Unable to get statistics for 3d raster map"
                                  " <%s>") % id)
            continue

        string += str(id) + fs + str(start) + fs + str(end)
        string += fs + str(stats["mean"]) + fs + str(stats["min"])
        string += fs + str(stats["max"]) + fs + str(stats["mean_of_abs"])
        string += fs + str(stats["stddev"]) + fs + str(stats["variance"])
        string += fs + str(stats["coeff_var"]) + fs + str(stats["sum"])
        string += fs + str(stats["null_cells"]) + fs + str(stats["cells"])
        string += fs + str(int(stats["cells"]) - int(stats["null_cells"]))
        if extended is True:
            string += fs + str(stats["first_quartile"]) + fs + str(stats["median"])
            string += fs + str(stats["third_quartile"]) + fs + str(stats["percentile_90"])

        if output is None:
            print(string)
        else:
            out_file.write(string + "\n")

    dbif.close()

    if output is not None:
        out_file.close()


###############################################################################


def print_vector_dataset_univar_statistics(input, output, twhere, layer, type, column,
                                           where, extended, no_header=False,
                                           fs="|"):
    """Print univariate statistics for a space time vector dataset

       :param input: The name of the space time dataset
       :param output: Name of the optional output file, if None stdout is used
       :param twhere: A temporal database where statement
       :param layer: The layer number used in case no layer is present
              in the temporal dataset
       :param type: options: point,line,boundary,centroid,area
       :param column: The name of the attribute column
       :param where: A temporal database where statement
       :param extended: If True compute extended statistics
       :param no_header: Suppress the printing of column names
       :param fs: Field separator
    """

    # We need a database interface
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    if output is not None:
        out_file = open(output, "w")

    mapset = get_current_mapset()

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = dataset_factory("stvds", id)

    if sp.is_in_db(dbif) is False:
        dbif.close()
        gscript.fatal(_("Space time %(sp)s dataset <%(i)s> not found") % {
                      'sp': sp.get_new_map_instance(None).get_type(), 'i': id})

    sp.select(dbif)

    rows = sp.get_registered_maps("id,name,mapset,start_time,end_time,layer",
                                  twhere, "start_time", dbif)

    if not rows:
        dbif.close()
        gscript.fatal(_("Space time %(sp)s dataset <%(i)s> is empty") % {
                      'sp': sp.get_new_map_instance(None).get_type(), 'i': id})

    string = ""
    if no_header is False:
        string += "id" + fs + "start" + fs + "end" + fs + "n" + \
            fs + "nmissing" + fs + "nnull" + fs
        string += "min" + fs + "max" + fs + "range"
        if type == "point" or type == "centroid":
            string += fs + "mean" + fs + "mean_abs" + fs + "population_stddev" +\
                fs + "population_variance" + fs
            string += "population_coeff_variation" + fs + \
                "sample_stddev" + fs + "sample_variance" + fs
            string += "kurtosis" + fs + "skewness"
            if extended is True:
                string += fs + "first_quartile" + fs + "median" + fs + \
                    "third_quartile" + fs + "percentile_90"

        if output is None:
            print(string)
        else:
            out_file.write(string + "\n")

    for row in rows:
        id = row["name"] + "@" + row["mapset"]
        start = row["start_time"]
        end = row["end_time"]
        mylayer = row["layer"]

        flags = "g"

        if extended is True:
            flags += "e"

        if not mylayer:
            mylayer = layer

        stats = gscript.parse_command("v.univar", map=id, where=where,
                                      column=column, layer=mylayer,
                                      type=type, flags=flags)

        string = ""

        if not stats:
            gscript.warning(_("Unable to get statistics for vector map <%s>")
                            % id)
            continue

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
            if extended is True:
                if "first_quartile" in stats:
                    string += fs + str(stats["first_quartile"]) + fs + \
                        str(stats["median"]) + fs + \
                        str(stats["third_quartile"]) + fs + \
                        str(stats["percentile_90"])
                else:
                    string += fs + fs + fs + fs

        if output is None:
            print(string)
        else:
            out_file.write(string + "\n")

    dbif.close()

    if output is not None:
        out_file.close()
