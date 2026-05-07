"""Extract functions for space time raster, 3d raster and vector datasets.

(C) 2012-2026 by the GRASS Development Team
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
    get_tgis_db_version,
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
) -> str:
    """Compile new map name with suffix and semantic label.

    :param sp: An open SpaceTimeDataSet (STDS)
    :param count: Running number of the map to be used as numeric suffix (if not time suffix)
    :param map_id: Map ID to compile new map name for
    :param time_suffix: Type of time suffix to use (or None)
    :param dbif: initialized TGIS database interface
    """
    if semantic_label:
        base = f"{base}_{semantic_label}"
    if (
        sp.get_temporal_type() != "absolute"
        or not time_suffix
        or time_suffix.startswith("num")
    ):
        return create_numeric_suffix(base, count, time_suffix)
    old_map = sp.get_new_map_instance(map_id)
    old_map.select(dbif)
    if time_suffix == "gran":
        suffix = create_suffix_from_datetime(
            old_map.temporal_extent.get_start_time(),
            sp.get_granularity(),
        )
    else:
        suffix = create_time_suffix(old_map)
    return f"{base}_{suffix}"


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
    new_expression += expression[old_idx:]
    return new_expression


def extract_dataset(
    input: str,
    output: str,
    type: str,
    where: str,
    expression: str,
    base: str,
    time_suffix: str,
    nprocs: int = 1,
    register_null: bool = False,
    layer: int = 1,
    vtype: str = "point,line,boundary,centroid,area,face",
) -> None:
    """Extract a subset of a space time raster, raster3d or vector dataset.

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

    tgis_version = get_tgis_db_version()

    sp = open_old_stds(input, type, dbif)
    has_semantic_labels = bool(
        tgis_version > 2 and type == "raster" and sp.metadata.semantic_labels,
    )

    # Check the new stds
    new_sp = check_new_stds(output, type, dbif, gs.overwrite())
    if type == "vector":
        rows = sp.get_registered_maps("id,name,mapset,layer", where, "start_time", dbif)
    else:
        rows = sp.get_registered_maps(
            f"id{',semantic_label' if has_semantic_labels else ''}",
            where,
            "start_time",
            dbif,
        )

    new_maps = {}
    if not rows:
        gs.warning(
            _(
                "Nothing found in the database for space time dataset <{name}> "
                "(type: {element_type}): {detail}",
            ).format(
                name=input,
                element_type=type,
                detail=(
                    _(
                        "Dataset is empty or where clause is too constrained or "
                        "incorrect",
                    )
                    if where
                    else _("Dataset is empty")
                ),
            ),
        )
        dbif.close()
        return

    num_rows = len(rows)

    msgr.percent(0, num_rows, 1)

    # Run the mapcalc expression
    if expression:
        proc_count = 0
        proc_list = []

        # Make sure STRDS is in the expression referenced with fully qualified name
        expression = replace_stds_names(
            expression,
            sp.base.get_name(),
            sp.base.get_map_id(),
        )
        for count, row in enumerate(rows, 1):
            if count % 10 == 0:
                msgr.percent(count, num_rows, 1)

            map_name = compile_new_map_name(
                sp,
                base,
                count,
                row["id"],
                row["semantic_label"] if has_semantic_labels else None,
                time_suffix,
                dbif,
            )

            # We need to modify the r(3).mapcalc expression
            if type != "vector":
                expr = expression
                expr = expr.replace(sp.base.get_map_id(), row["id"])
                expr = f"{map_name} = {expr}"

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
                            ", use overwrite flag to overwrite",
                        )
                        % (new_map.get_map_id()),
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
                msgr.verbose(_('Applying v.extract where statement: "%s"') % expression)
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
                        ),
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
                        ),
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

    _temporal_type, semantic_type, title, description = sp.get_initial_values()
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
    for count, row in enumerate(rows, 1):
        if count % 10 == 0:
            msgr.percent(count, num_rows, 1)

        old_map = sp.get_new_map_instance(row["id"])
        old_map.select(dbif)

        if not expression:
            # Just register the source map in the new STRDS if no
            # the expression option is not used
            if not old_map.is_in_db():
                # Maps that are not part of the temporal database
                # in the current mapset need to be inserted first
                old_map.insert(dbif)
            new_sp.register_map(old_map, dbif)
            continue

        # Register the new maps
        if row["id"] in new_maps:
            new_map = new_maps[row["id"]]

            # Read the raster map data
            new_map.load()

            # In case of a empty map continue, do not register empty
            # maps
            if not register_null and (
                (
                    type in {"raster", "raster3d"}
                    and new_map.metadata.get_min() is None
                    and new_map.metadata.get_max() is None
                )
                or (
                    type == "vector" and not new_map.metadata.get_number_of_primitives()
                )
            ):
                empty_maps.append(new_map)
                continue

            # Set the time stamp
            new_map.set_temporal_extent(old_map.get_temporal_extent())

            if type == "raster" and has_semantic_labels:
                # Set the semantic label
                new_map.set_semantic_label(row["semantic_label"])

            # Insert map in temporal database
            new_map.insert(dbif)
            new_sp.register_map(new_map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    new_sp.update_from_registered_maps(dbif)

    msgr.percent(num_rows, num_rows, 1)

    # Remove empty maps
    if len(empty_maps) > 0:
        gs.run_command(
            "g.remove",
            flags="f",
            type=type.replace("_", ""),
            name=",".join(
                [
                    empty_map.get_name()
                    for empty_map in empty_maps
                    if empty_map.get_name()
                ]
            ),
            superquiet=True,
        )

    dbif.close()


###############################################################################
# Helper functions


def run_mapcalc2d(expr: str) -> None:
    """Run r.mapcalc on a single core in quiet mode.

    Wrapper function setting nprocs to 1 and verbosity to quiet. It is
    intended to be run in parallel in `extract_dataset`.
    """
    try:
        gs.run_command(
            "r.mapcalc",
            expression=expr,
            nprocs=1,
            overwrite=gs.overwrite(),
            quiet=True,
        )
    except CalledModuleError:
        sys.exit(1)


def run_mapcalc3d(expr: str) -> None:
    """Run r3.mapcalc on a single core in quiet mode.

    Wrapper function setting nprocs to 1 and verbosity to quiet. It is
    intended to be run in parallel in `extract_dataset`.
    """
    try:
        gs.run_command(
            "r3.mapcalc",
            expression=expr,
            nprocs=1,
            overwrite=gs.overwrite(),
            quiet=True,
        )
    except CalledModuleError:
        sys.exit(1)


def run_vector_extraction(
    input_map: str,
    output: str,
    layer: str | int,
    type: str,
    where: str,
) -> None:
    """Run v.extract in quiet mode.

    Wrapper function setting verbosity to quiet. It is intended to be
    run in parallel in `extract_dataset`. Parallel execution requires
    a database backend that support it, like PostgreSQL.
    """
    try:
        gs.run_command(
            "v.extract",
            input=input_map,
            output=output,
            layer=layer,
            type=type,
            where=where,
            overwrite=gs.overwrite(),
            quiet=True,
        )
    except CalledModuleError:
        sys.exit(1)
