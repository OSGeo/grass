#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tv.oberve.rast
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Observe specific locations in a space time raster dataset over a periode of time using vector points
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Observe specific locations in a space time raster dataset over a periode of time using vector points
#% keywords: temporal
#% keywords: sampling
#%end

#%option G_OPT_STVDS_INPUT
#%end

#%option
#% key: strds
#% type: string
#% description: The space time raster dataset to use
#% required: yes
#% multiple: no
#%end

#%option
#% key: column
#% type: string
#% description: Name of the vector column to be created and to store sampled raster values, otherwise the space time raster name is used as column name 
#% required: no
#% multiple: no
#%end

#%option
#% key: output
#% type: string
#% description: Name the new created space time vector dataset and the new vector map 
#% required: yes
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
    strds = options["strds"]
    where = options["where"]
    column = options["column"]
    tempwhere = options["t_where"]

    if where == "" or where == " " or where == "\n":
        where = None

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # We need a database interface
    dbif = tgis.sql_database_interface()
    dbif.connect()
   
    mapset =  grass.gisenv()["MAPSET"]

    if strds.find("@") >= 0:
        strds_id = strds
    else:
        strds_id = strds + "@" + mapset

    strds_sp = tgis.space_time_raster_dataset(strds_id)
    
    if strds_sp.is_in_db() == False:
        dbif.close()
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    strds_sp.select(dbif)

    if output.find("@") >= 0:
        id = output
    else:
        id = output + "@" + mapset

    out_sp = tgis.space_time_vector_dataset(id)
    
    if out_sp.is_in_db(dbif) == True and grass.overwrite() == False: 
        dbif.close()
        grass.fatal(_("Dataset <%s> found in temporal database, use the overwrite flag to overwrite") % (id))

    # Overwrite existing stvds
    if out_sp.is_in_db(dbif) == True and grass.overwrite() == True:
        out_sp.select()
        out_sp.delete()
        out_sp = tgis.space_time_vector_dataset(id)

    # Select the raster maps
    rows = strds_sp.get_registered_maps("name,mapset,start_time,end_time", tempwhere, "start_time", dbif)

    if not rows:
        dbif.close()
        grass.fatal(_("Space time vector dataset <%s> is empty") % sg.get_id())

    # Create the output space time vector dataset

    out_sp.set_initial_values(strds_sp.get_temporal_type(), \
                              strds_sp.get_semantic_type(),\
                              _("Observation of space time raster dataset <%s>")%(strds_id),\
                              _("Observattion of space time raster dataset <%s> with vector map <%s>")%(strds_id, input))

    out_sp.insert(dbif)

    num_rows = len(rows)

    # We copy the vector table and create the new layers
    layers= ""
    first = True
    for layer in range(num_rows):
        layer += 1
        if first:
            layers += "%i"%(layer)    
            first = False
        else:
            layers += ",%i"%(layer)    

    print layers

    vectmap = output
    ret = grass.run_command("v.category", input=input, layer=layers, output=vectmap, option="transfer", overwrite=grass.overwrite())
    if ret != 0:
        grass.fatal(_("Unable to create new layers for vector map <%s>") % (vectmap))
        
    dummy = out_sp.get_new_map_instance(None)

    # Sample the space time raster dataset with the vector map at specific layer with v.what.rast
    count = 1
    for row in rows:
        start = row["start_time"]
        end = row["end_time"]
        rastmap = row["name"] + "@" + row["mapset"]

	if column:
	    col_name = column
	else:
	    # Create a new column with name of the sampled space time raster dataset
	    col_name = row["name"]

	coltype = "DOUBLE PRECISION"
	# Get raster map type
	rasterinfo = raster.raster_info(rastmap)
	if rasterinfo["datatype"] == "CELL":
	    coltype = "INT"
	
        # Try to add a column
	ret = grass.run_command("v.db.addcolumn", map=vectmap, layer=count, column="%s %s" % (col_name, coltype), overwrite=grass.overwrite())
	if ret != 0:
            # Try to add a new table
            grass.message("Add table to layer %i"%(count))
	    ret = grass.run_command("v.db.addtable", map=vectmap, layer=count, columns="%s %s" % (col_name, coltype), overwrite=grass.overwrite())
	    if ret != 0:
	        dbif.close()
	        grass.fatal(_("Unable to add column %s to vector map <%s> with layer %i")%(col_name, vectmap, count))

	# Call v.what.rast
	ret = grass.run_command("v.what.rast", map=vectmap, layer=count, raster=rastmap, column=col_name, where=where)
	if ret != 0:
	    dbif.close()
	    grass.fatal(_("Unable to run v.what.rast for vector map <%s> with layer %i and raster map <%s>")%(vectmap, count, rastmap))
        
        vect = out_sp.get_new_map_instance(dummy.build_id(vectmap, mapset, str(count)))
        vect.load()
 
        if out_sp.is_time_absolute():
            vect.set_absolute_time(start, end)
        else:
            vect.set_relative_time(start, end, strds_ds.get_relative_time_unit())
       
        if vect.is_in_db():
            vect.update(dbif)
        else:
            vect.insert(dbif)

        out_sp.register_map(vect, dbif) 
        count += 1

    out_sp.update_from_registered_maps(dbif)
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

