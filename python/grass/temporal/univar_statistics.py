"""
Univariate statistic function for space time datasets

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.print_gridded_dataset_univar_statistics(
        type, input, output, where, extended, no_header, fs, rast_region
    )

..

(C) 2012-2024 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from multiprocessing import Pool
from subprocess import PIPE

import grass.script as gs
from grass.pygrass.modules import Module

from .core import SQLDatabaseInterfaceConnection, get_current_mapset
from .factory import dataset_factory
from .open_stds import open_old_stds

###############################################################################


def compute_univar_stats(
    registered_map_info, stats_module, fs, rast_region: bool = False
):
    """Compute univariate statistics for a map of a space time raster or raster3d
    dataset

    :param registered_map_info: dict or db row with tgis info for a registered map
    :param stats_module: Pre-configured PyGRASS Module to compute univariate statistics
                        with
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
    if rast_region and (stats_module.inputs.zones or stats_module.name == "r3.univar"):
        stats_module.env = gs.region_env(raster=id)
    stats_module.run()

    univar_stats = stats_module.outputs.stdout

    if not univar_stats:
        gs.warning(
            _("Unable to get statistics for raster map <%s>") % id
            if stats_module.name == "r.univar"
            else _("Unable to get statistics for 3d raster map <%s>") % id
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
                        "percentile_"
                        f"{str(perc).rstrip('0').rstrip('.').replace('.', '_')}"
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
    no_header: bool = False,
    fs: str = "|",
    rast_region: bool = False,
    region_relation=None,
    zones=None,
    percentile=None,
    nprocs: int = 1,
) -> None:
    """Print univariate statistics for a space time raster or raster3d dataset.
    Returns None if the space time raster dataset is empty or if applied
    filters (where, region_relation) do not return any maps to process.

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
    :param region_relation: Process only maps with the given spatial relation
           to the computational region. A string with one of the following values:
           "overlaps": maps that spatially overlap ("intersect")
                       within the provided spatial extent
           "is_contained": maps that are fully within the provided spatial extent
           "contains": maps that contain (fully cover) the provided spatial extent
    :param zones: raster map with zones to calculate statistics for
    """
    # We need a database interface
    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = open_old_stds(input, type, dbif)

    spatial_extent = None
    if region_relation:
        spatial_extent = gs.parse_command("g.region", flags="3gu")

    strds_cols = (
        "id,start_time,end_time,semantic_label"
        if type == "strds"
        else "id,start_time,end_time"
    )
    rows = sp.get_registered_maps(
        strds_cols,
        where,
        "start_time",
        dbif,
        spatial_extent=spatial_extent,
        spatial_relation=region_relation,
    )

    if not rows and rows != [""]:
        dbif.close()
        gs.verbose(
            _(
                "No maps found to process. "
                "Space time {type} dataset <{id}> is either empty "
                "or the where condition (if used) does not return any maps "
                "or no maps with the requested spatial relation to the "
                "computational region exist in the dataset."
            ).format(type=sp.get_new_map_instance(None).get_type(), id=sp.get_id())
        )

        return

    if output is not None:
        out_file = open(output, "w")

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
                        "percentile_"
                        f"{str(perc).rstrip('0').rstrip('.').replace('.', '_')}"
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
    if type == "strds" and rast_region is True and not zones:
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


def print_vector_dataset_univar_statistics(
    input,
    output,
    twhere,
    layer,
    type,
    column,
    where,
    extended,
    no_header: bool = False,
    fs: str = "|",
) -> None:
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

    id = input if input.find("@") >= 0 else input + "@" + mapset

    sp = dataset_factory("stvds", id)

    if sp.is_in_db(dbif) is False:
        dbif.close()
        gs.fatal(
            _("Space time %(sp)s dataset <%(i)s> not found")
            % {"sp": sp.get_new_map_instance(None).get_type(), "i": id}
        )

    sp.select(dbif)

    rows = sp.get_registered_maps(
        "id,name,mapset,start_time,end_time,layer", twhere, "start_time", dbif
    )

    if not rows:
        dbif.close()
        gs.fatal(
            _("Space time %(sp)s dataset <%(i)s> is empty")
            % {"sp": sp.get_new_map_instance(None).get_type(), "i": id}
        )

    string = ""
    if no_header is False:
        string += (
            "id"
            + fs
            + "start"
            + fs
            + "end"
            + fs
            + "n"
            + fs
            + "nmissing"
            + fs
            + "nnull"
            + fs
        )
        string += "min" + fs + "max" + fs + "range"
        if type in {"point", "centroid"}:
            string += (
                fs
                + "mean"
                + fs
                + "mean_abs"
                + fs
                + "population_stddev"
                + fs
                + "population_variance"
                + fs
            )
            string += (
                "population_coeff_variation"
                + fs
                + "sample_stddev"
                + fs
                + "sample_variance"
                + fs
            )
            string += "kurtosis" + fs + "skewness"
            if extended is True:
                string += (
                    fs
                    + "first_quartile"
                    + fs
                    + "median"
                    + fs
                    + "third_quartile"
                    + fs
                    + "percentile_90"
                )

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

        stats = gs.parse_command(
            "v.univar",
            map=id,
            where=where,
            column=column,
            layer=mylayer,
            type=type,
            flags=flags,
        )

        string = ""

        if not stats:
            gs.warning(_("Unable to get statistics for vector map <%s>") % id)
            continue

        string += str(id) + fs + str(start) + fs + str(end)
        string += (
            fs
            + str(stats["n"])
            + fs
            + str(stats["nmissing"])
            + fs
            + str(stats["nnull"])
        )
        if "min" in stats:
            string += (
                fs
                + str(stats["min"])
                + fs
                + str(stats["max"])
                + fs
                + str(stats["range"])
            )
        else:
            string += fs + fs + fs

        if type in {"point", "centroid"}:
            if "mean" in stats:
                string += (
                    fs
                    + str(stats["mean"])
                    + fs
                    + str(stats["mean_abs"])
                    + fs
                    + str(stats["population_stddev"])
                    + fs
                    + str(stats["population_variance"])
                )

                string += (
                    fs
                    + str(stats["population_coeff_variation"])
                    + fs
                    + str(stats["sample_stddev"])
                    + fs
                    + str(stats["sample_variance"])
                )

                string += fs + str(stats["kurtosis"]) + fs + str(stats["skewness"])
            else:
                string += fs + fs + fs + fs + fs + fs + fs + fs + fs
            if extended is True:
                if "first_quartile" in stats:
                    string += (
                        fs
                        + str(stats["first_quartile"])
                        + fs
                        + str(stats["median"])
                        + fs
                        + str(stats["third_quartile"])
                        + fs
                        + str(stats["percentile_90"])
                    )
                else:
                    string += fs + fs + fs + fs

        if output is None:
            print(string)
        else:
            out_file.write(string + "\n")

    dbif.close()

    if output is not None:
        out_file.close()
