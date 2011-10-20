"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

@code
import grass.temporal as tgis

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import uuid
import copy
from temporal_extent import *
from spatial_extent import *
from metadata import *

class abstract_dataset(object):
    """This is the base class for all datasets (raster, vector, raster3d, strds, stvds, str3ds)"""

    def reset(self, ident):
	"""Reset the internal structure and set the identifier

           @param ident: The identifier of the dataset
        """
	raise IOError("This method must be implemented in the subclasses")

    def get_type(self):
        """Return the type of this class"""
        raise IOError("This method must be implemented in the subclasses")
    
    def get_new_instance(self, ident):
        """Return a new instance with the type of this class

           @param ident: The identifier of the dataset
        """
        raise IOError("This method must be implemented in the subclasses")

    def get_id(self):
        return self.base.get_id()

    def get_valid_time(self):
        """Returns a tuple of the start, the end valid time, this can be either datetime or double values
           @return A tuple of (start_time, end_time)
        """

        start = None
        end = None
               
	if self.is_time_absolute():
            start = self.absolute_time.get_start_time()
            end = self.absolute_time.get_end_time()
        if self.is_time_relative():
            start = self.relative_time.get_start_time()
            end = self.relative_time.get_end_time()
        
        return (start, end)
 
    def get_absolute_time(self):
        """Returns a tuple of the start, the end valid time and the timezone of the map
           @return A tuple of (start_time, end_time, timezone)
        """
               
        start = self.absolute_time.get_start_time()
        end = self.absolute_time.get_end_time()
        tz = self.absolute_time.get_timezone()
        
        return (start, end, tz)
    
    def get_relative_time(self):
        """Returns the relative time interval (start_time, end_time) or None if not present"""

        start = self.relative_time.get_start_time()
        end = self.relative_time.get_end_time()

        return (start, end)

    def get_temporal_type(self):
        """Return the temporal type of this dataset"""
        return self.base.get_ttype()
    
    def get_spatial_extent(self):
        """Return a tuple of spatial extent (north, south, east, west, top, bottom) """
        
        north = self.spatial_extent.get_north()
        south = self.spatial_extent.get_south()
        east = self.spatial_extent.get_east()
        west = self.spatial_extent.get_west()
        top = self.spatial_extent.get_top()
        bottom = self.spatial_extent.get_bottom()
        
        return (north, south, east, west, top, bottom)
        
    def select(self, dbif=None):
	"""Select temporal dataset entry from database and fill up the internal structure"""

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        dbif.cursor.execute("BEGIN TRANSACTION")

	self.base.select(dbif)
	if self.is_time_absolute():
	    self.absolute_time.select(dbif)
        if self.is_time_relative():
	    self.relative_time.select(dbif)
	self.spatial_extent.select(dbif)
	self.metadata.select(dbif)

        dbif.cursor.execute("COMMIT TRANSACTION")

        if connect:
            dbif.close()
        
    def is_in_db(self, dbif=None):
	"""Check if the temporal dataset entry is in the database"""
	return self.base.is_in_db(dbif)

    def delete(self):
	"""Delete temporal dataset entry from database if it exists"""
        raise IOError("This method must be implemented in the subclasses")

    def insert(self, dbif=None):
	"""Insert temporal dataset entry into database from the internal structure"""

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        dbif.cursor.execute("BEGIN TRANSACTION")


	self.base.insert(dbif)
	if self.is_time_absolute():
	    self.absolute_time.insert(dbif)
        if self.is_time_relative():
	    self.relative_time.insert(dbif)
	self.spatial_extent.insert(dbif)
	self.metadata.insert(dbif)

        dbif.cursor.execute("COMMIT TRANSACTION")

        if connect:
            dbif.close()
 
    def update(self, dbif=None):
	"""Update temporal dataset entry of database from the internal structure
	   excluding None variables
	"""

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        dbif.cursor.execute("BEGIN TRANSACTION")


	self.base.update(dbif)
	if self.is_time_absolute():
	    self.absolute_time.update(dbif)
        if self.is_time_relative():
	    self.relative_time.update(dbif)
	self.spatial_extent.update(dbif)
	self.metadata.update(dbif)

        dbif.cursor.execute("COMMIT TRANSACTION")

        if connect:
            dbif.close()
 
    def update_all(self, dbif=None):
	"""Update temporal dataset entry of database from the internal structure
	   and include None varuables.

           @param dbif: The database interface to be used
	"""

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        dbif.cursor.execute("BEGIN TRANSACTION")


	self.base.update_all(dbif)
	if self.is_time_absolute():
	    self.absolute_time.update_all(dbif)
        if self.is_time_relative():
	    self.relative_time.update_all(dbif)
	self.spatial_extent.update_all(dbif)
	self.metadata.update_all(dbif)

        dbif.cursor.execute("COMMIT TRANSACTION")

        if connect:
            dbif.close()
 
    def print_self(self):
	"""Print the content of the internal structure to stdout"""
	self.base.print_self()
	if self.is_time_absolute():
	    self.absolute_time.print_self()
        if self.is_time_relative():
	    self.relative_time.print_self()
	self.spatial_extent.print_self()
	self.metadata.print_self()

    def print_info(self):
        """Print information about this class in human readable style"""
        
        if self.get_type() == "raster":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Raster Dataset ----------------------------------------+"
        if self.get_type() == "raster3d":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Raster3d Dataset --------------------------------------+"
        if self.get_type() == "vector":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Vector Dataset ----------------------------------------+"
        if self.get_type() == "strds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Space Time Raster Dataset -----------------------------+"
        if self.get_type() == "str3ds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Space Time Raster3d Dataset ---------------------------+"
        if self.get_type() == "stvds":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print ""
            print " +-------------------- Space Time Vector Dataset -----------------------------+"
        print " |                                                                            |"
	self.base.print_info()
	if self.is_time_absolute():
	    self.absolute_time.print_info()
        if self.is_time_relative():
	    self.relative_time.print_info()
	self.spatial_extent.print_info()
	self.metadata.print_info()
        print " +----------------------------------------------------------------------------+"

    def print_shell_info(self):
        """Print information about this class in shell style"""
	self.base.print_shell_info()
	if self.is_time_absolute():
	    self.absolute_time.print_shell_info()
        if self.is_time_relative():
	    self.relative_time.print_shell_info()
	self.spatial_extent.print_shell_info()
	self.metadata.print_shell_info()

    def set_time_to_absolute(self):
	self.base.set_ttype("absolute")

    def set_time_to_relative(self):
        self.base.set_ttype("relative")

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
	"""Return the temporal relation of this and the provided temporal map"""
	if self.is_time_absolute() and map.is_time_absolute():
	    return self.absolute_time.temporal_relation(map.absolute_time)
        if self.is_time_relative() and map.is_time_relative():
	    return self.relative_time.temporal_relation(map.relative_time)
    	return None

