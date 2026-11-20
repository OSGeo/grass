#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.unregister
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Unregister raster, vector and raster3d maps from the temporal database or a specific space time dataset
# SPDX-FileCopyrightText: 2011-2017 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# %module
# % description: Unregisters raster, vector and raster3d maps from the temporal database or a specific space time dataset.
# % keyword: temporal
# % keyword: map management
# % keyword: unregister
# % keyword: time
# %end

# %option G_OPT_STDS_INPUT
# % required: no
# %end

# %option G_OPT_F_INPUT
# % key: file
# % description: Input file with map names, one per line
# % required: no
# %end

# %option G_OPT_MAP_TYPE
# % guidependency: input,maps
# %end

# %option G_OPT_MAP_INPUTS
# % description: Name(s) of existing raster, vector or raster3d map(s) to unregister
# % required: no
# %end

# %rules
# % exclusive: maps, file
# % required: file, maps
# %end

from pathlib import Path

import grass.script as gs

# lazy imports at the end of the file

############################################################################


def main() -> None:
    """Unregister datasets from the temporal database or space time dataset."""
    # Get the options
    file = options["file"]
    input_stds = options["input"]
    maps = options["maps"]
    stds_type = options["type"]

    # Do not create a temporal database just to try to unregister maps
    tgis.init(skip_db_init=True)

    mapset = tgis.get_current_mapset()

    dbif = tgis.SQLDatabaseInterfaceConnection(mapsets=mapset)
    if mapset not in dbif.tgis_mapsets:
        gs.warning(
            _(
                "Unable to connect to a temporal database in the current mapset."
                " Nothing to unregister"
            )
        )
        dbif.close()
        return
    dbif.connect()

    # In case a space time dataset is specified
    if input_stds:
        sp = tgis.open_old_stds(input_stds, stds_type, dbif)
        if sp.get_mapset() != mapset:
            dbif.close()
            gs.fatal(
                _(
                    "Maps can only be unregistered from a space time dataset in the"
                    " current mapset, but <{mapid}> is in mapset <{mapset}>"
                ).format(mapid=sp.get_id(), mapset=sp.get_mapset())
            )

    maplist = []

    dummy = tgis.RasterDataset(None)

    # Map names as single string or comma separated list of string
    if maps:
        maplist = maps.split(",")

        # Build the maplist
        maplist = [
            dummy.build_id(mapname.strip(), mapset)
            for mapname in maplist
            if mapname.strip()
        ]

    # Read the map list from file
    if file:
        try:
            file = Path(file)
            with file.open("r", encoding="utf-8") as fd:
                for line in fd:
                    mapname = line.strip()
                    if not mapname:
                        continue
                    maplist.append(dummy.build_id(mapname, mapset))
        except Exception as e:
            gs.fatal(
                _("Unable to read map list from file <{filename}>: {error}").format(
                    filename=file, error=e
                )
            )

    num_maps = len(maplist)
    update_stds_ids = set()
    statement = ""
    # Unregister already registered maps
    gs.verbose(_("Collecting SQL statements to unregister {} maps").format(num_maps))
    for count, mapid in enumerate(maplist, 1):
        if count % 10 == 0:
            gs.percent(count, num_maps, 1)

        tmap = tgis.dataset_factory(stds_type, mapid)

        # Unregister map if in database
        if tmap.is_in_db(dbif, mapset=mapset):
            # Unregister from a single dataset
            if input_stds:
                # Collect SQL statements
                statement += sp.unregister_map(map=tmap, dbif=dbif, execute=False)

            # Unregister from temporal database
            else:
                # We need to update all datasets after the removement of maps
                tmap.metadata.select(dbif, mapset=mapset)
                datasets = tmap.get_registered_stds(dbif, mapset=mapset)
                # Store all unique dataset ids in a dictionary
                if datasets:
                    update_stds_ids.update(datasets)
                # Collect SQL statements
                statement += tmap.delete(dbif=dbif, update=False, execute=False)
        else:
            gs.warning(
                _(
                    "Unable to find {map_type} map <{map_id}> in temporal database"
                ).format(map_type=tmap.get_type(), map_id=tmap.get_id()),
            )

    # Execute the collected SQL statements
    if statement:
        dbif.execute_transaction(statement, mapset=mapset)

    gs.percent(num_maps, num_maps, 1)

    # Update space time datasets
    if input_stds:
        gs.message(
            _("Unregistering maps from space time dataset <{}>").format(input_stds)
        )
        sp.update_from_registered_maps(dbif)
        sp.update_command_string(dbif=dbif)
    elif update_stds_ids:
        gs.message(_("Unregistering maps from the temporal database"))
        for count, mapid in enumerate(update_stds_ids, 1):
            sp = tgis.open_old_stds(mapid, stds_type, dbif)
            sp.update_from_registered_maps(dbif)
            gs.percent(count, len(update_stds_ids), 1)

    dbif.close()


###############################################################################

if __name__ == "__main__":
    options, flags = gs.parser()

    # lazy imports
    import grass.temporal as tgis

    tgis.profile_function(main)
