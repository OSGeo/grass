# -*- coding: utf-8 -*-
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
from abstract_dataset import *
from datetime_math import *

###############################################################################

class abstract_map_dataset(abstract_dataset):
    """This is the base class for all maps (raster, vector, raster3d) 
       providing additional function to set the valid time and the spatial extent.
    """
      
    def get_new_stds_instance(self, ident):
        """Return a new space time dataset instance in which maps are stored with the type of this class

           @param ident: The identifier of the dataset
        """
        raise IOError("This method must be implemented in the subclasses")
    
    def get_stds_register(self):
        """Return the space time dataset register table name in which stds are listed in which this map is registered"""
        raise IOError("This method must be implemented in the subclasses")

    def set_stds_register(self, name):
        """Set the space time dataset register table name.
        
           This table stores all space time datasets in which this map is registered.

           @param ident: The name of the register table
        """
        raise IOError("This method must be implemented in the subclasses")
      
    def get_timestamp_module_name(self):
        """Return the name of the C-module to set the time stamp in the file system"""
        raise IOError("This method must be implemented in the subclasses")
   
    def load(self):
        """Load the content of this object from map files"""
        raise IOError("This method must be implemented in the subclasses")
 
    def check_resolution_with_current_region(self):
        """Check if the raster or voxel resolution is finer than the current resolution
           Return "finer" in case the raster/voxel resolution is finer than the current region
           Return "coarser" in case the raster/voxel resolution is coarser than the current region

           Vector maps are alwyas finer than the current region
        """
        raise IOError("This method must be implemented in the subclasses")

    def get_map_id(self):
	"""Return the map id. The map id is the unique map identifier in grass and must not be equal to the 
	   primary key identifier (id) of the map in the database. Since vector maps may have layer information,
	   the unique id is a combination of name, layer and mapset.
	   
	   Use get_map_id() every time your need to access the grass map in the file system but not to identify
	   map information in the temporal database.
	
	"""
        return self.base.get_map_id()

    def build_id(self, name, mapset, layer=None):
	"""Convenient method to build the unique identifier
	
	    Existing layer and mapset definitions in the name string will be reused

           @param return the id of the vector map as name(:layer)@mapset while layer is optional
        """
        
        # Check if the name includes any mapset
	if name.find("@") >= 0:
	    name, mapset = name.split("@")[0]

        # Check for layer number in map name
	if name.find(":") >= 0:
	    name, layer = name.split(":")
	    
        if layer:	    
	    return "%s:%s@%s"%(name, layer, mapset)
	else:
	    return "%s@%s"%(name, mapset)
	    
    def get_layer(self):
	"""Return the layer of the map or None in case no layer is defined"""
	return self.base.get_layer()
        
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
        print " |                                                                            |"
	self.base.print_info()
	if self.is_time_absolute():
	    self.absolute_time.print_info()
        if self.is_time_relative():
	    self.relative_time.print_info()
	self.spatial_extent.print_info()
	self.metadata.print_info()
        datasets = self.get_registered_datasets()
        count = 0
        string = ""
        if datasets:
            for ds in datasets:
                if count == 0:
                    string += ds["id"]
                else:
                    string += ",%s" % ds["id"]
                count += 1
                if count > 2:
                    string += " | ............................ "
        print " | Registered datasets ........ " + string
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
        datasets = self.get_registered_datasets()
        count = 0
        string = ""
        for ds in datasets:
            if count == 0:
                string += ds["id"]
            else:
                string += ",%s" % ds["id"]
            count += 1
        print "registered_datasets=" + string

    def set_absolute_time(self, start_time, end_time=None, timezone=None):
        """Set the absolute time interval with start time and end time
        
           @param start_time: a datetime object specifying the start time of the map
           @param end_time: a datetime object specifying the end time of the map
           @param timezone: Thee timezone of the map
        
        """
        if start_time and not isinstance(start_time, datetime) :
	    if self.get_layer():
		core.fatal(_("Start time must be of type datetime for %s map <%s> with layer: %s") % (self.get_type(), self.get_map_id(), self.get_layer()))
	    else:
		core.fatal(_("Start time must be of type datetime for %s map <%s>") % (self.get_type(), self.get_map_id()))

        if end_time and not isinstance(end_time, datetime) :
	    if self.get_layer():
		core.fatal(_("End time must be of type datetime for %s map <%s> with layer: %s") % (self.get_type(), self.get_map_id(), self.get_layer()))
	    else:
		core.fatal(_("End time must be of type datetime for %s map <%s>") % (self.get_type(), self.get_map_id()))

        if start_time and end_time:
            if start_time > end_time:
		if self.get_layer():
		    core.fatal(_("End time must be greater than start time for %s map <%s> with layer: %s") % (self.get_type(), self.get_map_id(), self.get_layer()))
		else:
		    core.fatal(_("End time must be greater than start time for %s map <%s>") % (self.get_type(), self.get_map_id()))
            else:
                # Do not create an interval in case start and end time are equal
                if start_time == end_time:
                    end_time = None

        self.base.set_ttype("absolute")
        
        self.absolute_time.set_start_time(start_time)
        self.absolute_time.set_end_time(end_time)
        self.absolute_time.set_timezone(timezone)

    def update_absolute_time(self, start_time, end_time=None, timezone=None, dbif = None):
        """Update the absolute time

           This method should always be used to set the absolute time. Do not use insert() or update()
           to the the time. This update functions assures that the *.timestamp commands are invoked.

           @param start_time: a datetime object specifying the start time of the map
           @param end_time: a datetime object specifying the end time of the map
           @param timezone: Thee timezone of the map
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        self.set_absolute_time(start_time, end_time, timezone)
        self.absolute_time.update_all(dbif)
        self.base.update(dbif)

        if connect == True:
            dbif.close()

        self.write_absolute_time_to_file()

    def write_absolute_time_to_file(self):
        """Start the grass timestamp module to set the time in the file system"""

        start_time, end_time, unit = self.get_absolute_time()
        start = datetime_to_grass_datetime_string(start_time)
        if end_time:
            end = datetime_to_grass_datetime_string(end_time)
            start += " / %s"%(end)

        core.run_command(self.get_timestamp_module_name(), map=self.get_map_id(), date=start)

    def set_relative_time(self, start_time, end_time, unit):
        """Set the relative time interval 
        
           @param start_time: A double value 
           @param end_time: A double value 
           @param unit: The unit of the relative time. Supported units: years, months, days, hours, minutes, seconds

           Return True for success and False otherwise

        """

        if not self.check_relative_time_unit(unit):
	    if self.get_layer():
		core.error(_("Unsupported relative time unit type for %s map <%s> with layer %s: %s") % (self.get_type(), self.get_id(), self.get_layer(), unit))
	    else:
		core.error(_("Unsupported relative time unit type for %s map <%s>: %s") % (self.get_type(), self.get_id(), unit))
            return False
        

        if start_time != None and end_time != None:
            if int(start_time) > int(end_time):
		if self.get_layer():
		    core.error(_("End time must be greater than start time for %s map <%s> with layer %s") % (self.get_type(), self.get_id(), self.get_layer()))
		else:
		    core.error(_("End time must be greater than start time for %s map <%s>") % (self.get_type(), self.get_id()))
                return False
            else:
                # Do not create an interval in case start and end time are equal
                if start_time == end_time:
                    end_time = None

        self.base.set_ttype("relative")
        
        self.relative_time.set_unit(unit)
        self.relative_time.set_start_time(int(start_time))
        if end_time != None:
            self.relative_time.set_end_time(int(end_time))
        else:
            self.relative_time.set_end_time(None)

        return True

    def update_relative_time(self, start_time, end_time, unit, dbif = None):
        """Update the relative time interval

           This method should always be used to set the absolute time. Do not use insert() or update()
           to the the time. This update functions assures that the *.timestamp commands are invoked.

           @param start_time: A double value 
           @param end_time: A double value 
           @param dbif: The database interface to be used
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        if self.set_relative_time(start_time, end_time, unit):
            self.relative_time.update_all(dbif)
            self.base.update(dbif)
            dbif.connection.commit()

        if connect == True:
            dbif.close()

        self.write_relative_time_to_file()

    def write_relative_time_to_file(self):
        """Start the grass timestamp module to set the time in the file system"""

        start_time, end_time, unit = self.get_relative_time()
        start = "%i %s"%(int(start_time), unit)
        if end_time != None:
            end = "%i %s"%(int(end_time), unit)
            start += " / %s"%(end)
        core.run_command(self.get_timestamp_module_name(), map=self.get_map_id(), date=start)

    def set_spatial_extent(self, north, south, east, west, top=0, bottom=0):
        """Set the spatial extent of the map

           @param north: The northern edge
           @param south: The southern edge
           @param east: The eastern edge
           @param west: The western edge
           @param top: The top edge
           @param bottom: The bottom edge
        """
        self.spatial_extent.set_spatial_extent(north, south, east, west, top, bottom)
        
    def check_valid_time(self):
        """Check for correct valid time"""
        if self.is_time_absolute():
            start, end, tz = self.get_absolute_time()
        else:
            start, end, unit = self.get_relative_time()

        if start != None:
            if end != None:
                if start >= end:
		    if self.get_layer():
			core.error(_("Map <%s> with layer %s has incorrect time interval, start time is greater than end time") % (self.get_map_id(), self.get_layer()))
		    else:
			core.error(_("Map <%s> has incorrect time interval, start time is greater than end time") % (self.get_map_id()))
                    return False
        else:
            core.error(_("Map <%s> has incorrect start time") % (self.get_map_id()))
            return False

        return True

    def delete(self, dbif=None, update=True):
	"""Delete a map entry from database if it exists
        
            Remove dependent entries:
            * Remove the map entry in each space time dataset in which this map is registered
            * Remove the space time dataset register table
            
           @param dbif: The database interface to be used
           @param update: Call for each unregister statement the update from registered maps 
                          of the space time dataset. This can slow down the un-registration process significantly.
        """

        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        if self.is_in_db(dbif):
 
            # SELECT all needed informations from the database
            self.select(dbif)
           
            # First we unregister from all dependent space time datasets
            self.unregister(dbif, update)

            # Remove the strds register table
            if self.get_stds_register():
                sql = "DROP TABLE " + self.get_stds_register()
                #print sql
                try:
                    dbif.cursor.execute(sql)
                except:
                    core.error(_("Unable to remove space time dataset register table <%s>") % (self.get_stds_register()))

            core.verbose(_("Delete %s dataset <%s> from temporal database") % (self.get_type(), self.get_id()))

            # Delete yourself from the database, trigger functions will take care of dependencies
            self.base.delete(dbif)

        # Remove the timestamp from the file system
        if self.get_type() == "vect":
	    if self.get_layer():
		core.run_command(self.get_timestamp_module_name(), map=self.get_map_id(), layer=self.get_layer(), date="none")
	    else:
		core.run_command(self.get_timestamp_module_name(), map=self.get_map_id(), date="none")
	else:
	    core.run_command(self.get_timestamp_module_name(), map=self.get_map_id(), date="none")

        self.reset(None)
        dbif.connection.commit()

        if connect == True:
            dbif.close()

    def unregister(self, dbif=None, update=True):
	""" Remove the map entry in each space time dataset in which this map is registered

           @param dbif: The database interface to be used
           @param update: Call for each unregister statement the update from registered maps 
                          of the space time dataset. This can slow down the un-registration process significantly.
        """

	if self.get_layer():
	    core.verbose(_("Unregister %s map <%s> with layer %s from space time datasets") % \
	                   (self.get_type(), self.get_map_id(), self.get_layer()))
	else:
	    core.verbose(_("Unregister %s map <%s> from space time datasets") % (self.get_type(), self.get_map_id()))
        
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        # Get all datasets in which this map is registered
        rows = self.get_registered_datasets(dbif)

        # For each stds in which the map is registered
        if rows:
            count = 0
            num_sps = len(rows)
            for row in rows:
                core.percent(count, num_sps, 1)
                count += 1
                # Create a space time dataset object to remove the map
                # from its register
                stds = self.get_new_stds_instance(row["id"])
                stds.select(dbif)
                stds.unregister_map(self, dbif)
                # Take care to update the space time dataset after
                # the map has been unregistered
                if update == True:
                    stds.update_from_registered_maps(dbif)

            core.percent(1, 1, 1)
        dbif.connection.commit()

        if connect == True:
            dbif.close()
            
    def get_registered_datasets(self, dbif=None):
        """Return all space time dataset ids in which this map is registered as
           dictionary like rows with column "id" or None if this map is not registered in any
           space time dataset.

           @param dbif: The database interface to be used
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        rows = None

        try:
            if self.get_stds_register() != None:
                # Select all stds tables in which this map is registered
                sql = "SELECT id FROM " + self.get_stds_register()
                dbif.cursor.execute(sql)
                rows = dbif.cursor.fetchall()
        except:
            core.error(_("Unable to select space time dataset register table <%s>") % (self.get_stds_register()))

        if connect == True:
            dbif.close()
            
        return rows

