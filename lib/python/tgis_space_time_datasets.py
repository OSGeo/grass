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

        if ident != None:
            self.base.set_name(self.ident.split("@")[0])
            self.base.set_mapset(self.ident.split("@")[1])
        self.base.set_creator(str(getpass.getuser()))
        self.absolute_time = stvds_absolute_time(ident=ident)
        self.relative_time = stvds_relative_time(ident=ident)
	self.spatial_extent = stvds_spatial_extent(ident=ident)
	self.metadata = stvds_metadata(ident=ident)

###############################################################################

def register_maps_in_space_time_dataset(type, name, maps, start=None, increment=None):
    """Use this method to register maps in space time datasets. This function is generic and
       can handle raster, vector and raster3d maps as well as there space time datasets.

       Additionally a start time string and an increment string can be specified
       to assign a time interval automatically to the maps.

       It takes care of the correct update of the space time datasets from all
       registered maps.

       @type The type of the maps raster, raster3d or vector
       @name The name of the space time dataset
       @maps A comma separated list of map names
       @start The start date and time of the first raster map, in case the map has no date (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative 5.0)
       @increment Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
    """

    # We may need the mapset
    mapset =  core.gisenv()["MAPSET"]

    # Check if the dataset name contains the mapset as well
    if name.find("@") < 0:
        id = name + "@" + mapset
    else:
        id = name

    if type == "raster":
        sp = space_time_raster_dataset(id)
    if type == "raster3d":
        sp = space_time_raster3d_dataset(id)
    if type == "vector":
        sp = space_time_vector_dataset(id)

    # Read content from temporal database
    sp.select()

    if sp.is_in_db() == False:
        core.fatal("Space time " + sp.get_new_map_instance(None).get_type() + " dataset <" + name + "> not found")

    if maps.find(",") == -1:
        maplist = (maps,)
    else:
        maplist = tuple(maps.split(","))

    count = 0
    for mapname in maplist:
        mapname = mapname.strip()
        # Check if the map name contains the mapset as well
        if mapname.find("@") < 0:
            mapid = mapname + "@" + mapset
        else:
            mapid = mapname
        # Get a new instance of the space time dataset map type
        map = sp.get_new_map_instance(mapid)

        # In case the map is already registered print a message and continue to the next map

        # Put the map into the database
        if map.is_in_db() == False:
            # Break in case no valid time is provided
            if start == "" or start == None:
                core.fatal("Unable to register " + map.get_type() + " map <" + map.get_id() + ">. The map has no valid time and the start time is not set.")
            # Load the data from the grass file database
            map.load()
            #  Put it into the temporal database
            map.insert()
        else:
            map.select()
            if map.get_temporal_type() != sp.get_temporal_type():
                core.fatal("Unable to register " + map.get_type() + " map <" + map.get_id() + ">. The temporal types are different.")

        # Set the valid time
        if start:
            assign_valid_time_to_map(sp.get_temporal_type(), map, start, increment, count)

        # Finally Register map in the space time dataset
        sp.register_map(map)
        count += 1

    # Update the space time tables
    sp.update_from_registered_maps()

###############################################################################

def unregister_maps_from_space_time_datasets(type, name, maps):
    """Unregister maps from a single space time dataset or, in case no dataset name is provided,
       unregister from all datasets within the maps are registered.

       @type The type of the maps raster, vector or raster3d
       @name Name of an existing space time raster dataset. If no name is provided the raster map(s) are unregistered from all space time datasets in which they are registered.
       @maps Name(s) of existing map(s) to unregister
    """
    mapset =  core.gisenv()["MAPSET"]

    # In case a space time dataset is specified
    if name:
        # Check if the dataset name contains the mapset as well
        if name.find("@") < 0:
            id = name + "@" + mapset
        else:
            id = name

        if type == "raster":
            sp = space_time_raster_dataset(id)
        if type == "raster3d":
            sp = space_time_raster3d_dataset(id)
        if type == "vector":
            sp = space_time_vector_dataset(id)

        if sp.is_in_db() == False:
            core.fatal("Space time " + sp.get_new_map_instance(None).get_type() + " dataset <" + name + "> not found")

    # Build the list of maps
    if maps.find(",") == -1:
        maplist = (maps,)
    else:
        maplist = tuple(maps.split(","))

    for mapname in maplist:
        mapname = mapname.strip()
        # Check if the map name contains the mapset as well
        if mapname.find("@") < 0:
            mapid = mapname + "@" + mapset
        else:
            mapid = mapname
            
        # Create a new instance with the map type
        if type == "raster":
            map = raster_dataset(mapid)
        if type == "raster3d":
            map = raster3d_dataset(mapid)
        if type == "vector":
            map = vector_dataset(mapid)

        # Unregister map if in database
        if map.is_in_db() == True:
            if name:
                sp.select()
                sp.unregister_map(map)
            else:
                map.select()
                map.unregister()

    if name:
        sp.update_from_registered_maps()

def assign_valid_time_to_map(ttype, map, start, increment=None, mult=1):
    """Assign the valid time to a map dataset

       @ttype The temporal type which should be assigned and which the time format is of
       @map A map dataset object derived from abstract_map_dataset
       @start The start date and time of the first raster map, in case the map has no date (format absolute: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd", format relative 5.0)
       @increment Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
       @multi A multiplier for the increment
    """
    if ttype == "absolute":
        # Create the start time object
        if start.find(":") > 0:
            time_format = "%Y-%m-%d %H:%M:%S"
        else:
            time_format = "%Y-%m-%d"

        start_time = datetime.strptime(start, time_format)
        end_time = None

        # Add the increment
        if increment:
            start_time = increment_datetime_by_string(start_time, increment, mult)
            end_time = increment_datetime_by_string(start_time, increment, 1)

        core.verbose("Set absolute valid time for map <" + map.get_id() + "> to " + str(start_time) + " - " + str(end_time))
        map.update_absolute_time(start_time, end_time)
    else:
        if increment:
            interval = float(start) + mult * float(increment)
        else:
            interval = float(start)
        core.verbose("Set relative valid time for map <" + map.get_id() + "> to " + str(interval))
        map.update_relative_time(interval)
