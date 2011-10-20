#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.import
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Import a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Import space time raster dataset 
#% keywords: dataset
#% keywords: spacetime
#% keywords: raster
#% keywords: import
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time raster dataset archive file
#% required: yes
#% multiple: no
#%end

#%option
#% key: output
#% type: string
#% description: Name of a space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: extrdir
#% type: string
#% description: Path to the extraction directory
#% required: yes
#% multiple: no
#%end

#%option
#% key: title
#% type: string
#% description: Title of the new space time dataset
#% required: no
#% multiple: no
#%end

#%option
#% key: description
#% type: string
#% description: Description of the new space time dataset
#% required: no
#% multiple: no
#%end

#%flag
#% key: l
#% description: Link the raster files using r.external
#%end

#%flag
#% key: e
#% description: Extend location extents based on new dataset
#%end

#%flag
#% key: o
#% description: verride projection (use location's projection)
#%end


import shutil
import os
import tarfile
import tempfile
import grass.script as grass
import grass.temporal as tgis

proj_file_name = "proj.txt"
init_file_name = "init.txt"
list_file_name = "list.txt"

############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    extrdir = options["extrdir"]
    title = options["title"]
    descr = options["description"]
    link = flags["l"]
    exp = flags["e"]
    overr = flags["o"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    
    # Check if input file and extraction directory exits
    if not os.path.exists(input): 
        grass.fatal(_("Space time raster dataset archive <%s> not found.") % input)
    if not os.path.exists(extrdir): 
        grass.fatal(_("Extraction directory <%s> not found.") % extrdir)

    tar = tarfile.open(input)
    
    # Check for important files
    members = tar.getnames()

    if init_file_name not in members: 
        grass.fatal(_("Unable to find init file <%s>.") % init_file_name)
    if list_file_name not in members: 
        grass.fatal(_("Unable to find list file <%s>.") % list_file_name)
    if proj_file_name not in members: 
        grass.fatal(_("Unable to find projection file <%s>.") % proj_file_name)

    tar.extractall(path=extrdir)
    tar.close()

    # Switch into the extraction directory and check for files
    os.chdir(extrdir)

    fs = "|"
    maplist = []
    mapset =  grass.gisenv()["MAPSET"]
    list_file = open(list_file_name, "r")

    # Read the map list from file
    line_count = 0
    while True:
        line = list_file.readline()
        if not line:
            break

        line_list = line.split(fs)

        mapname = line_list[0].strip()
        mapid = mapname + "@" + mapset

        row = {}
        row["name"] = mapname
        row["id"] = mapid
        row["start"] = line_list[1].strip()
        row["end"] = line_list[2].strip()

        maplist.append(row)
        line_count += 1

    list_file.close()
    
    # Check if geotiff files exists
    for row in maplist:
        filename = str(row["name"]) + ".tif"
        if not os.path.exists(filename): 
            grass.fatal(_("Unable to find geotiff raster file <%s> in archive.") % filename)

    # Read the init file
    fs = "="
    init = {}
    init_file = open(init_file_name, "r")
    while True:
        line = init_file.readline()
        if not line:
            break

        kv = line.split(fs)
        init[kv[0]] = kv[1].strip()
    
    init_file.close()

    if not init.has_key("temporal_type") or \
       not init.has_key("semantic_type") or \
       not init.has_key("number_of_maps"):
        grass.fatal(_("Key words %s, %s or %s not found in init file.") % ("temporal_type", "semantic_type", "number_of_maps"))

    if line_count != int(init["number_of_maps"]):
        grass.fatal(_("Number of maps mismatch in init and list file."))

    # Check the space time dataset

    id = output + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)
    
    if sp.is_in_db() and grass.overwrite() == False:
        grass.fatal(_("Space time %s dataset <%s> is already in the database. Use the overwrite flag.") % name)

    # Try to import/link the raster files
    for row in maplist:
        name = row["name"]
        filename = str(row["name"]) + ".tif"
        impflags=""
        if overr:
            impflags += "o"
        if exp:
            impflags += "e"
        
        if link:
            ret = grass.run_command("r.external", input=filename, output=name, flags=impflags, overwrite=grass.overwrite())
        else:
            ret = grass.run_command("r.in.gdal", input=filename, output=name, flags=impflags, overwrite=grass.overwrite())

        if ret != 0:
            grass.fatal(_("Unable to import/link raster map <%s>.") % name)
    
    # Create the space time raster dataset
    if sp.is_in_db() and grass.overwrite() == True:
        grass.info(_("Overwrite space time %s dataset <%s> and unregister all maps.") % (sp.get_new_map_instance(None).get_type(), name))
        sp.delete()
        sp = sp.get_new_instance(id)

    temporal_type = init["temporal_type"]
    semantic_type = init["semantic_type"]
    grass.verbose(_("Create space time %s dataset.") % sp.get_new_map_instance(None).get_type())

    sp.set_initial_values(temporal_type=temporal_type, semantic_type=semantic_type, title=title, description=descr)
    sp.insert()

    # register the raster maps
    fs="|"
    tgis.register_maps_in_space_time_dataset(type="strds", name=output, file=list_file_name, start="file", end="file", dbif=None, fs=fs)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
