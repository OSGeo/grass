#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.merge
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Merge several space time datasets into a single one
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
# % description: Merges several space time datasets into a single space time dataset.
# % keyword: temporal
# % keyword: time management
# % keyword: merge
# % keyword: time
# %end

# %option G_OPT_STDS_INPUTS
# %end

# %option G_OPT_STDS_OUTPUT
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: inputs
# % guisection: Required
# %end

import grass.script as grass


############################################################################
grass.set_raise_on_error(True)


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    inputs = options["inputs"]
    output = options["output"]
    type = options["type"]

    # Make sure the temporal database exists
    tgis.init()

    # Get the current mapset to create the id of the space time dataset
    mapset = grass.gisenv()["MAPSET"]

    inputs_split = inputs.split(",")
    input_ids = []

    for input in inputs_split:
        if input.find("@") >= 0:
            input_ids.append(input)
        else:
            input_ids.append(input + "@" + mapset)

    # Set the output name correct
    if output.find("@") >= 0:
        out_mapset = output.split("@")[1]
        if out_mapset != mapset:
            grass.fatal(
                _("Output space time dataset <%s> must be located in this mapset")
                % (output)
            )
    else:
        output_id = output + "@" + mapset

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds_list = []
    first = None

    for id in input_ids:
        stds = tgis.open_old_stds(id, type, dbif)
        if first is None:
            first = stds

        if first.get_temporal_type() != stds.get_temporal_type():
            dbif.close()
            grass.fatal(
                _("Space time datasets to merge must have the same temporal type")
            )

        stds_list.append(stds)

    # Do nothing if nothing to merge
    if first is None:
        dbif.close()
        return

    # Check if the new id is in the database
    output_stds = tgis.dataset_factory(type, output_id)
    output_exists = output_stds.is_in_db(dbif=dbif)

    if output_exists and not grass.overwrite():
        dbif.close()
        grass.fatal(
            _(
                "Unable to merge maps into space time %s dataset <%s> "
                "please use the overwrite flag."
            )
            % (stds.get_new_map_instance(None).get_type(), output_id)
        )

    if not output_exists:
        output_stds = tgis.open_new_stds(
            output,
            type,
            first.get_temporal_type(),
            "Merged space time dataset",
            "Merged space time dataset",
            "mean",
            dbif=dbif,
            overwrite=False,
        )
    else:
        output_stds.select(dbif=dbif)

    registered_output_maps = {}
    # Maps that are already registered in an existing dataset
    # are not registered again
    if output_exists:
        rows = output_stds.get_registered_maps(columns="id", dbif=dbif)
        if rows:
            for row in rows:
                registered_output_maps[row["id"]] = row["id"]

    for stds in stds_list:
        # Avoid merging of already registered maps
        if stds.get_id() != output_stds.get_id():
            maps = stds.get_registered_maps_as_objects(dbif=dbif)

            if maps:
                for map in maps:
                    # Jump over already registered maps
                    if map.get_id() in registered_output_maps:
                        continue

                    map.select(dbif=dbif)
                    output_stds.register_map(map=map, dbif=dbif)
                    # Update the registered map list
                    registered_output_maps[map.get_id()] = map.get_id()

    output_stds.update_from_registered_maps(dbif=dbif)

    if output_exists:
        output_stds.update_command_string(dbif=dbif)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
