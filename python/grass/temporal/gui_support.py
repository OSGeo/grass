"""
GUI support functions


SPDX-FileCopyrightText: 2008-2011 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

:authors: Soeren Gebbert
"""

import grass.script as gs
from grass.exceptions import ScriptError

from .core import init_dbif
from .factory import dataset_factory

###############################################################################


def tlist_grouped(type, group_type: bool = False, dbif=None):
    """List of temporal elements grouped by mapsets.

    Returns a dictionary where the keys are mapset
    names and the values are lists of space time datasets in that
    mapset.

    :Example:
      .. code-block:: pycon

        >>> import grass.temporal as tgis
        >>> tgis.tlist_grouped("strds")["PERMANENT"]
        ['precipitation', 'temperature']

    :param type: element type (strds, str3ds, stvds)
    :param group_type: TBD

    :return: directory of mapsets/elements
    """
    result = {}
    type_ = type
    dbif, connection_state_changed = init_dbif(dbif)

    mapset = None
    types = ["strds", "str3ds", "stvds"] if type_ == "stds" else [type_]
    for type_ in types:
        try:
            tlist_result = tlist(type=type_, dbif=dbif)
        except ScriptError as e:
            gs.warning(e)
            continue

        for line in tlist_result:
            try:
                name, mapset = line.split("@")
            except ValueError:
                gs.warning(_("Invalid element '%s'") % line)
                continue

            if mapset not in result:
                if group_type:
                    result[mapset] = {}
                else:
                    result[mapset] = []

            if group_type:
                if type_ in result[mapset]:
                    result[mapset][type_].append(name)
                else:
                    result[mapset][type_] = [
                        name,
                    ]
            else:
                result[mapset].append(name)

    if connection_state_changed is True:
        dbif.close()

    return result


###############################################################################


def tlist(type, dbif=None):
    """Return a list of space time datasets of absolute and relative time

    :param type: element type (strds, str3ds, stvds)

    :return: a list of space time dataset ids
    """
    type_ = type
    id = None
    sp = dataset_factory(type_, id)
    dbif, connection_state_changed = init_dbif(dbif)

    mapsets = dbif.tgis_mapsets

    output = []
    temporal_type = ["absolute", "relative"]
    for type_ in temporal_type:
        # For each available mapset
        for mapset in mapsets.keys():
            # Table name
            if type_ == "absolute":
                table = sp.get_type() + "_view_abs_time"
            else:
                table = sp.get_type() + "_view_rel_time"

            # Create the sql selection statement
            sql = "SELECT id FROM " + table
            sql += " WHERE mapset = '%s'" % (mapset)
            sql += " ORDER BY id"

            dbif.execute(sql, mapset=mapset)
            rows = dbif.fetchall(mapset=mapset)

            # Append the ids of the space time datasets
            for row in rows:
                for col in row:
                    output.append(str(col))

    if connection_state_changed is True:
        dbif.close()

    return output


###############################################################################


def registered_maps_grouped(dbif=None):
    """Return maps registered in space time datasets, grouped by dataset.

    Scans all mapsets covered by *dbif* (the temporal database of the
    current search path for a default connection) and returns a dictionary
    with the dataset types "strds", "stvds" and "str3ds" as keys. Each
    value is a dictionary mapping full dataset ids (name@mapset) to the
    list of maps registered in that dataset, ordered by start time. Each
    map is described by a dictionary with the keys "id", "start_time",
    "end_time" and "unit" ("unit" is None for maps with absolute time).

    Time stamped maps that are not registered in any space time dataset
    are not included. Datasets without registered maps are not included
    either; use tlist_grouped() to list the datasets themselves.

    :param dbif: The database interface to be used

    :return: nested dictionary {dataset type: {dataset id: list of maps}}
    """
    stds_map_types = {"strds": "raster", "stvds": "vector", "str3ds": "raster3d"}
    result = {stds_type: {} for stds_type in stds_map_types}
    dbif, connection_state_changed = init_dbif(dbif)

    try:
        for stds_type, map_type in stds_map_types.items():
            datasets = result[stds_type]
            # One row per map registered in at least one dataset. A map has
            # either absolute or relative time, so exactly one of the joined
            # start times is not NULL.
            sql = (
                "SELECT reg.id AS id, reg.registered_stds AS registered_stds,"
                " abs_t.start_time AS abs_start, abs_t.end_time AS abs_end,"
                " rel_t.start_time AS rel_start, rel_t.end_time AS rel_end,"
                " rel_t.unit AS unit"
                f" FROM {map_type}_stds_register AS reg"
                f" LEFT JOIN {map_type}_absolute_time AS abs_t ON reg.id = abs_t.id"
                f" LEFT JOIN {map_type}_relative_time AS rel_t ON reg.id = rel_t.id"
                " WHERE reg.registered_stds IS NOT NULL"
                " AND reg.registered_stds != ''"
            )
            for mapset in dbif.tgis_mapsets:
                dbif.execute(sql, mapset=mapset)
                for row in dbif.fetchall(mapset=mapset):
                    absolute = row["abs_start"] is not None
                    map_info = {
                        "id": row["id"],
                        "start_time": row["abs_start"]
                        if absolute
                        else row["rel_start"],
                        "end_time": row["abs_end"] if absolute else row["rel_end"],
                        "unit": None if absolute else row["unit"],
                    }
                    for stds_id in row["registered_stds"].split(","):
                        if stds_id:
                            datasets.setdefault(stds_id, []).append(map_info)

        # A dataset is homogeneous in temporal type, so its start times are
        # mutually comparable; maps without a start time are listed last.
        def sort_key(map_info):
            start = map_info["start_time"]
            return (start is None, 0 if start is None else start)

        for datasets in result.values():
            for maps in datasets.values():
                maps.sort(key=sort_key)
    finally:
        if connection_state_changed:
            dbif.close()

    return result
