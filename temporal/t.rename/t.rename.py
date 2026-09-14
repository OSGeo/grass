#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.rename
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Renames a space time dataset
# SPDX-FileCopyrightText: 2011-2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# %module
# % description: Renames a space time dataset
# % keyword: temporal
# % keyword: map management
# % keyword: rename
# % keyword: time
# %end

# %option G_OPT_STDS_INPUT
# %end

# %option G_OPT_STDS_OUTPUT
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: input
# % guisection: Required
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    type = options["type"]

    # Get the current mapset to create the id of the space time dataset
    mapset = gs.gisenv()["MAPSET"]

    old_id = input if "@" in input else f"{input}@{mapset}"
    new_id = output if "@" in output else f"{output}@{mapset}"

    # Do not overwrite yourself
    if new_id == old_id:
        return

    # Try initializing the temporal database in the current mapset
    tgis.init(skip_db_init=True)
    dbif = tgis.SQLDatabaseInterfaceConnection(mapsets=mapset)
    if not dbif.tgis_mapsets:
        gs.fatal(_("No temporal database found in the current mapset."))
    dbif.connect()

    stds = tgis.dataset_factory(type, old_id)

    if new_id.split("@")[1] != mapset:
        gs.fatal(
            _(
                "Space time %s dataset <%s> can not be renamed. "
                "Mapset of the new identifier differs from the current "
                "mapset."
            )
            % (stds.get_new_map_instance(None).get_type(), old_id)
        )

    if not stds.is_in_db(dbif=dbif, mapset=mapset):
        dbif.close()
        gs.fatal(
            _("Space time %s dataset <%s> not found")
            % (stds.get_new_map_instance(None).get_type(), old_id)
        )

    # Check if the new id is in the database
    new_stds = tgis.dataset_factory(type, new_id)

    if new_stds.is_in_db(dbif=dbif, mapset=mapset):
        if not gs.overwrite():
            dbif.close()
            gs.fatal(
                _(
                    "Unable to rename Space time %s dataset <%s>. Name <%s> "
                    "is in use, please use the overwrite flag."
                )
                % (stds.get_new_map_instance(None).get_type(), old_id, new_id)
            )

        # Remove an already existing space time dataset
        new_stds.delete(dbif=dbif)

    stds.select(dbif=dbif, mapset=mapset)
    stds.rename(ident=new_id, dbif=dbif)
    stds.update_command_string(dbif=dbif)


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
