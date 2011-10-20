#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.export
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Export a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Export space time raster dataset 
#% keywords: dataset
#% keywords: spacetime
#% keywords: raster
#% keywords: export
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: output
#% type: string
#% description: Name of a space time raster dataset archive
#% required: yes
#% multiple: no
#%end

#%option
#% key: workdir
#% type: string
#% description: Path to the work directory, default is /tmp
#% required: no
#% multiple: no
#% answer: /tmp
#%end


#%option
#% key: compression
#% type: string
#% description: Chose the compression of the tar archive
#% required: no
#% multiple: no
#% options: no,gzip,bzip2
#% answer: bzip
#%end

#%option
#% key: where
#% type: string
#% description: A where statement for selected listing e.g: start_time < '2001-01-01' and end_time > '2001-01-01"'
#% required: no
#% multiple: no
#%end

import shutil
import os
import tarfile
import tempfile
import grass.script as grass
import grass.temporal as tgis

proj_file_name = "proj.txt"
init_file_name = "init.txt"
read_file_name = "readme.txt"
list_file_name = "list.txt"
tmp_tar_file_name = "archive" 

############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    compression = options["compression"]
    workdir = options["workdir"]
    where = options["where"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    mapset =  grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)
    
    if sp.is_in_db() == False:
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    # Save current working directory path
    old_cwd = os.getcwd()

    # Create the temporary directory and jump into it
    new_cwd = tempfile.mkdtemp(dir=workdir)
    os.chdir(new_cwd)

    sp.select()
       
    columns = "name,start_time,end_time"
    rows = sp.get_registered_maps(columns, where, "start_time", None)

    if compression == "gzip":
        flag = "w:gz"
    elif compression == "bzip2":
        flag = "w:bz2"
    else:
        flag = "w"

    # Open the tar archive to add the files
    tar = tarfile.open(tmp_tar_file_name, flag)
    list_file = open(list_file_name, "w")

    fs = "|"

    if rows:
        for row in rows:
            name = row["name"]
            start = row["start_time"]
            end = row["end_time"]
            if not end:
                end = start
            string = "%s%s%s%s%s\n" % (name, fs, start, fs, end)
            # Write the filename, the start_time and the end_time
            list_file.write(string)

            out_name = name + ".tif"

            # Export the raster map with r.out.gdal
            ret = grass.run_command("r.out.gdal", input=name, output=out_name, format="GTiff")
            if ret != 0:
                shutil.rmtree(new_cwd)
                tar.close()
                grass.fatal(_("Unable to export raster map <%s>" % name))

            tar.add(out_name)

    list_file.close()
   
    # Write projection and metadata
    proj = grass.read_command("g.proj", flags="j")

    proj_file = open(proj_file_name, "w")
    proj_file.write(proj)
    proj_file.close()

    init_file = open(init_file_name, "w")
    # Create the init string
    string = ""
    string += "%s=%s\n" % ("temporal_type", sp.get_temporal_type())
    string += "%s=%s\n" % ("semantic_type", sp.get_semantic_type())
    string += "%s=%s\n" % ("number_of_maps", sp.metadata.get_number_of_maps())
    north, south, east, west, top, bottom = sp.get_spatial_extent()
    string += "%s=%s\n" % ("north", north)
    string += "%s=%s\n" % ("south", south)
    string += "%s=%s\n" % ("east", east)
    string += "%s=%s\n" % ("west", west)
    init_file.write(string)
    init_file.close()

    read = grass.read_command("t.info", input=id)
    read_file = open(read_file_name, "w")
    read_file.write("This space time raster dataset was exported with tr.export of GRASS GIS 7\n")
    read_file.write(read)
    read_file.close()

    # Append the file list
    tar.add(list_file_name)
    tar.add(proj_file_name)
    tar.add(init_file_name)
    tar.add(read_file_name)
    tar.close()

    os.chdir(old_cwd)

    # Move the archive to its destination
    if output.find("/") >= 0 or output.find("\\") >= 0:
        shutil.move(tmp_tar_file_name, output)
    else:
        shutil.move(os.path.join(new_cwd, tmp_tar_file_name), output)

    # Remove the temporary created working directory
    shutil.rmtree(new_cwd)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
