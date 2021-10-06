#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.copy
# AUTHOR(S):	Markus Metz
#
# PURPOSE:	Create a copy of a space time dataset
# COPYRIGHT:	(C) 2021 by the GRASS Development Team
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
# % description: Creates a copy of a space time raster dataset.
# % keyword: temporal
# % keyword: copy
# % keyword: time
# %end

# %option G_OPT_STDS_INPUT
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: input
# % guisection: Required
# % options: strds, str3ds, stvds
# %end

# %option G_OPT_STDS_OUTPUT
# %end

# %flag
# % key: c
# % label: Also copy maps of the space-time dataset
# % description: By default the old maps are only registered in the new dataset.
# %end


import grass.script as gscript


############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    stdstype = options["type"]
    copy_maps = flags["c"]

    maptype = "raster"
    element = "cell"
    if stdstype == "str3ds":
        maptype = "raster_3d"
        element = "g3dcell"
    elif stdstype == "stvds":
        maptype = "vector"
        element = "vector"

    # Make sure the temporal database exists
    tgis.init()

    # Get the current mapset to create the id of the space time dataset
    mapset = gscript.gisenv()["MAPSET"]

    inname = input
    inmapset = None
    if "@" in input:
        inname, inmapset = input.split("@")

    outname = output
    outmapset = mapset
    if "@" in output:
        outname, outmapset = output.split("@")
        if outmapset != mapset:
            gscript.fatal(
                _("The output dataset <%s> must be in the current mapset<%s>.")
                % (input, mapset)
            )

    # simplified version of extract_dataset() in temporal/extract.py
    msgr = tgis.get_tgis_message_interface()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    old_sp = tgis.open_old_stds(input, stdstype, dbif)
    # Check the new stds
    new_sp = tgis.check_new_stds(output, stdstype, dbif, gscript.overwrite())
    if stdstype == "stvds":
        rows = old_sp.get_registered_maps(
            "id,name,mapset,layer", None, "start_time", dbif
        )
    else:
        rows = old_sp.get_registered_maps("id,name,mapset", None, "start_time", dbif)

    if rows is None or len(rows) == 0:
        gscript.message(
            _("Empty space-time %s dataset <%s>, nothing to copy") % (maptype, input)
        )
        dbif.close()
        return

    new_maps = {}
    num_rows = len(rows)

    msgr.percent(0, num_rows, 1)

    if copy_maps:
        count = 0

        for row in rows:
            count += 1

            if row["mapset"] != mapset:
                found = gscript.find_file(
                    name=row["name"], element=element, mapset=mapset
                )
                if found["name"] is not None and len(found["name"]) > 0:
                    gscript.fatal(
                        _("A %s map <%s> exists already in the current mapset <%s>.")
                        % (maptype, row["name"], mapset)
                    )

                kwargs = {maptype: "%s,%s" % (row["id"], row["name"])}
                gscript.run_command("g.copy", **kwargs)
            else:
                # the map is already in the current mapset
                gscript.message(
                    _("The %s map <%s> is already in the current mapset, not copying")
                    % (maptype, row["name"])
                )

            if count % 10 == 0:
                msgr.percent(count, num_rows, 1)

            # We need to build the id
            if maptype != "vector":
                map_id = tgis.AbstractMapDataset.build_id(row["name"], mapset)
            else:
                map_id = tgis.AbstractMapDataset.build_id(
                    row["name"], mapset, row["layer"]
                )

            new_map = old_sp.get_new_map_instance(map_id)

            # Check if new map is in the temporal database
            if new_map.is_in_db(dbif, mapset):
                if gscript.overwrite():
                    # Remove the existing temporal database entry
                    new_map.delete(dbif)
                    new_map = old_sp.get_new_map_instance(map_id)
                else:
                    msgr.error(
                        _(
                            "Map <%s> is already in temporal database"
                            ", use overwrite flag to overwrite"
                        )
                        % (new_map.get_map_id())
                    )
                    continue

            # Store the new maps
            new_maps[row["id"]] = new_map

    msgr.percent(0, num_rows, 1)

    temporal_type, semantic_type, title, description = old_sp.get_initial_values()
    new_sp = tgis.open_new_stds(
        output,
        stdstype,
        old_sp.get_temporal_type(),
        title,
        description,
        semantic_type,
        dbif,
        gscript.overwrite(),
    )

    # Register the maps in the database
    count = 0
    for row in rows:
        count += 1

        if count % 10 == 0:
            msgr.percent(count, num_rows, 1)

        old_map = old_sp.get_new_map_instance(row["id"])
        old_map.select(dbif)

        if copy_maps:
            # Register the new maps
            if row["id"] in new_maps:
                new_map = new_maps[row["id"]]

                # Read the raster map data
                new_map.load()

                # Set the time stamp
                new_map.set_temporal_extent(old_map.get_temporal_extent())

                if maptype == "raster":
                    # Set the band reference
                    band_reference = old_map.metadata.get_band_reference()
                    if band_reference is not None:
                        new_map.set_band_reference(band_reference)

                # Insert map in temporal database
                new_map.insert(dbif)

                new_sp.register_map(new_map, dbif)
        else:
            new_sp.register_map(old_map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    new_sp.update_from_registered_maps(dbif)

    msgr.percent(num_rows, num_rows, 1)

    dbif.close()


###############################################################################

if __name__ == "__main__":
    options, flags = gscript.parser()
    main()
