# -*- coding: utf-8 -*-
"""!@package grass.pygrass.massages

@brief Temporal Framework GRASS C-library interface

Fast and exit-safe interface to GRASS C-library functions
using ctypes and multiprocessing


(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import sys
from multiprocessing import Process, Lock, Pipe
import logging
from ctypes import *
from core import *
import grass.lib.gis as libgis
import grass.lib.raster as libraster
import grass.lib.vector as libvector
import grass.lib.date as libdate
import grass.lib.raster3d as libraster3d

###############################################################################

class RPCDefs(object):
    # Function identifier and index
    STOP=0
    HAS_TIMESTAMP=1
    WRITE_TIMESTAMP=2
    READ_TIMESTAMP=3
    REMOVE_TIMESTAMP=4
    READ_MAP_INFO=5
    MAP_EXISTS=6
    READ_MAP_INFO=7

    TYPE_RASTER=0
    TYPE_RASTER3D=1
    TYPE_VECTOR=2

###############################################################################

def _has_timestamp(lock, conn, data):
    """!Check if the file based GRASS timestamp is present and send
       True or False using the provided pipe.

       @param lock A multiprocessing.Lock instance
       @param conn A multiprocessing.Pipe instance used to send True or False
       @param data The list of data entries [function_id, maptype, name, mapset, layer]

    """
    maptype = data[1]
    name = data[2]
    mapset = data[3]
    layer= data[4]
    check = False
    if maptype == RPCDefs.TYPE_RASTER:
        if libgis.G_has_raster_timestamp(name, mapset) == 1:
            check = True
    elif maptype == RPCDefs.TYPE_VECTOR:
        if libgis.G_has_vector_timestamp(name, layer, mapset) == 1:
            check = True
    elif maptype == RPCDefs.TYPE_RASTER3D:
        if libgis.G_has_raster3d_timestamp(name, mapset) == 1:
            check = True
    conn.send(check)

###############################################################################

def _read_timestamp(lock, conn, data):
    """!Read the file based GRASS timestamp and send
       the result using the provided pipe.

       The tuple to be send via pipe: (return value of G_read_*_timestamp, timestamps).

       Please have a look at the documentation of G_read_raster_timestamp,
       G_read_vector_timestamp and G_read_raster3d_timestamp for the return
       values description.

       The timestamps to be send are tuples of values:
           - relative time (start, end, unit), start and end are of type
             integer, unit is of type string.
           - absolute time (start, end), start and end are of type datetime

       The end time may be None in case of a time instance.

       @param lock A multiprocessing.Lock instance
       @param conn A multiprocessing.Pipe instance used to send the result
       @param data The list of data entries [function_id, maptype, name, mapset, layer]

    """
    maptype = data[1]
    name = data[2]
    mapset = data[3]
    layer= data[4]
    check = False
    ts = libgis.TimeStamp()
    if maptype == RPCDefs.TYPE_RASTER:
        check = libgis.G_read_raster_timestamp(name, mapset, byref(ts))
    elif maptype == RPCDefs.TYPE_VECTOR:
        check = libgis.G_read_vector_timestamp(name, layer, mapset, byref(ts))
    elif maptype == RPCDefs.TYPE_RASTER3D:
        check = libgis.G_read_raster3d_timestamp(name, mapset, byref(ts))

    dates = _convert_timestamp_from_grass(ts)
    conn.send((check, dates))

###############################################################################

def _write_timestamp(lock, conn, data):
    """!Write the file based GRASS timestamp
       the return values of the called C-functions using the provided pipe.

       The value to be send via pipe is the return value of G_write_*_timestamp.

       Please have a look at the documentation of G_write_raster_timestamp,
       G_write_vector_timestamp and G_write_raster3d_timestamp for the return
       values description.

       @param lock A multiprocessing.Lock instance
       @param conn A multiprocessing.Pipe instance used to send True or False
       @param data The list of data entries [function_id, maptype, name, mapset, layer, timestring]
    """
    maptype = data[1]
    name = data[2]
    mapset = data[3]
    layer= data[4]
    timestring = data[5]
    check = -3
    ts = libgis.TimeStamp()
    check = libgis.G_scan_timestamp(byref(ts), timestring)

    if check != 1:
        logging.error("Unable to convert the timestamp: "+ timestring)
        return -2

    if maptype == RPCDefs.TYPE_RASTER:
        check = libgis.G_write_raster_timestamp(name, byref(ts))
    elif maptype == RPCDefs.TYPE_VECTOR:
        check = libgis.G_write_vector_timestamp(name, layer, byref(ts))
    elif maptype == RPCDefs.TYPE_RASTER3D:
        check = libgis.G_write_raster3d_timestamp(name, byref(ts))

    conn.send(check)

###############################################################################

def _remove_timestamp(lock, conn, data):
    """!Remove the file based GRASS timestamp
       the return values of the called C-functions using the provided pipe.

       The value to be send via pipe is the return value of G_remove_*_timestamp.

       Please have a look at the documentation of G_remove_raster_timestamp,
       G_remove_vector_timestamp and G_remove_raster3d_timestamp for the return
       values description.

       @param lock A multiprocessing.Lock instance
       @param conn A multiprocessing.Pipe instance used to send True or False
       @param data The list of data entries [function_id, maptype, name, mapset, layer]

    """
    maptype = data[1]
    name = data[2]
    mapset = data[3]
    layer= data[4]
    check = False
    if maptype == RPCDefs.TYPE_RASTER:
        check = libgis.G_remove_raster_timestamp(name, mapset)
    elif maptype == RPCDefs.TYPE_VECTOR:
        check = libgis.G_remove_vector_timestamp(name, layer, mapset)
    elif maptype == RPCDefs.TYPE_RASTER3D:
        check = libgis.G_remove_raster3d_timestamp(name, mapset)

    conn.send(check)

###############################################################################

def _map_exists(lock, conn, data):
    """!Check if a map exists in the spatial database

       The value to be send via pipe is True in case the map exists and False
       if not.

       @param lock A multiprocessing.Lock instance
       @param conn A multiprocessing.Pipe instance used to send True or False
       @param data The list of data entries [function_id, maptype, name, mapset]

    """
    maptype = data[1]
    name = data[2]
    mapset = data[3]
    check = False
    if maptype == RPCDefs.TYPE_RASTER:
         mapset = libgis.G_find_raster(name, mapset)
    elif maptype == RPCDefs.TYPE_VECTOR:
         mapset = libgis.G_find_vector(name, mapset)
    elif maptype == RPCDefs.TYPE_RASTER3D:
         mapset = libgis.G_find_raster3d(name, mapset)

    if mapset:
        check = True

    conn.send(check)

###############################################################################

def _read_map_info(lock, conn, data):
    """!Read map specific metadata from the spatial database using C-library
       functions

       @param lock A multiprocessing.Lock instance
       @param conn A multiprocessing.Pipe instance used to send True or False
       @param data The list of data entries [function_id, maptype, name, mapset]
    """
    maptype = data[1]
    name = data[2]
    mapset = data[3]
    if maptype == RPCDefs.TYPE_RASTER:
         kvp = _read_raster_info(name, mapset)
    elif maptype == RPCDefs.TYPE_VECTOR:
         kvp = _read_vector_info(name, mapset)
    elif maptype == RPCDefs.TYPE_RASTER3D:
         kvp = _read_raster3d_info(name, mapset)

    conn.send(kvp)

###############################################################################

def _read_raster_info(name, mapset):
    """!Read the raster map info from the file system and store the content
       into a dictionary

       This method uses the ctypes interface to the gis and raster libraries
       to read the map metadata information

       @param name The name of the map
       @param mapset The mapset of the map
       @return The key value pairs of the map specific metadata, or None in case of an error
    """

    kvp = {}

    if not libgis.G_find_raster(name, mapset):
        return None

    # Read the region information
    region = libgis.Cell_head()
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
    """!Read the 3D raster map info from the file system and store the content
       into a dictionary

       This method uses the ctypes interface to the gis and raster3d libraries
       to read the map metadata information

       @param name The name of the map
       @param mapset The mapset of the map
       @return The key value pairs of the map specific metadata, or None in case of an error
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
    """!Read the vector map info from the file system and store the content
       into a dictionary

       This method uses the ctypes interface to the vector libraries
       to read the map metadata information

       @param name The name of the map
       @param mapset The mapset of the map
       @return The key value pairs of the map specific metadata, or None in case of an error
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
    """!Convert a GRASS file based timestamp into the temporal framework
       format datetime or integer.

       A tuple of two datetime objects (start, end) is returned in case of absolute time.
       In case of relative time a tuple with start time, end time and the
       relative unit (start, end, unit) will be returned.

       Note:
       The end time will be set to None in case of a time instance.

       @param ts grass.lib.gis.TimeStamp object created by G_read_*_timestamp
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
        if count >= 1:
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
                unit = "minutess"
                start = dt1.minutes
            elif dt1.seconds > 0:
                unit = "seconds"
                start = dt1.seconds
        if count == 2:
            if dt2.year > 0:
                end = dt2.year
            elif dt2.month > 0:
                end = dt2.month
            elif dt2.day > 0:
                end = dt2.day
            elif dt2.hour > 0:
                end = dt2.hour
            elif dt2.minute > 0:
                end = dt2.minutes
            elif dt2.seconds > 0:
                end = dt2.seconds
        return (start, end, unit)

###############################################################################

def _stop(lock, conn, data):
	conn.close()
	lock.release()
	sys.exit()

###############################################################################

def c_library_server(lock, conn):
    """!The GRASS C-libraries server function designed to be a target for
       multiprocessing.Process

       @param lock A multiprocessing.Lock
       @param conn A multiprocessing.Pipe
    """
    # Crerate the function array
    functions = [0]*8
    functions[RPCDefs.STOP] = _stop
    functions[RPCDefs.HAS_TIMESTAMP] = _has_timestamp
    functions[RPCDefs.WRITE_TIMESTAMP] = _write_timestamp
    functions[RPCDefs.READ_TIMESTAMP] = _read_timestamp
    functions[RPCDefs.REMOVE_TIMESTAMP] = _remove_timestamp
    functions[RPCDefs.READ_MAP_INFO] = _read_map_info
    functions[RPCDefs.MAP_EXISTS] = _map_exists

    libgis.G_gisinit("c_library_server")

    while True:
        # Avoid busy waiting
        conn.poll(4)
        data = conn.recv()
        lock.acquire()
        functions[data[0]](lock, conn, data)
        lock.release()

class CLibrariesInterface(object):
    """!Fast and exit-safe interface to GRASS C-libraries functions

       This class implements a fast and exit-safe interface to the GRASS
       gis, raster, 3D raster and vector  C-libraries functions.

       The C-libraries functions are called via ctypes in a subprocess
       using a pipe (multiprocessing.Pipe) to transfer the text messages.
       Hence, the process that uses the CLibrariesInterface will not be
       exited, if a G_fatal_error() was invoked in the subprocess.
       In this case the CLibrariesInterface object will simply start a
       new subprocess and restarts the pipeline.


       Usage:

       @code
       >>> import grass.script as grass
       >>> import grass.temporal as tgis
       >>> grass.use_temp_region()
       >>> grass.run_command("g.region", n=80.0, s=0.0, e=120.0, w=0.0,
       ... t=1.0, b=0.0, res=10.0, res3=10.0)
       0
       >>> tgis.init()
       >>> grass.run_command("r.mapcalc", expression="test = 1", overwrite=True, quiet=True)
       0
       >>> grass.run_command("r3.mapcalc", expression="test = 1", overwrite=True, quiet=True)
       0
       >>> grass.run_command("v.random", output="test", n=10, overwrite=True, quiet=True)
       0
       >>> grass.run_command("r.timestamp", map="test", date='12 Mar 1995', overwrite=True, quiet=True)
       0
       >>> grass.run_command("r3.timestamp", map="test", date='12 Mar 1995', overwrite=True, quiet=True)
       0
       >>> grass.run_command("v.timestamp", map="test", date='12 Mar 1995', overwrite=True, quiet=True)
       0


       # Raster map
       >>> ciface = tgis.CLibrariesInterface()
       >>> check = ciface.raster_map_exists("test", tgis.get_current_mapset())
       >>> print check
       True
       >>> ciface.read_raster_info("test", tgis.get_current_mapset())
       {'rows': 12, 'north': 80.0, 'min': 1, 'datatype': 'CELL', 'max': 1, 'ewres': 10.0, 'cols': 8, 'west': 0.0, 'east': 120.0, 'nsres': 10.0, 'south': 0.0}
       >>> check = ciface.has_raster_timestamp("test", tgis.get_current_mapset())
       >>> print check
       True
       >>> if check:
       ...     res = ciface.read_raster_timestamp("test", tgis.get_current_mapset())
       ...     if res[0]:
       ...         print str(res[1][0]), str(res[1][0])
       ...         ciface.remove_raster_timestamp("test", tgis.get_current_mapset())
       1995-03-12 00:00:00 1995-03-12 00:00:00
       1
       >>> ciface.has_raster_timestamp("test", tgis.get_current_mapset())
       False
       >>> ciface.write_raster_timestamp("test", tgis.get_current_mapset(), "13 Jan 1999")
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
       1995-03-12 00:00:00 1995-03-12 00:00:00
       1
       >>> ciface.has_raster3d_timestamp("test", tgis.get_current_mapset())
       False
       >>> ciface.write_raster3d_timestamp("test", tgis.get_current_mapset(), "13 Jan 1999")
       1
       >>> ciface.has_raster3d_timestamp("test", tgis.get_current_mapset())
       True


       # Vector map
       >>> check = ciface.vector_map_exists("test", tgis.get_current_mapset())
       >>> print check
       True
       >>> kvp = ciface.read_vector_info("test", tgis.get_current_mapset())
       >>> print kvp['points']
       10
       >>> check = ciface.has_vector_timestamp("test", tgis.get_current_mapset(), None)
       >>> print check
       True
       >>> if check:
       ...     res = ciface.read_vector_timestamp("test", tgis.get_current_mapset())
       ...     if res[0]:
       ...         print str(res[1][0]), str(res[1][0])
       ...         ciface.remove_vector_timestamp("test", tgis.get_current_mapset())
       1995-03-12 00:00:00 1995-03-12 00:00:00
       1
       >>> ciface.has_vector_timestamp("test", tgis.get_current_mapset())
       False
       >>> ciface.write_vector_timestamp("test", tgis.get_current_mapset(), "13 Jan 1999")
       1
       >>> ciface.has_vector_timestamp("test", tgis.get_current_mapset())
       True

       >>> grass.del_temp_region()

       @endcode
    """
    def __init__(self):
        self.client_conn = None
        self.server_conn = None
        self.server = None
        self.start_server()

    def __del__(self):
        self.stop()

    def start_server(self):
        self.client_conn, self.server_conn = Pipe()
        self.lock = Lock()
        self.server = Process(target=c_library_server, args=(self.lock,
                                                          self.server_conn))
        self.server.daemon = True
        self.server.start()

    def _check_restart_server(self):
        """!Restart the server if it was terminated
        """
        if self.server.is_alive() is True:
            return
        self.client_conn.close()
        self.server_conn.close()
        self.start_server()
        logging.warning("Needed to restart the libgis server")

    def raster_map_exists(self, name, mapset):
        """!Check if a raster map exists in the spatial database

           @param name The name of the map
           @param mapset The mapset of the map
           @return True if exists, False if not
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.MAP_EXISTS, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.client_conn.recv()

    def read_raster_info(self, name, mapset):
        """!Read the raster map info from the file system and store the content
           into a dictionary

           @param name The name of the map
           @param mapset The mapset of the map
           @return The key value pairs of the map specific metadata, or None in case of an error
        """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.READ_MAP_INFO, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.client_conn.recv()

    def has_raster_timestamp(self, name, mapset):
        """!Check if a file based raster timetamp exists

           @param name The name of the map
           @param mapset The mapset of the map
           @return True if exists, False if not
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.HAS_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.client_conn.recv()

    def remove_raster_timestamp(self, name, mapset):
        """!Remove a file based raster timetamp

           Please have a look at the documentation G_remove_raster_timestamp
           for the return values description.

           @param name The name of the map
           @param mapset The mapset of the map
           @return The return value of G_remove_raster_timestamp
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.REMOVE_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.client_conn.recv()

    def read_raster_timestamp(self, name, mapset):
        """!Read a file based raster timetamp

           Please have a look at the documentation G_read_raster_timestamp
           for the return values description.

           The timestamps to be send are tuples of values:
               - relative time (start, end, unit), start and end are of type
                 integer, unit is of type string.
               - absolute time (start, end), start and end are of type datetime

           The end time may be None in case of a time instance.

           @param name The name of the map
           @param mapset The mapset of the map
           @return The return value of G_read_raster_timestamp
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.READ_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None])
        return self.client_conn.recv()

    def write_raster_timestamp(self, name, mapset, timestring):
        """!Write a file based raster timetamp

           Please have a look at the documentation G_write_raster_timestamp
           for the return values description.

           Note:
               Only timestamps of maps from the current mapset can written.

           @param name The name of the map
           @param mapset The mapset of the map
           @param timestring A GRASS datetime C-library compatible string
           @return The return value of G_write_raster_timestamp
        """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.WRITE_TIMESTAMP, RPCDefs.TYPE_RASTER,
                               name, mapset, None, timestring])
        return self.client_conn.recv()

    def raster3d_map_exists(self, name, mapset):
        """!Check if a 3D raster map exists in the spatial database

           @param name The name of the map
           @param mapset The mapset of the map
           @return True if exists, False if not
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.MAP_EXISTS, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.client_conn.recv()

    def read_raster3d_info(self, name, mapset):
        """!Read the 3D raster map info from the file system and store the content
           into a dictionary

           @param name The name of the map
           @param mapset The mapset of the map
           @return The key value pairs of the map specific metadata, or None in case of an error
        """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.READ_MAP_INFO, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.client_conn.recv()

    def has_raster3d_timestamp(self, name, mapset):
        """!Check if a file based 3D raster timetamp exists

           @param name The name of the map
           @param mapset The mapset of the map
           @return True if exists, False if not
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.HAS_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.client_conn.recv()

    def remove_raster3d_timestamp(self, name, mapset):
        """!Remove a file based 3D raster timetamp

           Please have a look at the documentation G_remove_raster3d_timestamp
           for the return values description.

           @param name The name of the map
           @param mapset The mapset of the map
           @return The return value of G_remove_raster3d_timestamp
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.REMOVE_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.client_conn.recv()

    def read_raster3d_timestamp(self, name, mapset):
        """!Read a file based 3D raster timetamp

           Please have a look at the documentation G_read_raster3d_timestamp
           for the return values description.

           The timestamps to be send are tuples of values:
               - relative time (start, end, unit), start and end are of type
                 integer, unit is of type string.
               - absolute time (start, end), start and end are of type datetime

           The end time may be None in case of a time instance.

           @param name The name of the map
           @param mapset The mapset of the map
           @return The return value of G_read_raster3d_timestamp
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.READ_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None])
        return self.client_conn.recv()

    def write_raster3d_timestamp(self, name, mapset, timestring):
        """!Write a file based 3D raster timetamp

           Please have a look at the documentation G_write_raster3d_timestamp
           for the return values description.

           Note:
               Only timestamps of maps from the current mapset can written.

           @param name The name of the map
           @param mapset The mapset of the map
           @param timestring A GRASS datetime C-library compatible string
           @return The return value of G_write_raster3d_timestamp
        """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.WRITE_TIMESTAMP, RPCDefs.TYPE_RASTER3D,
                               name, mapset, None, timestring])
        return self.client_conn.recv()

    def vector_map_exists(self, name, mapset):
        """!Check if a vector map exists in the spatial database

           @param name The name of the map
           @param mapset The mapset of the map
           @return True if exists, False if not
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.MAP_EXISTS, RPCDefs.TYPE_VECTOR,
                               name, mapset, None])
        return self.client_conn.recv()

    def read_vector_info(self, name, mapset):
        """!Read the vector map info from the file system and store the content
           into a dictionary

           @param name The name of the map
           @param mapset The mapset of the map
           @return The key value pairs of the map specific metadata, or None in case of an error
        """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.READ_MAP_INFO, RPCDefs.TYPE_VECTOR,
                               name, mapset, None])
        return self.client_conn.recv()

    def has_vector_timestamp(self, name, mapset, layer=None):
        """!Check if a file based vector timetamp exists

           @param name The name of the map
           @param mapset The mapset of the map
           @param layer The layer of the vector map
           @return True if exists, False if not
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.HAS_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer])
        return self.client_conn.recv()

    def remove_vector_timestamp(self, name, mapset, layer=None):
        """!Remove a file based vector timetamp

           Please have a look at the documentation G_remove_vector_timestamp
           for the return values description.

           @param name The name of the map
           @param mapset The mapset of the map
           @param layer The layer of the vector map
           @return The return value of G_remove_vector_timestamp
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.REMOVE_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer])
        return self.client_conn.recv()

    def read_vector_timestamp(self, name, mapset, layer=None):
        """!Read a file based vector timetamp

           Please have a look at the documentation G_read_vector_timestamp
           for the return values description.

           The timestamps to be send are tuples of values:
               - relative time (start, end, unit), start and end are of type
                 integer, unit is of type string.
               - absolute time (start, end), start and end are of type datetime

           The end time may be None in case of a time instance.

           @param name The name of the map
           @param mapset The mapset of the map
           @param layer The layer of the vector map
           @return The return value ofG_read_vector_timestamp and the timestamps
       """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.READ_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer])
        return self.client_conn.recv()

    def write_vector_timestamp(self, name, mapset, timestring, layer=None):
        """!Write a file based vector timetamp

           Please have a look at the documentation G_write_vector_timestamp
           for the return values description.

           Note:
               Only timestamps pf maps from the current mapset can written.

           @param name The name of the map
           @param mapset The mapset of the map
           @param timestring A GRASS datetime C-library compatible string
           @param layer The layer of the vector map
           @return The return value of G_write_vector_timestamp
        """
        self._check_restart_server()
        self.client_conn.send([RPCDefs.WRITE_TIMESTAMP, RPCDefs.TYPE_VECTOR,
                               name, mapset, layer, timestring])
        return self.client_conn.recv()

    def stop(self):
        """!Stop the messenger server and close the pipe
        """
        if self.server is not None and self.server.is_alive():
            self.client_conn.send([0,])
            self.server.join(5)
            self.server.terminate()
        if self.client_conn is not None:
            self.client_conn.close()

if __name__ == "__main__":
    import doctest
    doctest.testmod()
