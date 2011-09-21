"""!@package grass.script.tgis_core

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS core functions to be used in Python sripts.

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
import grass.script.core as core
import copy
from datetime import datetime, date, time, timedelta

###############################################################################

def get_grass_location_db_path():
    grassenv = core.gisenv()
    dbpath = os.path.join(grassenv["GISDBASE"], grassenv["LOCATION_NAME"])
    return os.path.join(dbpath, "grass.db")

###############################################################################

def get_sql_template_path():
    base = os.getenv("GISBASE")
    base_etc  = os.path.join(base, "etc")
    return os.path.join(base_etc, "sql")

def test_increment_datetime_by_string():

    dt = datetime(2001, 9, 1, 0, 0, 0)
    string = "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years"

    dt1 = datetime(2003,2,18,12,5,0)
    dt2 = increment_datetime_by_string(dt, string)

    delta = dt1 -dt2

    if delta.days != 0 or delta.seconds != 0:
        core.fatal("increment computation is wrong")

def increment_datetime_by_string(mydate, increment, mult = 1):
    """Return a new datetime object incremented with the provided relative dates specified as string.
       Additional a multiplier can be specified to multiply the increment bevor adding to the provided datetime object.

       @mydate A datetime object to incremented
       @increment A string providing increment information:
                  The string may include comma separated values of type seconds, minutes, hours, days, weeks, months and years
                  Example: Increment the datetime 2001-01-01 00:00:00 with "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years"
                  will result in the datetime 2003-02-18 12:05:00
        @mult A multiplier, default is 1
    """

    if increment:

        seconds = 0
        minutes = 0
        hours = 0
        days = 0
        weeks = 0
        months = 0
        years = 0

        inclist = []
        # Split the increment string
        incparts = increment.split(",")
        for incpart in incparts:
            inclist.append(incpart.strip().split(" "))

        for inc in inclist:
            if inc[1].find("seconds") >= 0:
                seconds = mult * int(inc[0])
            elif inc[1].find("minutes") >= 0:
                minutes = mult * int(inc[0])
            elif inc[1].find("hours") >= 0:
                hours = mult * int(inc[0])
            elif inc[1].find("days") >= 0:
                days = mult * int(inc[0])
            elif inc[1].find("weeks") >= 0:
                weeks = mult * int(inc[0])
            elif inc[1].find("months") >= 0:
                months = mult * int(inc[0])
            elif inc[1].find("years") >= 0:
                years = mult * int(inc[0])
            else:
                core.fatal("Wrong increment format: " + increment)

        return increment_datetime(mydate, years, months, weeks, days, hours, minutes, seconds)
    
    return mydate

###############################################################################

def increment_datetime(mydate, years=0, months=0, weeks=0, days=0, hours=0, minutes=0, seconds=0):
    """Return a new datetime object incremented with the provided relative dates and times"""

    tdelta_seconds = timedelta(seconds=seconds)
    tdelta_minutes = timedelta(minutes=minutes)
    tdelta_hours = timedelta(hours=hours)
    tdelta_days = timedelta(days=days)
    tdelta_weeks = timedelta(weeks=weeks)
    tdelta_months = timedelta(0)
    tdelta_years = timedelta(0)

    if months > 0:
        # Compute the actual number of days in the month to add as timedelta
        year = mydate.year
        month = mydate.month

        all_months = int(months + month)

        years_to_add = int(all_months/12)
        residual_months = all_months%12

        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)

        # Make sure the montha starts with a 1
        if residual_months == 0:
            residual_months = 1

        dt1 = dt1.replace(year = year + years_to_add, month = residual_months)
        tdelta_months = dt1 - mydate

    if years > 0:
        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)
        # Compute the number of days
        dt1 = dt1.replace(year=mydate.year + int(years))
        tdelta_years = dt1 - mydate

    return mydate + tdelta_seconds + tdelta_minutes + tdelta_hours + \
                    tdelta_days + tdelta_weeks + tdelta_months + tdelta_years

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
