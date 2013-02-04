#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.vect.oberve.rast
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Observe specific locations in a space time raster dataset over a period of time using vector points
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Observes specific locations in a space time raster dataset over a period of time using vector points.
#% keywords: temporal
#% keywords: sampling
#%end

#%option G_OPT_V_INPUT
#%end

#%option G_OPT_STRDS_INPUT
#% key: strds
#%end

#%option G_OPT_STVDS_OUTPUT
#%end

#%option G_OPT_V_OUTPUT
#% key: vector_output
#% description: Name of the new created vector map that stores the sampled values
#%end

#%option
#% key: column
#% type: string
#% description: Name of the vector column to be created and to store sampled raster values, otherwise the space time raster name is used as column name
#% required: no
#% multiple: no
#%end


#%option G_OPT_DB_WHERE
#%end

#%option G_OPT_T_WHERE
#% key: t_where
#%end

import grass.script as grass
import grass.temporal as tgis
import grass.script.raster as raster

############################################################################


def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    vector_output = options["vector_output"]
    strds = options["strds"]
    where = options["where"]
    column = options["column"]
    tempwhere = options["t_where"]

    if where == "" or where == " " or where == "\n":
        where = None

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = grass.gisenv()["MAPSET"]

    if strds.find("@") >= 0:
        strds_id = strds
    else:
        strds_id = strds + "@" + mapset

    strds_sp = tgis.SpaceTimeRasterDataset(strds_id)

    if strds_sp.is_in_db() == False:
        dbif.close()
        grass.fatal(_("Space time raster dataset <%s> not found") % (strds_id))

    strds_sp.select(dbif)

    if output.find("@") >= 0:
        id = output
    else:
        id = output + "@" + mapset

    out_sp = tgis.SpaceTimeVectorDataset(id)

    if out_sp.is_in_db(dbif) == True and grass.overwrite() == False:
        dbif.close()
        grass.fatal(_("Dataset <%s> found in temporal database, "
                      "use the overwrite flag to overwrite") % (id))

    # Overwrite existing stvds
    if out_sp.is_in_db(dbif) == True and grass.overwrite() == True:
        out_sp.select(dbif)
        out_sp.delete(dbif)
        out_sp = tgis.SpaceTimeVectorDataset(id)

    # Select the raster maps
    rows = strds_sp.get_registered_maps(
        "name,mapset,start_time,end_time", tempwhere, "start_time", dbif)

    if not rows:
        dbif.close()
        grass.fatal(_("Space time vector dataset <%s> is empty") %
                    out_sp.get_id())

    num_rows = len(rows)

    # Get the layer and database connections of the input vector
    vector_db = grass.vector.vector_db(input)

    # We copy the vector table and create the new layers
    if vector_db:
        # Use the first layer to copy the categories from
        layers = "1,"
    else:
        layers = ""
    first = True
    for layer in range(num_rows):
        layer += 1
        # Skip existing layer
        if vector_db and layer in vector_db and \
           vector_db[layer]["layer"] == layer:
            continue
        if first:
            layers += "%i" % (layer)
            first = False
        else:
            layers += ",%i" % (layer)

    # Use the name of the space time vector dataset as new vector name
    vectmap = vector_output

    # We create a new vector map using the categories of the original map
    ret = grass.run_command("v.category", input=input, layer=layers,
                            output=vectmap, option="transfer", 
                            overwrite=grass.overwrite())
    if ret != 0:
        grass.fatal(_("Unable to create new layers for vector map <%s>")
                    % (vectmap))

    # Create the output space time vector dataset
    out_sp.set_initial_values(strds_sp.get_temporal_type(),
                              strds_sp.get_semantic_type(),
                              _("Observaion of space time raster "
                                "dataset <%s>") % (strds_id),
                              _("Observation of space time raster dataset <%s>"
                                " with vector map <%s>") % (strds_id, input))

    out_sp.insert(dbif)

    dummy = out_sp.get_new_map_instance(None)

    # Sample the space time raster dataset with the vector 
    # map at specific layer with v.what.rast
    count = 1
    for row in rows:
        start = row["start_time"]
        end = row["end_time"]
        rastmap = row["name"] + "@" + row["mapset"]

        if column:
            col_name = column
        else:
            # Create a new column with name of the 
            # sampled space time raster dataset
            col_name = row["name"]

        coltype = "DOUBLE PRECISION"
        # Get raster map type
        rasterinfo = raster.raster_info(rastmap)
        if rasterinfo["datatype"] == "CELL":
            coltype = "INT"

        # Try to add a column
        if vector_db and count in vector_db and vector_db[count]["table"]:
            ret = grass.run_command("v.db.addcolumn", map=vectmap, 
                                    layer=count, 
                                    column="%s %s" % (col_name, coltype), 
                                    overwrite=grass.overwrite())
            if ret != 0:
                dbif.close()
                grass.fatal(_("Unable to add column %s to vector map <%s> "
                              "with layer %i") % (col_name, vectmap, count))
        else:
            # Try to add a new table
            grass.message("Add table to layer %i" % (count))
            ret = grass.run_command("v.db.addtable", map=vectmap, layer=count, 
                                    columns="%s %s" % (col_name, coltype), 
                                    overwrite=grass.overwrite())
            if ret != 0:
                dbif.close()
                grass.fatal(_("Unable to add table to vector map "
                              "<%s> with layer %i") % (vectmap, count))

        # Call v.what.rast
        ret = grass.run_command("v.what.rast", map=vectmap,
                                layer=count, raster=rastmap, 
                                column=col_name, where=where)
        if ret != 0:
            dbif.close()
            grass.fatal(_("Unable to run v.what.rast for vector map <%s> "
                          "with layer %i and raster map <%s>") % \
                        (vectmap, count, rastmap))

        vect = out_sp.get_new_map_instance(dummy.build_id(vectmap, 
                                                          mapset, str(count)))
        vect.load()

        if out_sp.is_time_absolute():
            vect.set_absolute_time(start, end)
        else:
            vect.set_relative_time(
                start, end, strds_sp.get_relative_time_unit())

        if vect.is_in_db(dbif):
            vect.update_all(dbif)
        else:
            vect.insert(dbif)

        out_sp.register_map(vect, dbif)
        count += 1

    out_sp.update_from_registered_maps(dbif)
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
