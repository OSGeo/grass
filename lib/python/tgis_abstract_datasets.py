"""!@package grass.script.tgis_map_datasets

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
from grass.script import tgis_map_datasets as grass

raster_ds = grass.raster_database("soil")
if raster_ds.is_in_db():
    raster_ds.select()
else:
    raster_ds.load()

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import uuid
from tgis_temporal_extent import *
from tgis_spatial_extent import *
from tgis_metadata import *

class abstract_dataset(object):
    """This is the base class for all datasets (raster, vector, raster3d, strds, stvds, str3ds)"""
    
    def get_type(self):
        """Return the type of this class"""
        raise IOError("This method must be implemented in the subclasses")
    
    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        raise IOError("This method must be implemented in the subclasses")

    def get_id(self):
        return self.base.get_id()

    def get_absolute_time(self):
        """Returns a tuple of the start, the end valid time and the timezone of the map
           @return A tuple of (start_time, end_time, timezone)
        """
               
        start = self.absolute_time.get_start_time()
        end = self.absolute_time.get_end_time()
        tz = self.absolute_time.get_timezone()
        
        return (start, end, tz)
    
    def get_relative_time(self):
        """Returns the relative time interval or None if not present"""
        return self.relative_time.get_interval()
    
    
    def get_spatial_extent(self):
        """Return a tuple of spatial extent (north, south, east, west, top, bottom) """
        
        north = self.spatial_extent.get_north()
        south = self.spatial_extent.get_south()
        east = self.spatial_extent.get_east()
        west = self.spatial_extent.get_west()
        top = self.spatial_extent.get_top()
        bottom = self.spatial_extent.get_bottom()
        
        return (north, south, east, west, top, bottom)
        
    def select(self):
	"""Select temporal dataset entry from database and fill up the internal structure"""
	self.base.select()
	if self.is_time_absolute():
	    self.absolute_time.select()
        if self.is_time_relative():
	    self.relative_time.select()
	self.spatial_extent.select()
	self.metadata.select()
        
    def is_in_db(self):
	"""Check if the temporal dataset entry is in the database"""
	return self.base.is_in_db()

    def delete(self):
	"""Delete temporal dataset entry from database if it exists"""
        raise IOError("This method must be implemented in the subclasses")

    def insert(self):
	"""Insert temporal dataset entry into database from the internal structure"""
	self.base.insert()
	if self.is_time_absolute():
	    self.absolute_time.insert()
        if self.is_time_relative():
	    self.relative_time.insert()
	self.spatial_extent.insert()
	self.metadata.insert()

    def update(self):
	"""Update temporal dataset entry of database from the internal structure"""
	self.base.update()
	if self.is_time_absolute():
	    self.absolute_time.update()
        if self.is_time_relative():
	    self.relative_time.update()
	self.spatial_extent.update()
	self.metadata.update()

    def print_self(self):
	"""Print the content of the internal structure to stdout"""
	self.base.print_self()
	if self.is_time_absolute():
	    self.absolute_time.print_self()
        if self.is_time_relative():
	    self.relative_time.print_self()
	self.spatial_extent.print_self()
	self.metadata.print_self()

    def print_info(self):
        """Print information about this class in human readable style"""
	self.base.print_info()
	if self.is_time_absolute():
	    self.absolute_time.print_info()
        if self.is_time_relative():
	    self.relative_time.print_info()
	self.spatial_extent.print_info()
	self.metadata.print_info()

    def print_shell_info(self):
        """Print information about this class in shell style"""
	self.base.print_shell_info()
	if self.is_time_absolute():
	    self.absolute_time.print_shell_info()
        if self.is_time_relative():
	    self.relative_time.print_shell_info()
	self.spatial_extent.print_shell_info()
	self.metadata.print_shell_info()

    def set_time_to_absolute(self):
	self.base.set_ttype("absolute")

    def set_time_to_relative(self):
        self.base.set_ttype("relative")

    def is_time_absolute(self):
	if self.base.D.has_key("temporal_type"):
	    return self.base.get_ttype() == "absolute"
        else:
	    return None

    def is_time_relative(self):
	if self.base.D.has_key("temporal_type"):
	    return self.base.get_ttype() == "relative"
        else:
	    return None

    def temporal_relation(self, map):
	"""Return the temporal relation of this and the provided temporal raster map"""
	if self.is_time_absolute() and map.is_time_absolute():
	    return self.absolute_time.temporal_relation(map.absolute_time)
        if self.is_time_relative() and map.is_time_relative():
	    return self.relative_time.temporal_relation(map.absolute_time)
    	return None

###############################################################################

class abstract_map_dataset(abstract_dataset):
    """This is the base class for all maps (raster, vector, raster3d) 
       providing additional function to set the valid time and the spatial extent.
       
       Valid time and spatial extent will be set automatically in the space-time datasets
    """
      
    def get_new_stds_instance(self, ident):
        """Return a new space time dataset instance in which maps are stored with the type of this class"""
        raise IOError("This method must be implemented in the subclasses")
    
    def get_stds_register(self):
        """Return the space time dataset register table name in which stds are listed in which this map is registered"""
        raise IOError("This method must be implemented in the subclasses")
        
    def set_stds_register(self, name):
        """Set the space time dataset register table name in which stds are listed in which this map is registered"""
        raise IOError("This method must be implemented in the subclasses")
        
    def set_absolute_time(self, start_time, end_time=None, timezone=None):
        """Set the absolute time interval with start time and end time
        
           @start_time a datetime object specifying the start time of the map
           @end_time a datetime object specifying the end time of the map
           @timezone Thee timezone of the map
        
        """
        self.base.set_ttype("absolute")
        
        self.absolute_time.set_start_time(start_time)
        self.absolute_time.set_end_time(end_time)
        self.absolute_time.set_timezone(timezone)
    
    def set_relative_time(self, interval):
        """Set the relative time interval 
        
           @interval A double value in days
        
        """
        self.base.set_ttype("relative")
        
        self.absolute_time.set_interval(interval)
        
    def set_spatial_extent(self, north, south, east, west, top=0, bottom=0):
        """Set the spatial extent of the map"""
        self.spatial_extent.set_spatial_extent(north, south, east, west, top, bottom)
        
    def delete(self):
	"""Delete a map entry from database if it exists
        
            Remove dependent entries:
            * Remove the map entry in each space time dataset in which this map is registered
            * Remove the space time dataset register table
        """
        if self.is_in_db():
            # Get all data
            self.select()
            # Remove the map from all registered space time datasets
            if self.get_stds_register() != None:
                # Select all stds tables in which this map is registered
                sql = "SELECT id FROM " + self.get_stds_register()
                #print sql
                self.base.connect()
                self.base.cursor.execute(sql)
                rows = self.base.cursor.fetchall()
                self.base.close()
        
                # For each stds in which the map is registered
                if rows:
                    for row in rows:
                        # Create a space time dataset object to remove the map
                        # from its register
                        strds = self.get_new_stds_instance(row["id"])
                        strds.select()
                        strds.unregister_map(self)
                
                # Remove the strds register table
                sql = "DROP TABLE " + self.get_stds_register()
                #print sql
                self.base.connect()
                self.base.cursor.execute(sql)
                self.base.close()
            
            # Delete yourself from the database, trigger functions will take care of dependencies
            self.base.delete()

###############################################################################

class abstract_space_time_dataset(abstract_dataset):
    """Abstract space time dataset class

       This class represents a space time dataset. Convenient functions
       to select, update, insert or delete objects of this type int the SQL
       temporal database exists as well as functions to register or unregister
       raster maps.

       Parts of the temporal logic are implemented in the SQL temporal database,
       like the computation of the temporal and spatial extent as well as the
       collecting of metadata.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        raise IOError("This method must be implemented in the subclasses")

    def get_new_map_instance(self, ident):
        """Return a new instance of a map dataset which is associated with the type of this class"""
        raise IOError("This method must be implemented in the subclasses")

    def get_map_register(self):
        """Return the name of the map register table"""
        raise IOError("This method must be implemented in the subclasses")

    def set_map_register(self, name):
        """Set the name of the map register table"""
        raise IOError("This method must be implemented in the subclasses")

    def reset(self, ident):
	"""Reset the internal structure and set the identifier"""
	raise IOError("This method must be implemented in the subclasses")

    def set_initial_values(self, granularity, temporal_type, semantic_type, \
                           title=None, description=None):

        if temporal_type == "absolute":
            self.set_time_to_absolute()
            self.absolute_time.set_granularity(granularity)
        elif temporal_type == "relative":
            self.set_time_to_relative()
            self.relative_time.set_granularity(granularity)
        else:
            core.fatal("Unkown temporal type \"" + temporal_type + "\"")

        self.base.set_semantic_type(semantic_type)
        self.metadata.set_title(title)
        self.metadata.set_description(description)

    def delete(self):
        """Delete a space time dataset from the database"""
        # First we need to check if maps are registered in this dataset and
        # unregister them

        core.info("Delete space time " + self.get_new_map_instance(ident=None).get_type() + " dataset: " + self.get_id())

        if self.get_map_register():
            sql = "SELECT id FROM " + self.get_map_register()
            self.base.connect()
            self.base.cursor.execute(sql)
            rows = self.base.cursor.fetchall()
            self.base.close()
            # Unregister each registered map in the table
            if rows:
                for row in rows:
                    # Unregister map
                    map = self.get_new_map_instance(row["id"])
                    self.unregister_map(map)

            # Drop remove the map register table
            sql = "DROP TABLE " + self.get_map_register()
            self.base.connect()
            self.base.cursor.execute(sql)
            self.base.close()

        # Remove the primary key, the foreign keys will be removed by trigger
        self.base.delete()

    def register_map(self, map):
        """Register a map in the space time dataset.

            This method takes care of the registration of a map
            in a space time dataset.

            Break with a warning in case the map is already registered.
        """

        if map.is_in_db() == False:
            core.fatal("Only maps with absolute or relative valid time can be registered")

        core.info("Register " + map.get_type() + " map: " + map.get_id())

        # First select all data from the database
        map.select()
        map_id = map.base.get_id()
        map_name = map.base.get_name()
        map_mapset = map.base.get_mapset()
        map_register_table = map.get_stds_register()

        #print "Map register table", map_register_table

        # Get basic info
        stds_name = self.base.get_name()
        stds_mapset = self.base.get_mapset()
        stds_register_table = self.get_map_register()

        #print "STDS register table", stds_register_table

        if stds_mapset != map_mapset:
            core.fatal("You can only register maps from the same mapset")

        # Check if map is already registred
        if stds_register_table:
            sql = "SELECT id FROM " + stds_register_table + " WHERE id = (?)"
            self.base.connect()
            self.base.cursor.execute(sql, (map_id,))
            row = self.base.cursor.fetchone()
            self.base.close()
            # In case of no entry make a new one
            if row and row[0] == map_id:
                core.warning("Map " + map_id + "is already registered")
                return False

        # Create tables
        sql_path = get_sql_template_path()

        # We need to create the stmap raster register table bevor we can register the map
        if map_register_table == None:
            # Create a unique id
            uuid_rand = "map_" + str(uuid.uuid4()).replace("-", "")

            # Read the SQL template
            sql = open(os.path.join(sql_path, "map_stds_register_table_template.sql"), 'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("MAP_NAME", map_name + "_" + map_mapset )
            sql = sql.replace("TABLE_NAME", uuid_rand )
            sql = sql.replace("MAP_ID", map_id)
            sql = sql.replace("STDS", self.get_type())

            self.base.connect()
            self.base.cursor.executescript(sql)
            self.base.close()

            map_register_table = uuid_rand + "_" + self.get_type() + "_register"
            # Set the stds register table name and put it into the DB
            map.set_stds_register(map_register_table)
            map.metadata.update()

            #print "Created map register table ",  map_register_table

        # We need to create the table and register it
        if stds_register_table == None:
            # Read the SQL template
            sql = open(os.path.join(sql_path, "stds_map_register_table_template.sql"), 'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
            sql = sql.replace("SPACETIME_ID", self.base.get_id())
            sql = sql.replace("STDS", self.get_type())

            self.base.connect()
            self.base.cursor.executescript(sql)
            self.base.close()

            # We need raster specific trigger
            sql = open(os.path.join(sql_path, "stds_" + map.get_type() + "_register_trigger_template.sql"), 'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
            sql = sql.replace("SPACETIME_ID", self.base.get_id())
            sql = sql.replace("STDS", self.get_type())

            self.base.connect()
            self.base.cursor.executescript(sql)
            self.base.close()

            stds_register_table = stds_name + "_" + stds_mapset + "_" + map.get_type() + "_register"

            # Set the map register table name and put it into the DB
            self.set_map_register(stds_register_table)
            self.metadata.update()

            #print "Created stds register table ",  stds_register_table

        # Register the stds in the map stds register table
        # Check if the entry is already there
        sql = "SELECT id FROM " + map_register_table + " WHERE id = ?"
        self.base.connect()
        self.base.cursor.execute(sql, (self.base.get_id(),))
      	row = self.base.cursor.fetchone()
	self.base.close()

        # In case of no entry make a new one
        if row == None:
            sql = "INSERT INTO " + map_register_table + " (id) " + "VALUES (?)"
            #print sql
            self.base.connect()
            self.base.cursor.execute(sql, (self.base.get_id(),))
            self.base.close()

        # Now put the raster name in the stds map register table
        sql = "INSERT INTO " + stds_register_table + " (id) " + "VALUES (?)"
        #print sql
        self.base.connect()
        self.base.cursor.execute(sql, (map_id,))
        self.base.close()

        return True

    def unregister_map(self, map):
        """Remove a register a map from the space time dataset.

            This method takes care of the unregistration of a map
            in a space time dataset.
        """

        if map.is_in_db() == False:
            core.fatal("Only maps with absolute or relative valid time can be registered")

        core.info("Unegister " + map.get_type() + " map: " + map.get_id())

        # First select all data from the database
        map.select()
        map_id = map.base.get_id()
        map_register_table = map.get_stds_register()

        # Get basic info
        stds_register_table = self.get_map_register()

        # Check if the map is registered in the space time raster dataset
        sql = "SELECT id FROM " + map_register_table + " WHERE id = ?"
        self.base.connect()
        self.base.cursor.execute(sql, (self.base.get_id(),))
      	row = self.base.cursor.fetchone()
	self.base.close()

        # Break if the map is not registered
        if row == None:
            core.fatal("Map " + map_id + " is not registered in space time dataset " + self.base.get_id())

        # Remove the space time raster dataset from the raster dataset register
        if map_register_table != None:
            sql = "DELETE FROM " + map_register_table + " WHERE id = ?"
            self.base.connect()
            self.base.cursor.execute(sql, (self.base.get_id(),))
            self.base.close()

        # Remove the raster map from the space time raster dataset register
        if stds_register_table != None:
            sql = "DELETE FROM " + stds_register_table + " WHERE id = ?"
            self.base.connect()
            self.base.cursor.execute(sql, (map_id,))
            self.base.close()