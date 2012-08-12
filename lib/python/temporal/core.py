"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS core functions to be used in library modules and scripts.

This module provides the functionality to create the temporal
SQL database and to establish a connection to the database.

Usage:

\code

>>> import grass.temporal as tgis
>>> # Create the temporal database
>>> tgis.create_temporal_database()
>>> # Establish a database connection
>>> dbif, connected = tgis.init_dbif(None)
>>> dbif.connect()
>>> # Execute a SQL statement
>>> dbif.execute_transaction("SELECT datetime(0, 'unixepoch', 'localtime');")
>>> # Mogrify an SQL statement
>>> dbif.mogrify_sql_statement(["SELECT name from raster_base where name = ?", 
... ("precipitation",)])
"SELECT name from raster_base where name = 'precipitation'"
>>> dbif.close()

\endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import os
import copy
import grass.script.core as core

###############################################################################

# The chosen DBMI back-end can be defined on runtime
# Check the grass environment before import
core.run_command("t.connect", flags="c")
kv = core.parse_command("t.connect", flags="pg")
if "driver" in kv:
    if kv["driver"] == "sqlite":
        import sqlite3 as dbmi
    elif kv["driver"] == "pg":
        import psycopg2 as dbmi
        # Needed for dictionary like cursors
        import psycopg2.extras
    else:
        core.fatal(_("Unable to initialize the temporal DBMI interface. Use "
                     "t.connect to specify the driver and the database string"))
else:
    # Use the default sqlite variable
    core.run_command("t.connect", flags="d")
    import sqlite3 as dbmi

###############################################################################


def get_temporal_dbmi_init_string():
    kv = core.parse_command("t.connect", flags="pg")
    grassenv = core.gisenv()
    if dbmi.__name__ == "sqlite3":
        if "database" in kv:
            string = kv["database"]
            string = string.replace("$GISDBASE", grassenv["GISDBASE"])
            string = string.replace(
                "$LOCATION_NAME", grassenv["LOCATION_NAME"])
            return string
        else:
            core.fatal(_("Unable to initialize the temporal GIS DBMI "
                         "interface. Use t.connect to specify the driver "
                         "and the database string"))
    elif dbmi.__name__ == "psycopg2":
        if "database" in kv:
            string = kv["database"]
            return string
        else:
            core.fatal(_("Unable to initialize the temporal GIS DBMI "
                         "interface. Use t.connect to specify the driver "
                         "and the database string"))
            return "dbname=grass_test user=soeren password=abcdefgh"

###############################################################################


def get_sql_template_path():
    base = os.getenv("GISBASE")
    base_etc = os.path.join(base, "etc")
    return os.path.join(base_etc, "sql")

###############################################################################


def create_temporal_database():
    """!This function creates the grass location database structure for raster, 
       vector and raster3d maps as well as for the space-time datasets strds, 
       str3ds and stvds

       This functions must be called before any spatio-temporal processing 
       can be started
    """

    database = get_temporal_dbmi_init_string()

    db_exists = False

    # Check if the database already exists
    if dbmi.__name__ == "sqlite3":
        # Check path of the sqlite database
        if os.path.exists(database):
            db_exists = True
    elif dbmi.__name__ == "psycopg2":
        # Connect to database
        connection = dbmi.connect(database)
        cursor = connection.cursor()
        # Check for raster_base table
        cursor.execute("SELECT EXISTS(SELECT * FROM information_schema.tables "
                       "WHERE table_name=%s)", ('raster_base',))
        db_exists = cursor.fetchone()[0]
        connection.commit()
        cursor.close()

    if db_exists == True:
        return

    core.message(_("Create temporal database: %s" % (database)))

    # Read all SQL scripts and templates
    map_tables_template_sql = open(os.path.join(
        get_sql_template_path(), "map_tables_template.sql"), 'r').read()
    raster_metadata_sql = open(os.path.join(
        get_sql_template_path(), "raster_metadata_table.sql"), 'r').read()
    raster3d_metadata_sql = open(os.path.join(get_sql_template_path(
        ), "raster3d_metadata_table.sql"), 'r').read()
    vector_metadata_sql = open(os.path.join(
        get_sql_template_path(), "vector_metadata_table.sql"), 'r').read()
    stds_tables_template_sql = open(os.path.join(
        get_sql_template_path(), "stds_tables_template.sql"), 'r').read()
    strds_metadata_sql = open(os.path.join(
        get_sql_template_path(), "strds_metadata_table.sql"), 'r').read()
    str3ds_metadata_sql = open(os.path.join(
        get_sql_template_path(), "str3ds_metadata_table.sql"), 'r').read()
    stvds_metadata_sql = open(os.path.join(
        get_sql_template_path(), "stvds_metadata_table.sql"), 'r').read()

    # Create the raster, raster3d and vector tables
    raster_tables_sql = map_tables_template_sql.replace("GRASS_MAP", "raster")
    vector_tables_sql = map_tables_template_sql.replace("GRASS_MAP", "vector")
    raster3d_tables_sql = map_tables_template_sql.replace(
        "GRASS_MAP", "raster3d")

    # Create the space-time raster, raster3d and vector dataset tables
    strds_tables_sql = stds_tables_template_sql.replace("STDS", "strds")
    stvds_tables_sql = stds_tables_template_sql.replace("STDS", "stvds")
    str3ds_tables_sql = stds_tables_template_sql.replace("STDS", "str3ds")

    # Connect to database
    connection = dbmi.connect(database)
    cursor = connection.cursor()

    if dbmi.__name__ == "sqlite3":

        sqlite3_delete_trigger_sql = open(os.path.join(get_sql_template_path(
        ), "sqlite3_delete_trigger.sql"), 'r').read()

        # Execute the SQL statements for sqlite
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
        cursor.executescript(sqlite3_delete_trigger_sql)
    elif dbmi.__name__ == "psycopg2":
        # Execute the SQL statements for postgresql
        # Create the global tables for the native grass datatypes
        cursor.execute(raster_tables_sql)
        cursor.execute(raster_metadata_sql)
        cursor.execute(vector_tables_sql)
        cursor.execute(vector_metadata_sql)
        cursor.execute(raster3d_tables_sql)
        cursor.execute(raster3d_metadata_sql)
        # Create the tables for the new space-time datatypes
        cursor.execute(strds_tables_sql)
        cursor.execute(strds_metadata_sql)
        cursor.execute(stvds_tables_sql)
        cursor.execute(stvds_metadata_sql)
        cursor.execute(str3ds_tables_sql)
        cursor.execute(str3ds_metadata_sql)

    connection.commit()
    cursor.close()

###############################################################################


class SQLDatabaseInterfaceConnection():
    """!This class represents the database interface connection

       The following DBMS are supported:
       * sqlite via the sqlite3 standard library
       * postgresql via psycopg2

    """
    def __init__(self):
        self.connected = False

    def connect(self):
        """!Connect to the DBMI to execute SQL statements

           Supported backends are sqlite3 and postgresql
        """
        init = get_temporal_dbmi_init_string()
        #print "Connect to",  self.database
        if dbmi.__name__ == "sqlite3":
            self.connection = dbmi.connect(init, 
                    detect_types=dbmi.PARSE_DECLTYPES | dbmi.PARSE_COLNAMES)
            self.connection.row_factory = dbmi.Row
            self.connection.isolation_level = None
            self.cursor = self.connection.cursor()
            self.cursor.execute("PRAGMA synchronous = OFF")
            self.cursor.execute("PRAGMA journal_mode = MEMORY")
        elif dbmi.__name__ == "psycopg2":
            self.connection = dbmi.connect(init)
            #self.connection.set_isolation_level(dbmi.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
            self.cursor = self.connection.cursor(
                cursor_factory=dbmi.extras.DictCursor)
        self.connected = True

    def close(self):
        """!Close the DBMI connection"""
        #print "Close connection to",  self.database
        self.connection.commit()
        self.cursor.close()
        self.connected = False

    def mogrify_sql_statement(self, content):
        """!Return the SQL statement and arguments as executable SQL string
        """
        sql = content[0]
        args = content[1]

        if dbmi.__name__ == "psycopg2":
            if len(args) == 0:
                return sql
            else:
                if self.connected:
                    try:
                        return self.cursor.mogrify(sql, args)
                    except:
                        print sql, args
                        raise
                else:
                    self.connect()
                    statement = self.cursor.mogrify(sql, args)
                    self.close()
                    return statement

        elif dbmi.__name__ == "sqlite3":
            if len(args) == 0:
                return sql
            else:
                # Unfortunately as sqlite does not support
                # the transformation of sql strings and qmarked or
                # named arguments we must make our hands dirty
                # and do it by ourself. :(
                # Doors are open for SQL injection because of the
                # limited python sqlite3 implementation!!!
                pos = 0
                count = 0
                maxcount = 100
                statement = sql

                while count < maxcount:
                    pos = statement.find("?", pos + 1)
                    if pos == -1:
                        break

                    if args[count] is None:
                        statement = "%sNULL%s" % (statement[0:pos], 
                                                  statement[pos + 1:])
                    elif isinstance(args[count], (int, long)):
                        statement = "%s%d%s" % (statement[0:pos], args[count],
                                                statement[pos + 1:])
                    elif isinstance(args[count], float):
                        statement = "%s%f%s" % (statement[0:pos], args[count],
                                                statement[pos + 1:])
                    else:
                        # Default is a string, this works for datetime 
                        # objects too
                        statement = "%s\'%s\'%s" % (statement[0:pos], 
                                                    str(args[count]), 
                                                    statement[pos + 1:])
                    count += 1

                return statement

    def execute_transaction(self, statement):
        """!Execute a transactional SQL statement

            The BEGIN and END TRANSACTION statements will be added automatically
            to the sql statement

            @param statement The executable SQL statement or SQL script
        """
        connect = False
        if not self.connected:
            self.connect()
            connect = True

        sql_script = ""
        sql_script += "BEGIN TRANSACTION;\n"
        sql_script += statement
        sql_script += "END TRANSACTION;"
        
        try:
            if dbmi.__name__ == "sqlite3":
                self.cursor.executescript(statement)
            else:
                self.cursor.execute(statement)
            self.connection.commit()
        except:
            if connect:
                self.close()
            core.error(_("Unable to execute transaction:\n %(sql)s" % \
                         {"sql":statement}))
            raise

        if connect:
            self.close()

###############################################################################

def init_dbif(dbif):
    """!This method checks if the database interface connection exists, 
        if not a new one will be created, connected and True will be returned

        Usage code sample:
        \code
        
        dbif, connect = tgis.init_dbif(dbif)
        if connect:
            dbif.close()
        
        \code
    """
    if dbif is None:
        dbif = SQLDatabaseInterfaceConnection()
        dbif.connect()
        return dbif, True

    return dbif, False

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
