"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS core functions to be used in library modules and scripts.

This module provides the functionality to create the temporal
SQL database and to establish a connection to the database.

Usage:

@code

>>> import grass.temporal as tgis
>>> # Create the temporal database
>>> tgis.init()
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

@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import os
import grass.script.core as core
from datetime import datetime
# Import all supported database backends
# Ignore import errors since they are checked later
try:
    import sqlite3
except ImportError:
    pass
# Postgresql is optional, existence is checked when needed
try:
    import psycopg2
    import psycopg2.extras
except:
    pass

# Uncomment this to raise and exception in case of a fatal error
# core.set_raise_on_error(True)

# Global variable that defines the backend
# of the temporal GIS
# It can either be "sqlite" or "pg"
tgis_backend = None

# The version of the temporal framework
# this value must be an integer larger than 0
# Increase this value in case of backward incompatible changes in the TGIS API
tgis_version=1
# The version of the temporal database since framework and database version can differ
# this value must be an integer larger than 0
# Increase this value in case of backward incompatible changes
# temporal database SQL layout
tgis_db_version=1

# We need to access the current mapset quite often in the framework, so we make
# global variable that will be initiated when init() is called
current_mapset=None

###############################################################################

def get_current_mapset():
    """!Return the current mapset

       This is the fastest way to receive the current mapset.
       The current mapset is set by init() and stored in a global variable.
       This function provides access to this global variable.
    """
    global current_mapset
    return current_mapset

def _set_current_mapset():
    """!This functions set the global current mapset variable to the current mapset
    by calling g.gisenv. 
    """
    global current_mapset
    current_mapset = core.gisenv()["MAPSET"]


###############################################################################

def get_tgis_version():
    """!Get the verion number of the temporal framework
       @return The version number of the temporal framework as string
    """
    global tgis_version
    return tgis_version

###############################################################################

def get_tgis_metadata(dbif=None):
    """!Return the tgis metadata table as a list of rows (dicts)
               or None if not present

       @param dbif The database interface to be used
       @return The selected rows with key/value comumns or None
    """

    dbif, connected = init_dbif(dbif)

    # Select metadata if the table is present
    try:
        statement = "SELECT * FROM tgis_metadata;\n"
        dbif.cursor.execute(statement)
        rows = dbif.cursor.fetchall()
    except:
        rows = None

    if connected:
        dbif.close()

    return rows

###############################################################################

def get_temporal_dbmi_init_string():
    kv = core.parse_command("t.connect", flags="pg")
    grassenv = core.gisenv()
    global tgis_backend
    if tgis_backend == "sqlite":
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
    elif tgis_backend == "pg":
        if "database" in kv:
            string = kv["database"]
            return string
    else:
        core.fatal(_("Unable to initialize the temporal GIS DBMI "
                     "interface. Use t.connect to specify the driver "
                     "and the database string"))

###############################################################################

# This variable specifies if the ctypes interface to the grass
# libraries should be used to read map specific data. If set to False
# the grass scripting library will be used to get map informations.
# The advantage of the ctypes inteface is speed, the disadvantage is that
# the GRASS C functions may call G_fatal_error() which exits the process.
# That is not catchable in Python.
use_ctypes_map_access = True

def set_use_ctypes_map_access(use_ctype = True):
    """!Define the map access method for the temporal GIS library

       Using ctypes to read map metadata is much faster
       then using the grass.script interface that calls grass modules.
       The disadvantage is that GRASS C-library function will call
       G_fatal_error() that will exit the calling process.

       GUI developer should set this flag to False.

       @param use_ctype True use ctypes interface, False use grass.script interface
    """
    global use_ctypes_map_access
    use_ctypes_map_access = use_ctype

###############################################################################

def get_use_ctypes_map_access():
    """!Return true if ctypes is used for map access """
    global use_ctypes_map_access
    return use_ctypes_map_access

###############################################################################

def get_sql_template_path():
    base = os.getenv("GISBASE")
    base_etc = os.path.join(base, "etc")
    return os.path.join(base_etc, "sql")

###############################################################################

def init():
    """!This function set the correct database backend from the environmental variables
       and creates the grass location database structure for raster,
       vector and raster3d maps as well as for the space-time datasets strds,
       str3ds and stvds in case it not exists.

        ATTENTION: This functions must be called before any spatio-temporal processing
                   can be started
    """
    # We need to set the correct database backend from the environment variables
    global tgis_backend

    # Set the global variable current_mapset for fast mapset access 
    _set_current_mapset()

    core.run_command("t.connect", flags="c")
    kv = core.parse_command("t.connect", flags="pg")

    if "driver" in kv:
        if kv["driver"] == "sqlite":
            tgis_backend = kv["driver"]
            try:
                import sqlite3
            except ImportError:
                core.error("Unable to locate the sqlite SQL Python interface module sqlite3.")
                raise
            dbmi = sqlite3
        elif kv["driver"] == "pg":
            tgis_backend = kv["driver"]
            try:
                import psycopg2
            except ImportError:
                core.error("Unable to locate the Postgresql SQL Python interface module psycopg2.")
                raise
            dbmi = psycopg2
        else:
            core.fatal(_("Unable to initialize the temporal DBMI interface. Use "
                         "t.connect to specify the driver and the database string"))
            dbmi = sqlite3
    else:
        # Set the default sqlite3 connection in case nothing was defined
        core.run_command("t.connect", flags="d")

    db_exists = False
    database = get_temporal_dbmi_init_string()
    dbif = SQLDatabaseInterfaceConnection()

    # Check if the database already exists
    if tgis_backend == "sqlite":
        # Check path of the sqlite database
        if os.path.exists(database):
            dbif.connect()
            # Check for raster_base table
            dbif.cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='raster_base';")
            name = dbif.cursor.fetchone()
            if name and name[0] == "raster_base":
                db_exists = True
            dbif.close()

    elif tgis_backend == "pg":
        # Connect to database
        dbif.connect()
        # Check for raster_base table
        dbif.cursor.execute("SELECT EXISTS(SELECT * FROM information_schema.tables "
                   "WHERE table_name=%s)", ('raster_base',))
        if dbif.cursor.fetchone()[0]:
            db_exists = True

    if db_exists:
        # Check if we have to add the command column
        add_command_col = True
        rows = get_tgis_metadata(dbif)
        if rows:
            for row in rows:
                if row["key"] == "tgis_db_version":
                    version = int(row["value"])
                    if version >= 1:
                        add_command_col = False

        if add_command_col:
            # Try to add the command column to the space time dataset metadata tables
            # this is due backward compatibility with old databases
            try:
                dbif.cursor.execute('ALTER TABLE strds_metadata ADD COLUMN command VARCHAR;')
            except:
                pass
            try:
                dbif.cursor.execute('ALTER TABLE str3ds_metadata ADD COLUMN command VARCHAR;')
            except:
                pass
            try:
                dbif.cursor.execute('ALTER TABLE stvds_metadata ADD COLUMN command VARCHAR;')
            except:
                pass

    if db_exists == True:
        dbif.close()
        return

    create_temporal_database(dbif, database)

###############################################################################

def create_temporal_database(dbif, database):
    """!This function will create the temporal database

       It will create all tables and triggers that are needed to run
       the temporal GIS

       @param dbif The database interface to be used
    """
    global tgis_backend
    global tgis_version
    global tgis_db_version

    template_path = get_sql_template_path()

    # Read all SQL scripts and templates
    map_tables_template_sql = open(os.path.join(
        template_path, "map_tables_template.sql"), 'r').read()
    raster_metadata_sql = open(os.path.join(
        get_sql_template_path(), "raster_metadata_table.sql"), 'r').read()
    raster3d_metadata_sql = open(os.path.join(template_path,
                                              "raster3d_metadata_table.sql"),
                                              'r').read()
    vector_metadata_sql = open(os.path.join(template_path,
                                            "vector_metadata_table.sql"),
                                            'r').read()
    raster_views_sql = open(os.path.join(template_path, "raster_views.sql"),
                            'r').read()
    raster3d_views_sql = open(os.path.join(template_path,
                                           "raster3d_views.sql"), 'r').read()
    vector_views_sql = open(os.path.join(template_path, "vector_views.sql"),
                            'r').read()

    stds_tables_template_sql = open(os.path.join(template_path,
                                                 "stds_tables_template.sql"),
                                                 'r').read()
    strds_metadata_sql = open(os.path.join(template_path,
                                           "strds_metadata_table.sql"),
                                           'r').read()
    str3ds_metadata_sql = open(os.path.join(template_path,
                                            "str3ds_metadata_table.sql"),
                                            'r').read()
    stvds_metadata_sql = open(os.path.join(template_path,
                                           "stvds_metadata_table.sql"),
                                           'r').read()
    strds_views_sql = open(os.path.join(template_path, "strds_views.sql"),
                           'r').read()
    str3ds_views_sql = open(os.path.join(template_path, "str3ds_views.sql"),
                            'r').read()
    stvds_views_sql = open(os.path.join(template_path, "stvds_views.sql"),
                           'r').read()

    # Create the raster, raster3d and vector tables SQL statements
    raster_tables_sql = map_tables_template_sql.replace("GRASS_MAP", "raster")
    vector_tables_sql = map_tables_template_sql.replace("GRASS_MAP", "vector")
    raster3d_tables_sql = map_tables_template_sql.replace(
        "GRASS_MAP", "raster3d")

    # Create the space-time raster, raster3d and vector dataset tables
    # SQL statements
    strds_tables_sql = stds_tables_template_sql.replace("STDS", "strds")
    stvds_tables_sql = stds_tables_template_sql.replace("STDS", "stvds")
    str3ds_tables_sql = stds_tables_template_sql.replace("STDS", "str3ds")

    core.message(_("Create temporal database: %s" % (database)))

    if tgis_backend == "sqlite":
        # We need to create the sqlite3 database path if it does not exists
        tgis_dir = os.path.dirname(database)
        if not os.path.exists(tgis_dir):
            os.makedirs(tgis_dir)
        # Sqlite needs some trigger to emulate the foreign keys
        sqlite3_delete_trigger_sql = open(os.path.join(template_path,
                                                       "sqlite3_delete_trigger.sql"),
                                                       'r').read()

    # Connect now to the database
    if not dbif.connected:
        dbif.connect()

    # Execute the SQL statements for sqlite
    # Create the global tables for the native grass datatypes
    dbif.execute_transaction(raster_tables_sql)
    dbif.execute_transaction(raster_metadata_sql)
    dbif.execute_transaction(raster_views_sql)
    dbif.execute_transaction(vector_tables_sql)
    dbif.execute_transaction(vector_metadata_sql)
    dbif.execute_transaction(vector_views_sql)
    dbif.execute_transaction(raster3d_tables_sql)
    dbif.execute_transaction(raster3d_metadata_sql)
    dbif.execute_transaction(raster3d_views_sql)
    # Create the tables for the new space-time datatypes
    dbif.execute_transaction(strds_tables_sql)
    dbif.execute_transaction(strds_metadata_sql)
    dbif.execute_transaction(strds_views_sql)
    dbif.execute_transaction(stvds_tables_sql)
    dbif.execute_transaction(stvds_metadata_sql)
    dbif.execute_transaction(stvds_views_sql)
    dbif.execute_transaction(str3ds_tables_sql)
    dbif.execute_transaction(str3ds_metadata_sql)
    dbif.execute_transaction(str3ds_views_sql)

    if tgis_backend == "sqlite":
        dbif.execute_transaction(sqlite3_delete_trigger_sql)

    # Create the tgis metadata table to store the database
    # initial configuration
    # The metadata table content
    metadata = {}
    metadata["tgis_version"] = tgis_version
    metadata["tgis_db_version"] = tgis_db_version
    metadata["has_command_column"] = True
    metadata["creation_time"] = datetime.today()
    _create_tgis_metadata_table(metadata, dbif)

    dbif.close()

###############################################################################

def _create_tgis_metadata_table(content, dbif=None):
    """!Create the temporal gis metadata table which stores all metadata
       information about the temporal database.

       @param content The dictionary that stores the key:value metadata
                      that should be stored in the metadata table
       @param dbif The database interface to be used
    """
    dbif, connected = init_dbif(dbif)
    statement = "CREATE TABLE tgis_metadata (key VARCHAR NOT NULL, value VARCHAR);\n";
    dbif.execute_transaction(statement)

    for key in content.keys():
        statement = "INSERT INTO tgis_metadata (key, value) VALUES " + \
                     "(\'%s\' , \'%s\');\n"%(str(key), str(content[key]))
        dbif.execute_transaction(statement)

    if connected:
        dbif.close()

###############################################################################

class SQLDatabaseInterfaceConnection():
    """!This class represents the database interface connection
       and provides access to the chisen backend modules.

       The following DBMS are supported:
         - sqlite via the sqlite3 standard library
         - postgresql via psycopg2

    """
    def __init__(self):
        self.connected = False
        global tgis_backend
        if tgis_backend == "sqlite":
            self.dbmi = sqlite3
        else:
            self.dbmi = psycopg2

    def rollback(self):
        """
            Roll back the last transaction. This must be called
            in case a new query should be performed after a db error.

            This is only relevant for postgresql database.
        """
        if self.dbmi.__name__ == "psycopg2":
            if self.connected:
                self.connection.rollback()

    def connect(self):
        """!Connect to the DBMI to execute SQL statements

           Supported backends are sqlite3 and postgresql
        """
        self.database = get_temporal_dbmi_init_string()
        #print "Connect to",  self.database
        if self.dbmi.__name__ == "sqlite3":
            self.connection = self.dbmi.connect(self.database,
                    detect_types = self.dbmi.PARSE_DECLTYPES | self.dbmi.PARSE_COLNAMES)
            self.connection.row_factory = self.dbmi.Row
            self.connection.isolation_level = None
            self.cursor = self.connection.cursor()
            self.cursor.execute("PRAGMA synchronous = OFF")
            self.cursor.execute("PRAGMA journal_mode = MEMORY")
        elif self.dbmi.__name__ == "psycopg2":
            self.connection = self.dbmi.connect(self.database)
            #self.connection.set_isolation_level(dbmi.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
            self.cursor = self.connection.cursor(
                cursor_factory = self.dbmi.extras.DictCursor)
        self.connected = True

    def close(self):
        """!Close the DBMI connection"""
        #print "Close connection to",  self.database
        self.connection.commit()
        self.cursor.close()
        self.connected = False

    def mogrify_sql_statement(self, content):
        """!Return the SQL statement and arguments as executable SQL string

           @param content The content as tuple with two entries, the first
                           entry is the SQL statement with DBMI specific
                           place holder (?), the second entry is the argument
                           list that should substitue the place holder.

           Usage:

           @code

           >>> init()
           >>> dbif = SQLDatabaseInterfaceConnection()
           >>> dbif.mogrify_sql_statement(["SELECT ctime FROM raster_base WHERE id = ?",
           ... ["soil@PERMANENT",]])
           "SELECT ctime FROM raster_base WHERE id = 'soil@PERMANENT'"

           @endcode
        """
        sql = content[0]
        args = content[1]

        if self.dbmi.__name__ == "psycopg2":
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

        elif self.dbmi.__name__ == "sqlite3":
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

    def check_table(self, table_name):
        """!Check if a table exists in the temporal database
        
           @param table_name The name of the table to be checked for existance
           @return True if the table exists, False otherwise
        """
        table_exists = False
        connected = False
        if not self.connected:
            self.connect()
            connected = True

        # Check if the database already exists
        if self.dbmi.__name__ == "sqlite3":

            self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='%s';"%table_name)
            name = self.cursor.fetchone()
            if name and name[0] == table_name:
                table_exists = True
        else:
            # Check for raster_base table
            self.cursor.execute("SELECT EXISTS(SELECT * FROM information_schema.tables "
                    "WHERE table_name=%s)", ('%s'%table_name,))
            if self.cursor.fetchone()[0]:
                table_exists = True

        if connected:
            self.close()

        return table_exists
        
    def execute_transaction(self, statement):
        """!Execute a transactional SQL statement

            The BEGIN and END TRANSACTION statements will be added automatically
            to the sql statement

            @param statement The executable SQL statement or SQL script
        """
        connected = False
        if not self.connected:
            self.connect()
            connected = True

        sql_script = ""
        sql_script += "BEGIN TRANSACTION;\n"
        sql_script += statement
        sql_script += "END TRANSACTION;"

        try:
            if self.dbmi.__name__ == "sqlite3":
                self.cursor.executescript(statement)
            else:
                self.cursor.execute(statement)
            self.connection.commit()
        except:
            if connected:
                self.close()
            core.error(_("Unable to execute transaction:\n %(sql)s" % \
                         {"sql":statement}))
            raise

        if connected:
            self.close()

###############################################################################

def init_dbif(dbif):
    """!This method checks if the database interface connection exists,
        if not a new one will be created, connected and True will be returned

        Usage code sample:
        @code

        dbif, connect = tgis.init_dbif(None)

        sql = dbif.mogrify_sql_statement(["SELECT * FROM raster_base WHERE ? = ?"],
                                               ["id", "soil@PERMANENT"])
        dbif.execute_transaction(sql)

        if connect:
            dbif.close()

        @endcode
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
