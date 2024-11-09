#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.rename
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Renames a space time dataset
# COPYRIGHT:    (C) 2011-2017 by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
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

    # Make sure the temporal database exists
    tgis.init()

    # Get the current mapset to create the id of the space time dataset
    mapset = gs.gisenv()["MAPSET"]

    old_id = input if input.find("@") >= 0 else input + "@" + mapset
    new_id = output if output.find("@") >= 0 else output + "@" + mapset

    # Do not overwrite yourself
    if new_id == old_id:
        return

    dbif = tgis.SQLDatabaseInterfaceConnection()
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

    if not stds.is_in_db(dbif=dbif):
        dbif.close()
        gs.fatal(
            _("Space time %s dataset <%s> not found")
            % (stds.get_new_map_instance(None).get_type(), old_id)
        )

    # Check if the new id is in the database
    new_stds = tgis.dataset_factory(type, new_id)

    if new_stds.is_in_db(dbif=dbif) and not gs.overwrite():
        dbif.close()
        gs.fatal(
            _(
                "Unable to rename Space time %s dataset <%s>. Name <%s> "
                "is in use, please use the overwrite flag."
            )
            % (stds.get_new_map_instance(None).get_type(), old_id, new_id)
        )

    # Remove an already existing space time dataset
    if new_stds.is_in_db(dbif=dbif):
        new_stds.delete(dbif=dbif)

    stds.select(dbif=dbif)
    stds.rename(ident=new_id, dbif=dbif)
    stds.update_command_string(dbif=dbif)


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
