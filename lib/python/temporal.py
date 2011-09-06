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
        print sql
        self.cursor.execute(sql)
	self.close()

    def get_is_in_db_statement(self):
	return "SELECT id FROM " + self.table + " WHERE id = \"" + str(self.ident) + "\""
    
    def is_in_db(self):
	self.connect()
	sql = self.get_is_in_db_statement()
        print sql
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
    def __init__(self, ident=None, name=None, mapset=None,semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "strds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

class str3ds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None,semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "str3ds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

class stvds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None, semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "stvds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

###############################################################################

class absolute_timestamp(sql_database_interface):
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

class raster_absolute_time(absolute_timestamp):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_timestamp.__init__(self, "raster_absolute_time", ident, start_time, end_time, timezone)

class raster3d_absolute_time(absolute_timestamp):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_timestamp.__init__(self, "raster3d_absolute_time", ident, start_time, end_time, timezone)

class vector_absolute_time(absolute_timestamp):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_timestamp.__init__(self, "vector_absolute_time", ident, start_time, end_time, timezone)

###############################################################################

class stds_absolute_time(absolute_timestamp):
    def __init__(self, table=None, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        absolute_timestamp.__init__(self, table, ident, start_time, end_time, timezone)

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

class relative_timestamp(sql_database_interface):
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

class raster_relative_time(relative_timestamp):
    def __init__(self, ident=None, interval=None):
        relative_timestamp.__init__(self, "raster_relative_time", ident, interval)

class raster3d_relative_time(relative_timestamp):
    def __init__(self, ident=None, interval=None):
        relative_timestamp.__init__(self, "raster3d_relative_time", ident, interval)

class vector_relative_time(relative_timestamp):
    def __init__(self, ident=None, interval=None):
        relative_timestamp.__init__(self, "vector_relative_time", ident, interval)
        
###############################################################################

class stds_relative_time(relative_timestamp):
    def __init__(self, table=None, ident=None, interval=None, granularity=None):
        relative_timestamp.__init__(self, table, ident, interval)

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
    def __init__(self, table=None, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_north(north)
	self.set_south(south)
	self.set_east(east)
	self.set_west(west)
	self.set_top(top)
	self.set_bottom(bottom)

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

class abstract_dataset():
    def __init__(self, ident):
	self.reset(ident)

    def reset(self, ident):
	"""Reset the internal structure and set the identifier"""
        raise IOError("This method must be implemented in the subclasses")

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

class raster_dataset(abstract_dataset):
    def __init__(self, ident):
	self.reset(ident)
        
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
                
        self.spatial_extent.set_north(kvp["north"])
        self.spatial_extent.set_south(kvp["south"])
        self.spatial_extent.set_west(kvp["west"])
        self.spatial_extent.set_east(kvp["east"])
        self.spatial_extent.set_top(0)
        self.spatial_extent.set_bottom(0)
        
        # Fill metadata
        
        self.metadata.set_nsres(kvp["nsres"])
        self.metadata.set_ewres(kvp["ewres"])
        self.metadata.set_datatype(kvp["datatype"])
        self.metadata.set_min(kvp["min"])
        self.metadata.set_max(kvp["max"])
        
        rows = (kvp["north"] - kvp["south"])/kvp["nsres"]
        cols = (kvp["east"] - kvp["west"])/kvp["ewres"]
        
        ncells = cols * rows
        
        self.metadata.set_cols(cols)
        self.metadata.set_rows(rows)
        self.metadata.set_number_of_cells(ncells)

###############################################################################

class raster3d_dataset(abstract_dataset):
    def __init__(self, ident):
	self.reset(ident)
        
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
                
        self.spatial_extent.set_north(kvp["north"])
        self.spatial_extent.set_south(kvp["south"])
        self.spatial_extent.set_west(kvp["west"])
        self.spatial_extent.set_east(kvp["east"])
        self.spatial_extent.set_top(kvp["top"])
        self.spatial_extent.set_bottom(kvp["bottom"])
        
        # Fill metadata
        
        self.metadata.set_nsres(kvp["nsres"])
        self.metadata.set_ewres(kvp["ewres"])
        self.metadata.set_tbres(kvp["tbres"])
        self.metadata.set_datatype(kvp["datatype"])
        self.metadata.set_min(kvp["min"])
        self.metadata.set_max(kvp["max"])
        
        rows = (kvp["north"] - kvp["south"])/kvp["nsres"]
        cols = (kvp["east"] - kvp["west"])/kvp["ewres"]
        depths = (kvp["top"] - kvp["bottom"])/kvp["tbres"]
        
        ncells = cols * rows * depths
        
        self.metadata.set_cols(cols)
        self.metadata.set_rows(depths)
        self.metadata.set_depths(rows)
        self.metadata.set_number_of_cells(ncells)
        

###############################################################################

class vector_dataset(abstract_dataset):
    def __init__(self, ident):
	self.reset(ident)
        
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
                
        self.spatial_extent.set_north(kvp["north"])
        self.spatial_extent.set_south(kvp["south"])
        self.spatial_extent.set_west(kvp["west"])
        self.spatial_extent.set_east(kvp["east"])
        self.spatial_extent.set_top(kvp["top"])
        self.spatial_extent.set_bottom(kvp["bottom"])
        
        # Fill metadata .. no metadata yet
        
