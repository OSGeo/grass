"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.


>>> import grass.script as grass

>>> grass.run_command("r3.mapcalc", overwrite=True, expression="str3ds_map_test_case = 1")
0
>>> grass.run_command("v.random", overwrite=True, output="stvds_map_test_case", 
... n=100, zmin=0, zmax=100, flags="z", column="elevation")
0

@author Soeren Gebbert
"""
import getpass
from ctypes import *
import grass.lib.gis as libgis
import grass.lib.raster as libraster
import grass.lib.vector as libvector
import grass.lib.raster3d as libraster3d
import grass.script.array as garray
import grass.script.raster as raster
import grass.script.vector as vector
import grass.script.raster3d as raster3d

from abstract_space_time_dataset import *

###############################################################################

class RasterDataset(AbstractMapDataset):
    """!Raster dataset class

       This class provides functions to select, update, insert or delete raster
       map information and valid time stamps into the SQL temporal database.
       
       Usage:
        
        @code
        
        >>> import grass.script as grass
        >>> grass.use_temp_region()
        >>> grass.run_command("g.region", n=80.0, s=0.0, e=120.0, w=0.0, 
        ... t=1.0, b=0.0, res=10.0)
        0
        >>> grass.run_command("r.mapcalc", overwrite=True, 
        ... expression="strds_map_test_case = 1")
        0
        >>> mapset = grass.gisenv()["MAPSET"]
        >>> name = "strds_map_test_case"
        >>> identifier = "%s@%s" % (name, mapset)
        >>> rmap = RasterDataset(identifier)
        >>> rmap.set_absolute_time(start_time=datetime(2001,1,1), 
        ...                        end_time=datetime(2012,1,1))
        >>> rmap.map_exists()
        True
        >>> rmap.load()
        >>> rmap.spatial_extent.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 0.0
         | East:.. .................... 120.0
         | West:....................... 0.0
         | Top:........................ 0.0
         | Bottom:..................... 0.0
        >>> rmap.absolute_time.print_info()
         +-------------------- Absolute time -----------------------------------------+
         | Start time:................. 2001-01-01 00:00:00
         | End time:................... 2012-01-01 00:00:00
        >>> rmap.metadata.print_info()
         +-------------------- Metadata information ----------------------------------+
         | Datatype:................... CELL
         | Number of columns:.......... 8
         | Number of rows:............. 12
         | Number of cells:............ 96
         | North-South resolution:..... 10.0
         | East-west resolution:....... 10.0
         | Minimum value:.............. 1.0
         | Maximum value:.............. 1.0
         | STRDS register table ....... None
         
        >>> newmap = rmap.get_new_instance("new@PERMANENT")
        >>> isinstance(newmap, RasterDataset)
        True
        >>> newstrds = rmap.get_new_stds_instance("new@PERMANENT")
        >>> isinstance(newstrds, SpaceTimeRasterDataset)
        True
        >>> rmap.get_type()
        'raster'
        >>> rmap.get_stds_register()
        >>> rmap.get_absolute_time()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0), None)
        >>> rmap.get_valid_time()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0))
        >>> rmap.get_name()
        'strds_map_test_case'
        >>> rmap.get_mapset() == mapset
        True
        >>> rmap.get_temporal_type()
        'absolute'
        >>> rmap.get_spatial_extent()
        (80.0, 0.0, 120.0, 0.0, 0.0, 0.0)
        >>> rmap.is_time_absolute()
        True
        >>> rmap.is_time_relative()
        False
        
        >>> grass.run_command("g.remove", rast=name)
        0
        >>> grass.del_temp_region()
        
        @endcode
    """
    def __init__(self, ident):
        AbstractMapDataset.__init__(self)
        self.reset(ident)

    def get_type(self):
        return 'raster'

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return RasterDataset(ident)

    def get_new_stds_instance(self, ident):
        """!Return a new space time dataset instance in which maps 
        are stored with the type of this class"""
        return SpaceTimeRasterDataset(ident)

    def get_stds_register(self):
        """!Return the space time dataset register table name in which stds 
        are listed in which this map is registered"""
        return self.metadata.get_strds_register()

    def set_stds_register(self, name):
        """!Set the space time dataset register table name in which stds 
        are listed in which this map is registered"""
        self.metadata.set_strds_register(name)

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents 2d overlap"""

        return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """!Return the two dimensional spatial relation"""

        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)

    def get_np_array(self):
        """!Return this raster map as memmap numpy style array to access the raster
           values in numpy style without loading the whole map in the RAM.

           In case this raster map does exists in the grass spatial database, 
           the map will be exported using r.out.bin to a temporary location 
           and assigned to the memmap object that is returned by this function.

           In case the raster map does not exists, an empty temporary 
           binary file will be created and assigned to the memap object.

           You need to call the write function to write the memmap 
           array back into grass.
        """

        a = garray.array()

        if self.map_exists():
            a.read(self.get_map_id())

        return a

    def reset(self, ident):
        """!Reset the internal structure and set the identifier"""
        self.base = RasterBase(ident=ident)
        self.absolute_time = RasterAbsoluteTime(ident=ident)
        self.relative_time = RasterRelativeTime(ident=ident)
        self.spatial_extent = RasterSpatialExtent(ident=ident)
        self.metadata = RasterMetadata(ident=ident)

    def has_grass_timestamp(self):
        """!Check if a grass file bsased time stamp exists for this map.
        """
        if G_has_raster_timestamp(self.get_name(), self.get_mapset()):
            return True
        else:
            return False

    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata in 
           the grass file system based spatial database.

           Internally the libgis API functions are used for writing
        """

        ts = libgis.TimeStamp()

        libgis.G_scan_timestamp(byref(ts), self._convert_timestamp())
        check = libgis.G_write_raster_timestamp(self.get_name(), byref(ts))

        if check == -1:
            core.error(_("Unable to create timestamp file "
                         "for raster map <%s>" % (self.get_map_id())))

        if check == -2:
            core.error(_("Invalid datetime in timestamp for raster map <%s>" %
                         (self.get_map_id())))

    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based 
           spatial database

           Internally the libgis API functions are used for removal
        """
        check = libgis.G_remove_raster_timestamp(self.get_name())

        if check == -1:
            core.error(_("Unable to remove timestamp for raster map <%s>" %
                         (self.get_name())))

    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database

           @return True if map exists, False otherwise
        """
        mapset = libgis.G_find_raster(self.get_name(), self.get_mapset())

        if not mapset:
            return False

        return True

    def read_info(self):
        """!Read the raster map info from the file system and store the content
           into a dictionary

           This method uses the ctypes interface to the gis and raster libraries
           to read the map metadata information
        """

        kvp = {}

        name = self.get_name()
        mapset = self.get_mapset()

        if not self.map_exists():
            core.fatal(_("Raster map <%s> not found" % name))

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
                core.fatal(_("Unable to read range file"))
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
                core.fatal(_("Unable to read range file"))
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

    def load(self):
        """!Load all info from an existing raster map into the internal s
           tructure"""

        # Fill base information
        self.base.set_creator(str(getpass.getuser()))

        # Get the data from an existing raster map
        global use_ctypes_map_access

	if use_ctypes_map_access:
            kvp = self.read_info()
        else:
            kvp = raster.raster_info(self.get_id())

        # Fill spatial extent

        self.set_spatial_extent(north=kvp["north"], south=kvp["south"],
                                east=kvp["east"], west=kvp["west"])

        # Fill metadata

        self.metadata.set_nsres(kvp["nsres"])
        self.metadata.set_ewres(kvp["ewres"])
        self.metadata.set_datatype(kvp["datatype"])
        self.metadata.set_min(kvp["min"])
        self.metadata.set_max(kvp["max"])

        rows = int(kvp["rows"])
        cols = int(kvp["cols"])

        ncells = cols * rows

        self.metadata.set_cols(cols)
        self.metadata.set_rows(rows)
        self.metadata.set_number_of_cells(ncells)

###############################################################################

class Raster3DDataset(AbstractMapDataset):
    """!Raster3d dataset class

       This class provides functions to select, update, insert or delete raster3d
       map information and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
        AbstractMapDataset.__init__(self)
        self.reset(ident)

    def get_type(self):
        return "raster3d"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return Raster3DDataset(ident)

    def get_new_stds_instance(self, ident):
        """!Return a new space time dataset instance in which maps 
        are stored with the type of this class"""
        return SpaceTimeRaster3DDataset(ident)

    def get_stds_register(self):
        """!Return the space time dataset register table name in 
        which stds are listed in which this map is registered"""
        return self.metadata.get_str3ds_register()

    def set_stds_register(self, name):
        """!Set the space time dataset register table name in 
        which stds are listed in which this map is registered"""
        self.metadata.set_str3ds_register(name)

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents overlap"""

        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.overlapping(dataset.spatial_extent)
        else:
            return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """!Return the two or three dimensional spatial relation"""

        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.spatial_relation(dataset.spatial_extent)
        else:
            return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)

    def reset(self, ident):
        """!Reset the internal structure and set the identifier"""
        self.base = Raster3DBase(ident=ident)
        self.absolute_time = Raster3DAbsoluteTime(ident=ident)
        self.relative_time = Raster3DRelativeTime(ident=ident)
        self.spatial_extent = Raster3DSpatialExtent(ident=ident)
        self.metadata = Raster3DMetadata(ident=ident)

    def has_grass_timestamp(self):
        """!Check if a grass file bsased time stamp exists for this map.
        """
        if G_has_raster3d_timestamp(self.get_name(), self.get_mapset()):
            return True
        else:
            return False

    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata 
        in the grass file system based spatial database.

           Internally the libgis API functions are used for writing
        """

        ts = libgis.TimeStamp()

        libgis.G_scan_timestamp(byref(ts), self._convert_timestamp())
        check = libgis.G_write_raster3d_timestamp(self.get_name(), byref(ts))

        if check == -1:
            core.error(_("Unable to create timestamp file "
                         "for raster3d map <%s>" % (self.get_map_id())))

        if check == -2:
            core.error(_("Invalid datetime in timestamp "
                         "for raster3d map <%s>" % (self.get_map_id())))

    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based spatial database

           Internally the libgis API functions are used for removal
        """
        check = libgis.G_remove_raster3d_timestamp(self.get_name())

        if check == -1:
            core.error(_("Unable to remove timestamp for raster3d map <%s>" %
                         (self.get_name())))

    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database

           @return True if map exists, False otherwise
        """
        mapset = libgis.G_find_raster3d(self.get_name(), self.get_mapset())

        if not mapset:
            return False

        return True

    def read_info(self):
        """!Read the raster3d map info from the file system and store the content
           into a dictionary

           This method uses the ctypes interface to the gis and raster3d libraries
           to read the map metadata information
        """

        kvp = {}

        name = self.get_name()
        mapset = self.get_mapset()

        if not self.map_exists():
            core.fatal(_("Raster3d map <%s> not found" % name))

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
            core.fatal(_("Unable to open 3D raster map <%s>" % (name)))

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
            core.fatal(_("Unable to load range of 3D raster map <%s>" %
                         (name)))
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
            G_fatal_error(_("Unable to close 3D raster map <%s>" % (name)))

        return kvp

    def load(self):
        """!Load all info from an existing raster3d map into the internal structure"""

        # Fill base information
        self.base.set_creator(str(getpass.getuser()))

        # Fill spatial extent

        # Get the data from an existing 3D raster map
        global use_ctypes_map_access

        if use_ctypes_map_access:
            kvp = self.read_info()
        else:
            kvp = raster3d.raster3d_info(self.get_id())

        self.set_spatial_extent(north=kvp["north"], south=kvp["south"],
                                east=kvp["east"], west=kvp["west"],
                                top=kvp["top"], bottom=kvp["bottom"])

        # Fill metadata
        self.metadata.set_nsres(kvp["nsres"])
        self.metadata.set_ewres(kvp["ewres"])
        self.metadata.set_tbres(kvp["tbres"])
        self.metadata.set_datatype(kvp["datatype"])
        self.metadata.set_min(kvp["min"])
        self.metadata.set_max(kvp["max"])

        rows = int(kvp["rows"])
        cols = int(kvp["cols"])
        depths = int(kvp["depths"])

        ncells = cols * rows

        ncells = cols * rows * depths

        self.metadata.set_cols(cols)
        self.metadata.set_rows(rows)
        self.metadata.set_depths(depths)
        self.metadata.set_number_of_cells(ncells)

###############################################################################

class VectorDataset(AbstractMapDataset):
    """!Vector dataset class

       This class provides functions to select, update, insert or delete vector
       map information and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
        AbstractMapDataset.__init__(self)
        self.reset(ident)

    def get_type(self):
        return "vector"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return VectorDataset(ident)

    def get_new_stds_instance(self, ident):
        """!Return a new space time dataset instance in which maps 
        are stored with the type of this class"""
        return SpaceTimeVectorDataset(ident)

    def get_stds_register(self):
        """!Return the space time dataset register table name in 
        which stds are listed in which this map is registered"""
        return self.metadata.get_stvds_register()

    def set_stds_register(self, name):
        """!Set the space time dataset register table name in 
        which stds are listed in which this map is registered"""
        self.metadata.set_stvds_register(name)

    def get_layer(self):
        """!Return the layer"""
        return self.base.get_layer()

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents 2d overlap"""

        return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """!Return the two dimensional spatial relation"""

        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)

    def reset(self, ident):
        """!Reset the internal structure and set the identifier"""
        self.base = VectorBase(ident=ident)
        self.absolute_time = VectorAbsoluteTime(ident=ident)
        self.relative_time = VectorRelativeTime(ident=ident)
        self.spatial_extent = VectorSpatialExtent(ident=ident)
        self.metadata = VectorMetadata(ident=ident)

    def has_grass_timestamp(self):
        """!Check if a grass file bsased time stamp exists for this map.
        """
        if G_has_vector_timestamp(self.get_name(), self.get_layer(), 
                                  self.get_mapset()):
            return True
        else:
            return False

    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata in 
           the grass file system based spatial database.

           Internally the libgis API functions are used for writing
        """

        ts = libgis.TimeStamp()

        libgis.G_scan_timestamp(byref(ts), self._convert_timestamp())
        check = libgis.G_write_vector_timestamp(
            self.get_name(), self.get_layer(), byref(ts))

        if check == -1:
            core.error(_("Unable to create timestamp file "
                         "for vector map <%s>" % (self.get_map_id())))

        if check == -2:
            core.error(_("Invalid datetime in timestamp for vector map <%s>" %
                         (self.get_map_id())))

    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based spatial 
           database

           Internally the libgis API functions are used for removal
        """
        check = libgis.G_remove_vector_timestamp(
            self.get_name(), self.get_layer())

        if check == -1:
            core.error(_("Unable to remove timestamp for vector map <%s>" %
                         (self.get_name())))

    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database

           @return True if map exists, False otherwise
        """
        mapset = libgis.G_find_vector(self.get_name(), self.get_mapset())

        if not mapset:
            return False

        return True

    def read_info(self):
        """!Read the vector map info from the file system and store the content
           into a dictionary

           This method uses the ctypes interface to the vector libraries
           to read the map metadata information
        """

        kvp = {}

        name = self.get_name()
        mapset = self.get_mapset()

        if not self.map_exists():
            core.fatal(_("Vector map <%s> not found" % name))

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
            core.message(_("Open map without topology support"))
            if libvector.Vect_open_old2(byref(Map), name, mapset, "1") < 1:
                core.fatal(_("Unable to open vector map <%s>" % 
                             (libvector.Vect_get_full_name(byref(Map)))))

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

    def load(self):
        """!Load all info from an existing vector map into the internal 
        structure"""

        # Fill base information
        self.base.set_creator(str(getpass.getuser()))

        # Get the data from an existing vector map
        global use_ctypes_map_access

	if use_ctypes_map_access:
            kvp = self.read_info()
        else:
            kvp = vector.vector_info(self.get_map_id())

        # Fill spatial extent
        self.set_spatial_extent(north=kvp["north"], south=kvp["south"],
                                east=kvp["east"], west=kvp["west"],
                                top=kvp["top"], bottom=kvp["bottom"])

        # Fill metadata
        self.metadata.set_3d_info(kvp["map3d"])
        self.metadata.set_number_of_points(kvp["points"])
        self.metadata.set_number_of_lines(kvp["lines"])
        self.metadata.set_number_of_boundaries(kvp["boundaries"])
        self.metadata.set_number_of_centroids(kvp["centroids"])
        self.metadata.set_number_of_faces(kvp["faces"])
        self.metadata.set_number_of_kernels(kvp["kernels"])
        self.metadata.set_number_of_primitives(kvp["primitives"])
        self.metadata.set_number_of_nodes(kvp["nodes"])
        self.metadata.set_number_of_areas(kvp["areas"])
        self.metadata.set_number_of_islands(kvp["islands"])
        self.metadata.set_number_of_holes(kvp["holes"])
        self.metadata.set_number_of_volumes(kvp["volumes"])

###############################################################################

class SpaceTimeRasterDataset(AbstractSpaceTimeDataset):
    """!Space time raster dataset class
    """
    def __init__(self, ident):
        AbstractSpaceTimeDataset.__init__(self, ident)

    def get_type(self):
        return "strds"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return SpaceTimeRasterDataset(ident)

    def get_new_map_instance(self, ident):
        """!Return a new instance of a map dataset which is associated "
        "with the type of this class"""
        return RasterDataset(ident)

    def get_map_register(self):
        """!Return the name of the map register table"""
        return self.metadata.get_raster_register()

    def set_map_register(self, name):
        """!Set the name of the map register table"""
        self.metadata.set_raster_register(name)

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents 2d overlap"""

        return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """!Return the two dimensional spatial relation"""

        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)

    def reset(self, ident):

        """!Reset the internal structure and set the identifier"""
        self.base = STRDSBase(ident=ident)
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = STRDSAbsoluteTime(ident=ident)
        self.relative_time = STRDSRelativeTime(ident=ident)
        self.spatial_extent = STRDSSpatialExtent(ident=ident)
        self.metadata = STRDSMetadata(ident=ident)

###############################################################################

class SpaceTimeRaster3DDataset(AbstractSpaceTimeDataset):
    """!Space time raster3d dataset class
    """

    def __init__(self, ident):
        AbstractSpaceTimeDataset.__init__(self, ident)

    def get_type(self):
        return "str3ds"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return SpaceTimeRaster3DDataset(ident)

    def get_new_map_instance(self, ident):
        """!Return a new instance of a map dataset which is associated 
        with the type of this class"""
        return Raster3DDataset(ident)

    def get_map_register(self):
        """!Return the name of the map register table"""
        return self.metadata.get_raster3d_register()

    def set_map_register(self, name):
        """!Set the name of the map register table"""
        self.metadata.set_raster3d_register(name)

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents overlap"""

        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.overlapping(dataset.spatial_extent)
        else:
            return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """!Return the two or three dimensional spatial relation"""

        if self.get_type() == dataset.get_type() or \
           dataset.get_type() == "str3ds":
            return self.spatial_extent.spatial_relation(dataset.spatial_extent)
        else:
            return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)

    def reset(self, ident):

        """!Reset the internal structure and set the identifier"""
        self.base = STR3DSBase(ident=ident)
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = STR3DSAbsoluteTime(ident=ident)
        self.relative_time = STR3DSRelativeTime(ident=ident)
        self.spatial_extent = STR3DSSpatialExtent(ident=ident)
        self.metadata = STR3DSMetadata(ident=ident)

###############################################################################


class SpaceTimeVectorDataset(AbstractSpaceTimeDataset):
    """!Space time vector dataset class
    """

    def __init__(self, ident):
        AbstractSpaceTimeDataset.__init__(self, ident)

    def get_type(self):
        return "stvds"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return SpaceTimeVectorDataset(ident)

    def get_new_map_instance(self, ident):
        """!Return a new instance of a map dataset which is associated 
        with the type of this class"""
        return VectorDataset(ident)

    def get_map_register(self):
        """!Return the name of the map register table"""
        return self.metadata.get_vector_register()

    def set_map_register(self, name):
        """!Set the name of the map register table"""
        self.metadata.set_vector_register(name)

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents 2d overlap"""

        return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """!Return the two dimensional spatial relation"""

        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)

    def reset(self, ident):

        """!Reset the internal structure and set the identifier"""
        self.base = STVDSBase(ident=ident)
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = STVDSAbsoluteTime(ident=ident)
        self.relative_time = STVDSRelativeTime(ident=ident)
        self.spatial_extent = STVDSSpatialExtent(ident=ident)
        self.metadata = STVDSMetadata(ident=ident)

###############################################################################


class AbstractDatasetComparisonKeyStartTime(object):
    """!This comparison key can be used to sort lists of abstract datasets 
       by start time

        Example:

        # Return all maps in a space time raster dataset as map objects
        map_list = strds.get_registered_maps_as_objects()

        # Sort the maps in the list by start time
        sorted_map_list = sorted(
            map_list, key=AbstractDatasetComparisonKeyStartTime)
    """
    def __init__(self, obj, *args):
        self.obj = obj

    def __lt__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return startA < startB

    def __gt__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return startA > startB

    def __eq__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return startA == startB

    def __le__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return startA <= startB

    def __ge__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return startA >= startB

    def __ne__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return startA != startB

###############################################################################


class AbstractDatasetComparisonKeyEndTime(object):
    """!This comparison key can be used to sort lists of abstract datasets 
       by end time

        Example:

        # Return all maps in a space time raster dataset as map objects
        map_list = strds.get_registered_maps_as_objects()

        # Sort the maps in the list by end time
        sorted_map_list = sorted(
            map_list, key=AbstractDatasetComparisonKeyEndTime)
    """
    def __init__(self, obj, *args):
        self.obj = obj

    def __lt__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return endA < endB

    def __gt__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return endA > endB

    def __eq__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return endA == endB

    def __le__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return endA <= endB

    def __ge__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return endA >= endB

    def __ne__(self, other):
        startA, endA = self.obj.get_valid_time()
        startB, endB = other.obj.get_valid_time()
        return endA != endB

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
