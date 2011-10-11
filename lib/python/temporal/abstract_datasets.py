"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

@code
import grass.temporal as tgis

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import uuid
import copy
from temporal_extent import *
from spatial_extent import *
from metadata import *

class abstract_dataset(object):
    """This is the base class for all datasets (raster, vector, raster3d, strds, stvds, str3ds)"""

    def reset(self, ident):
	"""Reset the internal structure and set the identifier

           @param ident: The identifier of the dataset
        """
	raise IOError("This method must be implemented in the subclasses")

    def get_type(self):
        """Return the type of this class"""
        raise IOError("This method must be implemented in the subclasses")
    
    def get_new_instance(self, ident):
        """Return a new instance with the type of this class

           @param ident: The identifier of the dataset
        """
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
        """Returns the relative time interval (start_time, end_time) or None if not present"""

        start = self.relative_time.get_start_time()
        end = self.relative_time.get_end_time()

        return (start, end)

    def get_temporal_type(self):
        """Return the temporal type of this dataset"""
        return self.base.get_ttype()
    
    def get_spatial_extent(self):
        """Return a tuple of spatial extent (north, south, east, west, top, bottom) """
        
        north = self.spatial_extent.get_north()
        south = self.spatial_extent.get_south()
        east = self.spatial_extent.get_east()
        west = self.spatial_extent.get_west()
        top = self.spatial_extent.get_top()
        bottom = self.spatial_extent.get_bottom()
        
        return (north, south, east, west, top, bottom)
        
    def select(self, dbif=None):
	"""Select temporal dataset entry from database and fill up the internal structure"""
	self.base.select(dbif)
	if self.is_time_absolute():
	    self.absolute_time.select(dbif)
        if self.is_time_relative():
	    self.relative_time.select(dbif)
	self.spatial_extent.select(dbif)
	self.metadata.select(dbif)
        
    def is_in_db(self, dbif=None):
	"""Check if the temporal dataset entry is in the database"""
	return self.base.is_in_db(dbif)

    def delete(self):
	"""Delete temporal dataset entry from database if it exists"""
        raise IOError("This method must be implemented in the subclasses")

    def insert(self, dbif=None):
	"""Insert temporal dataset entry into database from the internal structure"""
	self.base.insert(dbif)
	if self.is_time_absolute():
	    self.absolute_time.insert(dbif)
        if self.is_time_relative():
	    self.relative_time.insert(dbif)
	self.spatial_extent.insert(dbif)
	self.metadata.insert(dbif)

    def update(self, dbif=None):
	"""Update temporal dataset entry of database from the internal structure
	   excluding None variables
	"""
	self.base.update(dbif)
	if self.is_time_absolute():
	    self.absolute_time.update(dbif)
        if self.is_time_relative():
	    self.relative_time.update(dbif)
	self.spatial_extent.update(dbif)
	self.metadata.update(dbif)

    def update_all(self, dbif=None):
	"""Update temporal dataset entry of database from the internal structure
	   and include None varuables.

           @param dbif: The database interface to be used
	"""
	self.base.update_all(dbif)
	if self.is_time_absolute():
	    self.absolute_time.update_all(dbif)
        if self.is_time_relative():
	    self.relative_time.update_all(dbif)
	self.spatial_extent.update_all(dbif)
	self.metadata.update_all(dbif)

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
        
        if self.get_type() == "raster":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Raster Dataset ----------------------------------------+"
        if self.get_type() == "raster3d":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Raster3d Dataset --------------------------------------+"
        if self.get_type() == "vector":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Vector Dataset ----------------------------------------+"
        if self.get_type() == "strds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Space Time Raster Dataset -----------------------------+"
        if self.get_type() == "str3ds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Space Time Raster3d Dataset ---------------------------+"
        if self.get_type() == "stvds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Space Time Vector Dataset -----------------------------+"
        print " |                                                                            |"
	self.base.print_info()
	if self.is_time_absolute():
	    self.absolute_time.print_info()
        if self.is_time_relative():
	    self.relative_time.print_info()
	self.spatial_extent.print_info()
	self.metadata.print_info()
        print " +----------------------------------------------------------------------------+"

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
	"""Return the temporal relation of this and the provided temporal map"""
	if self.is_time_absolute() and map.is_time_absolute():
	    return self.absolute_time.temporal_relation(map.absolute_time)
        if self.is_time_relative() and map.is_time_relative():
	    return self.relative_time.temporal_relation(map.relative_time)
    	return None

###############################################################################

class abstract_map_dataset(abstract_dataset):
    """This is the base class for all maps (raster, vector, raster3d) 
       providing additional function to set the valid time and the spatial extent.
    """
      
    def get_new_stds_instance(self, ident):
        """Return a new space time dataset instance in which maps are stored with the type of this class

           @param ident: The identifier of the dataset
        """
        raise IOError("This method must be implemented in the subclasses")
    
    def get_stds_register(self):
        """Return the space time dataset register table name in which stds are listed in which this map is registered"""
        raise IOError("This method must be implemented in the subclasses")
        
    def set_stds_register(self, name):
        """Set the space time dataset register table name.
        
           This table stores all space time datasets in which this map is registered.

           @param ident: The name of the register table
        """
        raise IOError("This method must be implemented in the subclasses")
        
    def set_absolute_time(self, start_time, end_time=None, timezone=None):
        """Set the absolute time interval with start time and end time
        
           @param start_time: a datetime object specifying the start time of the map
           @param end_time: a datetime object specifying the end time of the map
           @param timezone: Thee timezone of the map
        
        """
        if start_time != None and not isinstance(start_time, datetime) :
            core.fatal(_("Start time must be of type datetime"))

        if end_time != None and not isinstance(end_time, datetime) :
            core.fatal(_("End time must be of type datetime"))

        if start_time != None and end_time != None:
            if start_time >= end_time:
                core.error(_("End time must be later than start time"))
                return False

        self.base.set_ttype("absolute")
        
        self.absolute_time.set_start_time(start_time)
        self.absolute_time.set_end_time(end_time)
        self.absolute_time.set_timezone(timezone)

        return True

    def update_absolute_time(self, start_time, end_time=None, timezone=None, dbif = None):
        """Update the absolute time

           @param start_time: a datetime object specifying the start time of the map
           @param end_time: a datetime object specifying the end time of the map
           @param timezone: Thee timezone of the map
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        self.set_absolute_time(start_time, end_time, timezone)
        self.absolute_time.update_all(dbif)
        self.base.update(dbif)

        if connect == True:
            dbif.close()

    def set_relative_time(self, start_time, end_time=None):
        """Set the relative time interval 
        
           @param start_time: A double value in days
           @param end_time: A double value in days

        """
        if start_time != None and end_time != None:
            if abs(float(start_time)) >= abs(float(end_time)):
                core.error(_("End time must be greater than start time"))
                return False

        self.base.set_ttype("relative")
        
        self.relative_time.set_start_time(float(start_time))
        if end_time != None:
            self.relative_time.set_end_time(float(end_time))
        else:
            self.relative_time.set_end_time(None)

        return True

    def update_relative_time(self, start_time, end_time=None, dbif = None):
        """Update the relative time interval

           @param start_time: A double value in days
           @param end_time: A double value in days
           @param dbif: The database interface to be used
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        self.set_relative_time(start_time, end_time)
        self.relative_time.update_all(dbif)
        self.base.update(dbif)
        dbif.connection.commit()

        if connect == True:
            dbif.close()

    def set_spatial_extent(self, north, south, east, west, top=0, bottom=0):
        """Set the spatial extent of the map

           @param north: The northern edge
           @param south: The southern edge
           @param east: The eastern edge
           @param west: The western edge
           @param top: The top edge
           @param bottom: The bottom ege
        """
        self.spatial_extent.set_spatial_extent(north, south, east, west, top, bottom)
        
    def delete(self, dbif=None):
	"""Delete a map entry from database if it exists
        
            Remove dependent entries:
            * Remove the map entry in each space time dataset in which this map is registered
            * Remove the space time dataset register table
            
           @param dbif: The database interface to be used
        """

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        if self.is_in_db(dbif):
 
            # SELECT all needed informations from the database
            self.select(dbif)
           
            # First we unregister from all dependent space time datasets
            self.unregister(dbif)

            # Remove the strds register table
            if self.get_stds_register():
                sql = "DROP TABLE " + self.get_stds_register()
                #print sql
                try:
                    dbif.cursor.execute(sql)
                except:
                    core.error(_("Unable to remove space time dataset register table <%s>") % (self.get_stds_register()))

            core.verbose(_("Delete %s dataset <%s> from temporal database") % (self.get_type(), self.get_id()))

            # Delete yourself from the database, trigger functions will take care of dependencies
            self.base.delete(dbif)

        self.reset(None)
        dbif.connection.commit()

        if connect == True:
            dbif.close()

    def unregister(self, dbif=None):
	""" Remove the map entry in each space time dataset in which this map is registered

           @param dbif: The database interface to be used
        """

        core.verbose(_("Unregister %s dataset <%s> from space time datasets") % (self.get_type(), self.get_id()))
        
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        # Get all datasets in which this map is registered
        rows = self.get_registered_datasets(dbif)

        # For each stds in which the map is registered
        if rows:
            for row in rows:
                # Create a space time dataset object to remove the map
                # from its register
                stds = self.get_new_stds_instance(row["id"])
                stds.select(dbif)
                stds.unregister_map(self, dbif)
                # Take care to update the space time dataset after
                # the map has been unregistred
                stds.update_from_registered_maps(dbif)

        dbif.connection.commit()

        if connect == True:
            dbif.close()
            
    def get_registered_datasets(self, dbif=None):
        """Return all space time dataset ids in which this map is registered as
           dictionary like rows with column "id" or None if this map is not registered in any
           space time dataset.

           @param dbif: The database interface to be used
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        rows = None

        try:
            if self.get_stds_register() != None:
                # Select all stds tables in which this map is registered
                sql = "SELECT id FROM " + self.get_stds_register()
                dbif.cursor.execute(sql)
                rows = dbif.cursor.fetchall()
        except:
            core.error(_("Unable to select space time dataset register table <%s>") % (self.get_stds_register()))

        if connect == True:
            dbif.close()
            
        return rows

###############################################################################

class abstract_space_time_dataset(abstract_dataset):
    """Abstract space time dataset class

       This class represents a space time dataset. Convenient functions
       to select, update, insert or delete objects of this type in the SQL
       temporal database exists as well as functions to register or unregister
       raster maps.

       Parts of the temporal logic are implemented in the SQL temporal database,
       like the computation of the temporal and spatial extent as well as the
       collecting of metadata.
    """
    def __init__(self, ident):
	self.reset(ident)

    def get_new_instance(self, ident=None):
        """Return a new instance with the type of this class

           @param ident: The unique identifier of the new object
        """
        raise IOError("This method must be implemented in the subclasses")

    def get_new_map_instance(self, ident=None):
        """Return a new instance of a map dataset which is associated with the type of this class

           @param ident: The unique identifier of the new object
        """
        raise IOError("This method must be implemented in the subclasses")

    def get_map_register(self):
        """Return the name of the map register table"""
        raise IOError("This method must be implemented in the subclasses")

    def set_map_register(self, name):
        """Set the name of the map register table

        This table stores all map names which are registered in this space time dataset.

           @param name: The name of the register table
        """
        raise IOError("This method must be implemented in the subclasses")

    def set_initial_values(self, granularity, temporal_type, semantic_type, \
                           title=None, description=None):
        """Set the initial values of the space time dataset

           @param granularity: The temporal granularity of this dataset. This value
                               should be computed by the space time dataset itself,
                               based on the granularity of the registered maps
           @param temporal_type: The temporal type of this space time dataset (absolute or relative)
           @param semantic_type: The semantic type of this dataset
           @param title: The title
           @param description: The description of this dataset
        """

        if temporal_type == "absolute":
            self.set_time_to_absolute()
            self.absolute_time.set_granularity(granularity)
        elif temporal_type == "relative":
            self.set_time_to_relative()
            self.relative_time.set_granularity(granularity)
        else:
            core.fatal(_("Unknown temporal type \"%s\"") % (temporal_type))

        self.base.set_semantic_type(semantic_type)
        self.metadata.set_title(title)
        self.metadata.set_description(description)

    def get_initial_values(self):
        """Return the initial values: granularity, temporal_type, semantic_type, title, description"""
        
        temporal_type = self.get_temporal_type()

        if temporal_type == "absolute":
            granularity   = self.absolute_time.get_granularity()
        elif temporal_type == "relative":
            granularity = self.relative_time.get_granularity()

        semantic_type = self.base.get_semantic_type()
        title = self.metadata.get_title()
        description = self.metadata.get_description()

        return granularity, temporal_type, semantic_type, title, description

    def get_temporal_relation_matrix(self, dbif=None):
        """Return the temporal relation matrix of all registered maps as listof lists

           The temproal relation matrix includes the temporal relations between
           all registered maps. The relations are strings stored in a list of lists.
           
           @param dbif: The database interface to be used
        """

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        matrix = []

        maps = self.get_registered_maps_as_objects(where=None, order="start_time", dbif=dbif)

        # Create the temporal relation matrix
        # Add the map names first
        row = []
        for map in maps:
            row.append(map.get_id())
        matrix.append(row)

        for mapA in maps:
            row = []
            for mapB in maps:
                row.append(mapA.temporal_relation(mapB))
            matrix.append(row)

        if connect == True:
            dbif.close()

        return matrix

    def get_registered_maps_as_objects(self, where = None, order = None, dbif=None):
        """Return all registered maps as ordered object list

           @param where: The SQL where statement to select a subset of the registered maps without "WHERE"
           @param order: The SQL order statement to be used to order the objects in the list without "ORDER BY"
           @param dbif: The database interface to be used

           In case nothing found None is returned
        """

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        obj_list = []
        
        rows = self.get_registered_maps("id", where, order, dbif)

        if rows:
            for row in rows:
                map = self.get_new_map_instance(row["id"])
                map.select(dbif)
                obj_list.append(copy.copy(map))

        if connect == True:
            dbif.close()

        return obj_list

    def get_registered_maps(self, columns=None, where = None, order = None, dbif=None):
        """Return sqlite rows of all registered maps.
        
           Each row includes all columns specified in the datatype specific view

           @param columns: Columns to be selected as SQL compliant string
           @param where: The SQL where statement to select a subset of the registered maps without "WHERE"
           @param order: The SQL order statement to be used to order the objects in the list without "ORDER BY"
           @param dbif: The database interface to be used

           In case nothing found None is returned
        """

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        rows = None

        if self.get_map_register():
            # Use the correct temporal table
            if self.get_temporal_type() == "absolute":
                map_view = self.get_new_map_instance(None).get_type() + "_view_abs_time"
            else:
                map_view = self.get_new_map_instance(None).get_type() + "_view_rel_time"

            if columns:
                sql = "SELECT %s FROM %s  WHERE %s.id IN (SELECT id FROM %s)" % (columns, map_view, map_view, self.get_map_register())
            else:
                sql = "SELECT * FROM %s  WHERE %s.id IN (SELECT id FROM %s)" % (map_view, map_view, self.get_map_register())

            if where:
                sql += " AND %s" % (where)
            if order:
                sql += " ORDER BY %s" % (order)

            try:
                dbif.cursor.execute(sql)
                rows = dbif.cursor.fetchall()
            except:
                if connect == True:
                    dbif.close()
                core.error(_("Unable to get map ids from register table <%s>") % (self.get_map_register()))
                raise

        if connect == True:
            dbif.close()

        return rows

    def delete(self, dbif=None):
        """Delete a space time dataset from the temporal database

           This method removes the space time dataset from the temporal database and drops its map register table

           @param dbif: The database interface to be used
        """
        # First we need to check if maps are registered in this dataset and
        # unregister them

        core.verbose(_("Delete space time %s  dataset <%s> from temporal database") % (self.get_new_map_instance(ident=None).get_type(), self.get_id()))

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        # SELECT all needed informations from the database
        self.select(dbif)

        core.verbose(_("Drop map register table: %s") %  (self.get_map_register()))
        if self.get_map_register():
            rows = self.get_registered_maps("id", None, None, dbif)
            # Unregister each registered map in the table
            if rows:
                for row in rows:
                    # Unregister map
                    map = self.get_new_map_instance(row["id"])
                    self.unregister_map(map, dbif)
            try:
                # Drop the map register table
                sql = "DROP TABLE " + self.get_map_register()
                dbif.cursor.execute(sql)
                dbif.connection.commit()
            except:
                if connect == True:
                    dbif.close()
                core.error(_("Unable to drop table <%s>") % (self.get_map_register()))
                raise

        # Remove the primary key, the foreign keys will be removed by trigger
        self.base.delete(dbif)
        self.reset(None)

        if connect == True:
            dbif.close()
            
    def register_map(self, map, dbif=None):
        """ Register a map in the space time dataset.

            This method takes care of the registration of a map
            in a space time dataset.

            In case the map is already registered this function will break with a warning
            and return False

           @param dbif: The database interface to be used
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        if map.is_in_db(dbif) == False:
            dbif.close()
            core.fatal(_("Only maps with absolute or relative valid time can be registered"))

        core.verbose(_("Register %s map <%s> in space time %s dataset <%s>") %  (map.get_type(), map.get_id(), map.get_type(), self.get_id()))

        # First select all data from the database
        map.select(dbif)
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
            dbif.close()
            core.fatal(_("Only maps from the same mapset can be registered"))

        # Check if map is already registred
        if stds_register_table:
	    if dbmi.paramstyle == "qmark":
		sql = "SELECT id FROM " + stds_register_table + " WHERE id = (?)"
	    else:
		sql = "SELECT id FROM " + stds_register_table + " WHERE id = (%s)"
            dbif.cursor.execute(sql, (map_id,))
            row = dbif.cursor.fetchone()
            # In case of no entry make a new one
            if row and row[0] == map_id:
                if connect == True:
                    dbif.close()
                core.warning(_("Map <%s> is already registered.") % (map_id))
                return False

        # Create tables
        sql_path = get_sql_template_path()

        # We need to create the stmap raster register table bevor we can register the map
        if map_register_table == None:
            # Create a unique id
            uuid_rand = "map_" + str(uuid.uuid4()).replace("-", "")

            map_register_table = uuid_rand + "_" + self.get_type() + "_register"
            
            # Read the SQL template
            sql = open(os.path.join(sql_path, "map_stds_register_table_template.sql"), 'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("MAP_NAME", map_name + "_" + map_mapset )
            sql = sql.replace("TABLE_NAME", uuid_rand )
            sql = sql.replace("MAP_ID", map_id)
            sql = sql.replace("STDS", self.get_type())
            try:
		if dbmi.__name__ == "sqlite3":
		    dbif.cursor.executescript(sql)
		else:
		    dbif.cursor.execute(sql)
            except:
                if connect == True:
                    dbif.close()
                core.error(_("Unable to create the space time %s dataset register table for <%s>") % \
                            (map.get_type(), map.get_id()))
                raise

            # Set the stds register table name and put it into the DB
            map.set_stds_register(map_register_table)
            map.metadata.update(dbif)
            
            core.verbose(_("Created register table <%s> for %s map <%s>") % \
                          (map_register_table, map.get_type(), map.get_id()))

        # We need to create the table and register it
        if stds_register_table == None:
            # Create table name
            stds_register_table = stds_name + "_" + stds_mapset + "_" + map.get_type() + "_register"
            # Read the SQL template
            sql = open(os.path.join(sql_path, "stds_map_register_table_template.sql"), 'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
            sql = sql.replace("SPACETIME_ID", self.base.get_id())
            sql = sql.replace("STDS", self.get_type())

            sql_script = ""
            sql_script += "BEGIN TRANSACTION;\n"
            sql_script += sql
            sql_script += "\n"
            sql_script += "END TRANSACTION;"
            try:
		if dbmi.__name__ == "sqlite3":
		    dbif.cursor.executescript(sql_script)
		else:
		    dbif.cursor.execute(sql_script)
                dbif.connection.commit()
            except:
                if connect == True:
                    dbif.close()
                core.error(_("Unable to create the space time %s dataset register table for <%s>") % \
                            (map.get_type(), map.get_id()))
                raise

            # Set the map register table name and put it into the DB
            self.set_map_register(stds_register_table)
            self.metadata.update(dbif)

            core.verbose(_("Created register table <%s> for space time %s  dataset <%s>") % \
                          (stds_register_table, map.get_type(), self.get_id()))

        # Register the stds in the map stds register table
        # Check if the entry is already there
	if dbmi.paramstyle == "qmark":
	    sql = "SELECT id FROM " + map_register_table + " WHERE id = ?"
	else:
	    sql = "SELECT id FROM " + map_register_table + " WHERE id = %s"
        dbif.cursor.execute(sql, (self.base.get_id(),))
      	row = dbif.cursor.fetchone()

        # In case of no entry make a new one
        if row == None:
	    if dbmi.paramstyle == "qmark":
		sql = "INSERT INTO " + map_register_table + " (id) " + "VALUES (?)"
	    else:
		sql = "INSERT INTO " + map_register_table + " (id) " + "VALUES (%s)"
            #print sql
            dbif.cursor.execute(sql, (self.base.get_id(),))

        # Now put the raster name in the stds map register table
	if dbmi.paramstyle == "qmark":
	    sql = "INSERT INTO " + stds_register_table + " (id) " + "VALUES (?)"
	else:
	    sql = "INSERT INTO " + stds_register_table + " (id) " + "VALUES (%s)"
        #print sql
        dbif.cursor.execute(sql, (map_id,))

        if connect == True:
            dbif.close()
            
        return True

    def unregister_map(self, map, dbif = None):
        """Unregister a map from the space time dataset.

           This method takes care of the unregistration of a map
           from a space time dataset.

           @param map: The map object to unregister
           @param dbif: The database interface to be used
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        if map.is_in_db(dbif) == False:
            dbif.close()
            core.fatal(_("Unable to find map <%s> in temporal database") % (map.get_id()))

        core.verbose(_("Unregister %s map <%s>") % (map.get_type(), map.get_id()))

        # First select all data from the database
        map.select(dbif)
        map_id = map.base.get_id()
        map_register_table = map.get_stds_register()
        stds_register_table = self.get_map_register()

        # Check if the map is registered in the space time raster dataset
	if dbmi.paramstyle == "qmark":
	    sql = "SELECT id FROM " + map_register_table + " WHERE id = ?"
	else:
	    sql = "SELECT id FROM " + map_register_table + " WHERE id = %s"
        dbif.cursor.execute(sql, (self.base.get_id(),))
      	row = dbif.cursor.fetchone()

        # Break if the map is not registered
        if row == None:
            core.warning(_("Map <%s> is not registered in space time dataset") %(map_id, self.base.get_id()))
            if connect == True:
                dbif.close()
            return False

        # Remove the space time raster dataset from the raster dataset register
        if map_register_table != None:
	    if dbmi.paramstyle == "qmark":
		sql = "DELETE FROM " + map_register_table + " WHERE id = ?"
	    else:
		sql = "DELETE FROM " + map_register_table + " WHERE id = %s"
            dbif.cursor.execute(sql, (self.base.get_id(),))

        # Remove the raster map from the space time raster dataset register
        if stds_register_table != None:
	    if dbmi.paramstyle == "qmark":
		sql = "DELETE FROM " + stds_register_table + " WHERE id = ?"
	    else:
		sql = "DELETE FROM " + stds_register_table + " WHERE id = %s"
            dbif.cursor.execute(sql, (map_id,))

        if connect == True:
            dbif.close()
            
    def update_from_registered_maps(self, dbif = None):
        """This methods updates the spatial and temporal extent as well as
           type specific metadata. It should always been called after maps are registered
           or unregistered/deleted from the space time dataset.

           The update of the temporal extent checks if the end time is set correctly.
           In case the registered maps have no valid end time (None) the maximum start time
           will be used. If the end time is earlier than the maximum start time, it will
           be replaced by the maximum start time.

           An other solution to automate this is to use the diactivated trigger
           in the SQL files. But this will result in a huge performance issue
           in case many maps are registred (>1000).
           
           @param dbif: The database interface to be used
        """
        core.verbose(_("Update metadata, spatial and temporal extent from all registered maps of <%s>") % (self.get_id()))

        # Nothing to do if the register is not present
        if not self.get_map_register():
            return

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        map_time = None

        use_start_time = False

        # Get basic info
        stds_name = self.base.get_name()
        stds_mapset = self.base.get_mapset()
        sql_path = get_sql_template_path()

        #We create a transaction
        sql_script = ""
        sql_script += "BEGIN TRANSACTION;\n"
        
        # Update the spatial and temporal extent from registered maps
        # Read the SQL template
        sql = open(os.path.join(sql_path, "update_stds_spatial_temporal_extent_template.sql"), 'r').read()
        sql = sql.replace("GRASS_MAP", self.get_new_map_instance(None).get_type())
        sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
        sql = sql.replace("SPACETIME_ID", self.base.get_id())
        sql = sql.replace("STDS", self.get_type())

        sql_script += sql
        sql_script += "\n"

        # Update type specific metadata
        sql = open(os.path.join(sql_path, "update_" + self.get_type() + "_metadata_template.sql"), 'r').read()
        sql = sql.replace("GRASS_MAP", self.get_new_map_instance(None).get_type())
        sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
        sql = sql.replace("SPACETIME_ID", self.base.get_id())
        sql = sql.replace("STDS", self.get_type())

        sql_script += sql
        sql_script += "\n"

        sql_script += "END TRANSACTION;"

	if dbmi.__name__ == "sqlite3":
	    dbif.cursor.executescript(sql_script)
	else:
	    dbif.cursor.execute(sql_script)
	    
        # Read and validate the selected end time
        self.select()

        if self.is_time_absolute():
            start_time, end_time, tz = self.get_absolute_time()
        else:
            start_time, end_time = self.get_relative_time()

        # In case no end time is set, use the maximum start time of all registered maps as end time
        if end_time == None:
            use_start_time = True
        else:
            # Check if the end time is smaller than the maximum start time
            if self.is_time_absolute():
                sql = """SELECT max(start_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register);"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(None).get_type())
                sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
            else:
                sql = """SELECT max(start_time) FROM GRASS_MAP_relative_time WHERE GRASS_MAP_relative_time.id IN
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register);"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(None).get_type())
                sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )

            dbif.cursor.execute(sql)
            row = dbif.cursor.fetchone()

            if row != None:
                # This seems to be a bug in sqlite3 Python driver
		if dbmi.__name__ == "sqlite3":
		    tstring = row[0]
		    # Convert the unicode string into the datetime format
		    if tstring.find(":") > 0:
			time_format = "%Y-%m-%d %H:%M:%S"
		    else:
			time_format = "%Y-%m-%d"

		    max_start_time = datetime.strptime(tstring, time_format)
		else:
		    max_start_time = row[0]

		if end_time < max_start_time:
                    map_time = "mixed"
		    use_start_time = True
                else:
                    map_time = "interval"
		    
        # Set the maximum start time as end time
        if use_start_time:
            if self.is_time_absolute():
                sql = """UPDATE STDS_absolute_time SET end_time =
               (SELECT max(start_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
               ) WHERE id = 'SPACETIME_ID';"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(None).get_type())
                sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
                sql = sql.replace("SPACETIME_ID", self.base.get_id())
                sql = sql.replace("STDS", self.get_type())
            elif self.is_time_relative():
                sql = """UPDATE STDS_relative_time SET end_time =
               (SELECT max(start_time) FROM GRASS_MAP_relative_time WHERE GRASS_MAP_relative_time.id IN
                        (SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
               ) WHERE id = 'SPACETIME_ID';"""
                sql = sql.replace("GRASS_MAP", self.get_new_map_instance(None).get_type())
                sql = sql.replace("SPACETIME_NAME", stds_name + "_" + stds_mapset )
                sql = sql.replace("SPACETIME_ID", self.base.get_id())
                sql = sql.replace("STDS", self.get_type())

	    if dbmi.__name__ == "sqlite3":
		dbif.cursor.executescript(sql)
	    else:
		dbif.cursor.execute(sql)

            if end_time == None:
                map_time = "point"

        # Set the map time type
        if self.is_time_absolute():
            self.absolute_time.select(dbif)
            self.metadata.select(dbif)
            if self.metadata.get_number_of_maps() > 0:
                self.absolute_time.set_map_time(map_time)
            else:
                self.absolute_time.set_map_time(None)
            self.absolute_time.update_all(dbif)
        else:
            self.relative_time.select(dbif)
            self.metadata.select(dbif)
            if self.metadata.get_number_of_maps() > 0:
                self.relative_time.set_map_time(map_time)
            else:
                self.relative_time.set_map_time(None)
            self.relative_time.update_all(dbif)

        # TODO: Compute the granularity of the dataset and update the database entry

        if connect == True:
            dbif.close()
