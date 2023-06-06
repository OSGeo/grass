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
from multiprocessing import Pool
from subprocess import PIPE

from .core import SQLDatabaseInterfaceConnection
from .open_stds import open_old_stds
import grass.script as gs
from grass.pygrass.modules import Module

###############################################################################


def compute_univar_stats(registered_map_info, stats_module, fs, rast_region=False):
    """Compute univariate statistics for a map of a space time raster or raster3d dataset

    :param registered_map_info: dict or db row with tgis info for a registered map
    :param stats_module: Pre-configured PyGRASS Module to compute univariate statistics with
    :param fs: Field separator
    :param rast_region: If set True ignore the current region settings
           and use the raster map regions for univar statistical calculation.
           Only available for strds.
    """
    string = ""
    id = registered_map_info["id"]
    start = registered_map_info["start_time"]
    end = registered_map_info["end_time"]
    semantic_label = (
        ""
        if stats_module.name == "r3.univar" or not registered_map_info["semantic_label"]
        else registered_map_info["semantic_label"]
    )

    stats_module.inputs.map = id
    if rast_region:
        stats_module.env = gs.region_env(raster=id)
    stats_module.run()

    univar_stats = stats_module.outputs.stdout

    if not univar_stats:
        gs.warning(
            _(
                "Unable to get statistics for {voxel}raster map "
                "<{rmap}>".format(
                    rmap=id, voxel="" if stats_module.name == "r.univar" else "3d "
                )
            )
        )
        return None
    eol = ""

    for idx, stats_kv in enumerate(univar_stats.split(";")):
        stats = gs.utils.parse_key_val(stats_kv)
        string += (
            f"{id}{fs}{semantic_label}{fs}{start}{fs}{end}"
            if stats_module.name == "r.univar"
            else f"{id}{fs}{start}{fs}{end}"
        )
        if stats_module.inputs.zones:
            if idx == 0:
                zone = str(stats["zone"])
                string = ""
                continue
            string += f"{fs}{zone}"
            if "zone" in stats:
                zone = str(stats["zone"])
                eol = "\n"
            else:
                eol = ""
        string += f'{fs}{stats["mean"]}{fs}{stats["min"]}'
        string += f'{fs}{stats["max"]}{fs}{stats["mean_of_abs"]}'
        string += f'{fs}{stats["stddev"]}{fs}{stats["variance"]}'
        string += f'{fs}{stats["coeff_var"]}{fs}{stats["sum"]}'
        string += f'{fs}{stats["null_cells"]}{fs}{stats["n"]}'
        string += f'{fs}{stats["n"]}'
        if "median" in stats:
            string += f'{fs}{stats["first_quartile"]}{fs}{stats["median"]}'
            string += f'{fs}{stats["third_quartile"]}'
            if stats_module.inputs.percentile:
                for perc in stats_module.inputs.percentile:
                    perc_value = stats[
                        f"percentile_{str(perc).rstrip('0').rstrip('.').replace('.','_')}"
                    ]
                    string += f"{fs}{perc_value}"
        string += eol
    return string


def print_gridded_dataset_univar_statistics(
    type,
    input,
    output,
    where,
    extended,
    no_header=False,
    fs="|",
    rast_region=False,
    zones=None,
    percentile=None,
    nprocs=1,
):
    """Print univariate statistics for a space time raster or raster3d dataset

    :param type: Type of Space-Time-Dataset, must be either strds or str3ds
    :param input: The name of the space time dataset
    :param output: Name of the optional output file, if None stdout is used
    :param where: A temporal database where statement
    :param extended: If True compute extended statistics
    :param percentile: List of percentiles to compute
    :param no_header: Suppress the printing of column names
    :param fs: Field separator
    :param nprocs: Number of cores to use for processing
    :param rast_region: If set True ignore the current region settings
           and use the raster map regions for univar statistical calculation.
           Only available for strds.
    :param zones: raster map with zones to calculate statistics for
    """
    # We need a database interface
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = open_old_stds(input, type, dbif)

    if output is not None:
        out_file = open(output, "w")

    strds_cols = (
        "id,start_time,end_time,semantic_label"
        if type == "strds"
        else "id,start_time,end_time"
    )
    rows = sp.get_registered_maps(strds_cols, where, "start_time", dbif)

    if not rows and rows != [""]:
        dbif.close()
        err = "Space time %(sp)s dataset <%(i)s> is empty"
        if where:
            err += " or where condition does not return any maps"
        gs.fatal(
            _(err) % {"sp": sp.get_new_map_instance(None).get_type(), "i": sp.get_id()}
        )

    if no_header is False:
        cols = (
            ["id", "semantic_label", "start", "end"]
            if type == "strds"
            else ["id", "start", "end"]
        )
        if zones:
            cols.append("zone")
        cols.extend(
            [
                "mean",
                "min",
                "max",
                "mean_of_abs",
                "stddev",
                "variance",
                "coeff_var",
                "sum",
                "null_cells",
                "cells",
                "non_null_cells",
            ]
        )
        if extended is True:
            cols.extend(["first_quartile", "median", "third_quartile"])
            if percentile:
                cols.extend(
                    [
                        f"percentile_{str(perc).rstrip('0').rstrip('.').replace('.','_')}"
                        for perc in percentile
                    ]
                )
        string = fs.join(cols)

        if output is None:
            print(string)
        else:
            out_file.write(string + "\n")

    # Define flags
    flag = "g"
    if extended is True:
        flag += "e"
    if type == "strds" and rast_region is True:
        flag += "r"

    # Setup pygrass module to use for computation
    univar_module = Module(
        "r.univar" if type == "strds" else "r3.univar",
        flags=flag,
        zones=zones,
        percentile=percentile,
        stdout_=PIPE,
        run_=False,
    )

    if nprocs == 1:
        strings = [
            compute_univar_stats(
                row,
                univar_module,
                fs,
            )
            for row in rows
        ]
    else:
        with Pool(min(nprocs, len(rows))) as pool:
            strings = pool.starmap(
                compute_univar_stats, [(dict(row), univar_module, fs) for row in rows]
            )

    if output is None:
        print("\n".join(filter(None, strings)))
    else:
        out_file.write("\n".join(filter(None, strings)))

    dbif.close()

    if output is not None:
        out_file.close()


###############################################################################
