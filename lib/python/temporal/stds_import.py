"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS export functions to be used in temporal modules

Usage:

@code
import grass.temporal as tgis

input="/tmp/temp_1950_2012.tar.gz"
output="temp_1950_2012"
extrdir="/tmp"
title="My new dataset"
descr="May new shiny dataset"
location=None
link=True
exp=True
overr=False
create=False
tgis.import_stds(input, output, extrdir, title, descr, location,
                link, exp, overr, create, "strds")
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import shutil
import os
import os.path
import tarfile
import tempfile
import time
import filecmp
import space_time_datasets_tools
from space_time_datasets_tools import *

proj_file_name = "proj.txt"
init_file_name = "init.txt"
list_file_name = "list.txt"

# This global variable is for unique vector map export,
# since single vector maps may have several layer
# and therefore several attribute tables
imported_maps = {}

############################################################################

def _import_raster_maps_from_geotiff(maplist, overr, exp, location, link):
    impflags = ""
    if overr:
        impflags += "o"
    if exp or location:
        impflags += "e"
    for row in maplist:
        name = row["name"]
        filename = str(row["name"]) + ".tif"

        if link:
            ret = core.run_command("r.external", input=filename,
                                   output=name,
                                   flags=impflags,
                                   overwrite=core.overwrite())
        else:
            ret = core.run_command("r.in.gdal", input=filename,
                                   output=name,
                                   flags=impflags,
                                   overwrite=core.overwrite())

        if ret != 0:
            core.fatal(_("Unable to import/link raster map <%s>.") % name)

        # Set the color rules if present
        filename = str(row["name"]) + ".color"
        if os.path.isfile(filename):
            ret = core.run_command("r.colors", map=name,
                                   rules=filename,
                                   overwrite=core.overwrite())
            if ret != 0:
                core.fatal(_("Unable to set the color rules for "
                             "raster map <%s>.") % name)

############################################################################

def _import_raster_maps(maplist):
    # We need to disable the projection check because of its 
    # simple implementation
    impflags = "o"
    for row in maplist:
        name = row["name"]
        filename = str(row["name"]) + ".pack"
        ret = core.run_command("r.unpack", input=filename,
                               output=name,
                               flags=impflags,
                               overwrite=core.overwrite(),
                               verbose=True)

        if ret != 0:
            core.fatal(_("Unable to unpack raster map <%s>.") % name)

############################################################################

def _import_vector_maps_from_gml(maplist, overr, exp, location, link):
    impflags = "o"
    if exp or location:
        impflags += "e"
    for row in maplist:
        name = row["name"]
        filename = str(row["name"]) + ".xml"

        ret = core.run_command("v.in.ogr", dsn=filename,
                               output=name,
                               flags=impflags,
                               overwrite=core.overwrite())

        if ret != 0:
            core.fatal(_("Unable to import vector map <%s>.") % name)

############################################################################

def _import_vector_maps(maplist):
    # We need to disable the projection check because of its 
    # simple implementation
    impflags = "o"
    for row in maplist:
        # Separate the name from the layer
        name = row["name"].split(":")[0]
        # Import only unique maps
        if name in imported_maps:
            continue
        filename = name + ".pack"
        ret = core.run_command("v.unpack", input=filename,
                               output=name,
                               flags=impflags,
                               overwrite=core.overwrite(),
                               verbose=True)

        if ret != 0:
            core.fatal(_("Unable to unpack vector map <%s>.") % name)

        imported_maps[name] = name
############################################################################

def import_stds(
    input, output, extrdir, title=None, descr=None, location=None,
        link=False, exp=False, overr=False, create=False, stds_type="strds"):
    """!Import space time datasets of type raster and vector

        @param input Name of the input archive file
        @param output The name of the output space time dataset
        @param extrdir The extraction directory
        @param title The title of the new created space time dataset
        @param descr The description of the new created 
                            space time dataset
        @param location The name of the location that should be created,
                        maps are imported into this location
        @param link Switch to link raster maps instead importing them
        @param exp Extend location extents based on new dataset
        @param overr Override projection (use location's projection)
        @param create Create the location specified by the "location" 
                      parameter and exit.
                      Do not import the space time datasets.
        @param stds_type The type of the space time dataset that 
                          should be imported
    """

    global raise_on_error
    old_state = core.raise_on_error
    core.set_raise_on_error(True)

    # Check if input file and extraction directory exits
    if not os.path.exists(input):
        core.fatal(_("Space time raster dataset archive <%s> not found")
                   % input)
    if not create and not os.path.exists(extrdir):
        core.fatal(_("Extraction directory <%s> not found") % extrdir)

    tar = tarfile.open(name=input, mode='r')

    # Check for important files
    members = tar.getnames()

    if init_file_name not in members:
        core.fatal(_("Unable to find init file <%s>") % init_file_name)
    if list_file_name not in members:
        core.fatal(_("Unable to find list file <%s>") % list_file_name)
    if proj_file_name not in members:
        core.fatal(_("Unable to find projection file <%s>") % proj_file_name)

    tar.extractall(path=extrdir)
    tar.close()

    # Save current working directory path
    old_cwd = os.getcwd()

    # Switch into the data directory
    os.chdir(extrdir)

    # Check projection information
    if not location:
        temp_name = core.tempfile()
        temp_file = open(temp_name, "w")
        proj_name = os.path.abspath(proj_file_name)

        p = core.start_command("g.proj", flags="j", stdout=temp_file)
        p.communicate()
        temp_file.close()

        if not core.compare_key_value_text_files(temp_name, proj_name, sep="="):
            if overr:
                core.warning(_("Projection information does not match. "
                               "Proceeding..."))
            else:
                core.fatal(_("Projection information does not match. Aborting."))

    # Create a new location based on the projection information and switch into it
    old_env = core.gisenv()
    if location:
        try:
            proj4_string = open(proj_file_name, 'r').read()
            core.create_location(dbase=old_env["GISDBASE"],
                                 location=location,
                                 proj4=proj4_string)
            # Just create a new location and return
            if create:
                os.chdir(old_cwd)
                return
        except Exception as e:
            core.fatal(_("Unable to create location %s. Reason: %s")
                       % (location, str(e)))
        # Switch to the new created location
        ret = core.run_command("g.mapset", mapset="PERMANENT",
                               location=location,
                               gisdbase=old_env["GISDBASE"])
        if ret != 0:
            core.fatal(_("Unable to switch to location %s") % location)
        # create default database connection
        ret = core.run_command("t.connect", flags="d")
        if ret != 0:
            core.fatal(_("Unable to create default temporal database "
                         "in new location %s") % location)

    try:
        # Make sure the temporal database exists
        space_time_datasets_tools.init()

        fs = "|"
        maplist = []
        mapset = core.gisenv()["MAPSET"]
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

        if "temporal_type" not in init or \
           "semantic_type" not in init or \
           "number_of_maps" not in init:
            core.fatal(_("Key words %s, %s or %s not found in init file.") %
                       ("temporal_type", "semantic_type", "number_of_maps"))

        if line_count != int(init["number_of_maps"]):
            core.fatal(_("Number of maps mismatch in init and list file."))

        _format = "GTiff"
        _type = "strds"

        if "stds_type" in init:
            _type = init["stds_type"]
        if "format" in init:
            _format = init["format"]

        if stds_type != _type:
            core.fatal(_("The archive file is of wrong space time dataset type"))

        # Check the existence of the files
        if _format == "GTiff":
            for row in maplist:
                filename = str(row["name"]) + ".tif"
                if not os.path.exists(filename):
                    core.fatal(_("Unable to find geotiff raster file "
                                 "<%s> in archive.") % filename)
        elif _format == "GML":
            for row in maplist:
                filename = str(row["name"]) + ".xml"
                if not os.path.exists(filename):
                    core.fatal(_("Unable to find GML vector file "
                                 "<%s> in archive.") % filename)
        elif _format == "pack":
            for row in maplist:
                if _type == "stvds":
                    filename = str(row["name"].split(":")[0]) + ".pack"
                else:
                    filename = str(row["name"]) + ".pack"
                if not os.path.exists(filename):
                    core.fatal(_("Unable to find GRASS package file "
                                 "<%s> in archive.") % filename)
        else:
            core.fatal(_("Unsupported input format"))

        # Check the space time dataset
        id = output + "@" + mapset
        sp = dataset_factory(_type, id)
        if sp.is_in_db() and core.overwrite() == False:
            core.fatal(_("Space time %s dataset <%s> is already in the "
                         "database. Use the overwrite flag.") % \
                        (_type, sp.get_id()))

        # Import the maps
        if _type == "strds":
            if _format == "GTiff":
                _import_raster_maps_from_geotiff(
                    maplist, overr, exp, location, link)
            if _format == "pack":
                _import_raster_maps(maplist)
        elif _type == "stvds":
            if _format == "GML":
                _import_vector_maps_from_gml(
                    maplist, overr, exp, location, link)
            if _format == "pack":
                _import_vector_maps(maplist)

        # Create the space time dataset
        if sp.is_in_db() and core.overwrite() == True:
            core.info(_("Overwrite space time %s dataset "
                        "<%s> and unregister all maps.") % \
                       (sp.get_new_map_instance(None).get_type(), sp.get_id()))
            sp.delete()
            sp = sp.get_new_instance(id)

        temporal_type = init["temporal_type"]
        semantic_type = init["semantic_type"]
        core.verbose(_("Create space time %s dataset.") %
                     sp.get_new_map_instance(None).get_type())

        sp.set_initial_values(temporal_type=temporal_type, 
                              semantic_type=semantic_type, title=title, 
                              description=descr)
        sp.insert()

        # register the maps
        fs = "|"
        register_maps_in_space_time_dataset(
            type=sp.get_new_map_instance(None).get_type(),
            name=output, file=list_file_name, start="file", 
            end="file", dbif=None, fs=fs)

        os.chdir(old_cwd)
    except:
        raise

    # Make sure the location is switched back correctly
    finally:
        if location:
            # Switch to the old location
            ret = core.run_command("g.mapset", mapset=old_env["MAPSET"],
                                   location=old_env["LOCATION_NAME"],
                                   gisdbase=old_env["GISDBASE"])
        
        core.set_raise_on_error(old_state)
