"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import getpass
import logging
from abstract_map_dataset import *
from abstract_space_time_dataset import *

###############################################################################

class RasterDataset(AbstractMapDataset):
    """!Raster dataset class

       This class provides functions to select, update, insert or delete raster
       map information and valid time stamps into the SQL temporal database.

       Usage:

        @code

        >>> import grass.script as grass
        >>> init()
        >>> grass.use_temp_region()
        >>> grass.run_command("g.region", n=80.0, s=0.0, e=120.0, w=0.0,
        ... t=1.0, b=0.0, res=10.0)
        0
        >>> grass.run_command("r.mapcalc", overwrite=True, quiet=True,
        ... expression="strds_map_test_case = 1")
        0
        >>> grass.run_command("r.timestamp", map="strds_map_test_case",
        ...                   date="15 jan 1999", quiet=True)
        0
        >>> mapset = get_current_mapset()
        >>> name = "strds_map_test_case"
        >>> identifier = "%s@%s" % (name, mapset)
        >>> rmap = RasterDataset(identifier)
        >>> rmap.map_exists()
        True
        >>> rmap.read_timestamp_from_grass()
        True
        >>> rmap.get_temporal_extent_as_tuple()
        (datetime.datetime(1999, 1, 15, 0, 0), None)
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
         | Start time:................. 1999-01-15 00:00:00
         | End time:................... None
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
        >>> rmap.set_absolute_time(start_time=datetime(2001,1,1),
        ...                        end_time=datetime(2012,1,1))
        True
        >>> rmap.get_absolute_time()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0), None)
        >>> rmap.get_temporal_extent_as_tuple()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0))
        >>> rmap.get_name()
        'strds_map_test_case'
        >>> rmap.get_mapset() == mapset
        True
        >>> rmap.get_temporal_type()
        'absolute'
        >>> rmap.get_spatial_extent_as_tuple()
        (80.0, 0.0, 120.0, 0.0, 0.0, 0.0)
        >>> rmap.is_time_absolute()
        True
        >>> rmap.is_time_relative()
        False

        >>> grass.run_command("g.remove", rast=name, quiet=True)
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

    def spatial_intersection(self, dataset):
        """!Return the two dimensional intersection as spatial_extent
           object or None in case no intersection was found.

           @param dataset The abstract dataset to intersect with
           @return The intersection spatial extent or None
        """
        return self.spatial_extent.intersect_2d(dataset.spatial_extent)

    def spatial_union(self, dataset):
        """!Return the two dimensional union as spatial_extent
           object or None in case the extents does not overlap or meet.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent or None
        """
        return self.spatial_extent.union_2d(dataset.spatial_extent)

    def spatial_disjoint_union(self, dataset):
        """!Return the two dimensional union as spatial_extent object.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
        return self.spatial_extent.disjoint_union_2d(dataset.spatial_extent)

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

           @return True if success, False on error
        """
        return self.ciface.has_raster_timestamp(self.get_name(),
                                                self.get_mapset())

    def read_timestamp_from_grass(self):
        """!Read the timestamp of this map from the map metadata
           in the grass file system based spatial database and
           set the internal time stamp that should be insert/updated
           in the temporal database.

           @return True if success, False on error
        """

        if not self.has_grass_timestamp():
            return False

        check, dates = self.ciface.read_raster_timestamp(self.get_name(),
                                                      self.get_mapset(),)

        if check < 1:
            self.msgr.error(_("Unable to read timestamp file "
                         "for raster map <%s>" % (self.get_map_id())))
            return False

        if len(dates) == 2:
            self.set_absolute_time(dates[0], dates[1], None)
        else:
            self.set_relative_time(dates[0], dates[1], dates[2])

        return True

    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata in
           the grass file system based spatial database.

           Internally the libgis API functions are used for writing

           @return True if success, False on error
        """
        check = self.ciface.write_raster_timestamp(self.get_name(),
                                                   self.get_mapset(),
                                                   self._convert_timestamp())

        if check == -1:
            self.msgr.error(_("Unable to create timestamp file "
                         "for raster map <%s>" % (self.get_map_id())))
            return False

        if check == -2:
            self.msgr.error(_("Invalid datetime in timestamp for raster map <%s>" %
                         (self.get_map_id())))
            return False

        if check == -3:
            self.msgr.error(_("Internal error"))
            return False

        return True

    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based
           spatial database

           Internally the libgis API functions are used for removal

           @return True if success, False on error
        """
        check = self.ciface.remove_raster_timestamp(self.get_name(),
                                                    self.get_mapset())

        if check == -1:
            self.msgr.error(_("Unable to remove timestamp for raster map <%s>" %
                         (self.get_name())))
            return False

        return True

    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database

           @return True if map exists, False otherwise
        """
        return self.ciface.raster_map_exists(self.get_name(),
                                             self.get_mapset())

    def load(self):
        """!Load all info from an existing raster map into the internal s
           tructure"""

        # Fill base information
        self.base.set_creator(str(getpass.getuser()))

        kvp = self.ciface.read_raster_info(self.get_name(),
                                           self.get_mapset())

        # Fill spatial extent
        self.set_spatial_extent_from_values(north=kvp["north"],
                                            south=kvp["south"],
                                            east=kvp["east"],
                                            west=kvp["west"])

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

       Usage:

        @code

        >>> import grass.script as grass
        >>> init()
        >>> grass.use_temp_region()
        >>> grass.run_command("g.region", n=80.0, s=0.0, e=120.0, w=0.0,
        ... t=100.0, b=0.0, res=10.0, res3=10.0)
        0
        >>> grass.run_command("r3.mapcalc", overwrite=True, quiet=True,
        ...                   expression="str3ds_map_test_case = 1")
        0
        >>> grass.run_command("r3.timestamp", map="str3ds_map_test_case",
        ...                   date="15 jan 1999", quiet=True)
        0
        >>> mapset = get_current_mapset()
        >>> name = "str3ds_map_test_case"
        >>> identifier = "%s@%s" % (name, mapset)
        >>> r3map = Raster3DDataset(identifier)
        >>> r3map.map_exists()
        True
        >>> r3map.read_timestamp_from_grass()
        True
        >>> r3map.get_temporal_extent_as_tuple()
        (datetime.datetime(1999, 1, 15, 0, 0), None)
        >>> r3map.load()
        >>> r3map.spatial_extent.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 0.0
         | East:.. .................... 120.0
         | West:....................... 0.0
         | Top:........................ 100.0
         | Bottom:..................... 0.0
        >>> r3map.absolute_time.print_info()
         +-------------------- Absolute time -----------------------------------------+
         | Start time:................. 1999-01-15 00:00:00
         | End time:................... None
        >>> r3map.metadata.print_info()
         +-------------------- Metadata information ----------------------------------+
         | Datatype:................... DCELL
         | Number of columns:.......... 8
         | Number of rows:............. 12
         | Number of cells:............ 960
         | North-South resolution:..... 10.0
         | East-west resolution:....... 10.0
         | Minimum value:.............. 1.0
         | Maximum value:.............. 1.0
         | Number of depths:........... 10
         | Top-Bottom resolution:...... 10.0
         | STR3DS register table ...... None

        >>> newmap = r3map.get_new_instance("new@PERMANENT")
        >>> isinstance(newmap, Raster3DDataset)
        True
        >>> newstr3ds = r3map.get_new_stds_instance("new@PERMANENT")
        >>> isinstance(newstr3ds, SpaceTimeRaster3DDataset)
        True
        >>> r3map.get_type()
        'raster3d'
        >>> r3map.get_stds_register()
        >>> r3map.set_absolute_time(start_time=datetime(2001,1,1),
        ...                        end_time=datetime(2012,1,1))
        True
        >>> r3map.get_absolute_time()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0), None)
        >>> r3map.get_temporal_extent_as_tuple()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0))
        >>> r3map.get_name()
        'str3ds_map_test_case'
        >>> r3map.get_mapset() == mapset
        True
        >>> r3map.get_temporal_type()
        'absolute'
        >>> r3map.get_spatial_extent_as_tuple()
        (80.0, 0.0, 120.0, 0.0, 100.0, 0.0)
        >>> r3map.is_time_absolute()
        True
        >>> r3map.is_time_relative()
        False
        >>> grass.run_command("g.remove", rast3d=name, quiet=True)
        0
        >>> grass.del_temp_region()
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

    def spatial_intersection(self, dataset):
        """!Return the three or two dimensional intersection as spatial_extent
           object or None in case no intersection was found.

           @param dataset The abstract dataset to intersect with
           @return The intersection spatial extent or None
        """
        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.intersect(dataset.spatial_extent)
        else:
            return self.spatial_extent.intersect_2d(dataset.spatial_extent)

    def spatial_union(self, dataset):
        """!Return the three or two dimensional union as spatial_extent
           object or None in case the extents does not overlap or meet.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent or None
        """
        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.union(dataset.spatial_extent)
        else:
            return self.spatial_extent.union_2d(dataset.spatial_extent)

    def spatial_disjoint_union(self, dataset):
        """!Return the three or two dimensional union as spatial_extent object.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.disjoint_union(dataset.spatial_extent)
        else:
            return self.spatial_extent.disjoint_union_2d(dataset.spatial_extent)

    def get_np_array(self):
        """!Return this 3D raster map as memmap numpy style array to access the 3D raster
           values in numpy style without loading the whole map in the RAM.

           In case this 3D raster map does exists in the grass spatial database,
           the map will be exported using r3.out.bin to a temporary location
           and assigned to the memmap object that is returned by this function.

           In case the 3D raster map does not exists, an empty temporary
           binary file will be created and assigned to the memap object.

           You need to call the write function to write the memmap
           array back into grass.
        """

        a = garray.array3d()

        if self.map_exists():
            a.read(self.get_map_id())

        return a

    def reset(self, ident):
        """!Reset the internal structure and set the identifier"""
        self.base = Raster3DBase(ident=ident)
        self.absolute_time = Raster3DAbsoluteTime(ident=ident)
        self.relative_time = Raster3DRelativeTime(ident=ident)
        self.spatial_extent = Raster3DSpatialExtent(ident=ident)
        self.metadata = Raster3DMetadata(ident=ident)

    def has_grass_timestamp(self):
        """!Check if a grass file bsased time stamp exists for this map.

           @return True if success, False on error
        """
        return self.ciface.has_raster3d_timestamp(self.get_name(),
                                                self.get_mapset())

    def read_timestamp_from_grass(self):
        """!Read the timestamp of this map from the map metadata
           in the grass file system based spatial database and
           set the internal time stamp that should be insert/updated
           in the temporal database.

           @return True if success, False on error
        """

        if not self.has_grass_timestamp():
            return False

        check, dates = self.ciface.read_raster3d_timestamp(self.get_name(),
                                                      self.get_mapset(),)

        if check < 1:
            self.msgr.error(_("Unable to read timestamp file "
                         "for 3D raster map <%s>" % (self.get_map_id())))
            return False

        if len(dates) == 2:
            self.set_absolute_time(dates[0], dates[1], None)
        else:
            self.set_relative_time(dates[0], dates[1], dates[2])

        return True

    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata
        in the grass file system based spatial database.

           Internally the libgis API functions are used for writing

           @return True if success, False on error
        """
        check = self.ciface.write_raster3d_timestamp(self.get_name(),
                                                     self.get_mapset(),
                                                     self._convert_timestamp())

        if check == -1:
            self.msgr.error(_("Unable to create timestamp file "
                         "for 3D raster map <%s>" % (self.get_map_id())))
            return False

        if check == -2:
            self.msgr.error(_("Invalid datetime in timestamp for 3D raster map <%s>" %
                         (self.get_map_id())))
            return False

        if check == -3:
            self.msgr.error(_("Internal error"))
            return False

        return True

    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based spatial database

           @return True if success, False on error
        """
        check = self.ciface.remove_raster3d_timestamp(self.get_name(),
                                                      self.get_mapset())

        if check == -1:
            self.msgr.error(_("Unable to remove timestamp for raster map <%s>" %
                         (self.get_name())))
            return False

        return True

    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database

           @return True if map exists, False otherwise
        """
        return self.ciface.raster3d_map_exists(self.get_name(),
                                               self.get_mapset())

    def load(self):
        """!Load all info from an existing raster3d map into the internal structure"""

        # Fill base information
        self.base.set_creator(str(getpass.getuser()))

        # Fill spatial extent
        kvp = self.ciface.read_raster3d_info(self.get_name(),
                                           self.get_mapset())

        self.set_spatial_extent_from_values(north=kvp["north"], south=kvp["south"],
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

       Usage:

        @code

        >>> import grass.script as grass
        >>> init()
        >>> grass.use_temp_region()
        >>> grass.run_command("g.region", n=80.0, s=0.0, e=120.0, w=0.0,
        ... t=1.0, b=0.0, res=10.0)
        0
        >>> grass.run_command("v.random", overwrite=True, output="stvds_map_test_case",
        ... n=100, zmin=0, zmax=100, flags="z", column="elevation", quiet=True)
        0
        >>> grass.run_command("v.timestamp", map="stvds_map_test_case",
        ...                   date="15 jan 1999", quiet=True)
        0
        >>> mapset = get_current_mapset()
        >>> name = "stvds_map_test_case"
        >>> identifier = "%s@%s" % (name, mapset)
        >>> vmap = VectorDataset(identifier)
        >>> vmap.map_exists()
        True
        >>> vmap.read_timestamp_from_grass()
        True
        >>> vmap.get_temporal_extent_as_tuple()
        (datetime.datetime(1999, 1, 15, 0, 0), None)
        >>> vmap.load()
        >>> vmap.absolute_time.print_info()
         +-------------------- Absolute time -----------------------------------------+
         | Start time:................. 1999-01-15 00:00:00
         | End time:................... None
        >>> vmap.metadata.print_info()
         +-------------------- Metadata information ----------------------------------+
         | STVDS register table ....... None
         | Is map 3d .................. True
         | Number of points ........... 100
         | Number of lines ............ 0
         | Number of boundaries ....... 0
         | Number of centroids ........ 0
         | Number of faces ............ 0
         | Number of kernels .......... 0
         | Number of primitives ....... 100
         | Number of nodes ............ 0
         | Number of areas ............ 0
         | Number of islands .......... 0
         | Number of holes ............ 0
         | Number of volumes .......... 0
        >>> newmap = vmap.get_new_instance("new@PERMANENT")
        >>> isinstance(newmap, VectorDataset)
        True
        >>> newstvds = vmap.get_new_stds_instance("new@PERMANENT")
        >>> isinstance(newstvds, SpaceTimeVectorDataset)
        True
        >>> vmap.get_type()
        'vector'
        >>> vmap.get_stds_register()
        >>> vmap.set_absolute_time(start_time=datetime(2001,1,1),
        ...                        end_time=datetime(2012,1,1))
        True
        >>> vmap.get_absolute_time()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0), None)
        >>> vmap.get_temporal_extent_as_tuple()
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2012, 1, 1, 0, 0))
        >>> vmap.get_name()
        'stvds_map_test_case'
        >>> vmap.get_mapset() == mapset
        True
        >>> vmap.get_temporal_type()
        'absolute'
        >>> vmap.is_time_absolute()
        True
        >>> vmap.is_time_relative()
        False
        >>> grass.run_command("g.remove", vect=name, quiet=True)
        0
        >>> grass.del_temp_region()

        @endcode
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

    def spatial_intersection(self, dataset):
        """!Return the two dimensional intersection as spatial_extent
           object or None in case no intersection was found.

           @param dataset The abstract dataset to intersect with
           @return The intersection spatial extent or None
        """
        return self.spatial_extent.intersect_2d(dataset.spatial_extent)

    def spatial_union(self, dataset):
        """!Return the two dimensional union as spatial_extent
           object or None in case the extents does not overlap or meet.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent or None
        """
        return self.spatial_extent.union_2d(dataset.spatial_extent)

    def spatial_disjoint_union(self, dataset):
        """!Return the two dimensional union as spatial_extent object.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
        return self.spatial_extent.disjoint_union_2d(dataset.spatial_extent)

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
        return self.ciface.has_vector_timestamp(self.get_name(),
                                                self.get_mapset(),
                                                self.get_layer())


    def read_timestamp_from_grass(self):
        """!Read the timestamp of this map from the map metadata
           in the grass file system based spatial database and
           set the internal time stamp that should be insert/updated
           in the temporal database.
        """

        if not self.has_grass_timestamp():
            return False

        check, dates = self.ciface.read_vector_timestamp(self.get_name(),
                                                      self.get_mapset(),)

        if check < 1:
            self.msgr.error(_("Unable to read timestamp file "
                         "for vector map <%s>" % (self.get_map_id())))
            return False

        if len(dates) == 2:
            self.set_absolute_time(dates[0], dates[1], None)
        else:
            self.set_relative_time(dates[0], dates[1], dates[2])

        return True

    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata in
           the grass file system based spatial database.

           Internally the libgis API functions are used for writing
        """
        check = self.ciface.write_vector_timestamp(self.get_name(),
                                                   self.get_mapset(),
                                                   self._convert_timestamp(),
                                                   self.get_layer())

        if check == -1:
            self.msgr.error(_("Unable to create timestamp file "
                         "for vector map <%s>" % (self.get_map_id())))
            return False

        if check == -2:
            self.msgr.error(_("Invalid datetime in timestamp for vector map <%s>" %
                         (self.get_map_id())))
            return False

        return True

    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based spatial
           database

           Internally the libgis API functions are used for removal
        """
        check = self.ciface.remove_vector_timestamp(self.get_name(),
                                                    self.get_mapset())

        if check == -1:
            self.msgr.error(_("Unable to remove timestamp for vector map <%s>" %
                         (self.get_name())))
            return False

        return True

    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database

           @return True if map exists, False otherwise
        """
        return self.ciface.vector_map_exists(self.get_name(),
                                             self.get_mapset())


    def load(self):
        """!Load all info from an existing vector map into the internal
        structure"""

        # Fill base information
        self.base.set_creator(str(getpass.getuser()))

        # Get the data from an existing vector map

        kvp = self.ciface.read_vector_info(self.get_name(),
                                           self.get_mapset())

        # Fill spatial extent
        self.set_spatial_extent_from_values(north=kvp["north"], south=kvp["south"],
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

    def spatial_intersection(self, dataset):
        """!Return the two dimensional intersection as spatial_extent
           object or None in case no intersection was found.

           @param dataset The abstract dataset to intersect with
           @return The intersection spatial extent or None
        """
        return self.spatial_extent.intersect_2d(dataset.spatial_extent)

    def spatial_union(self, dataset):
        """!Return the two dimensional union as spatial_extent
           object or None in case the extents does not overlap or meet.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent or None
        """
        return self.spatial_extent.union_2d(dataset.spatial_extent)

    def spatial_disjoint_union(self, dataset):
        """!Return the two dimensional union as spatial_extent object.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
        return self.spatial_extent.disjoint_union_2d(dataset.spatial_extent)

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

    def spatial_intersection(self, dataset):
        """!Return the three or two dimensional intersection as spatial_extent
           object or None in case no intersection was found.

           @param dataset The abstract dataset to intersect with
           @return The intersection spatial extent or None
        """
        if self.get_type() == dataset.get_type() or dataset.get_type() == "raster3d":
            return self.spatial_extent.intersect(dataset.spatial_extent)
        else:
            return self.spatial_extent.intersect_2d(dataset.spatial_extent)

    def spatial_union(self, dataset):
        """!Return the three or two dimensional union as spatial_extent
           object or None in case the extents does not overlap or meet.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent or None
        """
        if self.get_type() == dataset.get_type() or dataset.get_type() == "raster3d":
            return self.spatial_extent.union(dataset.spatial_extent)
        else:
            return self.spatial_extent.union_2d(dataset.spatial_extent)

    def spatial_disjoint_union(self, dataset):
        """!Return the three or two dimensional union as spatial_extent object.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
        if self.get_type() == dataset.get_type() or dataset.get_type() == "raster3d":
            return self.spatial_extent.disjoint_union(dataset.spatial_extent)
        else:
            return self.spatial_extent.disjoint_union_2d(dataset.spatial_extent)

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

    def spatial_intersection(self, dataset):
        """!Return the two dimensional intersection as spatial_extent
           object or None in case no intersection was found.

           @param dataset The abstract dataset to intersect with
           @return The intersection spatial extent or None
        """
        return self.spatial_extent.intersect_2d(dataset.spatial_extent)

    def spatial_union(self, dataset):
        """!Return the two dimensional union as spatial_extent
           object or None in case the extents does not overlap or meet.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent or None
        """
        return self.spatial_extent.union_2d(dataset.spatial_extent)

    def spatial_disjoint_union(self, dataset):
        """!Return the two dimensional union as spatial_extent object.

           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
        return self.spatial_extent.disjoint_union_2d(dataset.spatial_extent)

    def reset(self, ident):

        """!Reset the internal structure and set the identifier"""
        self.base = STVDSBase(ident=ident)
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = STVDSAbsoluteTime(ident=ident)
        self.relative_time = STVDSRelativeTime(ident=ident)
        self.spatial_extent = STVDSSpatialExtent(ident=ident)
        self.metadata = STVDSMetadata(ident=ident)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
