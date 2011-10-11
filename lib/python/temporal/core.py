"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS core functions to be used in Python sripts.

Usage:

@code
import grass.temporal as tgis

tgis.create_temporal_database()
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import os
import copy
from datetime import datetime, date, time, timedelta
import grass.script.core as core

###############################################################################

def get_temporal_dbmi_default_path(grassenv):
    dbpath = os.path.join(grassenv["GISDBASE"], grassenv["LOCATION_NAME"])
    dbpath = os.path.join(dbpath, "PERMANENT")
    return os.path.join(dbpath, "grass.db")


# The chosen DBMI backend can be defined on runtime
# Check the grass environment before import
grassenv = core.gisenv()
if grassenv.has_key("TDBMI"):
    if grassenv["TDBMI"] == "sqlite3":
        import sqlite3 as dbmi
    elif grassenv["TDBMI"] == "psycopg2":
        import psycopg2 as dbmi
        # Needed for dictionary like cursors
        import psycopg2.extras
    else:
        core.fatal(_("Unable to initialize the temporal DBMI interface: %s. \nPlease set g.gisenv set=\"TDBMI=sqlite3\" or g.gisenv set=\"TDBMI=psycopg2\"") % grassenv["TDBMI"])
else:
    # Use the default sqlite variable
    import sqlite3 as dbmi
    core.run_command("g.gisenv", set="TDBMI=sqlite3")
    core.run_command("g.gisenv", set="TDBMI_INIT=%s" % get_temporal_dbmi_default_path(grassenv))

###############################################################################

def get_temporal_dbmi_init_string():
    grassenv = core.gisenv()
    if dbmi.__name__ == "sqlite3":
        if grassenv.has_key("TDBMI_INIT"):
            return grassenv["TDBMI_INIT"]
        else:
            return get_temporal_dbmi_default_path(grassenv)
    elif dbmi.__name__ == "psycopg2":
        if grassenv.has_key("TDBMI_INIT"):
            return grassenv["TDBMI_INIT"]
        else:
	    return "dbname=grass_test user=soeren password=abcdefgh"

###############################################################################

def get_sql_template_path():
    base = os.getenv("GISBASE")
    base_etc  = os.path.join(base, "etc")
    return os.path.join(base_etc, "sql")

###############################################################################

def create_temporal_database():
    """This function creates the grass location database structure for raster, vector and raster3d maps
       as well as for the space-time datasets strds, str3ds and stvds

       This functions must be called befor any spatio-temporal processing is started
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
        cursor.execute("SELECT EXISTS(SELECT * FROM information_schema.tables WHERE table_name=%s)", ('raster_base',))
        db_exists = cursor.fetchone()[0]
        connection.commit()
        cursor.close()

    if db_exists == True:
	return
    
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

    # Connect to database
    connection = dbmi.connect(database)
    cursor = connection.cursor()

    if dbmi.__name__ == "sqlite3":
	
	sqlite3_delete_trigger_sql = open(os.path.join(get_sql_template_path(), "sqlite3_delete_trigger.sql"), 'r').read()
	
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
