"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

strds = tgis.space_time_raster_dataset("soils_1950_2010")

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import getpass
import grass.script.raster as raster
import grass.script.vector as vector
import grass.script.raster3d as raster3d

from ctypes import *
import grass.lib.gis as libgis
import grass.lib.raster as libraster

from datetime_math import *
from abstract_map_dataset import *
from abstract_space_time_dataset import *


###############################################################################

class raster_dataset(abstract_map_dataset):
    """!Raster dataset class

       This class provides functions to select, update, insert or delete raster
       map information and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_type(self):
        return "raster"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return raster_dataset(ident)

    def get_new_stds_instance(self, ident):
        """!Return a new space time dataset instance in which maps are stored with the type of this class"""
        return space_time_raster_dataset(ident)

    def get_stds_register(self):
        """!Return the space time dataset register table name in which stds are listed in which this map is registered"""
        return self.metadata.get_strds_register()

    def set_stds_register(self, name):
        """!Set the space time dataset register table name in which stds are listed in which this map is registered"""
        self.metadata.set_strds_register(name)

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents 2d overlap"""
        
        return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """Return the two dimensional spatial relation"""
        
        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)
	
    def reset(self, ident):
	"""!Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = raster_base(ident=ident)
	self.absolute_time = raster_absolute_time(ident=ident)
	self.relative_time = raster_relative_time(ident=ident)
	self.spatial_extent = raster_spatial_extent(ident=ident)
	self.metadata = raster_metadata(ident=ident)
		
    def has_grass_timestamp(self):
        """!Check if a grass file bsased time stamp exists for this map. 
        """
        if G_has_raster_timestamp(self.get_name(), self.get_mapset()):
	    return True
	else:
	    return False
 
    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata in the grass file system based spatial
           database. 
           
           Internally the libgis API functions are used for writing
        """
        
	ts = libgis.TimeStamp()

	libgis.G_scan_timestamp(byref(ts), self._convert_timestamp())
	check = libgis.G_write_raster_timestamp(self.get_name(), byref(ts))
	
	if check == -1:
		core.error(_("Unable to create timestamp file for raster map <%s>"%(self.get_map_id())))
		
	if check == -2:
		core.error(_("Invalid datetime in timestamp for raster map <%s>"%(self.get_map_id())))
			
    
    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based spatial database
        
           Internally the libgis API functions are used for removal
        """
        check = libgis.G_remove_raster_timestamp(self.get_name())
        
        if check == -1:
            core.error(_("Unable to remove timestamp for raster map <%s>"%(self.get_name())))
	
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
	    libraster.Rast_init_fp_range (byref(range))
	    libraster.Rast_read_fp_range(name, mapset, byref(range))
	    min = libgis.DCELL()
	    max = libgis.DCELL()
	    libraster.Rast_get_fp_range_min_max(byref(range), byref(min), byref(max))
	    kvp["min"] = float(min.value)
	    kvp["max"] = float(max.value)
	else:
	    range = libraster.Range()
	    libraster.Rast_init_range (byref(range))
	    libraster.Rast_read_range(name, mapset, byref(range))
	    min = libgis.CELL()
	    max = libgis.CELL()
	    libraster.Rast_get_fp_range_min_max(byref(range), byref(min), byref(max))
	    kvp["min"] = int(min.value)
	    kvp["max"] = int(max.value)
	
	return kvp

    def load(self):
        """!Load all info from an existing raster map into the internal structure"""


        # Fill base information

        self.base.set_name(self.ident.split("@")[0])
        self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))

        # Get the data from an existing raster map
        kvp = self.read_info()
        
        # Fill spatial extent

        self.set_spatial_extent(north=kvp["north"], south=kvp["south"], \
                                east=kvp["east"],   west=kvp["west"])

        # Fill metadata

        self.metadata.set_nsres(kvp["nsres"])
        self.metadata.set_ewres(kvp["ewres"])
        self.metadata.set_datatype(kvp["datatype"])
        self.metadata.set_min(kvp["min"])
        self.metadata.set_max(kvp["max"])

        rows = kvp["rows"]
        cols = kvp["cols"]

        ncells = cols * rows

        self.metadata.set_cols(cols)
        self.metadata.set_rows(rows)
        self.metadata.set_number_of_cells(ncells)

###############################################################################

class raster3d_dataset(abstract_map_dataset):
    """!Raster3d dataset class

       This class provides functions to select, update, insert or delete raster3d
       map information and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_type(self):
        return "raster3d"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return raster3d_dataset(ident)

    def get_new_stds_instance(self, ident):
        """!Return a new space time dataset instance in which maps are stored with the type of this class"""
        return space_time_raster3d_dataset(ident)

    def get_stds_register(self):
        """!Return the space time dataset register table name in which stds are listed in which this map is registered"""
        return self.metadata.get_str3ds_register()

    def set_stds_register(self, name):
        """!Set the space time dataset register table name in which stds are listed in which this map is registered"""
        self.metadata.set_str3ds_register(name)

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents overlap"""
        
        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.overlapping(dataset.spatial_extent)
        else:
            return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """Return the two or three dimensional spatial relation"""
        
        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.spatial_relation(dataset.spatial_extent)
        else:
            return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)
        
    def reset(self, ident):
	"""!Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = raster3d_base(ident=ident)
	self.absolute_time = raster3d_absolute_time(ident=ident)
	self.relative_time = raster3d_relative_time(ident=ident)
	self.spatial_extent = raster3d_spatial_extent(ident=ident)
	self.metadata = raster3d_metadata(ident=ident)

    def has_grass_timestamp(self):
        """!Check if a grass file bsased time stamp exists for this map. 
        """
        if G_has_raster3d_timestamp(self.get_name(), self.get_mapset()):
	    return True
	else:
	    return False
 
    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata in the grass file system based spatial
           database. 
           
           Internally the libgis API functions are used for writing
        """
        
	ts = libgis.TimeStamp()

	libgis.G_scan_timestamp(byref(ts), self._convert_timestamp())
	check = libgis.G_write_raster3d_timestamp(self.get_name(), byref(ts))
	
	if check == -1:
		core.error(_("Unable to create timestamp file for raster3d map <%s>"%(self.get_map_id())))
		
	if check == -2:
		core.error(_("Invalid datetime in timestamp for raster3d map <%s>"%(self.get_map_id())))
			
    
    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based spatial database
        
           Internally the libgis API functions are used for removal
        """
        check = libgis.G_remove_raster3d_timestamp(self.get_name())
        
        if check == -1:
            core.error(_("Unable to remove timestamp for raster3d map <%s>"%(self.get_name())))
	
    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database
        
           @return True if map exists, False otherwise
        """        
        mapset = libgis.G_find_raster3d(self.get_name(), self.get_mapset())
        
        if not mapset:
            return False
	
	return True
        
    def load(self):
        """!Load all info from an existing raster3d map into the internal structure"""

        # Get the data from an existing raster map
        kvp = raster3d.raster3d_info(self.get_map_id())

        # Fill base information

        self.base.set_name(self.ident.split("@")[0])
        self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))

        # Fill spatial extent

        self.set_spatial_extent(north=kvp["north"], south=kvp["south"], \
                                east=kvp["east"],   west=kvp["west"],\
                                top=kvp["top"], bottom=kvp["bottom"])

        # Fill metadata

        self.metadata.set_nsres(kvp["nsres"])
        self.metadata.set_ewres(kvp["ewres"])
        self.metadata.set_tbres(kvp["tbres"])
        self.metadata.set_datatype(kvp["datatype"])
        self.metadata.set_min(kvp["min"])
        self.metadata.set_max(kvp["max"])

        rows = int((kvp["north"] - kvp["south"])/kvp["nsres"] + 0.5)
        cols = int((kvp["east"] - kvp["west"])/kvp["ewres"] + 0.5)
        depths = int((kvp["top"] - kvp["bottom"])/kvp["tbres"] + 0.5)

        ncells = cols * rows * depths

        self.metadata.set_cols(cols)
        self.metadata.set_rows(rows)
        self.metadata.set_depths(depths)
        self.metadata.set_number_of_cells(ncells)

###############################################################################

class vector_dataset(abstract_map_dataset):
    """!Vector dataset class

       This class provides functions to select, update, insert or delete vector
       map information and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_type(self):
        return "vector"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return vector_dataset(ident)

    def get_new_stds_instance(self, ident):
        """!Return a new space time dataset instance in which maps are stored with the type of this class"""
        return space_time_vector_dataset(ident)

    def get_stds_register(self):
        """!Return the space time dataset register table name in which stds are listed in which this map is registered"""
        return self.metadata.get_stvds_register()

    def set_stds_register(self, name):
        """!Set the space time dataset register table name in which stds are listed in which this map is registered"""
        self.metadata.set_stvds_register(name)

    def get_layer(self):
        """!Return the layer"""
        return self.base.get_layer()

    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents 2d overlap"""
        
        return self.spatial_extent.overlapping_2d(dataset.spatial_extent)

    def spatial_relation(self, dataset):
        """Return the two dimensional spatial relation"""
        
        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)
	
    def reset(self, ident):
	"""!Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = vector_base(ident=ident)
	self.absolute_time = vector_absolute_time(ident=ident)
	self.relative_time = vector_relative_time(ident=ident)
	self.spatial_extent = vector_spatial_extent(ident=ident)
	self.metadata = vector_metadata(ident=ident)

    def has_grass_timestamp(self):
        """!Check if a grass file bsased time stamp exists for this map. 
        """
        if G_has_raster_timestamp(self.get_name(), self.get_layer(), self.get_mapset()):
	    return True
	else:
	    return False
 
    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata in the grass file system based spatial
           database. 
           
           Internally the libgis API functions are used for writing
        """
        
	ts = libgis.TimeStamp()

	libgis.G_scan_timestamp(byref(ts), self._convert_timestamp())
	check = libgis.G_write_vector_timestamp(self.get_name(), self.get_layer(), byref(ts))
	
	if check == -1:
		core.error(_("Unable to create timestamp file for vector map <%s>"%(self.get_map_id())))
		
	if check == -2:
		core.error(_("Invalid datetime in timestamp for vector map <%s>"%(self.get_map_id())))
			
    
    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file system based spatial database
        
           Internally the libgis API functions are used for removal
        """
        check = libgis.G_remove_vector_timestamp(self.get_name(), self.get_layer())
        
        if check == -1:
            core.error(_("Unable to remove timestamp for vector map <%s>"%(self.get_name())))
	
    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database
        
           @return True if map exists, False otherwise
        """        
        mapset = libgis.G_find_vector(self.get_name(), self.get_mapset())
        
        if not mapset:
            return False
	
	return True
        
    def load(self):
        """!Load all info from an existing vector map into the internal structure"""

        # Get the data from an existing raster map
        kvp = vector.vector_info(self.get_map_id())

        # Fill base information
	if self.ident.find(":") >= 0:
	    self.base.set_name(self.ident.split("@")[0].split(":")[0])
	    self.base.set_layer(self.ident.split("@")[0].split(":")[1])
	else:
	    self.base.set_name(self.ident.split("@")[0])
        self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))

        # Fill spatial extent

        self.set_spatial_extent(north=kvp["north"], south=kvp["south"], \
                                east=kvp["east"],   west=kvp["west"],\
                                top=kvp["top"], bottom=kvp["bottom"])

        # Fill metadata .. no metadata yet

###############################################################################

class space_time_raster_dataset(abstract_space_time_dataset):
    """!Space time raster dataset class
    """
    def __init__(self, ident):
        abstract_space_time_dataset.__init__(self, ident)

    def get_type(self):
        return "strds"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return space_time_raster_dataset(ident)

    def get_new_map_instance(self, ident):
        """!Return a new instance of a map dataset which is associated with the type of this class"""
        return raster_dataset(ident)

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
        """Return the two dimensional spatial relation"""
        
        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)
	
    def reset(self, ident):

	"""!Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = strds_base(ident=ident)

        if ident != None:
            self.base.set_name(self.ident.split("@")[0])
            self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = strds_absolute_time(ident=ident)
        self.relative_time = strds_relative_time(ident=ident)
	self.spatial_extent = strds_spatial_extent(ident=ident)
	self.metadata = strds_metadata(ident=ident)

###############################################################################

class space_time_raster3d_dataset(abstract_space_time_dataset):
    """!Space time raster3d dataset class
    """

    def __init__(self, ident):
        abstract_space_time_dataset.__init__(self, ident)

    def get_type(self):
        return "str3ds"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return space_time_raster3d_dataset(ident)

    def get_new_map_instance(self, ident):
        """!Return a new instance of a map dataset which is associated with the type of this class"""
        return raster3d_dataset(ident)

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
        """Return the two or three dimensional spatial relation"""
        
        if self.get_type() == dataset.get_type() or dataset.get_type() == "str3ds":
            return self.spatial_extent.spatial_relation(dataset.spatial_extent)
        else:
            return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)
        
    def reset(self, ident):

	"""!Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = str3ds_base(ident=ident)

        if ident != None:
            self.base.set_name(self.ident.split("@")[0])
            self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = str3ds_absolute_time(ident=ident)
        self.relative_time = str3ds_relative_time(ident=ident)
	self.spatial_extent = str3ds_spatial_extent(ident=ident)
	self.metadata = str3ds_metadata(ident=ident)

###############################################################################

class space_time_vector_dataset(abstract_space_time_dataset):
    """!Space time vector dataset class
    """

    def __init__(self, ident):
        abstract_space_time_dataset.__init__(self, ident)

    def get_type(self):
        return "stvds"

    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class"""
        return space_time_vector_dataset(ident)

    def get_new_map_instance(self, ident):
        """!Return a new instance of a map dataset which is associated with the type of this class"""
        return vector_dataset(ident)

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
        """Return the two dimensional spatial relation"""
        
        return self.spatial_extent.spatial_relation_2d(dataset.spatial_extent)

    def reset(self, ident):

	"""!Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = stvds_base(ident=ident)

        if ident != None:
            self.base.set_name(self.ident.split("@")[0])
            self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = stvds_absolute_time(ident=ident)
        self.relative_time = stvds_relative_time(ident=ident)
	self.spatial_extent = stvds_spatial_extent(ident=ident)
	self.metadata = stvds_metadata(ident=ident)

