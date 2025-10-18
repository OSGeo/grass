"""
Extract functions for space time raster, 3d raster and vector datasets

(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from __future__ import annotations

import re
import sys
from multiprocessing import Process

import grass.script as gs
from grass.exceptions import CalledModuleError

from .abstract_map_dataset import AbstractMapDataset

from .core import (
    SQLDatabaseInterfaceConnection,
    get_current_mapset,
    get_tgis_message_interface,
)
from .datetime_math import (
    create_numeric_suffix,
    create_suffix_from_datetime,
    create_time_suffix,
)
from .open_stds import check_new_stds, open_new_stds, open_old_stds

############################################################################


def compile_new_map_name(
    sp,
    base: str,
    count: int,
    map_id: str,
    semantic_label: str | None,
    time_suffix: str | None,
    dbif: SQLDatabaseInterfaceConnection,
):
    """Compile new map name with suffix and semantic label.

    :param sp: An open SpaceTimeDataSet (STDS)
    :param count: Running number of the map to be used as numeric suffix (if not time suffix)
    :param map_id: Map ID to compile new map name for
    :param time_suffix: Type of time suffix to use (or None)
    :param dbif: initialized TGIS database interface
    """
    if semantic_label:
        base = f"{base}_{semantic_label}"
    if sp.get_temporal_type() == "absolute" and time_suffix:
        old_map = sp.get_new_map_instance(map_id)
        old_map.select(dbif)
        if time_suffix == "gran":
            suffix = create_suffix_from_datetime(
                old_map.temporal_extent.get_start_time(), sp.get_granularity()
            )
        elif time_suffix == "time":
            suffix = create_time_suffix(old_map)
        return f"{base}_{suffix}"
    return create_numeric_suffix(base, count, time_suffix)


def replace_stds_names(expression: str, simple_name: str, full_name: str) -> str:
    """Safely replace simple with full STDS names.

     When users provide inconsistent input for STDS in the expression
     (with and without mapset componenet) or if the STDS name is part
     of the name of other raster maps in the expression, the final
     mapcalc expression may become invalid when the STDS name later is
     replaced with the name of the individual maps in the time series.
     The replacement with the fully qualified STDS names avoids that
     confusion.

    :param expression: The mapcalc expression to replace names in
    :param simple_name: STDS name *without* mapset component
    :param full_name: STDS name *with* mapset component
    """
    separators = r""" ,-+*/^:&|"'`()<>#^"""
    name_matches = re.finditer(simple_name, expression)
    new_expression = ""
    old_idx = 0
    for match in name_matches:
        # Fill-in expression component between matches
        new_expression += expression[old_idx : match.start()]
        # Only replace STDS name if pre- and succeeded by a separator
        # Match is either at the start or preceeeded by a separator
        if match.start() == 0 or expression[match.start() - 1] in separators:
            # Match is either at the end or succeeded by a separator
            if (
                match.end() + 1 > len(expression)
                or expression[match.end()] in separators
            ):
                new_expression += full_name
            else:
                new_expression += simple_name
        else:
            new_expression += simple_name
        old_idx = match.end()
    new_expression += expression[match.end() :]
    return new_expression


def extract_dataset(
    input,
    output,
    type,
    where,
    expression,
    base,
    time_suffix,
    nprocs: int = 1,
    register_null: bool = False,
    layer: int = 1,
    vtype="point,line,boundary,centroid,area,face",
) -> None:
    """Extract a subset of a space time raster, raster3d or vector dataset

    A mapcalc expression can be provided to process the temporal extracted
    maps.
    Mapcalc expressions are supported for raster and raster3d maps.

    :param input: The name of the input space time raster/raster3d dataset
    :param output: The name of the extracted new space time raster/raster3d
                  dataset
    :param type: The type of the dataset: "raster", "raster3d" or vector
    :param where: The temporal SQL WHERE statement for subset extraction
    :param expression: The r(3).mapcalc expression or the v.extract where
                      statement
    :param base: The base name of the new created maps in case a mapclac
                expression is provided
    :param time_suffix: string to choose which suffix to use: gran, time, num%*
                       (where * are digits)
    :param nprocs: The number of parallel processes to be used for mapcalc
                  processing
    :param register_null: Set this number True to register empty maps
                         (only raster and raster3d maps)
    :param layer: The vector layer number to be used when no timestamped
           layer is present, default is 1
    :param vtype: The feature type to be extracted for vector maps, default
           is point,line,boundary,centroid,area and face
    """

    # Check the parameters
    msgr = get_tgis_message_interface()

    if expression and not base:
        msgr.fatal(_("You need to specify the base name of new created maps"))

    mapset = get_current_mapset()

    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = open_old_stds(input, type, dbif)
    # Check the new stds
    new_sp = check_new_stds(output, type, dbif, gs.overwrite())
    if type == "vector":
        rows = sp.get_registered_maps(
            "id,name,mapset,layer,semantic_label", where, "start_time", dbif
        )
    else:
        rows = sp.get_registered_maps("id,semantic_label", where, "start_time", dbif)

    new_maps = {}
    if rows:
        num_rows = len(rows)

        msgr.percent(0, num_rows, 1)

        # Run the mapcalc expression
        if expression:
            count = 0
            proc_count = 0
            proc_list = []

            # Make sure STRDS is in the expression referenced with fully qualified name
            expression = replace_stds_names(
                expression, sp.base.get_name(), sp.base.get_map_id()
            )
            for row in rows:
                count += 1

                if count % 10 == 0:
                    msgr.percent(count, num_rows, 1)

                map_name = compile_new_map_name(
                    sp,
                    base,
                    count,
                    row["id"],
                    row["semantic_label"],
                    time_suffix,
                    dbif,
                )

                # We need to modify the r(3).mapcalc expression
                if type != "vector":
                    expr = expression
                    expr = expr.replace(sp.base.get_map_id(), row["id"])
                    expr = "%s = %s" % (map_name, expr)

                    # We need to build the id
                    map_id = AbstractMapDataset.build_id(map_name, mapset)
                else:
                    map_id = AbstractMapDataset.build_id(map_name, mapset, row["layer"])

                new_map = sp.get_new_map_instance(map_id)

                # Check if new map is in the temporal database
                if new_map.is_in_db(dbif):
                    if gs.overwrite():
                        # Remove the existing temporal database entry
                        new_map.delete(dbif)
                        new_map = sp.get_new_map_instance(map_id)
                    else:
                        msgr.error(
                            _(
                                "Map <%s> is already in temporal database"
                                ", use overwrite flag to overwrite"
                            )
                            % (new_map.get_map_id())
                        )
                        continue

                # Add process to the process list
                if type == "raster":
                    msgr.verbose(_('Applying r.mapcalc expression: "%s"') % expr)
                    proc_list.append(Process(target=run_mapcalc2d, args=(expr,)))
                elif type == "raster3d":
                    msgr.verbose(_('Applying r3.mapcalc expression: "%s"') % expr)
                    proc_list.append(Process(target=run_mapcalc3d, args=(expr,)))
                elif type == "vector":
                    msgr.verbose(
                        _('Applying v.extract where statement: "%s"') % expression
                    )
                    if row["layer"]:
                        proc_list.append(
                            Process(
                                target=run_vector_extraction,
                                args=(
                                    row["name"] + "@" + row["mapset"],
                                    map_name,
                                    row["layer"],
                                    vtype,
                                    expression,
                                ),
                            )
                        )
                    else:
                        proc_list.append(
                            Process(
                                target=run_vector_extraction,
                                args=(
                                    row["name"] + "@" + row["mapset"],
                                    map_name,
                                    layer,
                                    vtype,
                                    expression,
                                ),
                            )
                        )

                proc_list[proc_count].start()
                proc_count += 1

                # Join processes if the maximum number of processes are
                # reached or the end of the loop is reached
                if proc_count == nprocs or count == num_rows:
                    proc_count = 0
                    exitcodes = 0
                    for proc in proc_list:
                        proc.join()
                        exitcodes += proc.exitcode
                    if exitcodes != 0:
                        dbif.close()
                        msgr.fatal(_("Error in computation process"))

                    # Empty process list
                    proc_list = []

                # Store the new maps
                new_maps[row["id"]] = new_map

        msgr.percent(0, num_rows, 1)

        temporal_type, semantic_type, title, description = sp.get_initial_values()
        new_sp = open_new_stds(
            output,
            type,
            sp.get_temporal_type(),
            title,
            description,
            semantic_type,
            dbif,
            gs.overwrite(),
        )

        # collect empty maps to remove them
        empty_maps = []

        # Register the maps in the database
        count = 0
        for row in rows:
            count += 1

            if count % 10 == 0:
                msgr.percent(count, num_rows, 1)

            old_map = sp.get_new_map_instance(row["id"])
            old_map.select(dbif)

            if expression:
                # Register the new maps
                if row["id"] in new_maps:
                    new_map = new_maps[row["id"]]

                    # Read the raster map data
                    new_map.load()

                    # In case of a empty map continue, do not register empty
                    # maps
                    if type in {"raster", "raster3d"}:
                        if (
                            new_map.metadata.get_min() is None
                            and new_map.metadata.get_max() is None
                        ):
                            if not register_null:
                                empty_maps.append(new_map)
                                continue
                    elif type == "vector":
                        if (
                            new_map.metadata.get_number_of_primitives() == 0
                            or new_map.metadata.get_number_of_primitives() is None
                        ):
                            if not register_null:
                                empty_maps.append(new_map)
                                continue

                    # Set the time stamp
                    new_map.set_temporal_extent(old_map.get_temporal_extent())

                    if type == "raster":
                        # Set the semantic label
                        if row["semantic_label"] is not None:
                            new_map.set_semantic_label(row["semantic_label"])

                    # Insert map in temporal database
                    new_map.insert(dbif)

                    new_sp.register_map(new_map, dbif)
            else:
                new_sp.register_map(old_map, dbif)

        # Update the spatio-temporal extent and the metadata table entries
        new_sp.update_from_registered_maps(dbif)

        msgr.percent(num_rows, num_rows, 1)

        # Remove empty maps
        if len(empty_maps) > 0:
            names = ""
            count = 0
            for map in empty_maps:
                if count == 0:
                    names += "%s" % (map.get_name())
                else:
                    names += ",%s" % (map.get_name())
                count += 1
            if type == "raster":
                gs.run_command(
                    "g.remove", flags="f", type="raster", name=names, quiet=True
                )
            elif type == "raster3d":
                gs.run_command(
                    "g.remove", flags="f", type="raster_3d", name=names, quiet=True
                )
            elif type == "vector":
                gs.run_command(
                    "g.remove", flags="f", type="vector", name=names, quiet=True
                )

    dbif.close()


###############################################################################


def run_mapcalc2d(expr) -> None:
    """Helper function to run r.mapcalc in parallel"""
    try:
        gs.run_command(
            "r.mapcalc", expression=expr, overwrite=gs.overwrite(), quiet=True
        )
    except CalledModuleError:
        sys.exit(1)


def run_mapcalc3d(expr) -> None:
    """Helper function to run r3.mapcalc in parallel"""
    try:
        gs.run_command(
            "r3.mapcalc", expression=expr, overwrite=gs.overwrite(), quiet=True
        )
    except CalledModuleError:
        sys.exit(1)


def run_vector_extraction(input, output, layer, type, where) -> None:
    """Helper function to run r.mapcalc in parallel"""
    try:
        gs.run_command(
            "v.extract",
            input=input,
            output=output,
            layer=layer,
            type=type,
            where=where,
            overwrite=gs.overwrite(),
            quiet=True,
        )
    except CalledModuleError:
        sys.exit(1)
