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
#% keywords: temporal
#% keywords: export
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_F_OUTPUT
#% description: Name of a space time raster dataset archive
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
#% answer: bzip2
#%end

#%option G_OPT_T_WHERE
#%end

import shutil
import os
import tarfile
import tempfile
import grass.script as grass
import grass.temporal as tgis

proj_file_name = "proj.txt"
init_file_name = "init.txt"
metadata_file_name = "metadata.txt"
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
        grass.fatal(_("Space time %s dataset <%s> not found") % (sp.get_new_map_instance(None).get_type(), id))

    # Save current working directory path
    old_cwd = os.getcwd()

    # Create the temporary directory and jump into it
    new_cwd = tempfile.mkdtemp(dir=workdir)
    os.chdir(new_cwd)

    sp.select()
       
    columns = "name,start_time,end_time,min,max,datatype"
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
            max_val = row["max"]
            min_val = row["min"]
            datatype = row["datatype"]
            if not end:
                end = start
            string = "%s%s%s%s%s\n" % (name, fs, start, fs, end)
            # Write the filename, the start_time and the end_time
            list_file.write(string)

	    # Export the raster map with r.out.gdal as tif
            out_name = name + ".tif"
            if datatype == "CELL":
                nodata = max_val + 1
                if nodata < 256 and min_val >= 0:
                    gdal_type = "Byte" 
                elif nodata < 65536 and min_val >= 0:
                    gdal_type = "UInt16" 
                elif min_val >= 0:
                    gdal_type = "UInt32" 
                else:
                    gdal_type = "Int32" 
                ret = grass.run_command("r.out.gdal", flags="c", input=name, output=out_name, nodata=nodata, type=gdal_type, format="GTiff")
            else:
                ret = grass.run_command("r.out.gdal", flags="c", input=name, output=out_name, format="GTiff")
            if ret != 0:
                shutil.rmtree(new_cwd)
                tar.close()
                grass.fatal(_("Unable to export raster map <%s>" % name))
                
            tar.add(out_name)

	    # Export the color rules 
            out_name = name + ".color"
            ret = grass.run_command("r.colors.out", map=name, rules=out_name)
            if ret != 0:
                shutil.rmtree(new_cwd)
                tar.close()
                grass.fatal(_("Unable to export color rules for raster map <%s>" % name))
                
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

    metadata = grass.read_command("t.info", input=id)
    metadata_file = open(metadata_file_name, "w")
    metadata_file.write(metadata)
    metadata_file.close()
    
    read_file = open(read_file_name, "w")
    read_file.write("This space time raster dataset was exported with tr.export of GRASS GIS 7\n")
    read_file.write("\n")
    read_file.write("Files:\n")
                    #123456789012345678901234567890
    read_file.write("       *.tif  -- GeoTIFF time series raster files\n")
    read_file.write("     *.color  -- GRASS GIS raster color rules\n")
    read_file.write("%13s -- Projection information in PROJ.4 format\n" % (proj_file_name))
    read_file.write("%13s -- GRASS GIS space time raster dataset information\n" % (init_file_name))
    read_file.write("%13s -- Time series file, lists all maps by name with interval\n"  % (list_file_name))
    read_file.write("                 time stamps in ISO-Format. Field separator is |\n")
    read_file.write("%13s -- Projection information in PROJ.4 format\n" % (metadata_file_name))
    read_file.write("%13s -- This file\n" % (read_file_name))
    read_file.close()

    # Append the file list
    tar.add(list_file_name)
    tar.add(proj_file_name)
    tar.add(init_file_name)
    tar.add(read_file_name)
    tar.add(metadata_file_name)
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
