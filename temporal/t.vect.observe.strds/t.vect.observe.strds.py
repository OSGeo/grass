#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.vect.observe.strds
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

#%option G_OPT_STRDS_INPUTS
#% key: strds
#%end

#%option G_OPT_STVDS_OUTPUT
#%end

#%option G_OPT_V_OUTPUT
#% key: vector_output
#% description: Name of the new created vector map that stores the sampled values in different layers
#%end

#%option
#% key: columns
#% type: string
#% description: Names of the vector columns to be created and to store sampled raster values, one name for each STRDS
#% required: yes
#% multiple: yes
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

class Sample(object):
    def __init__(self, start = None, end = None, raster_names = None):
        self.start = start
        self.end = end
        if raster_names != None:
            self.raster_names = raster_names
        else:
            self.raster_names = []

    def __str__(self):
        return "Start: %s\nEnd: %s\nNames: %s\n"%(str(self.start), str(self.end), str(self.raster_names))
            
############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    vector_output = options["vector_output"]
    strds = options["strds"]
    where = options["where"]
    columns = options["columns"]
    tempwhere = options["t_where"]

    if where == "" or where == " " or where == "\n":
        where = None

    overwrite = grass.overwrite()

    # Check the number of sample strds and the number of columns
    strds_names = strds.split(",")
    column_names = columns.split(",")

    if len(strds_names) != len(column_names):
        grass.fatal(_("The number of columns must be equal to the number of space time raster datasets"))

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = grass.gisenv()["MAPSET"]

    out_sp = tgis.check_new_space_time_dataset(output, "stvds", dbif, overwrite)

    samples = []

    first_strds = tgis.open_old_space_time_dataset(strds_names[0], "strds", dbif)

    # Single space time raster dataset
    if len(strds_names) == 1:
        rows = first_strds.get_registered_maps(
            "name,mapset,start_time,end_time", tempwhere, "start_time", dbif)

        if not rows:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is empty") %
                        out_sp.get_id())

        for row in rows:
            start = row["start_time"]
            end = row["end_time"]
            raster_maps = [row["name"] + "@" + row["mapset"],]

            s = Sample(start, end, raster_maps)
            samples.append(s)
    else:
        # Multiple space time raster datasets
        for name in strds_names[1:]:
            dataset = tgis.open_old_space_time_dataset(name, "strds", dbif)
            if dataset.get_temporal_type() != first_strds.get_temporal_type():
                grass.fatal(_("Temporal type of space time raster datasets must be equal\n"
                              "<%(a)s> of type %(type_a)s do not match <%(b)s> of type %(type_b)s"%\
                              {"a":first_strds.get_id(),
                               "type_a":first_strds.get_temporal_type(),
                               "b":dataset.get_id(),
                               "type_b":dataset.get_temporal_type()}))

        mapmatrizes = tgis.sample_stds_by_stds_topology("strds", "strds", strds_names,
                                                      strds_names[0], False, None,
                                                      "equal", False, False)

        for i in xrange(len(mapmatrizes[0])):
            isvalid = True
            mapname_list = []
            for mapmatrix in mapmatrizes:
                
                entry = mapmatrix[i]

                if entry["samples"]:
                    sample = entry["samples"][0]
                    name = sample.get_id()
                    if name is None:
                        isvalid = False
                        break
                    else:
                        mapname_list.append(name)

            if isvalid:
                entry = mapmatrizes[0][i]
                map = entry["granule"]

                start, end = map.get_temporal_extent_as_tuple()
                s = Sample(start, end, mapname_list)
                samples.append(s)

    num_samples = len(samples)

    # Get the layer and database connections of the input vector
    vector_db = grass.vector.vector_db(input)

    # We copy the vector table and create the new layers
    if vector_db:
        # Use the first layer to copy the categories from
        layers = "1,"
    else:
        layers = ""
    first = True
    for layer in range(num_samples):
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

    vectmap = vector_output

    # We create a new vector map using the categories of the original map
    ret = grass.run_command("v.category", input=input, layer=layers,
                            output=vectmap, option="transfer",
                            overwrite=overwrite)
    if ret != 0:
        grass.fatal(_("Unable to create new layers for vector map <%s>")
                    % (vectmap))

    title = _("Observaion of space time raster dataset(s) <%s>") % (strds)
    description= _("Observation of space time raster dataset(s) <%s>"
                   " with vector map <%s>") % (strds, input)

    # Create the output space time vector dataset
    out_sp = tgis.open_new_space_time_dataset(output, "stvds",
                                              first_strds.get_temporal_type(),
                                              title, description,
                                              first_strds.get_semantic_type(),
                                              dbif, overwrite)

    dummy = out_sp.get_new_map_instance(None)

    # Sample the space time raster dataset with the vector
    # map at specific layer with v.what.rast
    count = 1
    for sample in samples:
        raster_names = sample.raster_names

        if len(raster_names) != len(column_names):
            grass.fatal(_("The number of raster maps in a granule must "
                          "be equal to the number of column names"))

        # Create the columns creation string
        columns_string = ""
        for name, column in zip(raster_names, column_names):
            # The column is by default double precision
            coltype = "DOUBLE PRECISION"
            # Get raster map type
            raster_map = tgis.RasterDataset(name)
            raster_map.load()
            if raster_map.metadata.get_datatype() == "CELL":
                coltype = "INT"

            tmp_string = "%s %s,"%(column, coltype)
            columns_string += tmp_string

        # Remove last comma
        columns_string = columns_string[0:len(columns_string) - 1]

        # Try to add a column
        if vector_db and count in vector_db and vector_db[count]["table"]:
            ret = grass.run_command("v.db.addcolumn", map=vectmap,
                                    layer=count, column=columns_string,
                                    overwrite=overwrite)
            if ret != 0:
                dbif.close()
                grass.fatal(_("Unable to add column %s to vector map <%s> "
                              "with layer %i") % (columns_string, vectmap, count))
        else:
            # Try to add a new table
            grass.message("Add table to layer %i" % (count))
            ret = grass.run_command("v.db.addtable", map=vectmap, layer=count,
                                    columns=columns_string, overwrite=overwrite)
            if ret != 0:
                dbif.close()
                grass.fatal(_("Unable to add table to vector map "
                              "<%s> with layer %i") % (vectmap, count))

        # Call v.what.rast for each raster map
        for name, column in zip(raster_names, column_names):
            ret = grass.run_command("v.what.rast", map=vectmap,
                                    layer=count, raster=name,
                                    column=column, where=where)
            if ret != 0:
                dbif.close()
                grass.fatal(_("Unable to run v.what.rast for vector map <%s> "
                            "with layer %i and raster map <%s>") % \
                            (vectmap, count, str(raster_names)))

        vect = out_sp.get_new_map_instance(dummy.build_id(vectmap,
                                                          mapset, str(count)))
        vect.load()
        
        start = sample.start
        end = sample.end
        
        if out_sp.is_time_absolute():
            vect.set_absolute_time(start, end)
        else:
            vect.set_relative_time(
                start, end, first_strds.get_relative_time_unit())

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
