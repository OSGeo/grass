# -*- coding: utf-8 -*-
"""
Fast and exit-safe interface to GRASS C-library functions
using ctypes and multiprocessing

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from grass.exceptions import FatalError
import sys
from multiprocessing import Process, Lock, Pipe
import logging
from ctypes import *
from datetime import datetime
import grass.lib.gis as libgis
import grass.lib.raster as libraster
import grass.lib.vector as libvector
import grass.lib.date as libdate
import grass.lib.raster3d as libraster3d
import grass.lib.temporal as libtgis
from grass.pygrass.rpc.base import RPCServerBase
from grass.pygrass.raster import RasterRow
from grass.pygrass.vector import VectorTopo
from grass.script.utils import encode
from grass.pygrass.utils import decode, set_path

###############################################################################


class RPCDefs(object):
    # Function identifier and index
    STOP = 0
    HAS_TIMESTAMP = 1
    WRITE_TIMESTAMP = 2
    READ_TIMESTAMP = 3
    REMOVE_TIMESTAMP = 4
    READ_MAP_INFO = 5
    MAP_EXISTS = 6
    READ_MAP_INFO = 7
    AVAILABLE_MAPSETS = 8
    GET_DRIVER_NAME = 9
    GET_DATABASE_NAME = 10
    G_MAPSET = 11
    G_LOCATION = 12
    G_GISDBASE = 13
    READ_MAP_FULL_INFO = 14
    WRITE_BAND_REFERENCE = 15
    READ_BAND_REFERENCE = 16
    REMOVE_BAND_REFERENCE = 17
    G_FATAL_ERROR = 49

    TYPE_RASTER = 0
    TYPE_RASTER3D = 1
    TYPE_VECTOR = 2

###############################################################################


def _read_map_full_info(lock, conn, data):
    """Read full map specific metadata from the spatial database using
       PyGRASS functions.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name, mapset]
    """
    info = None
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        if maptype == RPCDefs.TYPE_RASTER:
            info = _read_raster_full_info(name, mapset)
        elif maptype == RPCDefs.TYPE_VECTOR:
            info = _read_vector_full_info(name, mapset)
    except:
        raise
    finally:
        conn.send(info)

###############################################################################


def _read_raster_full_info(name, mapset):
    """Read raster info, history and cats using PyGRASS RasterRow
       and return a dictionary. Colors should be supported in the
       future.
    """

    info = {}
    r = RasterRow(name=name, mapset=mapset)
    if r.exist() is True:
        r.open("r")

        for item in r.info:
            info[item[0]] = item[1]

        for item in r.hist:
            info[item[0]] = item[1]

        info["full_name"] = r.name_mapset()
        info["mtype"] = r.mtype
        if r.cats:
            info["cats_title"] = r.cats_title
            info["cats"] = list(r.cats)
        r.close()

        ts = libgis.TimeStamp()
        check = libgis.G_read_raster_timestamp(name, mapset, byref(ts))

        if check:
            dates = _convert_timestamp_from_grass(ts)
            info["start_time"] = dates[0]
            info["end_time"] = dates[1]
            if len(dates) > 2:
                info["time_unit"] = dates[2]

    return(info)

###############################################################################


def _read_vector_full_info(name, mapset, layer = None):
    """Read vector info using PyGRASS VectorTopo
       and return a dictionary. C
    """

    info = {}

    v = VectorTopo(name=name, mapset=mapset)
    if v.exist() is True:
        v.open("r")
        # Bounding box
        for item in v.bbox().items():
            info[item[0]] = item[1]

        info["name"] = v.name
        info["mapset"] = v.mapset
        info["comment"] = v.comment
        info["full_name"] = v.full_name
        info["is_3d"] = v.is_3D()
        info["map_date"] = v.map_date
        info["maptype"] = v.maptype
        info["organization"] = v.organization
        info["person"] = v.person
        info["proj"] = v.proj
        info["proj_name"] = v.proj_name
        info["title"] = v.title
        info["thresh"] = v.thresh
        info["zone"] = v.zone
        vtypes = ['areas', 'dblinks', 'faces', 'holes', 'islands',
                  'kernels', 'lines', 'nodes', 'points', 'updated_lines',
                  'updated_nodes', 'volumes']
        for vtype in vtypes:
            info[vtype] = v.number_of(vtype)

        info.update(v.num_primitives())

        if v.table is not None:
            info["columns"] = v.table.columns

        ts = libgis.TimeStamp()
        check = libgis.G_read_vector_timestamp(name, layer, mapset, byref(ts))

        if check:
            dates = _convert_timestamp_from_grass(ts)
            info["start_time"] = dates[0]
            info["end_time"] = dates[1]
            if len(dates) > 2:
                info["time_unit"] = dates[2]

    return(info)

def _fatal_error(lock, conn, data):
    """Calls G_fatal_error()"""
    libgis.G_fatal_error("Fatal Error in C library server")

###############################################################################


def _get_mapset(lock, conn, data):
    """Return the current mapset

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The mapset as list entry 1 [function_id]

       :returns: Name of the current mapset
    """
    mapset = libgis.G_mapset()
    conn.send(decode(mapset))

###############################################################################


def _get_location(lock, conn, data):
    """Return the current location

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The mapset as list entry 1 [function_id]

       :returns: Name of the location
    """
    location = libgis.G_location()
    conn.send(decode(location))

###############################################################################


def _get_gisdbase(lock, conn, data):
    """Return the current gisdatabase

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The mapset as list entry 1 [function_id]

       :returns: Name of the gisdatabase
    """
    gisdbase = libgis.G_gisdbase()
    conn.send(decode(gisdbase))

###############################################################################


def _get_driver_name(lock, conn, data):
    """Return the temporal database driver of a specific mapset

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The mapset as list entry 1 [function_id, mapset]

       :returns: Name of the driver or None if no temporal database present
    """
    mapset = data[1]
    if not mapset:
        mapset = libgis.G_mapset()
    else:
        mapset = encode(mapset)

    drstring = libtgis.tgis_get_mapset_driver_name(mapset)
    conn.send(decode(drstring.data))

###############################################################################


def _get_database_name(lock, conn, data):
    """Return the temporal database name of a specific mapset

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The mapset as list entry 1 [function_id, mapset]

       :returns: Name of the database or None if no temporal database present
    """
    dbstring = None
    try:
        mapset = data[1]
        if not mapset:
            mapset = libgis.G_mapset()
        else:
            mapset = encode(mapset)
        dbstring = libtgis.tgis_get_mapset_database_name(mapset)
        dbstring = dbstring.data

        if dbstring:
            # We substitute GRASS variables if they are located in the database string
            # This behavior is in conjunction with db.connect
            dbstring = dbstring.replace(encode("$GISDBASE"), libgis.G_gisdbase())
            dbstring = dbstring.replace(encode("$LOCATION_NAME"), libgis.G_location())
            dbstring = dbstring.replace(encode("$MAPSET"), mapset)
    except:
        raise
    finally:
        conn.send(decode(dbstring))

###############################################################################


def _available_mapsets(lock, conn, data):
    """Return all available mapsets the user can access as a list of strings

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id]

       :returns: Names of available mapsets as list of strings
    """

    count = 0
    mapset_list = []
    try:
        # Initialize the accessible mapset list, this is bad C design!!!
        libgis.G_get_mapset_name(0)
        mapsets = libgis.G_get_available_mapsets()
        while mapsets[count]:
            char_list = ""
            mapset = mapsets[count]

            permission = libgis.G_mapset_permissions(mapset)
            in_search_path = libgis.G_is_mapset_in_search_path(mapset)

            c = 0
            while mapset[c] != b"\x00":
                val = decode(mapset[c])
                char_list += val
                c += 1

            if permission >= 0 and in_search_path == 1:
                mapset_list.append(char_list)

            libgis.G_debug(1, "c_library_server._available_mapsets: \n  mapset:  %s\n"
                              "  has permission %i\n  in search path: %i" % (char_list,
                              permission, in_search_path))
            count += 1

        # We need to sort the mapset list, but the first one should be
        # the current mapset
        current_mapset = decode(libgis.G_mapset())
        if current_mapset in mapset_list:
            mapset_list.remove(current_mapset)
        mapset_list.sort()
        mapset_list.reverse()
        mapset_list.append(current_mapset)
        mapset_list.reverse()
    except:
        raise
    finally:
        conn.send(mapset_list)

###############################################################################


def _has_timestamp(lock, conn, data):
    """Check if the file based GRASS timestamp is present and send
       True or False using the provided pipe.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name,
                    mapset, layer]

    """
    check = False
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        layer = data[4]
        if maptype == RPCDefs.TYPE_RASTER:
            if libgis.G_has_raster_timestamp(name, mapset) == 1:
                check = True
        elif maptype == RPCDefs.TYPE_VECTOR:
            if libgis.G_has_vector_timestamp(name, layer, mapset) == 1:
                check = True
        elif maptype == RPCDefs.TYPE_RASTER3D:
            if libgis.G_has_raster3d_timestamp(name, mapset) == 1:
                check = True
    except:
        raise
    finally:
        conn.send(check)

###############################################################################


def _read_timestamp(lock, conn, data):
    """Read the file based GRASS timestamp and send
       the result using the provided pipe.

       The tuple to be send via pipe: (return value of G_read_*_timestamp,
       timestamps).

       Please have a look at the documentation of G_read_raster_timestamp,
       G_read_vector_timestamp and G_read_raster3d_timestamp for the return
       values description.

       The timestamps to be send are tuples of values:

           - relative time (start, end, unit), start and end are of type
             integer, unit is of type string.
           - absolute time (start, end), start and end are of type datetime

       The end time may be None in case of a time instance.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send the result
       :param data: The list of data entries [function_id, maptype, name,
                    mapset, layer]

    """
    check = False
    dates = None
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        layer = data[4]
        ts = libgis.TimeStamp()
        if maptype == RPCDefs.TYPE_RASTER:
            check = libgis.G_read_raster_timestamp(name, mapset, byref(ts))
        elif maptype == RPCDefs.TYPE_VECTOR:
            check = libgis.G_read_vector_timestamp(name, layer, mapset, byref(ts))
        elif maptype == RPCDefs.TYPE_RASTER3D:
            check = libgis.G_read_raster3d_timestamp(name, mapset, byref(ts))

        dates = _convert_timestamp_from_grass(ts)
    except:
        raise
    finally:
        conn.send((check, dates))

###############################################################################


def _write_timestamp(lock, conn, data):
    """Write the file based GRASS timestamp
       the return values of the called C-functions using the provided pipe.

       The value to be send via pipe is the return value of G_write_*_timestamp.

       Please have a look at the documentation of G_write_raster_timestamp,
       G_write_vector_timestamp and G_write_raster3d_timestamp for the return
       values description.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name,
                    mapset, layer, timestring]
    """
    check = -3
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        layer = data[4]
        timestring = data[5]
        ts = libgis.TimeStamp()
        check = libgis.G_scan_timestamp(byref(ts), timestring)

        if check != 1:
            logging.error("Unable to convert the timestamp: " + timestring)
            return -2

        if maptype == RPCDefs.TYPE_RASTER:
            check = libgis.G_write_raster_timestamp(name, byref(ts))
        elif maptype == RPCDefs.TYPE_VECTOR:
            check = libgis.G_write_vector_timestamp(name, layer, byref(ts))
        elif maptype == RPCDefs.TYPE_RASTER3D:
            check = libgis.G_write_raster3d_timestamp(name, byref(ts))
    except:
        raise
    finally:
        conn.send(check)

###############################################################################


def _remove_timestamp(lock, conn, data):
    """Remove the file based GRASS timestamp
       the return values of the called C-functions using the provided pipe.

       The value to be send via pipe is the return value of G_remove_*_timestamp.

       Please have a look at the documentation of G_remove_raster_timestamp,
       G_remove_vector_timestamp and G_remove_raster3d_timestamp for the return
       values description.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name,
                    mapset, layer]

    """
    check = False
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        layer = data[4]
        if maptype == RPCDefs.TYPE_RASTER:
            check = libgis.G_remove_raster_timestamp(name, mapset)
        elif maptype == RPCDefs.TYPE_VECTOR:
            check = libgis.G_remove_vector_timestamp(name, layer, mapset)
        elif maptype == RPCDefs.TYPE_RASTER3D:
            check = libgis.G_remove_raster3d_timestamp(name, mapset)
    except:
        raise
    finally:
        conn.send(check)

###############################################################################


def _read_band_reference(lock, conn, data):
    """Read the file based GRASS band identifier
       the result using the provided pipe.

       The tuple to be send via pipe: (return value of
       Rast_read_band_reference).

       Please have a look at the documentation of
       Rast_read_band_reference, for the return values description.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name,
                    mapset, layer, timestring]

    """
    check = False
    band_ref = None
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        layer = data[4]

        if maptype == RPCDefs.TYPE_RASTER:
            p_filename = c_char_p()
            p_band_ref = c_char_p()
            check = libraster.Rast_read_band_reference(name, mapset,
                                                       byref(p_filename),
                                                       byref(p_band_ref))
            if check:
                band_ref = decode(p_band_ref.value)
                libgis.G_free(p_filename)
                libgis.G_free(p_band_ref)
        else:
            logging.error("Unable to read band reference. "
                          "Unsupported map type %s" % maptype)
            return -1
    except:
        raise
    finally:
        conn.send((check, band_ref))

###############################################################################


def _write_band_reference(lock, conn, data):
    """Write the file based GRASS band identifier
       the return values of the called C-functions using the provided pipe.

       The value to be send via pipe is the return value of Rast_write_band_reference.

       Please have a look at the documentation of
       Rast_write_band_reference, for the return values description.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name,
                    mapset, layer, timestring]

    """
    check = -3
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        layer = data[4]
        band_reference = data[5]

        if maptype == RPCDefs.TYPE_RASTER:
            from grass.bandref import BandReferenceReader

            reader = BandReferenceReader()
            # determine filename (assuming that band_reference is unique!)
            filename = reader.find_file(band_reference)

            check = libraster.Rast_write_band_reference(name,
                                                        filename,
                                                        band_reference)
        else:
            logging.error("Unable to write band reference. "
                          "Unsupported map type %s" % maptype)
            return -2
    except:
        raise
    finally:
        conn.send(check)

###############################################################################


def _remove_band_reference(lock, conn, data):
    """Remove the file based GRASS band identifier
       the return values of the called C-functions using the provided pipe.

       The value to be send via pipe is the return value of Rast_remove_band_reference.

       Please have a look at the documentation of
       Rast_remove_band_reference, for the return values description.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name,
                    mapset, layer, timestring]

    """
    check = False
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        layer = data[4]

        if maptype == RPCDefs.TYPE_RASTER:
            check = libraster.Rast_remove_band_reference(name)
        else:
            logging.error("Unable to remove band reference. "
                          "Unsupported map type %s" % maptype)
            return -2
    except:
        raise
    finally:
        conn.send(check)

###############################################################################


def _map_exists(lock, conn, data):
    """Check if a map exists in the spatial database

       The value to be send via pipe is True in case the map exists and False
       if not.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name, mapset]

    """
    check = False
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        if maptype == RPCDefs.TYPE_RASTER:
            mapset = libgis.G_find_raster(name, mapset)
        elif maptype == RPCDefs.TYPE_VECTOR:
            mapset = libgis.G_find_vector(name, mapset)
        elif maptype == RPCDefs.TYPE_RASTER3D:
            mapset = libgis.G_find_raster3d(name, mapset)

        if mapset:
            check = True
    except:
        raise
    finally:
        conn.send(check)

###############################################################################


def _read_map_info(lock, conn, data):
    """Read map specific metadata from the spatial database using C-library
       functions

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, maptype, name, mapset]
    """
    kvp = None
    try:
        maptype = data[1]
        name = data[2]
        mapset = data[3]
        if maptype == RPCDefs.TYPE_RASTER:
            kvp = _read_raster_info(name, mapset)
        elif maptype == RPCDefs.TYPE_VECTOR:
            kvp = _read_vector_info(name, mapset)
        elif maptype == RPCDefs.TYPE_RASTER3D:
            kvp = _read_raster3d_info(name, mapset)
    except:
        raise
    finally:
        conn.send(kvp)

###############################################################################


def _read_raster_info(name, mapset):
    """Read the raster map info from the file system and store the content
       into a dictionary

       This method uses the ctypes interface to the gis and raster libraries
       to read the map metadata information

       :param name: The name of the map
       :param mapset: The mapset of the map
       :returns: The key value pairs of the map specific metadata, or None in
                 case of an error
    """

    kvp = {}

    if not libgis.G_find_raster(name, mapset):
        return None

    # Read the region information
    region = libraster.struct_Cell_head()
    libraster.Rast_get_cellhd(name, mapset, byref(region))

    kvp["north"] = region.north
    kvp["south"] = region.south
    kvp["east"] = region.east
    kvp["west"] = region.west
    kvp["nsres"] = region.ns_res
    kvp["ewres"] = region.ew_res
    kvp["rows"] = region.cols
    kvp["cols"] = region.rows

    maptype = libraster.Rast_map_type(name, mapset)

    if maptype == libraster.DCELL_TYPE:
        kvp["datatype"] = "DCELL"
    elif maptype == libraster.FCELL_TYPE:
        kvp["datatype"] = "FCELL"
    elif maptype == libraster.CELL_TYPE:
        kvp["datatype"] = "CELL"

    # Read range
    if libraster.Rast_map_is_fp(name, mapset):
        range = libraster.FPRange()
        libraster.Rast_init_fp_range(byref(range))
        ret = libraster.Rast_read_fp_range(name, mapset, byref(range))
        if ret < 0:
            logging.error(_("Unable to read range file"))
            return None
        if ret == 2:
            kvp["min"] = None
            kvp["max"] = None
        else:
            min = libgis.DCELL()
            max = libgis.DCELL()
            libraster.Rast_get_fp_range_min_max(
                byref(range), byref(min), byref(max))
            kvp["min"] = min.value
            kvp["max"] = max.value
    else:
        range = libraster.Range()
        libraster.Rast_init_range(byref(range))
        ret = libraster.Rast_read_range(name, mapset, byref(range))
        if ret < 0:
            logging.error(_("Unable to read range file"))
            return None
        if ret == 2:
            kvp["min"] = None
            kvp["max"] = None
        else:
            min = libgis.CELL()
            max = libgis.CELL()
            libraster.Rast_get_range_min_max(
                byref(range), byref(min), byref(max))
            kvp["min"] = min.value
            kvp["max"] = max.value

    return kvp

###############################################################################


def _read_raster3d_info(name, mapset):
    """Read the 3D raster map info from the file system and store the content
       into a dictionary

       This method uses the ctypes interface to the gis and raster3d libraries
       to read the map metadata information

       :param name: The name of the map
       :param mapset: The mapset of the map
       :returns: The key value pairs of the map specific metadata, or None in
                 case of an error
    """

    kvp = {}

    if not libgis.G_find_raster3d(name, mapset):
        return None

    # Read the region information
    region = libraster3d.RASTER3D_Region()
    libraster3d.Rast3d_read_region_map(name, mapset, byref(region))

    kvp["north"] = region.north
    kvp["south"] = region.south
    kvp["east"] = region.east
    kvp["west"] = region.west
    kvp["nsres"] = region.ns_res
    kvp["ewres"] = region.ew_res
    kvp["tbres"] = region.tb_res
    kvp["rows"] = region.cols
    kvp["cols"] = region.rows
    kvp["depths"] = region.depths
    kvp["top"] = region.top
    kvp["bottom"] = region.bottom

    # We need to open the map, this function returns a void pointer
    # but we may need the correct type which is RASTER3D_Map, hence
    # the casting
    g3map = cast(libraster3d.Rast3d_open_cell_old(name, mapset,
                 libraster3d.RASTER3D_DEFAULT_WINDOW,
                 libraster3d.RASTER3D_TILE_SAME_AS_FILE,
                 libraster3d.RASTER3D_NO_CACHE),
                 POINTER(libraster3d.RASTER3D_Map))

    if not g3map:
        logging.error(_("Unable to open 3D raster map <%s>" % (name)))
        return None

    maptype = libraster3d.Rast3d_file_type_map(g3map)

    if maptype == libraster.DCELL_TYPE:
        kvp["datatype"] = "DCELL"
    elif maptype == libraster.FCELL_TYPE:
        kvp["datatype"] = "FCELL"

    # Read range
    min = libgis.DCELL()
    max = libgis.DCELL()
    ret = libraster3d.Rast3d_range_load(g3map)
    if not ret:
        logging.error(_("Unable to load range of 3D raster map <%s>" %
                      (name)))
        return None
    libraster3d.Rast3d_range_min_max(g3map, byref(min), byref(max))

    if min.value != min.value:
        kvp["min"] = None
    else:
        kvp["min"] = float(min.value)
    if max.value != max.value:
        kvp["max"] = None
    else:
        kvp["max"] = float(max.value)

    if not libraster3d.Rast3d_close(g3map):
        logging.error(_("Unable to close 3D raster map <%s>" % (name)))
        return None

    return kvp

###############################################################################


def _read_vector_info(name, mapset):
    """Read the vector map info from the file system and store the content
       into a dictionary

       This method uses the ctypes interface to the vector libraries
       to read the map metadata information

       :param name: The name of the map
       :param mapset: The mapset of the map
       :returns: The key value pairs of the map specific metadata, or None in
                 case of an error
    """

    kvp = {}

    if not libgis.G_find_vector(name, mapset):
        return None

    # The vector map structure
    Map = libvector.Map_info()

    # We open the maps always in topology mode first
    libvector.Vect_set_open_level(2)
    with_topo = True

    # Code lend from v.info main.c
    if libvector.Vect_open_old_head2(byref(Map), name, mapset, "1") < 2:
        # force level 1, open fully
        # NOTE: number of points, lines, boundaries, centroids,
        # faces, kernels is still available
        libvector.Vect_set_open_level(1)  # no topology
        with_topo = False
        if libvector.Vect_open_old2(byref(Map), name, mapset, "1") < 1:
            logging.error(_("Unable to open vector map <%s>" %
                          (libvector.Vect_get_full_name(byref(Map)))))
            return None

    # Release the vector spatial index memory when closed
    libvector.Vect_set_release_support(byref(Map))

    # Read the extent information
    bbox = libvector.bound_box()
    libvector.Vect_get_map_box(byref(Map), byref(bbox))

    kvp["north"] = bbox.N
    kvp["south"] = bbox.S
    kvp["east"] = bbox.E
    kvp["west"] = bbox.W
    kvp["top"] = bbox.T
    kvp["bottom"] = bbox.B

    kvp["map3d"] = bool(libvector.Vect_is_3d(byref(Map)))

    # Read number of features
    if with_topo:
        kvp["points"] = libvector.Vect_get_num_primitives(
            byref(Map), libvector.GV_POINT)
        kvp["lines"] = libvector.Vect_get_num_primitives(
            byref(Map), libvector.GV_LINE)
        kvp["boundaries"] = libvector.Vect_get_num_primitives(
            byref(Map), libvector.GV_BOUNDARY)
        kvp["centroids"] = libvector.Vect_get_num_primitives(
            byref(Map), libvector.GV_CENTROID)
        kvp["faces"] = libvector.Vect_get_num_primitives(
            byref(Map), libvector.GV_FACE)
        kvp["kernels"] = libvector.Vect_get_num_primitives(
            byref(Map), libvector.GV_KERNEL)

        # Summarize the primitives
        kvp["primitives"] = kvp["points"] + kvp["lines"] + \
            kvp["boundaries"] + kvp["centroids"]
        if kvp["map3d"]:
            kvp["primitives"] += kvp["faces"] + kvp["kernels"]

        # Read topology information
        kvp["nodes"] = libvector.Vect_get_num_nodes(byref(Map))
        kvp["areas"] = libvector.Vect_get_num_areas(byref(Map))
        kvp["islands"] = libvector.Vect_get_num_islands(byref(Map))
        kvp["holes"] = libvector.Vect_get_num_holes(byref(Map))
        kvp["volumes"] = libvector.Vect_get_num_primitives(
            byref(Map), libvector.GV_VOLUME)
    else:
        kvp["points"] = None
        kvp["lines"] = None
        kvp["boundaries"] = None
        kvp["centroids"] = None
        kvp["faces"] = None
        kvp["kernels"] = None
        kvp["primitives"] = None
        kvp["nodes"] = None
        kvp["areas"] = None
        kvp["islands"] = None
        kvp["holes"] = None
        kvp["volumes"] = None

    libvector.Vect_close(byref(Map))

    return kvp

###############################################################################


def _convert_timestamp_from_grass(ts):
    """Convert a GRASS file based timestamp into the temporal framework
       format datetime or integer.

       A tuple of two datetime objects (start, end) is returned in case of
       absolute time.
       In case of relative time a tuple with start time, end time and the
       relative unit (start, end, unit) will be returned.

       Note:
       The end time will be set to None in case of a time instance.

       :param ts grass.lib.gis.TimeStamp object created by G_read_*_timestamp
    """

    dt1 = libgis.DateTime()
    dt2 = libgis.DateTime()
    count = c_int()

    libgis.G_get_timestamps(byref(ts),
                            byref(dt1),
                            byref(dt2),
                            byref(count))

    if dt1.mode == libdate.DATETIME_ABSOLUTE:
        pdt1 = None
        pdt2 = None
        if count.value >= 1:
            pdt1 = datetime(int(dt1.year), int(dt1.month), int(dt1.day),
                            int(dt1.hour), int(dt1.minute),
                            int(dt1.second))
        if count.value == 2:
            pdt2 = datetime(int(dt2.year), int(dt2.month), int(dt2.day),
                            int(dt2.hour), int(dt2.minute),
                            int(dt2.second))

        # ATTENTION: We ignore the time zone
        # TODO: Write time zone support
        return (pdt1, pdt2)
    else:
        unit = None
        start = None
        end = None
        if count.value >= 1:
            if dt1.year > 0:
                unit = "years"
                start = dt1.year
            elif dt1.month > 0:
                unit = "months"
                start = dt1.month
            elif dt1.day > 0:
                unit = "days"
                start = dt1.day
            elif dt1.hour > 0:
                unit = "hours"
                start = dt1.hour
            elif dt1.minute > 0:
                unit = "minutes"
                start = dt1.minute
            elif dt1.second > 0:
                unit = "seconds"
                start = dt1.second
        if count.value == 2:
            if dt2.year > 0:
                end = dt2.year
            elif dt2.month > 0:
                end = dt2.month
            elif dt2.day > 0:
                end = dt2.day
            elif dt2.hour > 0:
                end = dt2.hour
            elif dt2.minute > 0:
                end = dt2.minute
            elif dt2.second > 0:
                end = dt2.second
        return (start, end, unit)

###############################################################################

def _stop(lock, conn, data):
    libgis.G_debug(1, "Stop C-interface server")
    conn.close()
    lock.release()
    sys.exit()

###############################################################################


def c_library_server(lock, conn):
    """The GRASS C-libraries server function designed to be a target for
       multiprocessing.Process

       :param lock: A multiprocessing.Lock
       :param conn: A multiprocessing.Pipe
    """

    def error_handler(data):
        """This function will be called in case of a fatal error in libgis"""
        #sys.stderr.write("Error handler was called\n")
        # We send an exception that will be handled in
        # the parent process, then close the pipe
        # and release any possible lock
        conn.send(FatalError())
        conn.close()
        lock.release()

    CALLBACK = CFUNCTYPE(c_void_p, c_void_p)
    CALLBACK.restype = c_void_p
    CALLBACK.argtypes = c_void_p

    cerror_handler = CALLBACK(error_handler)

    libgis.G_add_error_handler(cerror_handler, None)

    # Crerate the function array
    functions = [0]*50
    functions[RPCDefs.STOP] = _stop
    functions[RPCDefs.HAS_TIMESTAMP] = _has_timestamp
    functions[RPCDefs.WRITE_TIMESTAMP] = _write_timestamp
    functions[RPCDefs.READ_TIMESTAMP] = _read_timestamp
    functions[RPCDefs.REMOVE_TIMESTAMP] = _remove_timestamp
    functions[RPCDefs.READ_MAP_INFO] = _read_map_info
    functions[RPCDefs.MAP_EXISTS] = _map_exists
    functions[RPCDefs.AVAILABLE_MAPSETS] = _available_mapsets
    functions[RPCDefs.GET_DRIVER_NAME] = _get_driver_name
    functions[RPCDefs.GET_DATABASE_NAME] = _get_database_name
    functions[RPCDefs.G_MAPSET] = _get_mapset
    functions[RPCDefs.G_LOCATION] = _get_location
    functions[RPCDefs.G_GISDBASE] = _get_gisdbase
    functions[RPCDefs.READ_MAP_FULL_INFO] = _read_map_full_info
    functions[RPCDefs.WRITE_BAND_REFERENCE] = _write_band_reference
    functions[RPCDefs.READ_BAND_REFERENCE] = _read_band_reference
    functions[RPCDefs.REMOVE_BAND_REFERENCE] = _remove_band_reference
    functions[RPCDefs.G_FATAL_ERROR] = _fatal_error

    libgis.G_gisinit("c_library_server")
    libgis.G_debug(1, "Start C-interface server")

    while True:
        # Avoid busy waiting
        conn.poll(None)
        data = conn.recv()
        lock.acquire()
        functions[data[0]](lock, conn, data)
        lock.release()

class CLibrariesInterface(RPCServerBase):
    """Fast and exit-safe interface to GRASS C-libraries functions

       This class implements a fast and exit-safe interface to the GRASS
       gis, raster, 3D raster and vector  C-libraries functions.

       The C-libraries functions are called via ctypes in a subprocess
       using a pipe (multiprocessing.Pipe) to transfer the text messages.
       Hence, the process that uses the CLibrariesInterface will not be
       exited, if a G_fatal_error() was invoked in the subprocess.
       In this case the CLibrariesInterface object will simply start a
       new subprocess and restarts the pipeline.


       Usage:

       .. code-block:: python

           >>> import grass.script as gscript
           >>> import grass.temporal as tgis
           >>> gscript.use_temp_region()
           >>> gscript.run_command("g.region", n=80.0, s=0.0, e=120.0, w=0.0,
           ... t=1.0, b=0.0, res=10.0, res3=10.0)
           0
           >>> tgis.init()
           >>> gscript.run_command("r.mapcalc", expression="test = 1", overwrite=True, quiet=True)
           0
           >>> gscript.run_command("r3.mapcalc", expression="test = 1", overwrite=True, quiet=True)
           0
           >>> gscript.run_command("v.random", output="test", n=10, overwrite=True, quiet=True)
           0
           >>> gscript.run_command("r.timestamp", map="test", date='12 Mar 1995 10:34:40', overwrite=True, quiet=True)
           0
           >>> gscript.run_command("r3.timestamp", map="test", date='12 Mar 1995 10:34:40', overwrite=True, quiet=True)
           0
           >>> gscript.run_command("v.timestamp", map="test", date='12 Mar 1995 10:34:40', overwrite=True, quiet=True)
           0

           # Check mapsets
           >>> ciface = tgis.CLibrariesInterface()
           >>> mapsets = ciface.available_mapsets()
           >>> mapsets[0] == tgis.get_current_mapset()
           True

           # Raster map
           >>> ciface = tgis.CLibrariesInterface()
           >>> check = ciface.raster_map_exists("test", tgis.get_current_mapset())
           >>> print check
           True
           >>> ciface.read_raster_info("test", tgis.get_current_mapset())
           {'rows': 12, 'north': 80.0, 'min': 1, 'datatype': 'CELL', 'max': 1, 'ewres': 10.0, 'cols': 8, 'west': 0.0, 'east': 120.0, 'nsres': 10.0, 'south': 0.0}

           >>> info = ciface.read_raster_full_info("test", tgis.get_current_mapset())
           >>> info           # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
           {u'tbres': 1.0, ... 'keyword': 'generated by r.mapcalc',
            u'bottom': 0.0, 'end_time': None, 'title': 'test', u'south': 0.0}

           >>> info["start_time"]
           datetime.datetime(1995, 3, 12, 10, 34, 40)
           >>> info["end_time"]

           >>> check = ciface.has_raster_timestamp("test", tgis.get_current_mapset())
           >>> print check
           True
           >>> if check:
           ...     res = ciface.read_raster_timestamp("test", tgis.get_current_mapset())
           ...     if res[0]:
           ...         print str(res[1][0]), str(res[1][0])
           ...         ciface.remove_raster_timestamp("test", tgis.get_current_mapset())
           1995-03-12 10:34:40 1995-03-12 10:34:40
           1
           >>> ciface.has_raster_timestamp("test", tgis.get_current_mapset())
           False
           >>> ciface.write_raster_timestamp("test", tgis.get_current_mapset(), "13 Jan 1999 14:30:05")
           1
           >>> ciface.has_raster_timestamp("test", tgis.get_current_mapset())
           True


           # 3D raster map
           >>> check = ciface.raster3d_map_exists("test", tgis.get_current_mapset())
           >>> print check
           True
           >>> ciface.read_raster3d_info("test", tgis.get_current_mapset())
           {'tbres': 1.0, 'rows': 12, 'north': 80.0, 'bottom': 0.0, 'datatype': 'DCELL', 'max': 1.0, 'top': 1.0, 'min': 1.0, 'cols': 8, 'depths': 1, 'west': 0.0, 'ewres': 10.0, 'east': 120.0, 'nsres': 10.0, 'south': 0.0}
           >>> check = ciface.has_raster3d_timestamp("test", tgis.get_current_mapset())
           >>> print check
           True
           >>> if check:
           ...     res = ciface.read_raster3d_timestamp("test", tgis.get_current_mapset())
           ...     if res[0]:
           ...         print str(res[1][0]), str(res[1][0])
           ...         ciface.remove_raster3d_timestamp("test", tgis.get_current_mapset())
           1995-03-12 10:34:40 1995-03-12 10:34:40
           1
           >>> ciface.has_raster3d_timestamp("test", tgis.get_current_mapset())
           False
           >>> ciface.write_raster3d_timestamp("test", tgis.get_current_mapset(), "13 Jan 1999 14:30:05")
           1
           >>> ciface.has_raster3d_timestamp("test", tgis.get_current_mapset())
           True


           # Vector map
           >>> check = ciface.vector_map_exists("test", tgis.get_current_mapset())
           >>> print check
           True
           >>> kvp = ciface.read_vector_info("test", tgis.get_current_mapset())
           >>> kvp['points']
           10

           >>> kvp = ciface.read_vector_full_info("test", tgis.get_current_mapset())
           >>> print kvp['points']
           10
           >>> kvp['point']
           10
           >>> kvp['area']
           0
           >>> kvp['lines']
           0
           >>> kvp['line']
           0
           >>> 'columns' in kvp
           False
           >>> kvp["start_time"]
           datetime.datetime(1995, 3, 12, 10, 34, 40)
           >>> kvp["end_time"]

           >>> check = ciface.has_vector_timestamp("test", tgis.get_current_mapset(), None)
           >>> print check
           True
           >>> if check:
           ...     res = ciface.read_vector_timestamp("test", tgis.get_current_mapset())
           ...     if res[0]:
           ...         print str(res[1][0]), str(res[1][0])
           ...         ciface.remove_vector_timestamp("test", tgis.get_current_mapset())
           1995-03-12 10:34:40 1995-03-12 10:34:40
           1
           >>> ciface.has_vector_timestamp("test", tgis.get_current_mapset())
           False
           >>> ciface.write_vector_timestamp("test", tgis.get_current_mapset(), "13 Jan 1999 14:30:05")
           1
           >>> ciface.has_vector_timestamp("test", tgis.get_current_mapset())
           True

           >>> ciface.get_driver_name()
           'sqlite'
           >>> ciface.get_database_name().split("/")[-1]
           'sqlite.db'

           >>> mapset = ciface.get_mapset()
           >>> location = ciface.get_location()
           >>> gisdbase = ciface.get_gisdbase()

           >>> ciface.fatal_error() # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
           Traceback (most recent call last):
               raise FatalError("Exception raised: " + str(e) + " Message: " + message)
           FatalError: Exception raised:  ...

           >>> ciface.fatal_error() # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
           Traceback (most recent call last):
               raise FatalError("Exception raised: " + str(e) + " Message: " + message)
           FatalError: Exception raised:  ...

           >>> ciface.fatal_error() # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
           Traceback (most recent call last):
               raise FatalError("Exception raised: " + str(e) + " Message: " + message)
           FatalError: Exception raised:  ...

           >>> ciface.fatal_error() # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
           Traceback (most recent call last):
               raise FatalError("Exception raised: " + str(e) + " Message: " + message)
           FatalError: Exception raised:  ...

           >>> ciface.stop()

           >>> gscript.del_temp_region()

    """

    def __init__(self):
        RPCServerBase.__init__(self)

    def start_server(self):
        self.client_conn, self.server_conn = Pipe(True)
        self.lock = Lock()
        self.server = Process(target=c_library_server, args=(self.lock,
                                                             self.server_conn))
        self.server.daemon = True
        self.server.start()

    def raster_map_exists(self, name, mapset):
        """Check if a raster map exists in the spatial database

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: True if exists, False if not
       """
        self.check_server()
        self.client_conn.send([RPCDefs.MAP_EXISTS, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("raster_map_exists")

    def read_raster_info(self, name, mapset):
        """Read the raster map info from the file system and store the content
           into a dictionary

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The key value pairs of the map specific metadata,
                     or None in case of an error
        """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_MAP_INFO, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("read_raster_info")

    def read_raster_full_info(self, name, mapset):
        """Read raster info, history and cats using PyGRASS RasterRow
           and return a dictionary. Colors should be supported in the
           future.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The key value pairs of the map specific metadata,
                     or None in case of an error
        """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_MAP_FULL_INFO,
                               RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("read_raster_full_info")

    def has_raster_timestamp(self, name, mapset):
        """Check if a file based raster timestamp exists

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: True if exists, False if not
       """
        self.check_server()
        self.client_conn.send([RPCDefs.HAS_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("has_raster_timestamp")

    def remove_raster_timestamp(self, name, mapset):
        """Remove a file based raster timestamp

           Please have a look at the documentation G_remove_raster_timestamp
           for the return values description.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The return value of G_remove_raster_timestamp
       """
        self.check_server()
        self.client_conn.send([RPCDefs.REMOVE_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("remove_raster_timestamp")

    def read_raster_timestamp(self, name, mapset):
        """Read a file based raster timestamp

           Please have a look at the documentation G_read_raster_timestamp
           for the return values description.

           The timestamps to be send are tuples of values:
               - relative time (start, end, unit), start and end are of type
                 integer, unit is of type string.
               - absolute time (start, end), start and end are of type datetime

           The end time may be None in case of a time instance.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The return value of G_read_raster_timestamp
       """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("read_raster_timestamp")

    def write_raster_timestamp(self, name, mapset, timestring):
        """Write a file based raster timestamp

           Please have a look at the documentation G_write_raster_timestamp
           for the return values description.

           Note:
               Only timestamps of maps from the current mapset can written.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :param timestring: A GRASS datetime C-library compatible string
           :returns: The return value of G_write_raster_timestamp
        """
        self.check_server()
        self.client_conn.send([RPCDefs.WRITE_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None, timestring])
        return self.safe_receive("write_raster_timestamp")

    def remove_raster_band_reference(self, name, mapset):
        """Remove a file based raster band reference

           Please have a look at the documentation Rast_remove_band_reference
           for the return values description.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The return value of Rast_remove_band_reference
       """
        self.check_server()
        self.client_conn.send([RPCDefs.REMOVE_BAND_REFERENCE, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("remove_raster_timestamp")

    def read_raster_band_reference(self, name, mapset):
        """Read a file based raster band reference

           Please have a look at the documentation Rast_read_band_reference
           for the return values description.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The return value of Rast_read_band_reference
        """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_BAND_REFERENCE, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.safe_receive("read_raster_band_reference")

    def write_raster_band_reference(self, name, mapset, band_reference):
        """Write a file based raster band reference

           Please have a look at the documentation Rast_write_band_reference
           for the return values description.

           Note:
               Only band references of maps from the current mapset can written.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :param band_reference: band reference identifier
           :returns: The return value of Rast_write_band_reference
        """
        self.check_server()
        self.client_conn.send([RPCDefs.WRITE_BAND_REFERENCE, RPCDefs.TYPE_RASTER,
                               name, mapset, None, band_reference])
        return self.safe_receive("write_raster_band_reference")

    def raster3d_map_exists(self, name, mapset):
        """Check if a 3D raster map exists in the spatial database

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: True if exists, False if not
       """
        self.check_server()
        self.client_conn.send([RPCDefs.MAP_EXISTS, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.safe_receive("raster3d_map_exists")

    def read_raster3d_info(self, name, mapset):
        """Read the 3D raster map info from the file system and store the content
           into a dictionary

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The key value pairs of the map specific metadata,
                     or None in case of an error
        """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_MAP_INFO, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.safe_receive("read_raster3d_info")

    def has_raster3d_timestamp(self, name, mapset):
        """Check if a file based 3D raster timestamp exists

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: True if exists, False if not
       """
        self.check_server()
        self.client_conn.send([RPCDefs.HAS_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.safe_receive("has_raster3d_timestamp")

    def remove_raster3d_timestamp(self, name, mapset):
        """Remove a file based 3D raster timestamp

           Please have a look at the documentation G_remove_raster3d_timestamp
           for the return values description.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The return value of G_remove_raster3d_timestamp
       """
        self.check_server()
        self.client_conn.send([RPCDefs.REMOVE_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.safe_receive("remove_raster3d_timestamp")

    def read_raster3d_timestamp(self, name, mapset):
        """Read a file based 3D raster timestamp

           Please have a look at the documentation G_read_raster3d_timestamp
           for the return values description.

           The timestamps to be send are tuples of values:
               - relative time (start, end, unit), start and end are of type
                 integer, unit is of type string.
               - absolute time (start, end), start and end are of type datetime

           The end time may be None in case of a time instance.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The return value of G_read_raster3d_timestamp
       """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.safe_receive("read_raster3d_timestamp")

    def write_raster3d_timestamp(self, name, mapset, timestring):
        """Write a file based 3D raster timestamp

           Please have a look at the documentation G_write_raster3d_timestamp
           for the return values description.

           Note:
               Only timestamps of maps from the current mapset can written.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :param timestring: A GRASS datetime C-library compatible string
           :returns: The return value of G_write_raster3d_timestamp
        """
        self.check_server()
        self.client_conn.send([RPCDefs.WRITE_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None, timestring])
        return self.safe_receive("write_raster3d_timestamp")

    def vector_map_exists(self, name, mapset):
        """Check if a vector map exists in the spatial database

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: True if exists, False if not
       """
        self.check_server()
        self.client_conn.send([RPCDefs.MAP_EXISTS, RPCDefs.TYPE_VECTOR,
                               name, mapset, None])
        return self.safe_receive("vector_map_exists")

    def read_vector_info(self, name, mapset):
        """Read the vector map info from the file system and store the content
           into a dictionary

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The key value pairs of the map specific metadata,
                     or None in case of an error
        """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_MAP_INFO, RPCDefs.TYPE_VECTOR,
                               name, mapset, None])
        return self.safe_receive("read_vector_info")

    def read_vector_full_info(self, name, mapset):
        """Read vector info using PyGRASS VectorTopo
           and return a dictionary.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :returns: The key value pairs of the map specific metadata,
                     or None in case of an error
        """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_MAP_FULL_INFO,
                               RPCDefs.TYPE_VECTOR,
                               name, mapset, None])
        return self.safe_receive("read_vector_full_info")

    def has_vector_timestamp(self, name, mapset, layer=None):
        """Check if a file based vector timestamp exists

           :param name: The name of the map
           :param mapset: The mapset of the map
           :param layer: The layer of the vector map
           :returns: True if exists, False if not
       """
        self.check_server()
        self.client_conn.send([RPCDefs.HAS_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer])
        return self.safe_receive("has_vector_timestamp")

    def remove_vector_timestamp(self, name, mapset, layer=None):
        """Remove a file based vector timestamp

           Please have a look at the documentation G_remove_vector_timestamp
           for the return values description.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :param layer: The layer of the vector map
           :returns: The return value of G_remove_vector_timestamp
       """
        self.check_server()
        self.client_conn.send([RPCDefs.REMOVE_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer])
        return self.safe_receive("remove_vector_timestamp")

    def read_vector_timestamp(self, name, mapset, layer=None):
        """Read a file based vector timestamp

           Please have a look at the documentation G_read_vector_timestamp
           for the return values description.

           The timestamps to be send are tuples of values:
               - relative time (start, end, unit), start and end are of type
                 integer, unit is of type string.
               - absolute time (start, end), start and end are of type datetime

           The end time may be None in case of a time instance.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :param layer: The layer of the vector map
           :returns: The return value ofG_read_vector_timestamp and the timestamps
       """
        self.check_server()
        self.client_conn.send([RPCDefs.READ_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer])
        return self.safe_receive("read_vector_timestamp")

    def write_vector_timestamp(self, name, mapset, timestring, layer=None):
        """Write a file based vector timestamp

           Please have a look at the documentation G_write_vector_timestamp
           for the return values description.

           Note:
               Only timestamps pf maps from the current mapset can written.

           :param name: The name of the map
           :param mapset: The mapset of the map
           :param timestring: A GRASS datetime C-library compatible string
           :param layer: The layer of the vector map
           :returns: The return value of G_write_vector_timestamp
        """
        self.check_server()
        self.client_conn.send([RPCDefs.WRITE_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer, timestring])
        return self.safe_receive("write_vector_timestamp")

    def available_mapsets(self):
        """Return all available mapsets the user can access as a list of strings

           :returns: Names of available mapsets as list of strings
        """
        self.check_server()
        self.client_conn.send([RPCDefs.AVAILABLE_MAPSETS, ])
        return self.safe_receive("available_mapsets")

    def get_driver_name(self, mapset=None):
        """Return the temporal database driver of a specific mapset

           :param mapset: Name of the mapset

           :returns: Name of the driver or None if no temporal database present
        """
        self.check_server()
        self.client_conn.send([RPCDefs.GET_DRIVER_NAME, mapset])
        return self.safe_receive("get_driver_name")

    def get_database_name(self, mapset=None):
        """Return the temporal database name of a specific mapset

           :param mapset: Name of the mapset

           :returns: Name of the database or None if no temporal database present
        """
        self.check_server()
        self.client_conn.send([RPCDefs.GET_DATABASE_NAME, mapset])
        return self.safe_receive("get_database_name")

    def get_mapset(self):
        """Return the current mapset

           :returns: Name of the current mapset
        """
        self.check_server()
        self.client_conn.send([RPCDefs.G_MAPSET, ])
        return self.safe_receive("get_mapset")

    def get_location(self):
        """Return the location

           :returns: Name of the location
        """
        self.check_server()
        self.client_conn.send([RPCDefs.G_LOCATION, ])
        return self.safe_receive("get_location")

    def get_gisdbase(self):
        """Return the gisdatabase

           :returns: Name of the gisdatabase
        """
        self.check_server()
        self.client_conn.send([RPCDefs.G_GISDBASE, ])
        return self.safe_receive("get_gisdbase")

    def fatal_error(self, mapset=None):
        """Generate a fatal error in libgis.

            This function is only for testing purpose.
        """
        self.check_server()
        self.client_conn.send([RPCDefs.G_FATAL_ERROR])
        # The pipe should be closed in the checker thread
        return self.safe_receive("Fatal error")

if __name__ == "__main__":
    import doctest
    doctest.testmod()
