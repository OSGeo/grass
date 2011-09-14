"""!@package grass.script.tgis_core

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS core functions to be used in Python tgis packages.

This class provides the SQL interface for serialization and deserialization
of map and space time dataset data.

Usage:

@code
from grass.script import tgis_core as grass

grass.create_temporal_database()
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import os
import sqlite3
import core
from datetime import datetime, date, time

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

class dict_sql_serializer(object):
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
	return "DELETE FROM " + self.get_table_name() + " WHERE id = \"" + str(self.ident) + "\""
    
    def delete(self):
	self.connect()
	sql = self.get_delete_statement()
        #print sql
        self.cursor.execute(sql)
	self.close()

    def get_is_in_db_statement(self):
	return "SELECT id FROM " + self.get_table_name() + " WHERE id = \"" + str(self.ident) + "\""
    
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
	return self.serialize("SELECT", self.get_table_name(), "WHERE id = \"" + str(self.ident) + "\"")

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
	return self.serialize("INSERT", self.get_table_name())

    def insert(self):
	self.connect()
	sql, args = self.get_insert_statement()
	#print sql
	#print args
        self.cursor.execute(sql, args)
	self.close()

    def get_update_statement(self):
	return self.serialize("UPDATE", self.get_table_name(), "WHERE id = \"" + str(self.ident) + "\"")

    def update(self):
	if self.ident == None:
	    raise IOError("Missing identifer");

	sql, args = self.get_update_statement()
	#print sql
	#print args
	self.connect()
        self.cursor.execute(sql, args)
	self.close()
