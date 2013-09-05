"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS base classes to be used in other
Python temporal gis packages.

This packages includes all base classes to store basic information
like id, name, mapset creation and modification time as well as sql
serialization and de-serialization and the sql database interface.

Usage:

@code

>>> import grass.temporal as tgis
>>> tgis.init()
>>> rbase = tgis.RasterBase(ident="soil@PERMANENT")
>>> vbase = tgis.VectorBase(ident="soil:1@PERMANENT")
>>> r3base = tgis.Raster3DBase(ident="soil@PERMANENT")
>>> strdsbase = tgis.STRDSBase(ident="soil@PERMANENT")
>>> stvdsbase = tgis.STVDSBase(ident="soil@PERMANENT")
>>> str3dsbase = tgis.STR3DSBase(ident="soil@PERMANENT")

@endcode

(C) 2011-2012 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from datetime import datetime, date, time, timedelta
from core import *

###############################################################################


class DictSQLSerializer(object):
    def __init__(self):
        self.D = {}

    def serialize(self, type, table, where=None):
        """!Convert the internal dictionary into a string of semicolon
            separated SQL statements The keys are the column names and
            the values are the row entries

            Usage:

            \code
            >>> init()
            >>> t = DictSQLSerializer()
            >>> t.D["id"] = "soil@PERMANENT"
            >>> t.D["name"] = "soil"
            >>> t.D["mapset"] = "PERMANENT"
            >>> t.D["creator"] = "soeren"
            >>> t.D["creation_time"] = datetime(2001,1,1)
            >>> t.D["modification_time"] = datetime(2001,1,1)
            >>> t.serialize(type="SELECT", table="raster_base")
            ('SELECT  name  , creator  , creation_time  , modification_time  , mapset  , id  FROM raster_base ;\\n', ())
            >>> t.serialize(type="INSERT", table="raster_base")
            ('INSERT INTO raster_base ( name  ,creator  ,creation_time  ,modification_time  ,mapset  ,id ) VALUES (? ,? ,? ,? ,? ,?) ;\\n', ('soil', 'soeren', datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2001, 1, 1, 0, 0), 'PERMANENT', 'soil@PERMANENT'))
            >>> t.serialize(type="UPDATE", table="raster_base")
            ('UPDATE raster_base SET  name = ?  ,creator = ?  ,creation_time = ?  ,modification_time = ?  ,mapset = ?  ,id = ? ;\\n', ('soil', 'soeren', datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2001, 1, 1, 0, 0), 'PERMANENT', 'soil@PERMANENT'))
            >>> t.serialize(type="UPDATE ALL", table="raster_base")
            ('UPDATE raster_base SET  name = ?  ,creator = ?  ,creation_time = ?  ,modification_time = ?  ,mapset = ?  ,id = ? ;\\n', ('soil', 'soeren', datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2001, 1, 1, 0, 0), 'PERMANENT', 'soil@PERMANENT'))

            @param type must be SELECT. INSERT, UPDATE
            @param table The name of the table to select, insert or update
            @param where The optional where statement
            @return a tuple containing the SQL string and the arguments

            \endcode
        """

        dbif =  SQLDatabaseInterfaceConnection()

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
            sql += ";\n"

        # Create insert statement
        if type == "INSERT":
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
                    if dbif.dbmi.paramstyle == "qmark":
                        sql += '?'
                    else:
                        sql += '%s'
                else:
                    if dbif.dbmi.paramstyle == "qmark":
                        sql += ' ,?'
                    else:
                        sql += ' ,%s'
                count += 1
                args.append(self.D[key])
            sql += ') '

            if where:
                sql += where
            sql += ";\n"

        # Create update statement for existing entries
        if type == "UPDATE":
            count = 0
            sql += 'UPDATE ' + table + ' SET '
            for key in self.D.keys():
                # Update only entries which are not None
                if self.D[key] is not None:
                    if count == 0:
                        if dbif.dbmi.paramstyle == "qmark":
                            sql += ' %s = ? ' % key
                        else:
                            sql += ' %s ' % key
                            sql += '= %s '
                    else:
                        if dbif.dbmi.paramstyle == "qmark":
                            sql += ' ,%s = ? ' % key
                        else:
                            sql += ' ,%s ' % key
                            sql += '= %s '
                    count += 1
                    args.append(self.D[key])
            if where:
                sql += where
            sql += ";\n"

        # Create update statement for all entries
        if type == "UPDATE ALL":
            count = 0
            sql += 'UPDATE ' + table + ' SET '
            for key in self.D.keys():
                if count == 0:
                    if dbif.dbmi.paramstyle == "qmark":
                        sql += ' %s = ? ' % key
                    else:
                        sql += ' %s ' % key
                        sql += '= %s '
                else:
                    if dbif.dbmi.paramstyle == "qmark":
                        sql += ' ,%s = ? ' % key
                    else:
                        sql += ' ,%s ' % key
                        sql += '= %s '
                count += 1
                args.append(self.D[key])
            if where:
                sql += where
            sql += ";\n"

        return sql, tuple(args)

    def deserialize(self, row):
        """!Convert the content of the dbmi dictionary like row into the
           internal dictionary

           @param row The dictionary like row to store in the internal dict
        """
        self.D = {}
        for key in row.keys():
            self.D[key] = row[key]

    def clear(self):
        """!Initialize the internal storage"""
        self.D = {}

    def print_self(self):
        """!Print the content of the internal dictionary to stdout
        """
        print self.D

###############################################################################

class SQLDatabaseInterface(DictSQLSerializer):
    """!This class represents the SQL database interface

       Functions to insert, select and update the internal
       structure of this class in the temporal database are implemented.
       This is the base class for raster, raster3d, vector and
       space time datasets data management classes:
       - Identification information (base)
       - Spatial extent
       - Temporal extent
       - Metadata

       Usage:

       \code

        >>> init()
        >>> t = SQLDatabaseInterface("raster", "soil@PERMANENT")
        >>> t.D["name"] = "soil"
        >>> t.D["mapset"] = "PERMANENT"
        >>> t.D["creator"] = "soeren"
        >>> t.D["creation_time"] = datetime(2001,1,1)
        >>> t.get_delete_statement()
        "DELETE FROM raster WHERE id = 'soil@PERMANENT';\\n"
        >>> t.get_is_in_db_statement()
        "SELECT id FROM raster WHERE id = 'soil@PERMANENT';\\n"
        >>> t.get_select_statement()
        ("SELECT  creation_time  , mapset  , name  , creator  FROM raster WHERE id = 'soil@PERMANENT';\\n", ())
        >>> t.get_select_statement_mogrified()
        "SELECT  creation_time  , mapset  , name  , creator  FROM raster WHERE id = 'soil@PERMANENT';\\n"
        >>> t.get_insert_statement()
        ('INSERT INTO raster ( creation_time  ,mapset  ,name  ,creator ) VALUES (? ,? ,? ,?) ;\\n', (datetime.datetime(2001, 1, 1, 0, 0), 'PERMANENT', 'soil', 'soeren'))
        >>> t.get_insert_statement_mogrified()
        "INSERT INTO raster ( creation_time  ,mapset  ,name  ,creator ) VALUES ('2001-01-01 00:00:00' ,'PERMANENT' ,'soil' ,'soeren') ;\\n"
        >>> t.get_update_statement()
        ("UPDATE raster SET  creation_time = ?  ,mapset = ?  ,name = ?  ,creator = ? WHERE id = 'soil@PERMANENT';\\n", (datetime.datetime(2001, 1, 1, 0, 0), 'PERMANENT', 'soil', 'soeren'))
        >>> t.get_update_statement_mogrified()
        "UPDATE raster SET  creation_time = '2001-01-01 00:00:00'  ,mapset = 'PERMANENT'  ,name = 'soil'  ,creator = 'soeren' WHERE id = 'soil@PERMANENT';\\n"
        >>> t.get_update_all_statement()
        ("UPDATE raster SET  creation_time = ?  ,mapset = ?  ,name = ?  ,creator = ? WHERE id = 'soil@PERMANENT';\\n", (datetime.datetime(2001, 1, 1, 0, 0), 'PERMANENT', 'soil', 'soeren'))
        >>> t.get_update_all_statement_mogrified()
        "UPDATE raster SET  creation_time = '2001-01-01 00:00:00'  ,mapset = 'PERMANENT'  ,name = 'soil'  ,creator = 'soeren' WHERE id = 'soil@PERMANENT';\\n"

        \endcode
    """
    def __init__(self, table=None, ident=None):
        """!Constructor of this class

           @param table The name of the table
           @param ident The identifier (primary key) of this
                         object in the database table
        """
        DictSQLSerializer.__init__(self)

        self.table = table  # Name of the table, set in the subclass
        self.ident = ident

    def get_table_name(self):
        """!Return the name of the table in which the internal
           data are inserted, updated or selected
           @return The name of the table
           """
        return self.table

    def get_delete_statement(self):
        """!Return the delete string
           @return The DELETE string
        """
        return "DELETE FROM " + self.get_table_name() + \
               " WHERE id = \'" + str(self.ident) + "\';\n"

    def delete(self, dbif=None):
        """!Delete the entry of this object from the temporal database

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
        """
        sql = self.get_delete_statement()
        #print sql

        if dbif:
            dbif.cursor.execute(sql)
        else:
            dbif = SQLDatabaseInterfaceConnection()
            dbif.connect()
            dbif.cursor.execute(sql)
            dbif.close()

    def get_is_in_db_statement(self):
        """!Return the selection string that checks if this object is registered in the
           temporal database
           @return The SELECT string
        """
        return "SELECT id FROM " + self.get_table_name() + \
               " WHERE id = \'" + str(self.ident) + "\';\n"

    def is_in_db(self, dbif=None):
        """!Check if this object is present in the temporal database

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
           @return True if this object is present in the temporal database, False otherwise
        """

        sql = self.get_is_in_db_statement()
        #print sql

        if dbif:
            dbif.cursor.execute(sql)
            row = dbif.cursor.fetchone()
        else:
            dbif = SQLDatabaseInterfaceConnection()
            dbif.connect()
            dbif.cursor.execute(sql)
            row = dbif.cursor.fetchone()
            dbif.close()

        # Nothing found
        if row is None:
            return False

        return True

    def get_select_statement(self):
        """!Return the sql statement and the argument list in
           database specific style
           @return The SELECT string
        """
        return self.serialize("SELECT", self.get_table_name(),
                              "WHERE id = \'" + str(self.ident) + "\'")

    def get_select_statement_mogrified(self, dbif=None):
        """!Return the select statement as mogrified string

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
           @return The SELECT string
        """
        if not dbif:
            dbif = SQLDatabaseInterfaceConnection()

        return dbif.mogrify_sql_statement(self.get_select_statement())

    def select(self, dbif=None):
        """!Select the content from the temporal database and store it
           in the internal dictionary structure

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
        """
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
            dbif = SQLDatabaseInterfaceConnection()
            dbif.connect()
            if len(args) == 0:
                dbif.cursor.execute(sql)
            else:
                dbif.cursor.execute(sql, args)
            row = dbif.cursor.fetchone()
            dbif.close()

        # Nothing found
        if row is None:
            return False

        if len(row) > 0:
            self.deserialize(row)
        else:
            core.fatal(_("Object <%s> not found in the temporal database")
                       % self.get_id())

        return True

    def get_insert_statement(self):
        """!Return the sql statement and the argument
           list in database specific style
           @return The INSERT string"""
        return self.serialize("INSERT", self.get_table_name())

    def get_insert_statement_mogrified(self, dbif=None):
        """!Return the insert statement as mogrified string

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
           @return The INSERT string
        """
        if not dbif:
            dbif = SQLDatabaseInterfaceConnection()

        return dbif.mogrify_sql_statement(self.get_insert_statement())

    def insert(self, dbif=None):
        """!Serialize the content of this object and store it in the temporal
           database using the internal identifier

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
        """
        sql, args = self.get_insert_statement()
        #print sql
        #print args

        if dbif:
            dbif.cursor.execute(sql, args)
        else:
            dbif = SQLDatabaseInterfaceConnection()
            dbif.connect()
            dbif.cursor.execute(sql, args)
            dbif.close()

    def get_update_statement(self, ident=None):
        """!Return the sql statement and the argument list
           in database specific style

           @param ident The identifier to be updated, useful for renaming
           @return The UPDATE string

           """
        if ident:
            return self.serialize("UPDATE", self.get_table_name(),
                              "WHERE id = \'" + str(ident) + "\'")
        else:
            return self.serialize("UPDATE", self.get_table_name(),
                              "WHERE id = \'" + str(self.ident) + "\'")

    def get_update_statement_mogrified(self, dbif=None, ident=None):
        """!Return the update statement as mogrified string

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
           @param ident The identifier to be updated, useful for renaming
           @return The UPDATE string
        """
        if not dbif:
            dbif = SQLDatabaseInterfaceConnection()

        return dbif.mogrify_sql_statement(self.get_update_statement(ident))

    def update(self, dbif=None, ident=None):
        """!Serialize the content of this object and update it in the temporal
           database using the internal identifier

           Only object entries which are exists (not None) are updated

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
           @param ident The identifier to be updated, useful for renaming
        """
        if self.ident is None:
            raise IOError("Missing identifer")

        sql, args = self.get_update_statement(ident)
        #print sql
        #print args

        if dbif:
            dbif.cursor.execute(sql, args)
        else:
            dbif = SQLDatabaseInterfaceConnection()
            dbif.connect()
            dbif.cursor.execute(sql, args)
            dbif.close()

    def get_update_all_statement(self, ident=None):
        """!Return the sql statement and the argument
           list in database specific style

           @param ident The identifier to be updated, useful for renaming
           @return The UPDATE string
           """
        if ident:
            return self.serialize("UPDATE ALL", self.get_table_name(),
                              "WHERE id = \'" + str(ident) + "\'")
        else:
            return self.serialize("UPDATE ALL", self.get_table_name(),
                              "WHERE id = \'" + str(self.ident) + "\'")

    def get_update_all_statement_mogrified(self, dbif=None, ident=None):
        """!Return the update all statement as mogrified string

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
           @param ident The identifier to be updated, useful for renaming
           @return The UPDATE string
        """
        if not dbif:
            dbif = SQLDatabaseInterfaceConnection()

        return dbif.mogrify_sql_statement(self.get_update_all_statement(ident))

    def update_all(self, dbif=None, ident=None):
        """!Serialize the content of this object, including None objects,
        and update it in the temporal database using the internal identifier

           @param dbif The database interface to be used,
                        if None a temporary connection will be established
           @param ident The identifier to be updated, useful for renaming
        """
        if self.ident is None:
            raise IOError("Missing identifer")

        sql, args = self.get_update_all_statement(ident)
        #print sql
        #print args

        if dbif:
            dbif.cursor.execute(sql, args)
        else:
            dbif = SQLDatabaseInterfaceConnection()
            dbif.connect()
            dbif.cursor.execute(sql, args)
            dbif.close()

###############################################################################


class DatasetBase(SQLDatabaseInterface):
    """!This is the base class for all maps and spacetime datasets storing
        basic identification information

        Usage:

        \code

        >>> init()
        >>> t = DatasetBase("raster", "soil@PERMANENT", creator="soeren", ctime=datetime(2001,1,1), ttype="absolute")
        >>> t.id
        'soil@PERMANENT'
        >>> t.name
        'soil'
        >>> t.mapset
        'PERMANENT'
        >>> t.creator
        'soeren'
        >>> t.ctime
        datetime.datetime(2001, 1, 1, 0, 0)
        >>> t.ttype
        'absolute'
        >>> t.print_info()
         +-------------------- Basic information -------------------------------------+
         | Id: ........................ soil@PERMANENT
         | Name: ...................... soil
         | Mapset: .................... PERMANENT
         | Creator: ................... soeren
         | Creation time: ............. 2001-01-01 00:00:00
         | Temporal type: ............. absolute
        >>> t.print_shell_info()
        id=soil@PERMANENT
        name=soil
        mapset=PERMANENT
        creator=soeren
        creation_time=2001-01-01 00:00:00
        temporal_type=absolute

        \endcode
    """

    def __init__(self, table=None, ident=None, name=None, mapset=None,
                 creator=None, ctime=None,ttype=None):
        """!Constructor

            @param table The name of the temporal database table
                          that should be used to store the values
            @param ident The unique identifier must be a combination of
                          the dataset name, layer name and the mapset
                          "name@mapset" or "name:layer@mapset"
                          used as as primary key in the temporal database
            @param name The name of the map or dataset
            @param mapset The name of the mapset
            @param creator The name of the creator
            @param ctime The creation datetime object
            @param ttype The temporal type
                - "absolute" Identifier for absolute time
                - "relative" Identifier for relative time
        """

        SQLDatabaseInterface.__init__(self, table, ident)

        self.set_id(ident)
        if ident is not None and name is None and mapset is None:
            if ident.find("@") >= 0:
                name, mapset = ident.split("@")
            if name.find(":") >= 0:
                name, layer = ident.split(":")
        self.set_name(name)
        self.set_mapset(mapset)
        self.set_creator(creator)
        self.set_ctime(ctime)
        self.set_ttype(ttype)
        # Commented out for performance reasons
        #self.set_mtime(mtime)
        #self.set_revision(revision)

    def set_id(self, ident):
        """!Convenient method to set the unique identifier (primary key)

           @param ident The unique identifier must be a combination
                         of the dataset name, layer name and the mapset
                         "name@mapset" or "name:layer@mapset"
        """
        self.ident = ident
        self.D["id"] = ident

        if ident is not None:
            if ident.find("@") >= 0:
                name, mapset = ident.split("@")
                self.set_mapset(mapset)
                self.set_name(name)
            else:
                core.fatal(_("Wrong identifier, the mapset is missing"))
            if name.find(":") >= 0:
                name, layer = ident.split(":")
                self.set_layer(layer)
            self.set_name(name)


    def set_name(self, name):
        """!Set the name of the dataset

           @param name The name of the dataset
        """
        self.D["name"] = name

    def set_mapset(self, mapset):
        """!Set the mapset of the dataset

           @param mapset The name of the mapset in which this dataset is stored
        """
        self.D["mapset"] = mapset

    def set_layer(self, layer):
        """!Convenient method to set the layer of the map (part of primary key)

           Layer are supported for vector maps

           @param layer The layer of the map
        """
        self.D["layer"] = layer

    def set_creator(self, creator):
        """!Set the creator of the dataset

           @param creator The name of the creator
        """
        self.D["creator"] = creator

    def set_ctime(self, ctime=None):
        """!Set the creation time of the dataset,
           if nothing set the current time is used

           @param ctime The current time of type datetime
        """
        if ctime is None:
            self.D["creation_time"] = datetime.today()
        else:
            self.D["creation_time"] = ctime

    def set_ttype(self, ttype):
        """!Set the temporal type of the dataset: absolute or relative,
           if nothing set absolute time will assumed

           @param ttype The temporal type of the dataset "absolute or relative"
        """
        if ttype is None or (ttype != "absolute" and ttype != "relative"):
            self.D["temporal_type"] = "absolute"
        else:
            self.D["temporal_type"] = ttype

#    def set_mtime(self, mtime=None):
#       """!Set the modification time of the map, if nothing set the current time is used"""
#       if mtime == None:
#            self.D["modification_time"] = datetime.now()
#       else:
#            self.D["modification_time"] = mtime

#    def set_revision(self, revision=1):
#       """!Set the revision of the map: if nothing set revision 1 will assumed"""
#       self.D["revision"] = revision

    def get_id(self):
        """!Convenient method to get the unique identifier (primary key)

           @return None if not found
        """
        if "id" in self.D:
            return self.D["id"]
        else:
            return None

    def get_map_id(self):
        """!Convenient method to get the unique map identifier
           without layer information

           @return the name of the vector map as "name@mapset"
                  or None in case the id was not set
        """
        if self.id:
            if self.id.find(":") >= 0:
                # Remove the layer identifier from the id
                return self.id.split("@")[0].split(":")[0] + "@" + \
                       self.id.split("@")[1]
            else:
                return self.id
        else:
            return None

    def get_layer(self):
        """!Convenient method to get the layer of the map (part of primary key)

           Layer are currently supported for vector maps

           @return None if not found
        """
        if "layer" in self.D:
            return self.D["layer"]
        else:
            return None

    def get_name(self):
        """!Get the name of the dataset
           @return None if not found"""
        if "name" in self.D:
            return self.D["name"]
        else:
            return None

    def get_mapset(self):
        """!Get the name of mapset of this dataset
           @return None if not found"""
        if "mapset" in self.D:
            return self.D["mapset"]
        else:
            return None

    def get_creator(self):
        """!Get the creator of the dataset
           @return None if not found"""
        if "creator" in self.D:
            return self.D["creator"]
        else:
            return None

    def get_ctime(self):
        """!Get the creation time of the dataset, datatype is datetime
           @return None if not found"""
        if "creation_time" in self.D:
            return self.D["creation_time"]
        else:
            return None

    def get_ttype(self):
        """!Get the temporal type of the map
           @return None if not found"""
        if "temporal_type" in self.D:
            return self.D["temporal_type"]
        else:
            return None

#    def get_mtime(self):
#       """!Get the modification time of the map, datatype is datetime
#          @return None if not found"""
#       if self.D.has_key("modification_time"):
#           return self.D["modification_time"]
#       else:
#           return None

#    def get_revision(self):
#       """!Get the revision of the map
#          @return None if not found"""
#       if self.D.has_key("revision"):
#           return self.D["revision"]
#       else:
#           return None

    # Properties of this class
    id = property(fget=get_id, fset=set_id)
    map_id = property(fget=get_map_id, fset=None)
    name = property(fget=get_name, fset=set_name)
    mapset = property(fget=get_mapset, fset=set_mapset)
    ctime = property(fget=get_ctime, fset=set_ctime)
    ttype = property(fget=get_ttype, fset=set_ttype)
    creator = property(fget=get_creator, fset=set_creator)

    def print_info(self):
        """!Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " +-------------------- Basic information -------------------------------------+"
        print " | Id: ........................ " + str(self.get_id())
        print " | Name: ...................... " + str(self.get_name())
        print " | Mapset: .................... " + str(self.get_mapset())
        if self.get_layer():
            print " | Layer:...................... " + str(self.get_layer())
        print " | Creator: ................... " + str(self.get_creator())
        print " | Creation time: ............. " + str(self.get_ctime())
        print " | Temporal type: ............. " + str(self.get_ttype())
#        print " | Modification time: ......... " + str(self.get_mtime())
#        print " | Revision in database: ...... " + str(self.get_revision())

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        print "id=" + str(self.get_id())
        print "name=" + str(self.get_name())
        print "mapset=" + str(self.get_mapset())
        if self.get_layer():
            print "layer=" + str(self.get_layer())
        print "creator=" + str(self.get_creator())
        print "creation_time=" + str(self.get_ctime())
        print "temporal_type=" + str(self.get_ttype())
#        print "modification_time=" + str(self.get_mtime())
#        print "revision=" + str(self.get_revision())

###############################################################################


class RasterBase(DatasetBase):
    """!Time stamped raster map base information class"""
    def __init__(self, ident=None, name=None, mapset=None, creator=None,
                 creation_time=None, temporal_type=None):
        DatasetBase.__init__(self, "raster_base", ident, name, mapset,
                              creator, creation_time, temporal_type)


class Raster3DBase(DatasetBase):
    """!Time stamped 3D raster map base information class"""
    def __init__(self, ident=None, name=None, mapset=None, creator=None,
                 creation_time=None, temporal_type=None,):
        DatasetBase.__init__(self, "raster3d_base", ident, name,
                              mapset, creator, creation_time,
                              temporal_type)


class VectorBase(DatasetBase):
    """!Time stamped vector map base information class"""
    def __init__(self, ident=None, name=None, mapset=None, layer=None,
                 creator=None, creation_time=None, temporal_type=None):
        DatasetBase.__init__(self, "vector_base", ident, name, mapset,
                              creator, creation_time, temporal_type)

        self.set_id(ident)
        if ident is not None and name is None and mapset is None:
            if ident.find("@") >= 0:
                name, mapset = ident.split("@")
            if layer is None:
                if name.find(":") >= 0:
                    name, layer = name.split(":")
        self.set_name(name)
        self.set_mapset(mapset)
        # Layer currently only in use by vector maps
        self.set_layer(layer)

###############################################################################


class STDSBase(DatasetBase):
    """!Base class for space time datasets

       This class adds the semantic type member variable to the dataset
       base class.

    Usage:

    \code

    >>> init()
    >>> t = STDSBase("stds", "soil@PERMANENT", semantic_type="average", creator="soeren", ctime=datetime(2001,1,1), ttype="absolute")
    >>> t.semantic_type
    'average'
    >>> t.print_info()
     +-------------------- Basic information -------------------------------------+
     | Id: ........................ soil@PERMANENT
     | Name: ...................... soil
     | Mapset: .................... PERMANENT
     | Creator: ................... soeren
     | Creation time: ............. 2001-01-01 00:00:00
     | Temporal type: ............. absolute
     | Semantic type:.............. average
    >>> t.print_shell_info()
    id=soil@PERMANENT
    name=soil
    mapset=PERMANENT
    creator=soeren
    creation_time=2001-01-01 00:00:00
    temporal_type=absolute
    semantic_type=average

    \endcode
    """
    def __init__(self, table=None, ident=None, name=None, mapset=None,
                 semantic_type=None, creator=None, ctime=None,
                 ttype=None):
        DatasetBase.__init__(self, table, ident, name, mapset, creator,
                              ctime, ttype)

        self.set_semantic_type(semantic_type)

    def set_semantic_type(self, semantic_type):
        """!Set the semantic type of the space time dataset"""
        self.D["semantic_type"] = semantic_type

    def get_semantic_type(self):
        """!Get the semantic type of the space time dataset
           @return None if not found"""
        if "semantic_type" in self.D:
            return self.D["semantic_type"]
        else:
            return None

    semantic_type = property(fget=get_semantic_type, fset=set_semantic_type)

    def print_info(self):
        """!Print information about this class in human readable style"""
        DatasetBase.print_info(self)
        #      0123456789012345678901234567890
        print " | Semantic type:.............. " + str(
            self.get_semantic_type())

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        DatasetBase.print_shell_info(self)
        print "semantic_type=" + str(self.get_semantic_type())

###############################################################################


class STRDSBase(STDSBase):
    """!Space time raster dataset base information class"""
    def __init__(self, ident=None, name=None, mapset=None,
                 semantic_type=None, creator=None, ctime=None,
                 ttype=None):
        STDSBase.__init__(self, "strds_base", ident, name, mapset,
                           semantic_type, creator, ctime,
                           ttype)


class STR3DSBase(STDSBase):
    """!Space time 3D raster dataset base information class"""
    def __init__(self, ident=None, name=None, mapset=None,
                 semantic_type=None, creator=None, ctime=None,
                 ttype=None):
        STDSBase.__init__(self, "str3ds_base", ident, name, mapset,
                           semantic_type, creator, ctime,
                           ttype)


class STVDSBase(STDSBase):
    """!Space time vector dataset base information class"""
    def __init__(self, ident=None, name=None, mapset=None,
                 semantic_type=None, creator=None, ctime=None,
                 ttype=None):
        STDSBase.__init__(self, "stvds_base", ident, name, mapset,
                           semantic_type, creator, ctime,
                           ttype)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
