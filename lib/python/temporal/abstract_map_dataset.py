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
        
    def set_absolute_time(self, start_time, end_time=None, timezone=None):
        """Set the absolute time interval with start time and end time
        
           @param start_time: a datetime object specifying the start time of the map
           @param end_time: a datetime object specifying the end time of the map
           @param timezone: Thee timezone of the map
        
        """
        if start_time != None and not isinstance(start_time, datetime) :
            core.fatal(_("Start time must be of type datetime"))

        if end_time != None and not isinstance(end_time, datetime) :
            core.fatal(_("End time must be of type datetime"))

        if start_time != None and end_time != None:
            if start_time >= end_time:
                core.error(_("End time must be later than start time"))
                return False

        self.base.set_ttype("absolute")
        
        self.absolute_time.set_start_time(start_time)
        self.absolute_time.set_end_time(end_time)
        self.absolute_time.set_timezone(timezone)

        return True

    def update_absolute_time(self, start_time, end_time=None, timezone=None, dbif = None):
        """Update the absolute time

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

    def set_relative_time(self, start_time, end_time=None):
        """Set the relative time interval 
        
           @param start_time: A double value in days
           @param end_time: A double value in days

        """
        if start_time != None and end_time != None:
            if abs(float(start_time)) >= abs(float(end_time)):
                core.error(_("End time must be greater than start time"))
                return False

        self.base.set_ttype("relative")
        
        self.relative_time.set_start_time(float(start_time))
        if end_time != None:
            self.relative_time.set_end_time(float(end_time))
        else:
            self.relative_time.set_end_time(None)

        return True

    def update_relative_time(self, start_time, end_time=None, dbif = None):
        """Update the relative time interval

           @param start_time: A double value in days
           @param end_time: A double value in days
           @param dbif: The database interface to be used
        """
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        self.set_relative_time(start_time, end_time)
        self.relative_time.update_all(dbif)
        self.base.update(dbif)
        dbif.connection.commit()

        if connect == True:
            dbif.close()

    def set_spatial_extent(self, north, south, east, west, top=0, bottom=0):
        """Set the spatial extent of the map

           @param north: The northern edge
           @param south: The southern edge
           @param east: The eastern edge
           @param west: The western edge
           @param top: The top edge
           @param bottom: The bottom ege
        """
        self.spatial_extent.set_spatial_extent(north, south, east, west, top, bottom)
        
    def delete(self, dbif=None):
	"""Delete a map entry from database if it exists
        
            Remove dependent entries:
            * Remove the map entry in each space time dataset in which this map is registered
            * Remove the space time dataset register table
            
           @param dbif: The database interface to be used
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
            self.unregister(dbif)

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

        self.reset(None)
        dbif.connection.commit()

        if connect == True:
            dbif.close()

    def unregister(self, dbif=None):
	""" Remove the map entry in each space time dataset in which this map is registered

           @param dbif: The database interface to be used
        """

        core.verbose(_("Unregister %s dataset <%s> from space time datasets") % (self.get_type(), self.get_id()))
        
        connect = False

        if dbif == None:
            dbif = sql_database_interface()
            dbif.connect()
            connect = True

        # Get all datasets in which this map is registered
        rows = self.get_registered_datasets(dbif)

        # For each stds in which the map is registered
        if rows:
            for row in rows:
                # Create a space time dataset object to remove the map
                # from its register
                stds = self.get_new_stds_instance(row["id"])
                stds.select(dbif)
                stds.unregister_map(self, dbif)
                # Take care to update the space time dataset after
                # the map has been unregistred
                stds.update_from_registered_maps(dbif)

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

