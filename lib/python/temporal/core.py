"""
This module provides the functionality to create the temporal
SQL database and to establish a connection to the database.

Usage:

.. code-block:: python

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


(C) 2011-2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:author: Soeren Gebbert
"""
#import traceback
import os
import sys
import grass.script as gscript

from .c_libraries_interface import *
from grass.pygrass import messages
from grass.script.utils import decode, encode
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

import atexit
from datetime import datetime

if sys.version_info.major >= 3:
    long = int

###############################################################################


def profile_function(func):
    """Profiling function provided by the temporal framework"""
    do_profiling = os.getenv("GRASS_TGIS_PROFILE")

    if do_profiling == "True" or do_profiling == "1":
        import cProfile
        import pstats
        try:
            import StringIO as io
        except ImportError:
            import io
        pr = cProfile.Profile()
        pr.enable()
        func()
        pr.disable()
        s = io.StringIO()
        sortby = 'cumulative'
        ps = pstats.Stats(pr, stream=s).sort_stats(sortby)
        ps.print_stats()
        print(s.getvalue())
    else:
        func()

# Global variable that defines the backend
# of the temporal GIS
# It can either be "sqlite" or "pg"
tgis_backend = None


def get_tgis_backend():
    """Return the temporal GIS backend as string

       :returns: either "sqlite" or "pg"
    """
    global tgis_backend
    return tgis_backend

# Global variable that defines the database string
# of the temporal GIS
tgis_database = None


def get_tgis_database():
    """Return the temporal database string specified with t.connect
    """
    global tgis_database
    return tgis_database

# The version of the temporal framework
# this value must be an integer larger than 0
# Increase this value in case of backward incompatible changes in the TGIS API
tgis_version = 2
# The version of the temporal database since framework and database version
# can differ this value must be an integer larger than 0
# Increase this value in case of backward incompatible changes
# temporal database SQL layout
tgis_db_version = 3

# We need to know the parameter style of the database backend
tgis_dbmi_paramstyle = None


def get_tgis_dbmi_paramstyle():
    """Return the temporal database backend parameter style

       :returns: "qmark" or ""
    """
    global tgis_dbmi_paramstyle
    return tgis_dbmi_paramstyle

# We need to access the current mapset quite often in the framework, so we make
# a global variable that will be initiated when init() is called
current_mapset = None
current_location = None
current_gisdbase = None

###############################################################################


def get_current_mapset():
    """Return the current mapset

       This is the fastest way to receive the current mapset.
       The current mapset is set by init() and stored in a global variable.
       This function provides access to this global variable.
    """
    global current_mapset
    return current_mapset

###############################################################################


def get_current_location():
    """Return the current location

       This is the fastest way to receive the current location.
       The current location is set by init() and stored in a global variable.
       This function provides access to this global variable.
    """
    global current_location
    return current_location

###############################################################################


def get_current_gisdbase():
    """Return the current gis database (gisdbase)

       This is the fastest way to receive the current gisdbase.
       The current gisdbase is set by init() and stored in a global variable.
       This function provides access to this global variable.
    """
    global current_gisdbase
    return current_gisdbase

###############################################################################

# If this global variable is set True, then maps can only be registered in
# space time datasets with the same mapset. In addition, only maps in the
# current mapset can be inserted, updated or deleted from the temporal database.
# Overwrite this global variable by: g.gisenv set="TGIS_DISABLE_MAPSET_CHECK=True"
# ATTENTION: Be aware to face corrupted temporal database in case this global
#            variable is set to False. This feature is highly
#            experimental and violates the grass permission guidance.
enable_mapset_check = True
# If this global variable is set True, the timestamps of maps will be written
# as textfiles for each map that will be inserted or updated in the temporal
# database using the C-library timestamp interface.
# Overwrite this global variable by: g.gisenv set="TGIS_DISABLE_TIMESTAMP_WRITE=True"
# ATTENTION: Be aware to face corrupted temporal database in case this global
#            variable is set to False. This feature is highly
#            experimental and violates the grass permission guidance.
enable_timestamp_write = True


def get_enable_mapset_check():
    """Return True if the mapsets should be checked while insert, update,
       delete requests and space time dataset registration.

       If this global variable is set True, then maps can only be registered
       in space time datasets with the same mapset. In addition, only maps in
       the current mapset can be inserted, updated or deleted from the temporal
       database.
       Overwrite this global variable by: g.gisenv set="TGIS_DISABLE_MAPSET_CHECK=True"

       ..warning::

           Be aware to face corrupted temporal database in case this
           global variable is set to False. This feature is highly
           experimental and violates the grass permission guidance.

    """
    global enable_mapset_check
    return enable_mapset_check


def get_enable_timestamp_write():
    """Return True if the map timestamps should be written to the spatial
       database metadata as well.

       If this global variable is set True, the timestamps of maps will be
       written as textfiles for each map that will be inserted or updated in
       the temporal database using the C-library timestamp interface.
       Overwrite this global variable by: g.gisenv set="TGIS_DISABLE_TIMESTAMP_WRITE=True"

       ..warning::

           Be aware that C-libraries can not access timestamp information if
           they are not written as spatial database metadata, hence modules
           that make use of timestamps using the C-library interface will not
           work with maps that were created without writing the timestamps.
    """
    global enable_timestamp_write
    return enable_timestamp_write

###############################################################################

# The global variable that stores the PyGRASS Messenger object that
# provides a fast and exit safe interface to the C-library message functions
message_interface = None


def _init_tgis_message_interface(raise_on_error=False):
    """Initiate the global message interface

       :param raise_on_error: If True raise a FatalError exception in case of
                              a fatal error, call sys.exit(1) otherwise
    """
    global message_interface
    if message_interface is None:
        message_interface = messages.get_msgr(raise_on_error=raise_on_error)


def get_tgis_message_interface():
    """Return the temporal GIS message interface which is of type
       grass.pygrass.message.Messenger()

       Use this message interface to print messages to stdout using the
       GRASS C-library messaging system.
    """
    global message_interface
    return message_interface

###############################################################################

# The global variable that stores the C-library interface object that
# provides a fast and exit safe interface to the C-library libgis,
# libraster, libraster3d and libvector functions
c_library_interface = None


def _init_tgis_c_library_interface():
    """Set the global C-library interface variable that
       provides a fast and exit safe interface to the C-library libgis,
       libraster, libraster3d and libvector functions
    """
    global c_library_interface
    if c_library_interface is None:
        c_library_interface = CLibrariesInterface()


def get_tgis_c_library_interface():
    """Return the C-library interface that
       provides a fast and exit safe interface to the C-library libgis,
       libraster, libraster3d and libvector functions
    """
    global c_library_interface
    return c_library_interface

###############################################################################

# Set this variable True to raise a FatalError exception
# in case a fatal error occurs using the messenger interface
raise_on_error = False


def set_raise_on_error(raise_exp=True):
    """Define behavior on fatal error, invoked using the tgis messenger
    interface (msgr.fatal())

    The messenger interface will be restarted using the new error policy

    :param raise_exp: True to raise a FatalError exception instead of calling
                      sys.exit(1) when using the tgis messenger interface

    .. code-block:: python

        >>> import grass.temporal as tgis
        >>> tgis.init()
        >>> ignore = tgis.set_raise_on_error(False)
        >>> msgr = tgis.get_tgis_message_interface()
        >>> tgis.get_raise_on_error()
        False
        >>> msgr.fatal("Ohh no no no!")
        Traceback (most recent call last):
          File "__init__.py", line 239, in fatal
            sys.exit(1)
        SystemExit: 1

        >>> tgis.set_raise_on_error(True)
        False
        >>> msgr.fatal("Ohh no no no!")
        Traceback (most recent call last):
          File "__init__.py", line 241, in fatal
            raise FatalError(message)
        FatalError: Ohh no no no!

    :returns: current status
    """
    global raise_on_error
    tmp_raise = raise_on_error
    raise_on_error = raise_exp

    global message_interface
    if message_interface:
        message_interface.set_raise_on_error(raise_on_error)
    else:
        _init_tgis_message_interface(raise_on_error)

    return tmp_raise


def get_raise_on_error():
    """Return True if a FatalError exception is raised instead of calling
       sys.exit(1) in case a fatal error was invoked with msgr.fatal()
    """
    global raise_on_error
    return raise_on_error


###############################################################################


def get_tgis_version():
    """Get the version number of the temporal framework
       :returns: The version number of the temporal framework as string
    """
    global tgis_version
    return tgis_version

###############################################################################


def get_tgis_db_version():
    """Get the version number of the temporal framework
       :returns: The version number of the temporal framework as string
    """
    global tgis_db_version
    return tgis_db_version

###############################################################################


def get_tgis_metadata(dbif=None):
    """Return the tgis metadata table as a list of rows (dicts) or None if not
       present

       :param dbif: The database interface to be used
       :returns: The selected rows with key/value columns or None
    """

    dbif, connected = init_dbif(dbif)

    # Select metadata if the table is present
    try:
        statement = "SELECT * FROM tgis_metadata;\n"
        dbif.execute(statement)
        rows = dbif.fetchall()
    except:
        rows = None

    if connected:
        dbif.close()

    return rows

###############################################################################

# The temporal database string set with t.connect
# with substituted GRASS variables gisdbase, location and mapset
tgis_database_string = None


def get_tgis_database_string():
    """Return the preprocessed temporal database string

       This string is the temporal database string set with t.connect
       that was processed to substitue location, gisdbase and mapset
       variables.
    """
    global tgis_database_string
    return tgis_database_string

###############################################################################


def get_sql_template_path():
    base = os.getenv("GISBASE")
    base_etc = os.path.join(base, "etc")
    return os.path.join(base_etc, "sql")

###############################################################################


def stop_subprocesses():
    """Stop the messenger and C-interface subprocesses
       that are started by tgis.init()
    """
    global message_interface
    global c_library_interface
    if message_interface:
        message_interface.stop()
    if c_library_interface:
        c_library_interface.stop()

# We register this function to be called at exit
atexit.register(stop_subprocesses)


def get_available_temporal_mapsets():
    """Return a list of of mapset names with temporal database driver and names
        that are accessible from the current mapset.

        :returns: A dictionary, mapset names are keys, the tuple (driver,
                  database) are the values
    """
    global c_library_interface
    global message_interface

    mapsets = c_library_interface.available_mapsets()
    
    tgis_mapsets = {}

    for mapset in mapsets:
        mapset = mapset
        driver = c_library_interface.get_driver_name(mapset)
        database = c_library_interface.get_database_name(mapset)

        message_interface.debug(1, "get_available_temporal_mapsets: "
                                   "\n  mapset %s\n  driver %s\n  database %s" % (mapset,
                                   driver, database))
        if driver and database:
            # Check if the temporal sqlite database exists
            # We need to set non-existing databases in case the mapset is the current mapset
            # to create it
            if (driver == "sqlite" and os.path.exists(database)) or mapset == get_current_mapset():
                tgis_mapsets[mapset] = (driver, database)

            # We need to warn if the connection is defined but the database does not
            # exists
            if driver == "sqlite" and not os.path.exists(database):
                message_interface.warning("Temporal database connection defined as:\n" +
                                          database + "\nBut database file does not exist.")
    return tgis_mapsets

###############################################################################


def init(raise_fatal_error=False, skip_db_version_check=False):
    """This function set the correct database backend from GRASS environmental
       variables and creates the grass temporal database structure for raster,
       vector and raster3d maps as well as for the space-time datasets strds,
       str3ds and stvds in case it does not exist.

       Several global variables are initiated and the messenger and C-library
       interface subprocesses are spawned.

       Re-run this function in case the following GRASS variables change while
       the process runs:

       - MAPSET
       - LOCATION_NAME
       - GISDBASE
       - TGIS_DISABLE_MAPSET_CHECK
       - TGIS_DISABLE_TIMESTAMP_WRITE

       Re-run this function if the following t.connect variables change while
       the process runs:

       - temporal GIS driver (set by t.connect driver=)
       - temporal GIS database (set by t.connect database=)

       The following environmental variables are checked:

        - GRASS_TGIS_PROFILE (True, False, 1, 0)
        - GRASS_TGIS_RAISE_ON_ERROR (True, False, 1, 0)

        ..warning::

            This functions must be called before any spatio-temporal processing
            can be started

        :param raise_fatal_error: Set this True to assure that the init()
                                  function does not kill a persistent process
                                  like the GUI. If set True a
                                  grass.pygrass.messages.FatalError
                                  exception will be raised in case a fatal
                                  error occurs in the init process, otherwise
                                  sys.exit(1) will be called.
        :param skip_db_version_check: Set this True to skip mismatch temporal
                                      database version check.
                                      Recommended to be used only for
                                      upgrade_temporal_database().
    """
    # We need to set the correct database backend and several global variables
    # from the GRASS mapset specific environment variables of g.gisenv and t.connect
    global tgis_backend
    global tgis_database
    global tgis_database_string
    global tgis_dbmi_paramstyle
    global tgis_db_version
    global raise_on_error
    global enable_mapset_check
    global enable_timestamp_write
    global current_mapset
    global current_location
    global current_gisdbase

    raise_on_error = raise_fatal_error

    # We must run t.connect at first to create the temporal database and to
    # get the environmental variables
    gscript.run_command("t.connect", flags="c")
    grassenv = gscript.gisenv()

    # Set the global variable for faster access
    current_mapset = grassenv["MAPSET"]
    current_location = grassenv["LOCATION_NAME"]
    current_gisdbase = grassenv["GISDBASE"]

    # Check environment variable GRASS_TGIS_RAISE_ON_ERROR
    if os.getenv("GRASS_TGIS_RAISE_ON_ERROR") == "True" or \
       os.getenv("GRASS_TGIS_RAISE_ON_ERROR") == "1":
        raise_on_error = True

    # Check if the script library raises on error,
    # if so we do the same
    if gscript.get_raise_on_error() is True:
        raise_on_error = True

    # Start the GRASS message interface server
    _init_tgis_message_interface(raise_on_error)
    # Start the C-library interface server
    _init_tgis_c_library_interface()
    msgr = get_tgis_message_interface()
    msgr.debug(1, "Initiate the temporal database")

    msgr.debug(1, ("Raise on error id: %s" % str(raise_on_error)))

    ciface = get_tgis_c_library_interface()
    driver_string = ciface.get_driver_name()
    database_string = ciface.get_database_name()

    # Set the mapset check and the timestamp write
    if "TGIS_DISABLE_MAPSET_CHECK" in grassenv:
        if gscript.encode(grassenv["TGIS_DISABLE_MAPSET_CHECK"]) == "True" or \
           gscript.encode(grassenv["TGIS_DISABLE_MAPSET_CHECK"]) == "1":
            enable_mapset_check = False
            msgr.warning("TGIS_DISABLE_MAPSET_CHECK is True")

    if "TGIS_DISABLE_TIMESTAMP_WRITE" in grassenv:
        if gscript.encode(grassenv["TGIS_DISABLE_TIMESTAMP_WRITE"]) == "True" or \
           gscript.encode(grassenv["TGIS_DISABLE_TIMESTAMP_WRITE"]) == "1":
            enable_timestamp_write = False
            msgr.warning("TGIS_DISABLE_TIMESTAMP_WRITE is True")

    if driver_string is not None and driver_string != "":
        driver_string = decode(driver_string)
        if driver_string == "sqlite":
            tgis_backend = driver_string
            try:
                import sqlite3
            except ImportError:
                msgr.error("Unable to locate the sqlite SQL Python interface"
                           " module sqlite3.")
                raise
            dbmi = sqlite3
        elif driver_string == "pg":
            tgis_backend = driver_string
            try:
                import psycopg2
            except ImportError:
                msgr.error("Unable to locate the Postgresql SQL Python "
                           "interface module psycopg2.")
                raise
            dbmi = psycopg2
        else:
            msgr.fatal(_("Unable to initialize the temporal DBMI interface. "
                         "Please use t.connect to specify the driver and the"
                         " database string"))
    else:
        # Set the default sqlite3 connection in case nothing was defined
        gscript.run_command("t.connect", flags="d")
        driver_string = ciface.get_driver_name()
        database_string = ciface.get_database_name()
        tgis_backend = driver_string
        try:
            import sqlite3
        except ImportError:
            msgr.error("Unable to locate the sqlite SQL Python interface"
                       " module sqlite3.")
            raise
        dbmi = sqlite3

    tgis_database_string = database_string
    # Set the parameter style
    tgis_dbmi_paramstyle = dbmi.paramstyle

    # We do not know if the database already exists
    db_exists = False
    dbif = SQLDatabaseInterfaceConnection()

    # Check if the database already exists
    if tgis_backend == "sqlite":
        # Check path of the sqlite database
        if os.path.exists(tgis_database_string):
            dbif.connect()
            # Check for raster_base table
            dbif.execute("SELECT name FROM sqlite_master WHERE type='table' "
                         "AND name='raster_base';")
            name = dbif.fetchone()
            if name and name[0] == "raster_base":
                db_exists = True
            dbif.close()
    elif tgis_backend == "pg":
        # Connect to database
        dbif.connect()
        # Check for raster_base table
        dbif.execute("SELECT EXISTS(SELECT * FROM information_schema.tables "
                     "WHERE table_name=%s)", ('raster_base',))
        if dbif.fetchone()[0]:
            db_exists = True

    backup_howto = _("The format of your actual temporal database is not "
                     "supported any more.\n"
                     "Please create a backup of your temporal database "
                     "to avoid lossing data.\nSOLUTION: ")
    if tgis_db_version > 2:
        backup_howto += _("Run t.upgrade command installed from "
                          "GRASS Addons in order to upgrade your temporal database.\n")
    else:
        backup_howto += _("You need to export it by "
                          "restoring the GRASS GIS version used for creating this DB."
                          "Notes: Use t.rast.export and t.vect.export "
                          "to make a backup of your"
                          " existing space time datasets. To save the timestamps of"
                          " your existing maps and space time datasets, use "
                          "t.rast.list, t.vect.list and t.rast3d.list. "
                          "You can register the existing time stamped maps easily if"
                          " you export columns=id,start_time,end_time into text "
                          "files and use t.register to register them again in new"
                          " created space time datasets (t.create). After the backup"
                          " remove the existing temporal database, a new one will be"
                          " created automatically.\n")

    if db_exists is True:
        dbif.close()
        if  skip_db_version_check is True:
            return

        # Check the version of the temporal database
        dbif.connect()
        metadata = get_tgis_metadata(dbif)
        dbif.close()
        if metadata is None:
            msgr.fatal(_("Unable to receive temporal database metadata.\n"
                         "Current temporal database info:%(info)s") % (
                       {"info": get_database_info_string()}))
        for entry in metadata:
            if "tgis_version" in entry and entry[1] != str(get_tgis_version()):
                msgr.fatal(_("Unsupported temporal database: version mismatch."
                             "\n %(backup)s Supported temporal API version is:"
                             " %(api)i.\nPlease update your GRASS GIS "
                             "installation.\nCurrent temporal database info:"
                             "%(info)s") % ({"backup": backup_howto,
                                             "api": get_tgis_version(),
                                             "info": get_database_info_string()}))
            if "tgis_db_version" in entry and entry[1] != str(get_tgis_db_version()):
                msgr.fatal(_("Unsupported temporal database: version mismatch."
                             "\n %(backup)sSupported temporal database version"
                             " is: %(tdb)i\nCurrent temporal database info:"
                             "%(info)s") % ({"backup": backup_howto,
                                             "tdb": get_tgis_version(),
                                             "info": get_database_info_string()}))
        return

    create_temporal_database(dbif)

###############################################################################


def get_database_info_string():
    dbif = SQLDatabaseInterfaceConnection()

    info = "\nDBMI interface:..... " + str(dbif.get_dbmi().__name__)
    info += "\nTemporal database:.. " + str(get_tgis_database_string())
    return info

###############################################################################

def _create_temporal_database_views(dbif):
    """Create all views in the temporal database (internal use only)

    Used by create_temporal_database() and upgrade_temporal_database().

    :param dbif: The database interface to be used
    """
    template_path = get_sql_template_path()

    for sql_filename in ("raster_views",
                         "raster3d_views",
                         "vector_views",
                         "strds_views",
                         "str3ds_views",
                         "stvds_views"):
        sql_filepath = open(os.path.join(template_path,
                                         sql_filename + '.sql'),
                            'r').read()
        dbif.execute_transaction(sql_filepath)

def create_temporal_database(dbif):
    """This function will create the temporal database

       It will create all tables and triggers that are needed to run
       the temporal GIS

       :param dbif: The database interface to be used
    """
    global tgis_backend
    global tgis_version
    global tgis_db_version
    global tgis_database_string

    template_path = get_sql_template_path()
    msgr = get_tgis_message_interface()

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

    msgr.message(_("Creating temporal database: %s" % (str(tgis_database_string))))

    if tgis_backend == "sqlite":
        # We need to create the sqlite3 database path if it does not exist
        tgis_dir = os.path.dirname(tgis_database_string)
        if not os.path.exists(tgis_dir):
            try:
                os.makedirs(tgis_dir)
            except Exception as e:
                msgr.fatal(_("Unable to create SQLite temporal database\n"
                             "Exception: %s\nPlease use t.connect to set a "
                             "read- and writable temporal database path" % (e)))

        # Set up the trigger that takes care of
        # the correct deletion of entries across the different tables
        delete_trigger_sql = open(os.path.join(template_path,
                                               "sqlite3_delete_trigger.sql"),
                                  'r').read()
        indexes_sql = open(os.path.join(template_path, "sqlite3_indexes.sql"),
                           'r').read()
    else:
        # Set up the trigger that takes care of
        # the correct deletion of entries across the different tables
        delete_trigger_sql = open(os.path.join(template_path,
                                               "postgresql_delete_trigger.sql"),
                                  'r').read()
        indexes_sql = open(os.path.join(template_path,
                                        "postgresql_indexes.sql"), 'r').read()

    # Connect now to the database
    if dbif.connected is not True:
        dbif.connect()

    # Execute the SQL statements
    # Create the global tables for the native grass datatypes
    dbif.execute_transaction(raster_tables_sql)
    dbif.execute_transaction(raster_metadata_sql)
    dbif.execute_transaction(vector_tables_sql)
    dbif.execute_transaction(vector_metadata_sql)
    dbif.execute_transaction(raster3d_tables_sql)
    dbif.execute_transaction(raster3d_metadata_sql)
    # Create the tables for the new space-time datatypes
    dbif.execute_transaction(strds_tables_sql)
    dbif.execute_transaction(strds_metadata_sql)
    dbif.execute_transaction(stvds_tables_sql)
    dbif.execute_transaction(stvds_metadata_sql)
    dbif.execute_transaction(str3ds_tables_sql)
    dbif.execute_transaction(str3ds_metadata_sql)

    # Create views
    _create_temporal_database_views(dbif)

    # The delete trigger
    dbif.execute_transaction(delete_trigger_sql)
    # The indexes
    dbif.execute_transaction(indexes_sql)

    # Create the tgis metadata table to store the database
    # initial configuration
    # The metadata table content
    metadata = {}
    metadata["tgis_version"] = tgis_version
    metadata["tgis_db_version"] = tgis_db_version
    metadata["creation_time"] = datetime.today()
    _create_tgis_metadata_table(metadata, dbif)

    dbif.close()

###############################################################################


def upgrade_temporal_database(dbif):
    """This function will upgrade the temporal database if needed.

       It will update all tables and triggers that are requested by
       currently supported TGIS DB version.

       :param dbif: The database interface to be used
    """
    global tgis_database_string
    global tgis_db_version

    metadata = get_tgis_metadata(dbif)

    msgr = get_tgis_message_interface()
    if metadata is None:
        msgr.fatal(_("Unable to receive temporal database metadata.\n"
                     "Current temporal database info:%(info)s") % (
                         {"info": get_database_info_string()}))
    upgrade_db_from = None
    for entry in metadata:
        if "tgis_db_version" in entry and entry[1] != str(tgis_db_version):
            upgrade_db_from = entry[1]
            break

    if upgrade_db_from is None:
        msgr.message(_("Temporal database is up-to-date. Operation canceled"))
        dbif.close()
        return

    template_path = get_sql_template_path()
    try:
        upgrade_db_sql = open(os.path.join(
            template_path,
            "upgrade_db_%s_to_%s.sql" % (upgrade_db_from, tgis_db_version)),
            'r').read()
    except FileNotFoundError:
        msgr.fatal(_("Unsupported TGIS DB upgrade scenario: from version %s to %s") %
                   (upgrade_db_from, tgis_db_version))

    drop_views_sql = open(
        os.path.join(template_path, "drop_views.sql"),
        'r').read()

    msgr.message(
        _("Upgrading temporal database <%s> from version %s to %s...") %
        (tgis_database_string, upgrade_db_from, tgis_db_version))
    # Drop views
    dbif.execute_transaction(drop_views_sql)
    # Perform upgrade
    dbif.execute_transaction(upgrade_db_sql)
    # Recreate views
    _create_temporal_database_views(dbif)

    dbif.close()

###############################################################################


def _create_tgis_metadata_table(content, dbif=None):
    """!Create the temporal gis metadata table which stores all metadata
       information about the temporal database.

       :param content: The dictionary that stores the key:value metadata
                      that should be stored in the metadata table
       :param dbif: The database interface to be used
    """
    dbif, connected = init_dbif(dbif)
    statement = "CREATE TABLE tgis_metadata (key VARCHAR NOT NULL, value VARCHAR);\n"
    dbif.execute_transaction(statement)

    for key in content.keys():
        statement = "INSERT INTO tgis_metadata (key, value) VALUES " + \
                    "(\'%s\' , \'%s\');\n" % (str(key), str(content[key]))
        dbif.execute_transaction(statement)

    if connected:
        dbif.close()

###############################################################################


class SQLDatabaseInterfaceConnection(object):
    def __init__(self):
        self.tgis_mapsets = get_available_temporal_mapsets()
        self.current_mapset = get_current_mapset()
        self.connections = {}
        self.connected = False

        self.unique_connections = {}

        for mapset in self.tgis_mapsets.keys():
            driver, dbstring = self.tgis_mapsets[mapset]

            if dbstring not in self.unique_connections.keys():
                self.unique_connections[dbstring] = DBConnection(backend=driver,
                                                                 dbstring=dbstring)

            self.connections[mapset] = self.unique_connections[dbstring]

        self.msgr = get_tgis_message_interface()

    def get_dbmi(self, mapset=None):
        if mapset is None:
            mapset = self.current_mapset

        mapset = decode(mapset)
        return self.connections[mapset].dbmi

    def rollback(self, mapset=None):
        """
            Roll back the last transaction. This must be called
            in case a new query should be performed after a db error.

            This is only relevant for postgresql database.
        """
        if mapset is None:
            mapset = self.current_mapset

    def connect(self):
        """Connect to the DBMI to execute SQL statements

           Supported backends are sqlite3 and postgresql
        """
        for mapset in self.tgis_mapsets.keys():
            driver, dbstring = self.tgis_mapsets[mapset]
            conn = self.connections[mapset]
            if conn.is_connected() is False:
                conn.connect(dbstring)

        self.connected = True

    def is_connected(self):
        return self.connected

    def close(self):
        """Close the DBMI connection

           There may be several temporal databases in a location, hence
           close all temporal databases that have been opened.
        """
        for key in self.unique_connections.keys():
            self.unique_connections[key].close()

        self.connected = False

    def mogrify_sql_statement(self, content, mapset=None):
        """Return the SQL statement and arguments as executable SQL string

           :param content: The content as tuple with two entries, the first
                           entry is the SQL statement with DBMI specific
                           place holder (?), the second entry is the argument
                           list that should substitute the place holder.
           :param mapset: The mapset of the abstract dataset or temporal
                          database location, if None the current mapset
                          will be used
        """
        if mapset is None:
            mapset = self.current_mapset

        mapset = decode(mapset)
        if mapset not in self.tgis_mapsets.keys():
            self.msgr.fatal(_("Unable to mogrify sql statement. " +
                              self._create_mapset_error_message(mapset)))

        return self.connections[mapset].mogrify_sql_statement(content)

    def check_table(self, table_name, mapset=None):
        """Check if a table exists in the temporal database

           :param table_name: The name of the table to be checked for existence
           :param mapset: The mapset of the abstract dataset or temporal
                          database location, if None the current mapset
                          will be used
           :returns: True if the table exists, False otherwise

           TODO:
           There may be several temporal databases in a location, hence
           the mapset is used to query the correct temporal database.
        """
        if mapset is None:
            mapset = self.current_mapset

        mapset = decode(mapset)
        if mapset not in self.tgis_mapsets.keys():
            self.msgr.fatal(_("Unable to check table. " +
                              self._create_mapset_error_message(mapset)))

        return self.connections[mapset].check_table(table_name)

    def execute(self, statement, args=None, mapset=None):
        """

        :param mapset: The mapset of the abstract dataset or temporal
                       database location, if None the current mapset
                       will be used
        """
        if mapset is None:
            mapset = self.current_mapset

        mapset = decode(mapset)
        if mapset not in self.tgis_mapsets.keys():
            self.msgr.fatal(_("Unable to execute sql statement. " +
                              self._create_mapset_error_message(mapset)))

        return self.connections[mapset].execute(statement, args)

    def fetchone(self, mapset=None):
        if mapset is None:
            mapset = self.current_mapset

        mapset = decode(mapset)
        if mapset not in self.tgis_mapsets.keys():
            self.msgr.fatal(_("Unable to fetch one. " +
                              self._create_mapset_error_message(mapset)))

        return self.connections[mapset].fetchone()

    def fetchall(self, mapset=None):
        if mapset is None:
            mapset = self.current_mapset

        mapset = decode(mapset)
        if mapset not in self.tgis_mapsets.keys():
            self.msgr.fatal(_("Unable to fetch all. " +
                              self._create_mapset_error_message(mapset)))

        return self.connections[mapset].fetchall()

    def execute_transaction(self, statement, mapset=None):
        """Execute a transactional SQL statement

           The BEGIN and END TRANSACTION statements will be added automatically
           to the sql statement

           :param statement: The executable SQL statement or SQL script
        """
        if mapset is None:
            mapset = self.current_mapset

        mapset = decode(mapset)
        if mapset not in self.tgis_mapsets.keys():
            self.msgr.fatal(_("Unable to execute transaction. " +
                              self._create_mapset_error_message(mapset)))

        return self.connections[mapset].execute_transaction(statement)

    def _create_mapset_error_message(self, mapset):

        return("You have no permission to "
               "access mapset <%(mapset)s>, or "
               "mapset <%(mapset)s> has no temporal database. "
               "Accessible mapsets are: <%(mapsets)s>" %
               {"mapset": decode(mapset),
                "mapsets":','.join(self.tgis_mapsets.keys())})

###############################################################################


class DBConnection(object):
    """This class represents the database interface connection
       and provides access to the chosen backend modules.

       The following DBMS are supported:

         - sqlite via the sqlite3 standard library
         - postgresql via psycopg2
    """

    def __init__(self, backend=None, dbstring=None):
        """ Constructor of a database connection

            param backend:The database backend sqlite or pg
            param dbstring: The database connection string
        """
        self.connected = False
        if backend is None:
            global tgis_backend
            if decode(tgis_backend) == "sqlite":
                self.dbmi = sqlite3
            else:
                self.dbmi = psycopg2
        else:
            if decode(backend) == "sqlite":
                self.dbmi = sqlite3
            else:
                self.dbmi = psycopg2

        if dbstring is None:
            global tgis_database_string
            self.dbstring = tgis_database_string

        self.dbstring = dbstring

        self.msgr = get_tgis_message_interface()
        self.msgr.debug(1, "DBConnection constructor:"
                           "\n  backend: %s"
                           "\n  dbstring: %s" % (backend, self.dbstring))

    def __del__(self):
        if self.connected is True:
            self.close()

    def is_connected(self):
        return self.connected

    def rollback(self):
        """
            Roll back the last transaction. This must be called
            in case a new query should be performed after a db error.

            This is only relevant for postgresql database.
        """
        if self.dbmi.__name__ == "psycopg2":
            if self.connected:
                self.connection.rollback()

    def connect(self, dbstring=None):
        """Connect to the DBMI to execute SQL statements

            Supported backends are sqlite3 and postgresql

            param dbstring: The database connection string
        """
        # Connection in the current mapset
        if dbstring is None:
            dbstring = self.dbstring
        
        dbstring = decode(dbstring)

        try:
            if self.dbmi.__name__ == "sqlite3":
                self.connection = self.dbmi.connect(dbstring,
                        detect_types=self.dbmi.PARSE_DECLTYPES | self.dbmi.PARSE_COLNAMES)
                self.connection.row_factory = self.dbmi.Row
                self.connection.isolation_level = None
                self.connection.text_factory = str
                self.cursor = self.connection.cursor()
                self.cursor.execute("PRAGMA synchronous = OFF")
                self.cursor.execute("PRAGMA journal_mode = MEMORY")
            elif self.dbmi.__name__ == "psycopg2":
                self.connection = self.dbmi.connect(dbstring)
                #self.connection.set_isolation_level(dbmi.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
                self.cursor = self.connection.cursor(
                    cursor_factory=self.dbmi.extras.DictCursor)
            self.connected = True
        except Exception as e:
            self.msgr.fatal(_("Unable to connect to %(db)s database: "
                              "%(string)s\nException: \"%(ex)s\"\nPlease use"
                              " t.connect to set a read- and writable "
                              "temporal database backend") % (
                            {"db": self.dbmi.__name__,
                             "string": tgis_database_string, "ex": e, }))

    def close(self):
        """Close the DBMI connection
           TODO:
           There may be several temporal databases in a location, hence
           close all temporal databases that have been opened. Use a dictionary
           to manage different connections.
        """
        self.connection.commit()
        self.cursor.close()
        self.connected = False

    def mogrify_sql_statement(self, content):
        """Return the SQL statement and arguments as executable SQL string

           TODO:
           Use the mapset argument to identify the correct database driver

           :param content: The content as tuple with two entries, the first
                           entry is the SQL statement with DBMI specific
                           place holder (?), the second entry is the argument
                           list that should substitute the place holder.
           :param mapset: The mapset of the abstract dataset or temporal
                          database location, if None the current mapset
                          will be used

           Usage:

           .. code-block:: python

               >>> init()
               >>> dbif = SQLDatabaseInterfaceConnection()
               >>> dbif.mogrify_sql_statement(["SELECT ctime FROM raster_base WHERE id = ?",
               ... ["soil@PERMANENT",]])
               "SELECT ctime FROM raster_base WHERE id = 'soil@PERMANENT'"

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
                    except Exception as exc:
                        print(sql, args)
                        raise exc
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
                    elif isinstance(args[count], datetime):
                        statement = "%s\'%s\'%s" % (statement[0:pos], str(args[count]),
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
        """Check if a table exists in the temporal database

           :param table_name: The name of the table to be checked for existence
           :param mapset: The mapset of the abstract dataset or temporal
                          database location, if None the current mapset
                          will be used
           :returns: True if the table exists, False otherwise

           TODO:
           There may be several temporal databases in a location, hence
           the mapset is used to query the correct temporal database.
        """
        table_exists = False
        connected = False
        if not self.connected:
            self.connect()
            connected = True

        # Check if the database already exists
        if self.dbmi.__name__ == "sqlite3":

            self.cursor.execute("SELECT name FROM sqlite_master WHERE "
                                "type='table' AND name='%s';" % table_name)
            name = self.cursor.fetchone()
            if name and name[0] == table_name:
                table_exists = True
        else:
            # Check for raster_base table
            self.cursor.execute("SELECT EXISTS(SELECT * FROM information_schema.tables "
                                "WHERE table_name=%s)", ('%s' % table_name,))
            if self.cursor.fetchone()[0]:
                table_exists = True

        if connected:
            self.close()

        return table_exists

    def execute(self, statement, args=None):
        """Execute a SQL statement

           :param statement: The executable SQL statement or SQL script
        """
        connected = False
        if not self.connected:
            self.connect()
            connected = True
        try:
            if args:
                self.cursor.execute(statement, args)
            else:
                self.cursor.execute(statement)
        except:
            if connected:
                self.close()
            self.msgr.error(_("Unable to execute :\n %(sql)s" %
                            {"sql": statement}))
            raise

        if connected:
            self.close()

    def fetchone(self):
        if self.connected:
            return self.cursor.fetchone()
        return None

    def fetchall(self):
        if self.connected:
            return self.cursor.fetchall()
        return None

    def execute_transaction(self, statement, mapset=None):
        """Execute a transactional SQL statement

           The BEGIN and END TRANSACTION statements will be added automatically
           to the sql statement

           :param statement: The executable SQL statement or SQL script
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
            self.msgr.error(_("Unable to execute transaction:\n %(sql)s" %
                            {"sql": statement}))
            raise

        if connected:
            self.close()

###############################################################################


def init_dbif(dbif):
    """This method checks if the database interface connection exists,
        if not a new one will be created, connected and True will be returned.
        If the database interface exists but is connected, the connection will
        be established.

        :returns: the tuple (dbif, True|False)

        Usage code sample:

        .. code-block:: python

            dbif, connect = tgis.init_dbif(None)

            sql = dbif.mogrify_sql_statement(["SELECT * FROM raster_base WHERE ? = ?"],
                                                   ["id", "soil@PERMANENT"])
            dbif.execute_transaction(sql)

            if connect:
                dbif.close()

    """
    if dbif is None:
        dbif = SQLDatabaseInterfaceConnection()
        dbif.connect()
        return dbif, True
    elif dbif.is_connected() is False:
        dbif.connect()
        return dbif, True

    return dbif, False

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
