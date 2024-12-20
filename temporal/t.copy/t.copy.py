#!/usr/bin/env python3

############################################################################
#
# MODULE:	    t.copy
# AUTHOR(S):	Markus Metz
#
# PURPOSE:	    Create a copy of a space time dataset
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


import grass.script as gs

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
    mapset = gs.gisenv()["MAPSET"]

    inname = input
    inmapset = mapset
    if "@" in input:
        inname, inmapset = input.split("@")

    outname = output
    outmapset = mapset
    if "@" in output:
        outname, outmapset = output.split("@")
        if outmapset != mapset:
            gs.fatal(
                _("The output dataset <%s> must be in the current mapset<%s>.")
                % (input, mapset)
            )

    msgr = tgis.get_tgis_message_interface()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    old_sp = tgis.open_old_stds(input, stdstype, dbif)
    old_maps = old_sp.get_registered_maps_as_objects(dbif=dbif)

    if not old_maps:
        dbif.close()
        gs.warning(
            _("Empty space-time %s dataset <%s>, nothing to copy") % (maptype, input)
        )
        return

    overwrite = gs.overwrite()

    # Check the new stds
    new_sp = tgis.check_new_stds(output, stdstype, dbif, overwrite)

    new_maps = None
    if copy_maps:
        gs.message(_("Copying %s maps to the current mapset...") % maptype)
        new_maps = []
        num_maps = len(old_maps)
        count = 0

        for map in old_maps:
            count += 1
            map_id = map.get_id()
            map_name = map_id
            map_mapset = mapset
            if "@" in map_id:
                map_name, map_mapset = map_id.split("@")

            if map_mapset != mapset:
                found = gs.find_file(name=map_name, element=element, mapset=mapset)
                if found["name"] is not None and len(found["name"]) > 0:
                    gs.fatal(
                        _("A %s map <%s> exists already in the current mapset <%s>.")
                        % (maptype, map_name, mapset)
                    )

                kwargs = {maptype: "%s,%s" % (map_id, map_name)}
                gs.run_command("g.copy", **kwargs)
            else:
                # the map is already in the current mapset
                gs.message(
                    _("The %s map <%s> is already in the current mapset, not copying")
                    % (maptype, map_name)
                )

            if count % 10 == 0:
                msgr.percent(count, num_maps, 1)

            # We need to build the id
            if maptype != "vector":
                map_id = tgis.AbstractMapDataset.build_id(map_name, mapset)
            else:
                map_id = tgis.AbstractMapDataset.build_id(
                    map_name, mapset, map.get_layer()
                )

            new_map = tgis.open_new_map_dataset(
                map_name,
                None,
                type="raster",
                temporal_extent=map.get_temporal_extent(),
                overwrite=overwrite,
                dbif=dbif,
            )
            # semantic label
            semantic_label = map.metadata.get_semantic_label()
            if semantic_label is not None and semantic_label != "None":
                new_map.metadata.set_semantic_label(semantic_label)

            new_maps.append(new_map)
    else:
        # don't copy maps, use old maps
        new_maps = old_maps

    temporal_type, semantic_type, title, description = old_sp.get_initial_values()
    new_sp = tgis.open_new_stds(
        output,
        stdstype,
        old_sp.get_temporal_type(),
        title,
        description,
        semantic_type,
        dbif,
        gs.overwrite(),
    )

    # Register the maps in the database
    num_maps = len(new_maps)
    count = 0
    for map in new_maps:
        count += 1

        # Insert map in temporal database
        if not map.is_in_db(dbif, mapset):
            semantic_label = map.metadata.get_semantic_label()
            map.load()
            # semantic labels are not yet properly implemented in TGIS
            if semantic_label is not None and semantic_label != "None":
                map.metadata.set_semantic_label(semantic_label)
            map.insert(dbif)
        new_sp.register_map(map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    new_sp.update_from_registered_maps(dbif)

    dbif.close()


###############################################################################

if __name__ == "__main__":
    options, flags = gs.parser()
    main()
