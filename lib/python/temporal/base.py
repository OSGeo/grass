"""!@package grass.script.tgis_base

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS base classes to be used in other
Python temporal gis packages.

This packages includes all base classes to stor basic information like id, name,
mapset creation and modification time as well as sql serialization and deserialization
and the sqlite3 database interface.

Usage:

@code
from grass.script import tgis_core as grass

rbase = grass.raster_base(ident="soil")
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from core import *

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
        #print "Connect to",  self.database
	self.connection = sqlite3.connect(self.database, detect_types=sqlite3.PARSE_DECLTYPES|sqlite3.PARSE_COLNAMES)
	self.connection.row_factory = sqlite3.Row
        self.cursor = self.connection.cursor()

    def close(self):
        #print "Close connection to",  self.database
	self.connection.commit()
        self.cursor.close()

    def get_delete_statement(self):
	return "DELETE FROM " + self.get_table_name() + " WHERE id = \"" + str(self.ident) + "\""

    def delete(self, dbif=None):
	sql = self.get_delete_statement()
        #print sql
        
        if dbif:
            dbif.cursor.execute(sql)
        else:
            self.connect()
            self.cursor.execute(sql)
            self.close()

    def get_is_in_db_statement(self):
	return "SELECT id FROM " + self.get_table_name() + " WHERE id = \"" + str(self.ident) + "\""

    def is_in_db(self, dbif=None):

	sql = self.get_is_in_db_statement()
        #print sql

        if dbif:
            dbif.cursor.execute(sql)
            row = dbif.cursor.fetchone()
        else:
            self.connect()
            self.cursor.execute(sql)
            row = self.cursor.fetchone()
            self.close()

	# Nothing found
	if row == None:
	    return False

	return True

    def get_select_statement(self):
	return self.serialize("SELECT", self.get_table_name(), "WHERE id = \"" + str(self.ident) + "\"")

    def select(self, dbif=None):
	sql, args = self.get_select_statement()
	#print sql
	#print args

        if dbif:
            if len(args) == 0:
                dbif.cursor.execute(sql)
            else:
                dbif.cursor.execute(sql, args)
            row = dbif.cursor.fetchone()
        else:
            self.connect()
            if len(args) == 0:
                self.cursor.execute(sql)
            else:
                self.cursor.execute(sql, args)
            row = self.cursor.fetchone()
            self.close()

	# Nothing found
	if row == None:
	    return False

	if len(row) > 0:
	    self.deserialize(row)
	else:
	    raise IOError

	return True

    def get_insert_statement(self):
	return self.serialize("INSERT", self.get_table_name())

    def insert(self, dbif=None):
	sql, args = self.get_insert_statement()
	#print sql
	#print args

        if dbif:
            dbif.cursor.execute(sql, args)
        else:
            self.connect()
            self.cursor.execute(sql, args)
            self.close()

    def get_update_statement(self):
	return self.serialize("UPDATE", self.get_table_name(), "WHERE id = \"" + str(self.ident) + "\"")

    def update(self, dbif=None):
	if self.ident == None:
	    raise IOError("Missing identifer");

	sql, args = self.get_update_statement()
	#print sql
	#print args

        if dbif:
            dbif.cursor.execute(sql, args)
        else:
            self.connect()
            self.cursor.execute(sql, args)
            self.close()

###############################################################################

class dataset_base(sql_database_interface):
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

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " +-------------------- Basic information -------------------------------------+"
        print " | Id: ........................ " + str(self.get_id())
        print " | Name: ...................... " + str(self.get_name())
        print " | Mapset: .................... " + str(self.get_mapset())
        print " | Creator: ................... " + str(self.get_creator())
        print " | Creation time: ............. " + str(self.get_ctime())
        print " | Modification time: ......... " + str(self.get_mtime())
        print " | Temporal type: ............. " + str(self.get_ttype())
        print " | Revision in database: ...... " + str(self.get_revision())
        
    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "id=" + str(self.get_id())
        print "name=" + str(self.get_name())
        print "mapset=" + str(self.get_mapset())
        print "creator=" + str(self.get_creator())
        print "creation_time=" + str(self.get_ctime())
        print "modification_time=" + str(self.get_mtime())
        print "temporal_type=" + str(self.get_ttype())
        print "revision=" + str(self.get_revision())

###############################################################################

class raster_base(dataset_base):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, "raster_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

class raster3d_base(dataset_base):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, "raster3d_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

class vector_base(dataset_base):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, "vector_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

###############################################################################

class stds_base(dataset_base):
    def __init__(self, table=None, ident=None, name=None, mapset=None, semantic_type=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, table, ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

	self.set_semantic_type(semantic_type)

    def set_semantic_type(self, semantic_type):
	"""Set the semantic type of the space time dataset"""
	self.D["semantic_type"] = semantic_type

    def get_semantic_type(self):
	"""Get the semantic type of the space time dataset
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

