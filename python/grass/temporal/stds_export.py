"""
Export functions for space time datasets

Usage:

.. code-block:: python

    import grass.temporal as tgis

    input="temp_1950_2012@PERMANENT"
    output="/tmp/temp_1950_2012.tar.gz"
    compression="gzip"
    directory="/tmp"
    where=None
    format_="GTiff"
    type_="strds"
    tgis.export_stds(input, output, compression, directory, where, format_, type_)


(C) 2012-2021 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import shutil
import os
import tarfile
import tempfile

import grass.script as gscript
from grass.exceptions import CalledModuleError
from .open_stds import open_old_stds


proj_file_name = "proj.txt"
init_file_name = "init.txt"
metadata_file_name = "metadata.txt"
read_file_name = "readme.txt"
list_file_name = "list.txt"
tmp_tar_file_name = "archive"

# This global variable is for unique vector map export,
# since single vector maps may have several layer
# and therefore several attribute tables
exported_maps = {}

############################################################################


def _export_raster_maps_as_gdal(
    rows, tar, list_file, new_cwd, fs, format_, type_, **kwargs
):
    kwargs = {key: value for key, value in kwargs.items() if value is not None}
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

        try:
            if format_ == "GTiff":
                # Export the raster map with r.out.gdal as tif
                out_name = name + ".tif"
                if datatype == "CELL" and not type_:
                    nodata = max_val + 1
                    if nodata < 256 and min_val >= 0:
                        gdal_type = "Byte"
                    elif nodata < 65536 and min_val >= 0:
                        gdal_type = "UInt16"
                    elif min_val >= 0:
                        gdal_type = "UInt32"
                    else:
                        gdal_type = "Int32"
                    gscript.run_command(
                        "r.out.gdal",
                        flags="c",
                        input=name,
                        output=out_name,
                        nodata=nodata,
                        type=gdal_type,
                        format="GTiff",
                        **kwargs,
                    )
                elif type_:
                    gscript.run_command(
                        "r.out.gdal",
                        flags="cf",
                        input=name,
                        output=out_name,
                        type=type_,
                        format="GTiff",
                        **kwargs,
                    )
                else:
                    gscript.run_command(
                        "r.out.gdal",
                        flags="c",
                        input=name,
                        output=out_name,
                        format="GTiff",
                        **kwargs,
                    )
            elif format_ == "AAIGrid":
                # Export the raster map with r.out.gdal as Arc/Info ASCII Grid
                out_name = name + ".asc"
                gscript.run_command(
                    "r.out.gdal",
                    flags="c",
                    input=name,
                    output=out_name,
                    format="AAIGrid",
                    **kwargs,
                )

        except CalledModuleError:
            shutil.rmtree(new_cwd)
            tar.close()
            gscript.fatal(_("Unable to export raster map <%s>" % name))

        tar.add(out_name)

        # Export the color rules
        out_name = name + ".color"
        try:
            gscript.run_command("r.colors.out", map=name, rules=out_name)
        except CalledModuleError:
            shutil.rmtree(new_cwd)
            tar.close()
            gscript.fatal(
                _(
                    "Unable to export color rules for raster "
                    "map <%s> r.out.gdal" % name
                )
            )

        tar.add(out_name)


############################################################################


def _export_raster_maps(rows, tar, list_file, new_cwd, fs):
    for row in rows:
        name = row["name"]
        start = row["start_time"]
        end = row["end_time"]
        if not end:
            end = start
        string = "%s%s%s%s%s\n" % (name, fs, start, fs, end)
        # Write the filename, the start_time and the end_time
        list_file.write(string)
        # Export the raster map with r.pack
        try:
            gscript.run_command("r.pack", input=name, flags="c")
        except CalledModuleError:
            shutil.rmtree(new_cwd)
            tar.close()
            gscript.fatal(_("Unable to export raster map <%s> with r.pack" % name))

        tar.add(name + ".pack")


############################################################################


def _export_vector_maps_as_gml(rows, tar, list_file, new_cwd, fs):
    for row in rows:
        name = row["name"]
        start = row["start_time"]
        end = row["end_time"]
        layer = row["layer"]
        if not layer:
            layer = 1
        if not end:
            end = start
        string = "%s%s%s%s%s\n" % (name, fs, start, fs, end)
        # Write the filename, the start_time and the end_time
        list_file.write(string)
        # Export the vector map with v.out.ogr
        try:
            gscript.run_command(
                "v.out.ogr",
                input=name,
                output=(name + ".xml"),
                layer=layer,
                format="GML",
            )
        except CalledModuleError:
            shutil.rmtree(new_cwd)
            tar.close()
            gscript.fatal(
                _("Unable to export vector map <%s> as " "GML with v.out.ogr" % name)
            )

        tar.add(name + ".xml")
        tar.add(name + ".xsd")


############################################################################


def _export_vector_maps_as_gpkg(rows, tar, list_file, new_cwd, fs):
    for row in rows:
        name = row["name"]
        start = row["start_time"]
        end = row["end_time"]
        layer = row["layer"]
        if not layer:
            layer = 1
        if not end:
            end = start
        string = "%s%s%s%s%s\n" % (name, fs, start, fs, end)
        # Write the filename, the start_time and the end_time
        list_file.write(string)
        # Export the vector map with v.out.ogr
        try:
            gscript.run_command(
                "v.out.ogr",
                input=name,
                output=(name + ".gpkg"),
                layer=layer,
                format="GPKG",
            )
        except CalledModuleError:
            shutil.rmtree(new_cwd)
            tar.close()
            gscript.fatal(
                _("Unable to export vector map <%s> as " "GPKG with v.out.ogr" % name)
            )

        tar.add(name + ".gpkg")


############################################################################


def _export_vector_maps(rows, tar, list_file, new_cwd, fs):
    for row in rows:
        name = row["name"]
        start = row["start_time"]
        end = row["end_time"]
        layer = row["layer"]

        # Export unique maps only
        if name in exported_maps:
            continue

        if not layer:
            layer = 1
        if not end:
            end = start
        string = "%s:%s%s%s%s%s\n" % (name, layer, fs, start, fs, end)
        # Write the filename, the start_time and the end_time
        list_file.write(string)
        # Export the vector map with v.pack
        try:
            gscript.run_command("v.pack", input=name, flags="c")
        except CalledModuleError:
            shutil.rmtree(new_cwd)
            tar.close()
            gscript.fatal(_("Unable to export vector map <%s> with v.pack" % name))

        tar.add(name + ".pack")

        exported_maps[name] = name


############################################################################


def _export_raster3d_maps(rows, tar, list_file, new_cwd, fs):
    for row in rows:
        name = row["name"]
        start = row["start_time"]
        end = row["end_time"]
        if not end:
            end = start
        string = "%s%s%s%s%s\n" % (name, fs, start, fs, end)
        # Write the filename, the start_time and the end_time
        list_file.write(string)
        # Export the raster 3d map with r3.pack
        try:
            gscript.run_command("r3.pack", input=name, flags="c")
        except CalledModuleError:
            shutil.rmtree(new_cwd)
            tar.close()
            gscript.fatal(_("Unable to export raster map <%s> with r3.pack" % name))

        tar.add(name + ".pack")


############################################################################


def export_stds(
    input,
    output,
    compression,
    directory,
    where,
    format_="pack",
    type_="strds",
    datatype=None,
    **kwargs,
):
    """Export space time datasets as tar archive with optional compression

    This method should be used to export space time datasets
    of type raster and vector as tar archive that can be reimported
    with the method import_stds().

    :param input: The name of the space time dataset to export
    :param output: The name of the archive file
    :param compression: The compression of the archive file:

          - "no"  no compression
          - "gzip" GNU zip compression
          - "bzip2" Bzip compression

    :param directory: The working directory used for extraction and packing
    :param where: The temporal WHERE SQL statement to select a subset
                  of maps from the space time dataset
    :param format_: The export format:

          - "GTiff" Geotiff format, only for raster maps
          - "AAIGrid" Arc/Info ASCII Grid format, only for raster maps
          - "pack" The GRASS raster, 3D raster or vector Pack format,
                   this is the default setting
          - "GML" GML file export format, only for vector maps,
                  v.out.ogr export option
          - "GPKG" GPKG file export format, only for vector maps,
                  v.out.ogr export option

    :param type_: The space time dataset type

          - "strds" Space time raster dataset
          - "str3ds" Space time 3D raster dataset
          - "stvds" Space time vector dataset
    :param datatype: Force the output datatype for r.out.gdal
    """

    # Save current working directory path
    old_cwd = os.getcwd()

    # Create the temporary directory and jump into it
    new_cwd = tempfile.mkdtemp(dir=directory)
    os.chdir(new_cwd)

    if type_ == "strds":
        columns = "name,start_time,end_time,min,max,datatype"
    elif type_ == "stvds":
        columns = "name,start_time,end_time,layer"
    else:
        columns = "name,start_time,end_time"

    sp = open_old_stds(input, type_)
    rows = sp.get_registered_maps(columns, where, "start_time", None)

    if compression == "gzip":
        flag = "w:gz"
    elif compression == "bzip2":
        flag = "w:bz2"
    else:
        flag = "w:"

    # Open the tar archive to add the files
    tar = tarfile.open(tmp_tar_file_name, flag)
    list_file = open(list_file_name, "w")

    fs = "|"

    if rows:
        if type_ == "strds":
            if format_ == "GTiff" or format_ == "AAIGrid":
                _export_raster_maps_as_gdal(
                    rows, tar, list_file, new_cwd, fs, format_, datatype, **kwargs
                )
            else:
                _export_raster_maps(rows, tar, list_file, new_cwd, fs)
        elif type_ == "stvds":
            if format_ == "GML":
                _export_vector_maps_as_gml(rows, tar, list_file, new_cwd, fs)
            elif format_ == "GPKG":
                _export_vector_maps_as_gpkg(rows, tar, list_file, new_cwd, fs)
            else:
                _export_vector_maps(rows, tar, list_file, new_cwd, fs)
        elif type_ == "str3ds":
            _export_raster3d_maps(rows, tar, list_file, new_cwd, fs)

    list_file.close()

    # Write projection and metadata
    proj = gscript.read_command("g.proj", flags="j")

    proj_file = open(proj_file_name, "w")
    proj_file.write(proj)
    proj_file.close()

    init_file = open(init_file_name, "w")
    # Create the init string
    string = ""
    # This is optional, if not present strds will be assumed for backward
    # compatibility
    string += "%s=%s\n" % ("stds_type", sp.get_type())
    # This is optional, if not present gtiff will be assumed for
    # backward compatibility
    string += "%s=%s\n" % ("format", format_)
    string += "%s=%s\n" % ("temporal_type", sp.get_temporal_type())
    string += "%s=%s\n" % ("semantic_type", sp.get_semantic_type())
    if sp.is_time_relative():
        string += "%s=%s\n" % ("relative_time_unit", sp.get_relative_time_unit())
    # replace sp.metadata.get_number_of_maps() with len(rows)
    # sp.metadata.get_number_of_maps() doesn't  work with where option
    string += "%s=%s\n" % ("number_of_maps", len(rows))
    north, south, east, west, top, bottom = sp.get_spatial_extent_as_tuple()
    string += "%s=%s\n" % ("north", north)
    string += "%s=%s\n" % ("south", south)
    string += "%s=%s\n" % ("east", east)
    string += "%s=%s\n" % ("west", west)
    init_file.write(string)
    init_file.close()

    metadata = gscript.read_command("t.info", type=type_, input=sp.get_id())
    metadata_file = open(metadata_file_name, "w")
    metadata_file.write(metadata)
    metadata_file.close()

    read_file = open(read_file_name, "w")
    if type_ == "strds":
        read_file.write(
            "This space time raster dataset was exported with "
            "t.rast.export of GRASS GIS 7\n"
        )
    elif type_ == "stvds":
        read_file.write(
            "This space time vector dataset was exported with "
            "t.vect.export of GRASS GIS 7\n"
        )
    elif type_ == "str3ds":
        read_file.write(
            "This space time 3D raster dataset was exported "
            "with t.rast3d.export of GRASS GIS 7\n"
        )
    read_file.write("\n")
    read_file.write("Files:\n")
    if type_ == "strds":
        if format_ == "GTiff":
            # 123456789012345678901234567890
            read_file.write("       *.tif  -- GeoTIFF raster files\n")
            read_file.write("     *.color  -- GRASS GIS raster color rules\n")
        elif format_ == "pack":
            read_file.write("      *.pack  -- GRASS raster files packed with r.pack\n")
    elif type_ == "stvds":
        # 123456789012345678901234567890
        if format_ == "GML":
            read_file.write("       *.xml  -- Vector GML files\n")
        else:
            read_file.write("      *.pack  -- GRASS vector files packed with v.pack\n")
    elif type_ == "str3ds":
        read_file.write("      *.pack  -- GRASS 3D raster files packed with r3.pack\n")
    read_file.write(
        "%13s -- Projection information in PROJ.4 format\n" % (proj_file_name)
    )
    read_file.write(
        "%13s -- GRASS GIS space time %s dataset information\n"
        % (init_file_name, sp.get_new_map_instance(None).get_type())
    )
    read_file.write(
        "%13s -- Time series file, lists all maps by name "
        "with interval\n" % (list_file_name)
    )
    read_file.write(
        "                 time stamps in ISO-Format. Field separator is |\n"
    )
    read_file.write("%13s -- The output of t.info\n" % (metadata_file_name))
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
    shutil.move(os.path.join(new_cwd, tmp_tar_file_name), output)

    # Remove the temporary created working directory
    shutil.rmtree(new_cwd)
