"""!@package grass.script.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
from grass.script import temporal as grass

grass.create_temporal_database()
...
@endcode

(C) 2008-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import os
import sqlite3
import os
from datetime import datetime, date, time
import getpass
import core
import raster
import vector
import raster3d

def get_grass_location_db_path():
    grassenv = core.gisenv()
    dbpath = os.path.join(grassenv["GISDBASE"], grassenv["LOCATION_NAME"])
    return os.path.join(dbpath, "grass.db")

def get_sql_template_path():
    base = os.getenv("GISBASE")
    base_etc  = os.path.join(base, "etc")
    return os.path.join(base_etc, "sql")

###############################################################################

def create_temporal_database():
    """This function creates the grass location database structure for raster, vector and raster3d maps
    as well as for the space-time datasets strds, str3ds and stvds"""
    
    database = get_grass_location_db_path()

    # Check if it already exists
    if os.path.exists(database):
        return False

    # Read all SQL scripts and templates
    map_tables_template_sql = open(os.path.join(get_sql_template_path(), "map_tables_template.sql"), 'r').read()
    raster_metadata_sql = open(os.path.join(get_sql_template_path(), "raster_metadata_table.sql"), 'r').read()
    raster3d_metadata_sql = open(os.path.join(get_sql_template_path(), "raster3d_metadata_table.sql"), 'r').read()
    vector_metadata_sql = open(os.path.join(get_sql_template_path(), "vector_metadata_table.sql"), 'r').read()
    stds_tables_template_sql = open(os.path.join(get_sql_template_path(), "stds_tables_template.sql"), 'r').read()
    strds_metadata_sql = open(os.path.join(get_sql_template_path(), "strds_metadata_table.sql"), 'r').read()
    str3ds_metadata_sql = open(os.path.join(get_sql_template_path(), "str3ds_metadata_table.sql"), 'r').read()
    stvds_metadata_sql = open(os.path.join(get_sql_template_path(), "stvds_metadata_table.sql"), 'r').read()

    # Create the raster, raster3d and vector tables
    raster_tables_sql = map_tables_template_sql.replace("GRASS_MAP", "raster")
    vector_tables_sql = map_tables_template_sql.replace("GRASS_MAP", "vector")
    raster3d_tables_sql = map_tables_template_sql.replace("GRASS_MAP", "raster3d")
  
    # Create the space-time raster, raster3d and vector dataset tables
    strds_tables_sql = stds_tables_template_sql.replace("STDS", "strds")
    stvds_tables_sql = stds_tables_template_sql.replace("STDS", "stvds")
    str3ds_tables_sql = stds_tables_template_sql.replace("STDS", "str3ds")

    # Check for completion
    sqlite3.complete_statement(raster_tables_sql)
    sqlite3.complete_statement(vector_tables_sql)
    sqlite3.complete_statement(raster3d_tables_sql)
    sqlite3.complete_statement(raster_metadata_sql)
    sqlite3.complete_statement(vector_metadata_sql)
    sqlite3.complete_statement(raster3d_metadata_sql)
    sqlite3.complete_statement(strds_tables_sql)
    sqlite3.complete_statement(stvds_tables_sql)
    sqlite3.complete_statement(str3ds_tables_sql)
    sqlite3.complete_statement(strds_metadata_sql)
    sqlite3.complete_statement(stvds_metadata_sql)
    sqlite3.complete_statement(str3ds_metadata_sql)

    # Connect to database
    connection = sqlite3.connect(database)
    cursor = connection.cursor()

    # Execute the SQL statements
    # Create the global tables for the native grass datatypes
    cursor.executescript(raster_tables_sql)
    cursor.executescript(raster_metadata_sql)
    cursor.executescript(vector_tables_sql)
    cursor.executescript(vector_metadata_sql)
    cursor.executescript(raster3d_tables_sql)
    cursor.executescript(raster3d_metadata_sql)
    # Create the tables for the new space-time datatypes
    cursor.executescript(strds_tables_sql)
    cursor.executescript(strds_metadata_sql)
    cursor.executescript(stvds_tables_sql)
    cursor.executescript(stvds_metadata_sql)
    cursor.executescript(str3ds_tables_sql)
    cursor.executescript(str3ds_metadata_sql)

    connection.commit()
    cursor.close()

###############################################################################

class dict_sql_serializer():
    def __init__(self):
        self.D = {}
    def serialize(self, type, table, where=None):
	"""Convert the internal dictionary into a string of semicolon separated SQL statements
	   The keys are the colum names and the values are the row entries
	   
	   @type must be SELECT. INSERT, UPDATE
	   @table The name of the table to select, insert or update
	   @where The optinal where statment
	   @return the sql string
	"""

	sql = ""
	args = []

	# Create ordered select statement
	if type == "SELECT":
	    sql += 'SELECT '
	    count = 0
            for key in self.D.keys():
		if count == 0:
                    sql += ' %s ' % key
		else:
                    sql += ' , %s ' % key
		count += 1
            sql += ' FROM ' + table + ' ' 
	    if where:
	        sql += where

	# Create insert statement
	if type =="INSERT":
	    count = 0
	    sql += 'INSERT INTO ' + table + ' ('
            for key in self.D.keys():
		if count == 0:
                    sql += ' %s ' % key
		else:
                    sql += ' ,%s ' % key
		count += 1

	    count = 0
	    sql += ') VALUES ('
            for key in self.D.keys():
		if count == 0:
                    sql += '?'
		else:
                    sql += ',?'
		count += 1
		args.append(self.D[key])
	    sql += ') ' 

	    if where:
	        sql += where

	# Create update statement
	if type =="UPDATE":
	    count = 0
	    sql += 'UPDATE ' + table + ' SET '
            for key in self.D.keys():
		# Update only entries which are not None
		if self.D[key] != None:
		    if count == 0:
                        sql += ' %s = ? ' % key
		    else:
                        sql += ' ,%s = ? ' % key
		    count += 1
	            args.append(self.D[key]) 
	    if where:
	        sql += where

    	return sql, tuple(args)

    def deserialize(self, row):
	"""Convert the content of the sqlite row into the internal dictionary"""
	self.D = {}
	for key in row.keys():
	    self.D[key] = row[key]
     
    def clear(self):
	"""Remove all the content of this class"""
	self.D = {}
    
    def print_self(self):
        print self.D

    def test(self):
        t = dict_sql_serializer()
	t.D["id"] = "soil@PERMANENT"
	t.D["name"] = "soil"
	t.D["mapset"] = "PERMANENT"
	t.D["creator"] = "soeren"
	t.D["creation_time"] = datetime.now()
	t.D["modification_time"] = datetime.now()
	t.D["revision"] = 1
	sql, values = t.serialize(type="SELECT", table="raster_base")        
	print sql, '\n', values
	sql, values = t.serialize(type="INSERT", table="raster_base")        
	print sql, '\n', values
	sql, values = t.serialize(type="UPDATE", table="raster_base")        
	print sql, '\n', values

###############################################################################

class sql_database_interface(dict_sql_serializer):
    """This is the sql database interface to sqlite3"""
    def __init__(self, table=None, ident=None, database=None):

        dict_sql_serializer.__init__(self)

        self.table = table # Name of the table, set in the subclass
        if database == None:
            self.database = get_grass_location_db_path()
        else:
            self.database = database
        self.ident = ident
    
    def get_table_name(self):
        return self.table

    def connect(self):
	self.connection = sqlite3.connect(self.database, detect_types=sqlite3.PARSE_DECLTYPES|sqlite3.PARSE_COLNAMES)
	self.connection.row_factory = sqlite3.Row
        self.cursor = self.connection.cursor()

    def close(self):
	self.connection.commit()
        self.cursor.close()

    def get_delete_statement(self):
	return "DELETE FROM " + self.table + " WHERE id = \"" + str(self.ident) + "\""
    
    def delete(self):
	self.connect()
	sql = self.get_delete_statement()
        #print sql
        self.cursor.execute(sql)
	self.close()

    def get_is_in_db_statement(self):
	return "SELECT id FROM " + self.table + " WHERE id = \"" + str(self.ident) + "\""
    
    def is_in_db(self):
	self.connect()
	sql = self.get_is_in_db_statement()
        #print sql
        self.cursor.execute(sql)
	row = self.cursor.fetchone()
	self.close()
        
	# Nothing found
	if row == None:
	    return False
        
	return True

    def get_select_statement(self):
	return self.serialize("SELECT", self.table, "WHERE id = \"" + str(self.ident) + "\"")

    def select(self):
	self.connect()
	sql, args = self.get_select_statement()
	#print sql
	#print args
	if len(args) == 0:
            self.cursor.execute(sql)
	else:
            self.cursor.execute(sql, args)
	row = self.cursor.fetchone()

	# Nothing found
	if row == None:
	    return False

	if len(row) > 0:
	    self.deserialize(row)
	else:
	    raise IOError
	self.close()

	return True

    def get_insert_statement(self):
	return self.serialize("INSERT", self.table)

    def insert(self):
	self.connect()
	sql, args = self.get_insert_statement()
	#print sql
	#print args
        self.cursor.execute(sql, args)
	self.close()

    def get_update_statement(self):
	return self.serialize("UPDATE", self.table, "WHERE id = \"" + str(self.ident) + "\"")

    def update(self):
	if self.ident == None:
	    raise IOError("Missing identifer");

	sql, args = self.get_update_statement()
	#print sql
	#print args
	self.connect()
        self.cursor.execute(sql, args)
	self.close()

###############################################################################

class dataset_identifer(sql_database_interface):
    """This is the base class for all maps and spacetime datasets storing basic information"""
    def __init__(self, table=None, ident=None, name=None, mapset=None, creator=None, ctime=None,\
		    mtime=None, ttype=None, revision=1):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_name(name)
	self.set_mapset(mapset)
	self.set_creator(creator)
	self.set_ctime(ctime)
	self.set_mtime(mtime)
	self.set_ttype(ttype)
	self.set_revision(revision)

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_name(self, name):
	"""Set the name of the map"""
	self.D["name"] = name

    def set_mapset(self, mapset):
	"""Set the mapset of the map"""
	self.D["mapset"] = mapset

    def set_creator(self, creator):
	"""Set the creator of the map"""
	self.D["creator"] = creator

    def set_ctime(self, ctime=None):
	"""Set the creation time of the map, if nothing set the current time is used"""
	if ctime == None:
            self.D["creation_time"] = datetime.now()
	else:
            self.D["creation_time"] = ctime

    def set_mtime(self, mtime=None):
	"""Set the modification time of the map, if nothing set the current time is used"""
	if mtime == None:
            self.D["modification_time"] = datetime.now()
	else:
            self.D["modification_time"] = mtime

    def set_ttype(self, ttype):
	"""Set the temporal type of the map: absolute or relative, if nothing set absolute time will assumed"""
	if ttype == None or (ttype != "absolute" and ttype != "relative"):
	    self.D["temporal_type"] = "absolute"
        else:
	    self.D["temporal_type"] = ttype

    def set_revision(self, revision=1):
	"""Set the revision of the map: if nothing set revision 1 will assumed"""
	self.D["revision"] = revision

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_name(self):
	"""Get the name of the map
	   @return None if not found"""
	if self.D.has_key("name"):
	    return self.D["name"]
        else:
	    return None

    def get_mapset(self):
	"""Get the mapset of the map
	   @return None if not found"""
	if self.D.has_key("mapset"):
	    return self.D["mapset"]
        else:
	    return None

    def get_creator(self):
	"""Get the creator of the map
	   @return None if not found"""
	if self.D.has_key("creator"):
	    return self.D["creator"]
        else:
	    return None

    def get_ctime(self):
	"""Get the creation time of the map, datatype is datetime
	   @return None if not found"""
	if self.D.has_key("creation_time"):
	    return self.D["creation_time"]
        else:
	    return None

    def get_mtime(self):
	"""Get the modification time of the map, datatype is datetime
	   @return None if not found"""
	if self.D.has_key("modification_time"):
	    return self.D["modification_time"]
        else:
	    return None

    def get_ttype(self):
	"""Get the temporal type of the map
	   @return None if not found"""
	if self.D.has_key("temporal_type"):
	    return self.D["temporal_type"]
        else:
	    return None

    def get_revision(self):
	"""Get the revision of the map
	   @return None if not found"""
	if self.D.has_key("revision"):
	    return self.D["revision"]
        else:
	    return None

###############################################################################

class raster_base(dataset_identifer):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_identifer.__init__(self, "raster_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

class raster3d_base(dataset_identifer):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_identifer.__init__(self, "raster3d_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

class vector_base(dataset_identifer):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_identifer.__init__(self, "vector_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

###############################################################################

class stds_base(dataset_identifer):
    def __init__(self, table=None, ident=None, name=None, mapset=None, semantic_type=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_identifer.__init__(self, table, ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

	self.set_semantic_type(semantic_type)

    def set_semantic_type(self, semantic_type):
	"""Set the sematnic type of the space time dataset"""
	self.D["semantic_type"] = semantic_type

    def get_semantic_type(self):
	"""Get the semantic_type of the space time dataset
	   @return None if not found"""
	if self.D.has_key("semantic_type"):
	    return self.D["semantic_type"]
        else:
	    return None

###############################################################################

class strds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None, semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "strds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

class str3ds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None, semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "str3ds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

class stvds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None, semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "stvds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

###############################################################################

class absolute_temporal_extent(sql_database_interface):
    """This is the absolute time base class for all maps and spacetime datasets"""
    def __init__(self, table=None, ident=None, start_time=None, end_time=None, timezone=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_start_time(start_time)
	self.set_end_time(end_time)
	self.set_timezone(timezone)

    def starts(self, map):
	"""Return True if this absolute time object starts at the start of the provided absolute time object and finishes within it
	   A  |-----|
	   B  |---------|
	"""
	if self.D["start_time"] == map.D["start_time"] and self.D["end_time"] < map.D["end_time"]:
	    return True
        else:
	    return False

    def started(self, map):
	"""Return True if this absolute time object is started at the start of the provided absolute time object
	   A  |---------|
	   B  |-----|
	"""
	if self.D["start_time"] == map.D["start_time"] and self.D["end_time"] > map.D["end_time"]:
	    return True
        else:
	    return False

    def finishes(self, map):
	"""Return True if this absolute time object finishes at the end and within of the provided absolute time object
	   A      |-----|
	   B  |---------|
	"""
	if self.D["end_time"] == map.D["end_time"] and  self.D["start_time"] > map.D["start_time"] :
	    return True
        else:
	    return False

    def finished(self, map):
	"""Return True if this absolute time object finished at the end of the provided absolute time object
	   A  |---------|
	   B      |-----|
	"""
	if self.D["end_time"] == map.D["end_time"] and  self.D["start_time"] < map.D["start_time"] :
	    return True
        else:
	    return False

    def after(self, map):
	"""Return True if this absolute time object is temporal located after the provided absolute time object
	   A             |---------|
	   B  |---------|
	"""
	if self.D["start_time"] > map.D["end_time"]:
	    return True
        else:
	    return False

    def before(self, map):
	"""Return True if this absolute time object is temporal located bevor the provided absolute time object
	   A  |---------|
	   B             |---------|
	"""
	if self.D["end_time"] < map.D["start_time"]:
	    return True
        else:
	    return False

    def adjacent(self, map):
	"""Return True if this absolute time object is a meeting neighbour the provided absolute time object
	   A            |---------|
	   B  |---------|
	   A  |---------|
	   B            |---------|
	"""
	if (self.D["start_time"] == map.D["end_time"]) or (self.D["end_time"] == map.D["start_time"]):
	    return True
        else:
	    return False

    def follows(self, map):
	"""Return True if this absolute time object is temporal follows the provided absolute time object
	   A            |---------|
	   B  |---------|
	"""
	if self.D["start_time"] == map.D["end_time"]:
	    return True
        else:
	    return False

    def precedes(self, map):
	"""Return True if this absolute time object is temporal precedes the provided absolute time object
	   A  |---------|
	   B            |---------|
	"""
	if self.D["end_time"] == map.D["start_time"]:
	    return True
        else:
	    return False

    def during(self, map):
	"""Return True if this absolute time object is temporal located during the provided absolute time object
	   A   |-------|
	   B  |---------|
	"""
	if self.D["start_time"] > map.D["start_time"] and self.D["end_time"] < map.D["end_time"]:
	    return True
        else:
	    return False

    def contains(self, map):
	"""Return True if this absolute time object is temporal located during the provided absolute time object
	   A  |---------|
	   B   |-------|
	"""
	if self.D["start_time"] < map.D["start_time"] and self.D["end_time"] > map.D["end_time"]:
	    return True
        else:
	    return False

    def equivalent(self, map):
	"""Return True if this absolute time object is temporal located equivalent the provided absolute time object
	   A  |---------|
	   B  |---------|
	"""
	if self.D["start_time"] == map.D["start_time"] and self.D["end_time"] == map.D["end_time"]:
	    return True
        else:
	    return False

    def overlaps(self, map):
	"""Return True if this absolute time object is temporal overlaps the provided absolute time object
           A  |---------|
	   B    |---------|
	"""
	if self.D["start_time"] < map.D["start_time"] and self.D["end_time"] < map.D["end_time"] and\
	   self.D["end_time"] > map.D["start_time"]:
	    return True
        else:
	    return False

    def overlapped(self, map):
	"""Return True if this absolute time object is temporal overlaped by the provided absolute time object
	   A    |---------|
           B  |---------|
	"""
	if self.D["start_time"] > map.D["start_time"] and self.D["end_time"] > map.D["end_time"] and\
	   self.D["start_time"] < map.D["end_time"]:
	    return True
        else:
	    return False

    def temporal_relation(self, map):
	"""Returns the temporal relation between absolute time temporal objects
	   Temporal relationsships are implemented after [Allen and Ferguson 1994 Actions and Events in Interval Temporal Logic]
	"""
	if self.equivalent(map):
	    return "equivalent"
	if self.during(map):
	    return "during"
	if self.contains(map):
	    return "contains"
	if self.overlaps(map):
	    return "overlaps"
	if self.overlapped(map):
	    return "overlapped"
	if self.after(map):
	    return "after"
	if self.before(map):
	    return "before"
	if self.starts(map):
	    return "starts"
	if self.finishes(map):
	    return "finishes"
	if self.started(map):
	    return "started"
	if self.finished(map):
	    return "finished"
	if self.equivalent(map):
	    return "equivalent"
	if self.follows(map):
	    return "follows"
	if self.precedes(map):
	    return "precedes"
        return None

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_start_time(self, start_time):
	"""Set the valid start time of the map, this should be of type datetime"""
	self.D["start_time"] = start_time

    def set_end_time(self, end_time):
	"""Set the valid end time of the map, this should be of type datetime"""
	self.D["end_time"] = end_time

    def set_timezone(self, timezone):
	"""Set the timezone of the map, integer from 1 - 24"""
	self.D["timezone"] = timezone

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_start_time(self):
	"""Get the valid start time of the map
	   @return None if not found"""
	if self.D.has_key("start_time"):
	    return self.D["start_time"]
        else:
	    return None

    def get_end_time(self):
	"""Get the valid end time of the map
	   @return None if not found"""
	if self.D.has_key("end_time"):
	    return self.D["end_time"]
        else:
	    return None

    def get_timezone(self):
	"""Get the timezone of the map
	   @return None if not found"""
	if self.D.has_key("timezone"):
	    return self.D["timezone"]
        else:
	    return None

###############################################################################

class raster_absolute_time(absolute_temporal_extent):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_temporal_extent.__init__(self, "raster_absolute_time", ident, start_time, end_time, timezone)

class raster3d_absolute_time(absolute_temporal_extent):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_temporal_extent.__init__(self, "raster3d_absolute_time", ident, start_time, end_time, timezone)

class vector_absolute_time(absolute_temporal_extent):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_temporal_extent.__init__(self, "vector_absolute_time", ident, start_time, end_time, timezone)

###############################################################################

class stds_absolute_time(absolute_temporal_extent):
    def __init__(self, table=None, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        absolute_temporal_extent.__init__(self, table, ident, start_time, end_time, timezone)

	self.set_granularity(granularity)

    def set_granularity(self, granularity):
	"""Set the granularity of the space time dataset"""
	self.D["granularity"] = granularity

    def get_granularity(self):
	"""Get the granularity of the space time dataset
	   @return None if not found"""
	if self.D.has_key("granularity"):
	    return self.D["granularity"]
        else:
	    return None

###############################################################################

class strds_absolute_time(stds_absolute_time):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        stds_absolute_time.__init__(self, "strds_absolute_time", ident, start_time, end_time, granularity, timezone)

class str3ds_absolute_time(stds_absolute_time):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        stds_absolute_time.__init__(self, "str3ds_absolute_time", ident, start_time, end_time, granularity, timezone)

class stvds_absolute_time(stds_absolute_time):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        stds_absolute_time.__init__(self, "stvds_absolute_time", ident, start_time, end_time, granularity, timezone)

###############################################################################

class relative_temporal_extent(sql_database_interface):
    """This is the relative time base class for all maps and spacetime datasets"""
    def __init__(self, table=None, ident=None, interval=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_interval(interval)

    def after(self, map):
	"""Return True if this relative time object is temporal located after the provided relative time object
	   A   |
	   B  |
	"""
	if self.D["interval"] > map.D["interval"]:
	    return True
        else:
	    return False


    def before(self, map):
	"""Return True if this relative time object is temporal located bevor the provided relative time object
	   A  |
	   B   |
	"""
	if self.D["interval"] < map.D["interval"]:
	    return True
        else:
	    return False

    def equivalent(self, map):
	"""Return True if this relative time object is equivalent to the provided relative time object
	   A  |
	   B  |
	"""
	if self.D["interval"] == map.D["interval"]:
	    return True
        else:
	    return False

    def temporal_relation(self, map):
	"""Returns the temporal relation between relative time temporal objects
	"""
	if self.equivalent(map):
	    return "equivalent"
	if self.after(map):
	    return "after"
	if self.before(map):
	    return "before"
        return None

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_interval(self, interval):
	"""Set the valid interval time of the map, this should be of type datetime"""
	self.D["interval"] = interval

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_interval(self):
	"""Get the valid interval time of the map
	   @return None if not found"""
	if self.D.has_key("interval"):
	    return self.D["interval"]
        else:
	    return None

###############################################################################

class raster_relative_time(relative_temporal_extent):
    def __init__(self, ident=None, interval=None):
        relative_temporal_extent.__init__(self, "raster_relative_time", ident, interval)

class raster3d_relative_time(relative_temporal_extent):
    def __init__(self, ident=None, interval=None):
        relative_temporal_extent.__init__(self, "raster3d_relative_time", ident, interval)

class vector_relative_time(relative_temporal_extent):
    def __init__(self, ident=None, interval=None):
        relative_temporal_extent.__init__(self, "vector_relative_time", ident, interval)
        
###############################################################################

class stds_relative_time(relative_temporal_extent):
    def __init__(self, table=None, ident=None, interval=None, granularity=None):
        relative_temporal_extent.__init__(self, table, ident, interval)

	self.set_granularity(granularity)

    def set_granularity(self, granularity):
	"""Set the granularity of the space time dataset"""
	self.D["granularity"] = granularity

    def get_granularity(self):
	"""Get the granularity of the space time dataset
	   @return None if not found"""
	if self.D.has_key("granularity"):
	    return self.D["granularity"]
        else:
	    return None

###############################################################################

class strds_relative_time(stds_relative_time):
    def __init__(self, ident=None, interval=None, granularity=None):
        stds_relative_time.__init__(self, "strds_relative_time", ident, interval, granularity)

class str3ds_relative_time(stds_relative_time):
    def __init__(self, ident=None, interval=None, granularity=None):
        stds_relative_time.__init__(self, "str3ds_relative_time", ident, interval, granularity)

class stvds_relative_time(stds_relative_time):
    def __init__(self, ident=None, interval=None, granularity=None):
        stds_relative_time.__init__(self, "stvds_relative_time", ident, interval, granularity)

###############################################################################

class spatial_extent(sql_database_interface):
    """This is the spatial extent base class for all maps and spacetime datasets"""
    def __init__(self, table=None, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None, proj="XY"):

	sql_database_interface.__init__(self, table, ident)
        self.set_id(ident)
        self.set_spatial_extent(north, south, east, west, top, bottom)
	self.set_projection(proj)
        
    def overlap_2d(self, extent):
        """Return True if the 2d extents overlap. Code is lend from wind_overlap.c in lib/gis"""  
        
        if self.get_projection() != extent.get_projection():
            core.error("Projections are different. Unable to compute overlap_2d for spatial extents")
        
        N = extent.get_north()
        S = extent.get_south()
        E = extent.get_east()
        W = extent.get_west()
        
        if(self.get_north() <= S):
            return False
        
        if(self.get_south() >= N):
            return False
        
        # Adjust the east and west in case of LL projection
        if self.get_proj() == "LL":
            while E < self.get_west():
                E += 360.0
                W += 360.0

            while W > self.get_east():
                E -= 360.0
                W -= 360.0
            
        if self.get_east() <= W:
            return False
        
        if self.get_west() >= E:
            return False
        
        return True

    def overlap(self, extent):
        """Return True if the extents overlap."""  
        
        if self.overlap_2d(extent) == False:
            return False
            
        T = extent.get_top()
        B = extent.get_bottom()
        
        if self.get_top() <= B:
            return False
        
        if self.get_bottom() >= T:
            return False
        
        return True

    def set_spatial_extent(self, north, south, east, west, top, bottom):
        """Set the spatial extent"""

	self.set_north(north)
	self.set_south(south)
	self.set_east(east)
	self.set_west(west)
	self.set_top(top)
	self.set_bottom(bottom)        
        
    def set_projection(self, proj):
        """Set the projection of the spatial extent it should be XY or LL.
           As default the projection is XY
        """
        if proj == None or (proj != "XY" and proj != "LL"):
            self.D["proj"] = "XY"
        else:
            self.D["proj"] = proj

    def set_spatial_extent_2d(self, north, south, east, west):

	self.set_id(ident)
	self.set_north(north)
	self.set_south(south)
	self.set_east(east)
	self.set_west(west)
	self.set_top(0)
	self.set_bottom(0)

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_north(self, north):
	"""Set the northern edge of the map"""
	self.D["north"] = north

    def set_south(self, sourth):
	"""Set the sourthern edge of the map"""
	self.D["south"] = sourth

    def set_west(self, west):
	"""Set the western edge of the map"""
	self.D["west"] = west

    def set_east(self, east):
	"""Set the eastern edge of the map"""
	self.D["east"] = east

    def set_top(self, top):
	"""Set the top edge of the map"""
	self.D["top"] = top

    def set_bottom(self, bottom):
	"""Set the bottom edge of the map"""
	self.D["bottom"] = bottom

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_projection(self):
        """Get the projection of the spatial extent"""
        return self.D["proj"]
    
    def get_volume(self):
        """Compute the volume of the extent, in case z is zero (top == bottom or top - bottom = 1) the area is returned"""

        if self.get_projection() == "LL":
            core.error("Volume computation is not supported for LL projections")
        
        area = self.get_area()
        
        bbox = self.get_spatial_extent()
        
        z = abs(bbox[4] - bbox[5])
        
        if z == 0:
            z = 1.0
            
        return area*z
       
    def get_area(self):
        """Compute the area of the extent, extent in z direction is ignored"""
        
        if self.get_projection() == "LL":
            core.error("Area computation is not supported for LL projections")
        
        bbox = self.get_spatial_extent()
        
        y = abs(bbox[0] - bbox[1])
        x = abs(bbox[2] - bbox[3])
                    
        return x*y
    
    def get_spatial_extent(self):
        """Return a tuple (north, south, east, west, top, bottom) of the spatial extent"""
        
        return (self.get_north(), self.get_south, self.get_east(), self.get_west(), \
                self.get_top(), self.get_bottom())
                
    def get_spatial_extent_2d(self):
        """Return a tuple (north, south, east, west,) of the 2d spatial extent"""
        return (self.get_north(), self.get_south, self.get_east(), self.get_west())
    
    def get_north(self):
	"""Get the northern edge of the map
	   @return None if not found"""
	if self.D.has_key("north"):
	    return self.D["north"]
        else:
	    return None

    def get_south(self):
	"""Get the southern edge of the map
	   @return None if not found"""
	if self.D.has_key("south"):
	    return self.D["south"]
        else:
	    return None

    def get_east(self):
	"""Get the eastern edge of the map
	   @return None if not found"""
	if self.D.has_key("east"):
	    return self.D["east"]
        else:
	    return None

    def get_west(self):
	"""Get the western edge of the map
	   @return None if not found"""
	if self.D.has_key("west"):
	    return self.D["west"]
        else:
	    return None

    def get_top(self):
	"""Get the top edge of the map
	   @return None if not found"""
	if self.D.has_key("top"):
	    return self.D["top"]
        else:
	    return None

    def get_bottom(self):
	"""Get the bottom edge of the map
	   @return None if not found"""
	if self.D.has_key("bottom"):
	    return self.D["bottom"]
        else:
	    return None

###############################################################################

class raster_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "raster_spatial_extent", ident, north, south, east, west, top, bottom)

class raster3d_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "raster3d_spatial_extent", ident, north, south, east, west, top, bottom)

class vector_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "vector_spatial_extent", ident, north, south, east, west, top, bottom)

class strds_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "strds_spatial_extent", ident, north, south, east, west, top, bottom)

class str3ds_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "str3ds_spatial_extent", ident, north, south, east, west, top, bottom)

class stvds_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "stvds_spatial_extent", ident, north, south, east, west, top, bottom)

###############################################################################

class raster_metadata_base(sql_database_interface):
    """This is the raster metadata base class for raster and raster3d maps"""
    def __init__(self, table=None, ident=None, datatype=None, cols=None, rows=None, number_of_cells=None, nsres=None, ewres=None, min=None, max=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_datatype(datatype)
	self.set_cols(cols)
	self.set_rows(rows)
	self.set_number_of_cells(number_of_cells)
	self.set_nsres(nsres)
	self.set_ewres(ewres)
	self.set_min(min)
	self.set_max(max)

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_datatype(self, datatype):
	"""Set the datatype"""
	self.D["datatype"] = datatype

    def set_cols(self, cols):
	"""Set the number of cols"""
	self.D["cols"] = cols

    def set_rows(self, rows):
	"""Set the number of rows"""
	self.D["rows"] = rows

    def set_number_of_cells(self, number_of_cells):
	"""Set the number of cells"""
	self.D["number_of_cells"] = number_of_cells

    def set_nsres(self, nsres):
	"""Set the north-south resolution"""
	self.D["nsres"] = nsres

    def set_ewres(self, ewres):
	"""Set the east-west resolution"""
	self.D["ewres"] = ewres

    def set_min(self, min):
	"""Set the minimum raster value"""
	self.D["min"] = min

    def set_max(self, max):
	"""Set the maximum raster value"""
	self.D["max"] = max

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_datatype(self):
	"""Get the map type 
	   @return None if not found"""
	if self.D.has_key("datatype"):
	    return self.D["datatype"]
        else:
	    return None

    def get_cols(self):
	"""Get number of cols 
	   @return None if not found"""
	if self.D.has_key("cols"):
	    return self.D["cols"]
        else:
	    return None

    def get_rows(self):
	"""Get number of rows
	   @return None if not found"""
	if self.D.has_key("rows"):
	    return self.D["rows"]
        else:
	    return None

    def get_number_of_cells(self):
	"""Get number of cells 
	   @return None if not found"""
	if self.D.has_key("number_of_cells"):
	    return self.D["number_of_cells"]
        else:
	    return None

    def get_nsres(self):
	"""Get the north-south resolution
	   @return None if not found"""
	if self.D.has_key("nsres"):
	    return self.D["nsres"]
        else:
	    return None

    def get_ewres(self):
	"""Get east-west resolution
	   @return None if not found"""
	if self.D.has_key("ewres"):
	    return self.D["ewres"]
        else:
	    return None

    def get_min(self):
	"""Get the minimum cell value 
	   @return None if not found"""
	if self.D.has_key("min"):
	    return self.D["min"]
        else:
	    return None

    def get_max(self):
	"""Get the maximum cell value 
	   @return None if not found"""
	if self.D.has_key("max"):
	    return self.D["max"]
        else:
	    return None

###############################################################################

class raster_metadata(raster_metadata_base):
    """This is the raster metadata class"""
    def __init__(self, ident=None, strds_register=None, datatype=None, cols=None, rows=None, number_of_cells=None, nsres=None, ewres=None, min=None, max=None):

	raster_metadata_base.__init__(self, "raster_metadata", ident, datatype, cols, rows, number_of_cells, nsres, ewres, min, max)

	self.set_strds_register(strds_register)

    def set_strds_register(self, strds_register):
	"""Set the space time raster dataset register table name"""
	self.D["strds_register"] = strds_register

    def get_strds_register(self):
	"""Get the space time raster dataset register table name
	   @return None if not found"""
	if self.D.has_key("strds_register"):
	    return self.D["strds_register"]
        else:
	    return None

###############################################################################

class raster3d_metadata(raster_metadata_base):
    """This is the raster3d metadata class"""
    def __init__(self, ident=None, str3ds_register=None, datatype=None, cols=None, rows=None, depths=None, number_of_cells=None, nsres=None, ewres=None, tbres=None, min=None, max=None):

	raster_metadata_base.__init__(self, "raster3d_metadata", ident, datatype, cols, rows, number_of_cells, nsres, ewres, min, max)

	self.set_str3ds_register(str3ds_register)
	self.set_tbres(tbres)
	self.set_depths(depths)

    def set_str3ds_register(self, str3ds_register):
	"""Set the space time raster3d dataset register table name"""
	self.D["str3ds_register"] = str3ds_register

    def set_depths(self, depths):
	"""Set the number of depths"""
	self.D["depths"] = depths

    def set_tbres(self, tbres):
	"""Set the top-bottom resolution"""
	self.D["tbres"] = tbres

    def get_str3ds_register(self):
	"""Get the space time raster3d dataset register table name
	   @return None if not found"""
	if self.D.has_key("str3ds_register"):
	    return self.D["str3ds_register"]
        else:
	    return None

    def get_depths(self):
	"""Get number of depths
	   @return None if not found"""
	if self.D.has_key("depths"):
	    return self.D["depths"]
        else:
	    return None

    def get_tbres(self):
	"""Get top-bottom resolution
	   @return None if not found"""
	if self.D.has_key("tbres"):
	    return self.D["tbres"]
        else:
	    return None

###############################################################################

class vector_metadata(sql_database_interface):
    """This is the vector metadata class"""
    def __init__(self, ident=None, stvds_register=None):

	sql_database_interface.__init__(self, "vector_metadata", ident)

	self.set_id(ident)
	self.set_stvds_register(stvds_register)

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_stvds_register(self, stvds_register):
	"""Set the space time vector dataset register table name"""
	self.D["stvds_register"] = stvds_register

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_stvds_register(self):
	"""Get the space time vector dataset register table name
	   @return None if not found"""
	if self.D.has_key("stvds_register"):
	    return self.D["stvds_register"]
        else:
	    return None


###############################################################################

class stds_metadata_base(sql_database_interface):
    """This is the space time dataset metadata base class for strds, stvds and str3ds datasets
       setting/getting the id, the title and the description
    """
    def __init__(self, table=None, ident=None, title=None, description=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_title(title)
	self.set_description(description)
        # No setter for this
        self.D["number_of_maps"] = None

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_title(self, title):
	"""Set the title"""
	self.D["title"] = title

    def set_description(self, description):
	"""Set the number of cols"""
	self.D["description"] = description

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_title(self):
	"""Get the title 
	   @return None if not found"""
	if self.D.has_key("title"):
	    return self.D["title"]
        else:
	    return None

    def get_description(self):
	"""Get description 
	   @return None if not found"""
	if self.D.has_key("description"):
	    return self.D["description"]
        else:
	    return None

    def get_number_of_maps(self):
	"""Get the number of registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("number_of_maps"):
	    return self.D["number_of_maps"]
        else:
	    return None
    
###############################################################################
    
class stds_raster_metadata_base(stds_metadata_base):
    """This is the space time dataset metadata base class for strds and str3ds datasets
       
       Most of the metadata values are set by triggers in the database when 
       new raster of voxel maps are added. Therefor only some set- an many get-functions 
       are available.
    """
    def __init__(self, table=None, ident=None, title=None, description=None):

	stds_metadata_base.__init__(self, table, ident, title, description)
        
        # Initialize the dict to select all values from the db
        self.D["min_max"] = None
        self.D["max_max"] = None
        self.D["min_min"] = None
        self.D["max_min"] = None
        self.D["nsres_min"] = None
        self.D["nsres_max"] = None
        self.D["ewres_min"] = None
        self.D["ewres_max"] = None

    def get_max_min(self):
	"""Get the minimal maximum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("max_min"):
	    return self.D["max_min"]
        else:
	    return None

    def get_min_min(self):
	"""Get the minimal minimum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("min_min"):
	    return self.D["min_min"]
        else:
	    return None

    def get_max_max(self):
	"""Get the maximal maximum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("max_max"):
	    return self.D["max_max"]
        else:
	    return None

    def get_min_max(self):
	"""Get the maximal minimum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("min_max"):
	    return self.D["min_max"]
        else:
	    return None

    def get_min_max(self):
	"""Get the minimal maximum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("min_max"):
	    return self.D["min_max"]
        else:
	    return None

    def get_nsres_min(self):
	"""Get the minimal north-south resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("nsres_min"):
	    return self.D["nsres_min"]
        else:
	    return None

    def get_nsres_max(self):
	"""Get the maximal north-south resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("nsres_max"):
	    return self.D["nsres_max"]
        else:
	    return None

    def get_ewres_min(self):
	"""Get the minimal east-west resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("ewres_min"):
	    return self.D["ewres_min"]
        else:
	    return None

    def get_ewres_max(self):
	"""Get the maximal east-west resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("ewres_max"):
	    return self.D["ewres_max"]
        else:
	    return None


###############################################################################

class strds_metadata(stds_raster_metadata_base):
    """This is the raster metadata class"""
    def __init__(self, ident=None, raster_register=None,  title=None, description=None):

	stds_raster_metadata_base.__init__(self, "strds_metadata", ident, title, description)

	self.set_raster_register(raster_register)

    def set_raster_register(self, raster_register):
	"""Set the raster map register table name"""
	self.D["raster_register"] = raster_register

    def get_raster_register(self):
	"""Get the raster map register table name
	   @return None if not found"""
	if self.D.has_key("raster_register"):
	    return self.D["raster_register"]
        else:
	    return None
        
###############################################################################

class str3ds_metadata(stds_raster_metadata_base):
    """This is the space time raster3d metadata class"""
    def __init__(self, ident=None, raster3d_register=None,  title=None, description=None):

	stds_raster_metadata_base.__init__(self, "str3ds_metadata", ident, title, description)

	self.set_raster3d_register(raster3d_register)
        self.D["tbres_min"] = None
        self.D["tbres_max"] = None

    def set_raster3d_register(self, raster3d_register):
	"""Set the raster map register table name"""
	self.D["raster3d_register"] = raster3d_register

    def get_raster3d_register(self):
	"""Get the raster3d map register table name
	   @return None if not found"""
	if self.D.has_key("raster3d_register"):
	    return self.D["raster3d_register"]
        else:
	    return None

    def get_tbres_min(self):
	"""Get the minimal top-bottom resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("tbres_min"):
	    return self.D["tbres_min"]
        else:
	    return None

    def get_tbres_max(self):
	"""Get the maximal top-bottom resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("tbres_max"):
	    return self.D["tbres_max"]
        else:
	    return None

###############################################################################

class stvds_metadata(stds_metadata_base):
    """This is the raster metadata class"""
    def __init__(self, ident=None, vector_register=None,  title=None, description=None):

	stds_metadata_base.__init__(self, "stvds_metadata", ident, title, description)

	self.set_vector_register(vector_register)

    def set_vector_register(self, vector_register):
	"""Set the vector map register table name"""
	self.D["vector_register"] = vector_register

    def get_vector_register(self):
	"""Get the vector map register table name
	   @return None if not found"""
	if self.D.has_key("vector_register"):
	    return self.D["vector_register"]
        else:
	    return None

###############################################################################

class abstract_dataset():
    """This is the base class for all datasets (raster, vector, raster3d, strds, stvds, str3ds)"""
    
    def get_type(self):
        """Return the type of this class"""
        raise IOError("This method must be implemented in the subclasses")
    
    def get_new_instance(self, ident):
        """Return a new instance with the type of this class"""
        raise IOError("This method must be implemented in the subclasses")
  
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
        if self.is_in_db():
            self.base.delete()

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
    
    def set_time_to_absolute(self):
	self.base.set_ttype("absolute")

    def set_time_to_relative(self):
	self.base.Dset_ttype("relative")

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

class abstract_space_time_dataset(abstract_dataset):
    """Abstract space time dataset class
    
       This class represents a space time dataset. Convenient functions 
       to select, update, insert or delete objects of this type int the SQL 
       temporal database exists as well as functions to register or deregister 
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
            core.error("Unkown temporal type \"" + temporal_type + "\"")
            
        self.base.set_semantic_type(semantic_type)
        self.metadata.set_title(title)
        self.metadata.set_description(description)
        
    def register_map(self, map):
        """Register a map in the space time raster dataset.
        
            This method takes care of the registration of a map
            in a space time dataset. 
        """
        
        if map.is_in_db() == False:
            core.error("Only maps with absolute or relative valid time can be registered")
        
        # First select all data from the database
        map.select()
        map_id = map.base.get_id()
        map_name = map.base.get_name()
        map_mapset = map.base.get_mapset()
        map_register_table = map.get_stds_register()
        
        # Get basic info
        stds_name = self.base.get_name()
        stds_mapset = self.base.get_mapset()
        stds_register_table = self.get_map_register()
        
        if stds_mapset != map_mapset:
            core.error("You can only register raster maps from the same mapset")
            
        # Check if map is already registred
        if stds_register_table:
            sql = "SELECT id FROM " + stds_register_table + " WHERE id = (?)" 
            self.base.connect()
            self.base.cursor.execute(sql, (map_id,))
            row = self.base.cursor.fetchone()
            # In case of no entry make a new one
            if row and row[0] == map_id:
                core.error("Map is already registered")
            self.base.close()
        
        # Create tables
        sql_path = get_sql_template_path()
        
        # We need to create the stmap raster register table bevor we can register the map
        if map_register_table == None:
            # Read the SQL template
            sql = open(os.path.join(sql_path, "map_stds_register_table_template.sql"), 'r').read()
            # Create the raster, raster3d and vector tables
            sql = sql.replace("GRASS_MAP", map.get_type())
            sql = sql.replace("MAP_NAME", map_name + "_" + map_mapset )
            sql = sql.replace("MAP_ID", map_id)
            sql = sql.replace("STDS", self.get_type())
            
            self.base.connect()
            self.base.cursor.executescript(sql)
            self.base.close()
            
            map_register_table = map_name + "_" + map_mapset + "_" + self.get_type() + "_register"
            # Set the stds register table name and put it into the DB
            map.set_stds_register(map_register_table)
            map.metadata.update()
            
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
            self.base.connect()
            self.base.cursor.execute(sql, (self.base.get_id(),))
            self.base.close()
        
        # Now put the raster name in the stds map register table
        sql = "INSERT INTO " + stds_register_table + " (id) " + "VALUES (?)" 
        self.base.connect()
        self.base.cursor.execute(sql, (map_id,))
        self.base.close()
        
    def unregister_map(self, map):
        """Remove a register a raster map from the space time raster dataset.
        
            This method takes care of the unregistration of a raster map
            in a space time raster dataset. 
        """
        
        if map.is_in_db() == False:
            core.error("Only maps with absolute or relative valid time can be registered")
        
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
            core.error("Map " + map_id + " is not registered in space time dataset " + self.base.get_id())
            
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
            
###############################################################################

class space_time_raster_dataset(abstract_space_time_dataset):
    """Space time raster dataset class
    
       This class represents a space time raster dataset. Convenient functions 
       to select, update, insert or delete objects of this type int the SQL 
       temporal database exists as well as functions to register or deregister 
       raster maps.
       
       Parts of the temporal logic are implemented in the SQL temporal database,
       like the computation of the temporal and spatial extent as well as the
       collecting of metadata.
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
    
       This class represents a space time raster3d dataset. Convenient functions 
       to select, update, insert or delete objects of this type int the SQL 
       temporal database exists as well as functions to register or deregister 
       raster maps.
       
       Parts of the temporal logic are implemented in the SQL temporal database,
       like the computation of the temporal and spatial extent as well as the
       collecting of metadata.
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
    
       This class represents a space time vector dataset. Convenient functions 
       to select, update, insert or delete objects of this type int the SQL 
       temporal database exists as well as functions to register or deregister 
       raster maps.
       
       Parts of the temporal logic are implemented in the SQL temporal database,
       like the computation of the temporal and spatial extent as well as the
       collecting of metadata.
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
        return self.metadata.get_vectorr_register()
  
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
        
###############################################################################
