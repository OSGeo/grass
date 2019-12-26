# -*- coding: utf-8 -*-
"""
Fast and exit-safe interface to PyGRASS Raster and Vector layer
using multiprocessing

(C) 2015 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import time
import threading
import sys
from multiprocessing import Process, Lock, Pipe
from ctypes import *

from grass.exceptions import FatalError
from grass.pygrass.vector import *
from grass.pygrass.raster import *
import grass.lib.gis as libgis
from .base import RPCServerBase
from grass.pygrass.gis.region import Region
import grass.pygrass.utils as utils
import logging

###############################################################################
###############################################################################

class RPCDefs(object):
    # Function identifier and index
    STOP = 0
    GET_VECTOR_TABLE_AS_DICT = 1
    GET_VECTOR_FEATURES_AS_WKB = 2
    GET_RASTER_IMAGE_AS_NP = 3
    G_FATAL_ERROR = 14


def _get_raster_image_as_np(lock, conn, data):
    """Convert a raster map into an image and return
       a numpy array with RGB or Gray values.

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, raster_name, extent, color]
    """
    array = None
    try:
        name = data[1]
        mapset = data[2]
        extent = data[3]
        color = data[4]

        mapset = utils.get_mapset_raster(name, mapset)

        if not mapset:
            raise ValueError("Unable to find raster map <%s>"%(name))

        rast = RasterRow(name, mapset)

        if rast.exist():

            reg = Region()
            reg.from_rast(name)

            if extent is not None:
                if "north" in extent:
                    reg.north = extent["north"]
                if "south" in extent:
                    reg.south = extent["south"]
                if "east" in extent:
                    reg.east =  extent["east"]
                if "west" in extent:
                    reg.west =  extent["west"]
                if "rows" in extent:
                    reg.rows =  extent["rows"]
                if "cols" in extent:
                    reg.cols =  extent["cols"]
                reg.adjust()

            array = raster2numpy_img(name, reg, color)
    except:
        raise
    finally:
        conn.send(array)

def _get_vector_table_as_dict(lock, conn, data):
    """Get the table of a vector map layer as dictionary

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id, name, mapset, where]

    """
    ret = None
    try:
        name = data[1]
        mapset = data[2]
        where = data[3]

        mapset = utils.get_mapset_vector(name, mapset)

        if not mapset:
            raise ValueError("Unable to find vector map <%s>"%(name))

        layer = VectorTopo(name, mapset)

        if layer.exist() is True:
            layer.open("r")
            columns = None
            table = None
            if layer.table is not None:
                columns = layer.table.columns
                table = layer.table_to_dict(where=where)
            layer.close()

            ret = {}
            ret["table"] = table
            ret["columns"] = columns
    except:
        raise
    finally:
        conn.send(ret)

def _get_vector_features_as_wkb_list(lock, conn, data):
    """Return vector layer features as wkb list

       supported feature types:
       point, centroid, line, boundary, area

       :param lock: A multiprocessing.Lock instance
       :param conn: A multiprocessing.Pipe instance used to send True or False
       :param data: The list of data entries [function_id,name,mapset,extent,
                                              feature_type, field]

    """
    wkb_list = None
    try:
        name = data[1]
        mapset = data[2]
        extent = data[3]
        feature_type = data[4]
        field = data[5]
        bbox = None

        mapset = utils.get_mapset_vector(name, mapset)

        if not mapset:
            raise ValueError("Unable to find vector map <%s>"%(name))

        layer = VectorTopo(name, mapset)

        if layer.exist() is True:
            if extent is not None:
                bbox = basic.Bbox(north=extent["north"],
                                  south=extent["south"],
                                  east=extent["east"],
                                  west=extent["west"])

            layer.open("r")
            if feature_type.lower() == "area":
                wkb_list = layer.areas_to_wkb_list(bbox=bbox, field=field)
            else:
                wkb_list = layer.features_to_wkb_list(bbox=bbox,
                                                      feature_type=feature_type,
                                                      field=field)
            layer.close()
    except:
        raise
    finally:
        conn.send(wkb_list)

###############################################################################

def _fatal_error(lock, conn, data):
    """Calls G_fatal_error()"""
    libgis.G_fatal_error("Fatal Error in C library server")


###############################################################################

def _stop(lock, conn, data):
    conn.close()
    lock.release()
    sys.exit()

###############################################################################

def data_provider_server(lock, conn):
    """The PyGRASS data provider server designed to be a target for
       multiprocessing.Process

       :param lock: A multiprocessing.Lock
       :param conn: A multiprocessing.Pipe
    """

    def error_handler(data):
        """This function will be called in case of a fatal error in libgis"""
        # sys.stderr.write("Error handler was called\n")
        # We send an exception that will be handled in
        # the parent process, then close the pipe
        # and release any possible lock
        conn.send(FatalError("G_fatal_error() was called in the server process"))
        conn.close()
        lock.release()

    CALLBACK = CFUNCTYPE(c_void_p, c_void_p)
    CALLBACK.restype = c_void_p
    CALLBACK.argtypes = c_void_p

    cerror_handler = CALLBACK(error_handler)

    libgis.G_add_error_handler(cerror_handler, None)

    # Crerate the function array
    functions = [0]*15
    functions[RPCDefs.GET_VECTOR_TABLE_AS_DICT] = _get_vector_table_as_dict
    functions[RPCDefs.GET_VECTOR_FEATURES_AS_WKB] = _get_vector_features_as_wkb_list
    functions[RPCDefs.GET_RASTER_IMAGE_AS_NP] = _get_raster_image_as_np
    functions[RPCDefs.STOP] = _stop
    functions[RPCDefs.G_FATAL_ERROR] = _fatal_error

    while True:
        # Avoid busy waiting
        conn.poll(None)
        data = conn.recv()
        lock.acquire()
        functions[data[0]](lock, conn, data)
        lock.release()

test_vector_name="data_provider_vector_map"
test_raster_name="data_provider_raster_map"

class DataProvider(RPCServerBase):
    """Fast and exit-safe interface to PyGRASS data delivery functions

    """
    def __init__(self):
        RPCServerBase.__init__(self)

    def start_server(self):
        """This function must be re-implemented in the subclasses
        """
        self.client_conn, self.server_conn = Pipe(True)
        self.lock = Lock()
        self.server = Process(target=data_provider_server, args=(self.lock,
                                                             self.server_conn))
        self.server.daemon = True
        self.server.start()

    def get_raster_image_as_np(self, name, mapset=None, extent=None, color="RGB"):
        """Return the attribute table of a vector map as dictionary.

           See documentation of: pygrass.raster.raster2numpy_img

           Usage:

           .. code-block:: python

            >>> from grass.pygrass.rpc import DataProvider
            >>> import time
            >>> provider = DataProvider()
            >>> ret = provider.get_raster_image_as_np(name=test_raster_name)
            >>> len(ret)
            64

            >>> extent = {"north":30, "south":10, "east":30, "west":10,
            ...           "rows":2, "cols":2}
            >>> ret = provider.get_raster_image_as_np(name=test_raster_name,
            ...                                       extent=extent)
            >>> len(ret)
            16

            >>> extent = {"rows":3, "cols":1}
            >>> ret = provider.get_raster_image_as_np(name=test_raster_name,
            ...                                       extent=extent)
            >>> len(ret)
            12

            >>> extent = {"north":100, "south":10, "east":30, "west":10,
            ...           "rows":2, "cols":2}
            >>> ret = provider.get_raster_image_as_np(name=test_raster_name,
            ...                                       extent=extent)

            >>> provider.stop()
            >>> time.sleep(1)

            >>> extent = {"rows":3, "cols":1}
            >>> ret = provider.get_raster_image_as_np(name=test_raster_name,
            ...                                       extent=extent)
            >>> len(ret)
            12


            ..
        """
        self.check_server()
        self.client_conn.send([RPCDefs.GET_RASTER_IMAGE_AS_NP,
                               name, mapset, extent, color])
        return self.safe_receive("get_raster_image_as_np")

    def get_vector_table_as_dict(self, name, mapset=None, where=None):
        """Return the attribute table of a vector map as dictionary.

           See documentation of: pygrass.vector.VectorTopo::table_to_dict

           Usage:

           .. code-block:: python

            >>> from grass.pygrass.rpc import DataProvider
            >>> provider = DataProvider()
            >>> ret = provider.get_vector_table_as_dict(name=test_vector_name)
            >>> ret["table"]
            {1: [1, 'point', 1.0], 2: [2, 'line', 2.0], 3: [3, 'centroid', 3.0]}
            >>> ret["columns"]
            Columns([('cat', 'INTEGER'), ('name', 'varchar(50)'), ('value', 'double precision')])
            >>> ret = provider.get_vector_table_as_dict(name=test_vector_name,
            ...                                           where="value > 1")
            >>> ret["table"]
            {2: [2, 'line', 2.0], 3: [3, 'centroid', 3.0]}
            >>> ret["columns"]
            Columns([('cat', 'INTEGER'), ('name', 'varchar(50)'), ('value', 'double precision')])
            >>> provider.get_vector_table_as_dict(name="no_map",
            ...                                   where="value > 1")
            >>> provider.stop()

            ..
        """
        self.check_server()
        self.client_conn.send([RPCDefs.GET_VECTOR_TABLE_AS_DICT,
                               name, mapset, where])
        return self.safe_receive("get_vector_table_as_dict")

    def get_vector_features_as_wkb_list(self, name, mapset=None, extent=None,
                                        feature_type="point", field=1):
        """Return the features of a vector map as wkb list.

           :param extent: A dictionary of {"north":double, "south":double,
                                           "east":double, "west":double}
           :param feature_type: point, centroid, line, boundary or area

           See documentation: pygrass.vector.VectorTopo::features_to_wkb_list
                              pygrass.vector.VectorTopo::areas_to_wkb_list


           Usage:

           .. code-block:: python

            >>> from grass.pygrass.rpc import DataProvider
            >>> provider = DataProvider()
            >>> wkb = provider.get_vector_features_as_wkb_list(name=test_vector_name,
            ...                                                extent=None,
            ...                                                feature_type="point")
            >>> for entry in wkb:
            ...     f_id, cat, string = entry
            ...     print(f_id, cat, len(string))
            1 1 21
            2 1 21
            3 1 21

            >>> extent = {"north":6.6, "south":5.5, "east":14.5, "west":13.5}
            >>> wkb = provider.get_vector_features_as_wkb_list(name=test_vector_name,
            ...                                                extent=extent,
            ...                                                feature_type="point")
            >>> for entry in wkb:
            ...     f_id, cat, string = entry
            ...     print(f_id, cat, len(string))
            3 1 21

            >>> wkb = provider.get_vector_features_as_wkb_list(name=test_vector_name,
            ...                                                extent=None,
            ...                                                feature_type="line")
            >>> for entry in wkb:
            ...     f_id, cat, string = entry
            ...     print(f_id, cat, len(string))
            4 2 57
            5 2 57
            6 2 57


            >>> wkb = provider.get_vector_features_as_wkb_list(name=test_vector_name,
            ...                                                extent=None,
            ...                                                feature_type="centroid")
            >>> for entry in wkb:
            ...     f_id, cat, string = entry
            ...     print(f_id, cat, len(string))
            19 3 21
            18 3 21
            20 3 21
            21 3 21

            >>> wkb = provider.get_vector_features_as_wkb_list(name=test_vector_name,
            ...                                                extent=None,
            ...                                                feature_type="area")
            >>> for entry in wkb:
            ...     f_id, cat, string = entry
            ...     print(f_id, cat, len(string))
            1 3 225
            2 3 141
            3 3 93
            4 3 141

            >>> wkb = provider.get_vector_features_as_wkb_list(name=test_vector_name,
            ...                                                extent=None,
            ...                                                feature_type="boundary")
            >>> for entry in wkb:
            ...     f_id, cat, string = entry
            ...     print(f_id, cat, len(string))
            10 None 41
            7 None 41
            8 None 41
            9 None 41
            11 None 89
            12 None 41
            14 None 41
            13 None 41
            17 None 41
            15 None 41
            16 None 41

            >>> provider.stop()

            ..
        """
        self.check_server()
        self.client_conn.send([RPCDefs.GET_VECTOR_FEATURES_AS_WKB,
                               name, mapset, extent, feature_type, field])
        return self.safe_receive("get_vector_features_as_wkb_list")


if __name__ == "__main__":
    import doctest
    from grass.pygrass import utils
    from grass.pygrass.modules import Module
    Module("g.region", n=40, s=0, e=40, w=0, res=10)
    Module("r.mapcalc", expression="%s = row() + (10 * col())"%(test_raster_name),
                             overwrite=True)
    utils.create_test_vector_map(test_vector_name)

    doctest.testmod()

    """Remove the generated maps, if exist"""
    mset = utils.get_mapset_raster(test_raster_name, mapset='')
    if mset:
        Module("g.remove", flags='f', type='raster', name=test_raster_name)
    mset = utils.get_mapset_vector(test_vector_name, mapset='')
    if mset:
        Module("g.remove", flags='f', type='vector', name=test_vector_name)
