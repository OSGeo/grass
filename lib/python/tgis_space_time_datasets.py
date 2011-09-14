"""!@package grass.script.tgis_space_time_dataset

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
from grass.script import tgis_space_time_dataset as grass

strds = grass.space_time_raster_dataset("soils_1950_2010")

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import getpass
import raster
import vector
import raster3d
from tgis_abstract_datasets import *


###############################################################################

class raster_dataset(abstract_map_dataset):
    """Raster dataset class

       This class provides functions to select, update, insert or delete raster
       map informations and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_type(self):
        return "raster"

    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        return raster_dataset(ident)

    def get_new_stds_instance(self, ident):
        """Return a new space time dataset instance in which maps are stored with the type of this class"""
        return space_time_raster_dataset(ident)

    def get_stds_register(self):
        """Return the space time dataset register table name in which stds are listed in which this map is registered"""
        return self.metadata.get_strds_register()

    def set_stds_register(self, name):
        """Set the space time dataset register table name in which stds are listed in which this map is registered"""
        self.metadata.set_strds_register(name)

    def reset(self, ident):
	"""Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = raster_base(ident=ident)
	self.absolute_time = raster_absolute_time(ident=ident)
	self.relative_time = raster_relative_time(ident=ident)
	self.spatial_extent = raster_spatial_extent(ident=ident)
	self.metadata = raster_metadata(ident=ident)

    def load(self):
        """Load all info from an existing raster map into the internal structure"""

        # Get the data from an existing raster map
        kvp = raster.raster_info(self.ident)

        # Fill base information

        self.base.set_name(self.ident.split("@")[0])
        self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))

        # Fill spatial extent

        self.set_spatial_extent(north=kvp["north"], south=kvp["south"], \
                                east=kvp["east"],   west=kvp["west"])

        # Fill metadata

        self.metadata.set_nsres(kvp["nsres"])
        self.metadata.set_ewres(kvp["ewres"])
        self.metadata.set_datatype(kvp["datatype"])
        self.metadata.set_min(kvp["min"])
        self.metadata.set_max(kvp["max"])

        rows = int((kvp["north"] - kvp["south"])/kvp["nsres"] + 0.5)
        cols = int((kvp["east"] - kvp["west"])/kvp["ewres"] + 0.5)

        ncells = cols * rows

        self.metadata.set_cols(cols)
        self.metadata.set_rows(rows)
        self.metadata.set_number_of_cells(ncells)

###############################################################################

class raster3d_dataset(abstract_map_dataset):
    """Raster3d dataset class

       This class provides functions to select, update, insert or delete raster3d
       map informations and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_type(self):
        return "raster3d"

    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        return raster3d_dataset(ident)

    def get_new_stds_instance(self, ident):
        """Return a new space time dataset instance in which maps are stored with the type of this class"""
        return space_time_raster3d_dataset(ident)

    def get_stds_register(self):
        """Return the space time dataset register table name in which stds are listed in which this map is registered"""
        return self.metadata.get_str3ds_register()

    def set_stds_register(self, name):
        """Set the space time dataset register table name in which stds are listed in which this map is registered"""
        self.metadata.set_str3ds_register(name)

    def reset(self, ident):
	"""Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = raster3d_base(ident=ident)
	self.absolute_time = raster3d_absolute_time(ident=ident)
	self.relative_time = raster3d_relative_time(ident=ident)
	self.spatial_extent = raster3d_spatial_extent(ident=ident)
	self.metadata = raster3d_metadata(ident=ident)

    def load(self):
        """Load all info from an existing raster3d map into the internal structure"""

        # Get the data from an existing raster map
        kvp = raster3d.raster3d_info(self.ident)

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
    """Vector dataset class

       This class provides functions to select, update, insert or delete vector
       map informations and valid time stamps into the SQL temporal database.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_type(self):
        return "vector"

    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        return vector_dataset(ident)

    def get_new_stds_instance(self, ident):
        """Return a new space time dataset instance in which maps are stored with the type of this class"""
        return space_time_vector_dataset(ident)

    def get_stds_register(self):
        """Return the space time dataset register table name in which stds are listed in which this map is registered"""
        return self.metadata.get_stvds_register()

    def set_stds_register(self, name):
        """Set the space time dataset register table name in which stds are listed in which this map is registered"""
        self.metadata.set_stvds_register(name)

    def reset(self, ident):
	"""Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = vector_base(ident=ident)
	self.absolute_time = vector_absolute_time(ident=ident)
	self.relative_time = vector_relative_time(ident=ident)
	self.spatial_extent = vector_spatial_extent(ident=ident)
	self.metadata = vector_metadata(ident=ident)

    def load(self):
        """Load all info from an existing vector map into the internal structure"""

        # Get the data from an existing raster map
        kvp = vector.vector_info(self.ident)

        # Fill base information

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
    """Space time raster dataset class
    """
    def __init__(self, ident):
        abstract_space_time_dataset.__init__(self, ident)

    def get_type(self):
        return "strds"

    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        return space_time_raster_dataset(ident)

    def get_new_map_instance(self, ident):
        """Return a new instance of a map dataset which is associated with the type of this class"""
        return raster_dataset(ident)

    def get_map_register(self):
        """Return the name of the map register table"""
        return self.metadata.get_raster_register()

    def set_map_register(self, name):
        """Set the name of the map register table"""
        self.metadata.set_raster_register(name)

    def reset(self, ident):

	"""Reset the internal structure and set the identifier"""
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
    """Space time raster3d dataset class
    """

    def __init__(self, ident):
        abstract_space_time_dataset.__init__(self, ident)

    def get_type(self):
        return "str3ds"

    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        return space_time_raster3d_dataset(ident)

    def get_new_map_instance(self, ident):
        """Return a new instance of a map dataset which is associated with the type of this class"""
        return raster3d_dataset(ident)

    def get_map_register(self):
        """Return the name of the map register table"""
        return self.metadata.get_raster3d_register()

    def set_map_register(self, name):
        """Set the name of the map register table"""
        self.metadata.set_raster3d_register(name)

    def reset(self, ident):

	"""Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = str3ds_base(ident=ident)

        self.base.set_name(self.ident.split("@")[0])
        self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = str3ds_absolute_time(ident=ident)
        self.relative_time = str3ds_relative_time(ident=ident)
	self.spatial_extent = str3ds_spatial_extent(ident=ident)
	self.metadata = str3ds_metadata(ident=ident)

###############################################################################

class space_time_vector_dataset(abstract_space_time_dataset):
    """Space time vector dataset class
    """

    def __init__(self, ident):
        abstract_space_time_dataset.__init__(self, ident)

    def get_type(self):
        return "stvds"

    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        return space_time_vector_dataset(ident)

    def get_new_map_instance(self, ident):
        """Return a new instance of a map dataset which is associated with the type of this class"""
        return vector_dataset(ident)

    def get_map_register(self):
        """Return the name of the map register table"""
        return self.metadata.get_vector_register()

    def set_map_register(self, name):
        """Set the name of the map register table"""
        self.metadata.set_vector_register(name)

    def reset(self, ident):

	"""Reset the internal structure and set the identifier"""
	self.ident = ident

	self.base = stvds_base(ident=ident)

        self.base.set_name(self.ident.split("@")[0])
        self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = stvds_absolute_time(ident=ident)
        self.relative_time = stvds_relative_time(ident=ident)
	self.spatial_extent = stvds_spatial_extent(ident=ident)
	self.metadata = stvds_metadata(ident=ident)
        