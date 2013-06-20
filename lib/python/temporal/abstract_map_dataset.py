# -*- coding: utf-8 -*-
"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

>>> import grass.temporal as tgis
>>> amd = tgis.AbstractMapDataset()

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from abstract_dataset import *
from datetime_math import *


class AbstractMapDataset(AbstractDataset):
    """!This is the base class for all maps (raster, vector, raster3d).

        The temporal extent, the spatial extent and the metadata of maps
        are stored in the temporal database. Maps can be registered in the
        temporal database, updated and deleted.

        This class provides all functionalities that are needed to manage maps
        in the temporal database. That are:
        - insert() to register the map and therefore its spatio-temporal extent
                   and metadata in the temporal database
        - update() to update the map spatio-temporal extent and metadata in the
                   temporal database
        - unregister() to unregister the map from each space time dataset in
                       which this map is registered
        - delete() to remove the map from the temporal database
        - Methods to set relative and absolute time stamps
        - Abstract methods that must be implemented in the map specific
          subclasses
    """
    def __init__(self):
        AbstractDataset.__init__(self)

    def get_new_stds_instance(self, ident):
        """!Return a new space time dataset instance that store maps with the
           type of this map object (rast, rast3d or vect)

           @param ident The identifier of the space time dataset
           @return The new space time dataset instance
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def get_stds_register(self):
        """!Return the space time dataset register table name

            Maps can be registered in several different space time datasets.
            This method returns the name of the register table in the
            temporal database.

            @return The name of the stds register table
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def set_stds_register(self, name):
        """!Set the space time dataset register table name.

           This table stores all space time datasets in
           which this map is registered.

           @param name The name of the register table
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def check_resolution_with_current_region(self):
        """!Check if the raster or voxel resolution is
           finer than the current resolution

           - Return "finer" in case the raster/voxel resolution is finer
             than the current region
           - Return "coarser" in case the raster/voxel resolution is coarser
             than the current region

           Vector maps have no resolution, since they store the coordinates
           directly.

           @return "finer" or "coarser"
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def has_grass_timestamp(self):
        """!Check if a grass file based time stamp exists for this map.
            @return True is the grass file based time stamped exists for this
                    map
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def write_timestamp_to_grass(self):
        """!Write the timestamp of this map into the map metadata
           in the grass file system based spatial database.
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def remove_timestamp_from_grass(self):
        """!Remove the timestamp from the grass file
           system based spatial database
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def map_exists(self):
        """!Return True in case the map exists in the grass spatial database

           @return True if map exists, False otherwise
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def read_info(self):
        """!Read the map info from the grass file system based database and
           store the content into a dictionary
        """
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def load(self):
        """!Load the content of this object from the grass
           file system based database"""
        raise ImplementationError(
            "This method must be implemented in the subclasses")

    def _convert_timestamp(self):
        """!Convert the valid time into a grass datetime library
           compatible timestamp string

            This methods works for relative and absolute time

            @return the grass timestamp string
        """
        start = ""

        if self.is_time_absolute():
            start_time, end_time, tz = self.get_absolute_time()
            start = datetime_to_grass_datetime_string(start_time)
            if end_time is not None:
                end = datetime_to_grass_datetime_string(end_time)
                start += " / %s" % (end)
        else:
            start_time, end_time, unit = self.get_relative_time()
            start = "%i %s" % (int(start_time), unit)
            if end_time is not None:
                end = "%i %s" % (int(end_time), unit)
                start += " / %s" % (end)

        return start

    def get_map_id(self):
        """!Return the map id. The map id is the unique identifier
           in grass and must not be equal to the
           primary key identifier (id) of the map in the database.
           Since vector maps may have layer information,
           the unique id is a combination of name, layer and mapset.

           Use get_map_id() every time your need to access the grass map
           in the file system but not to identify
           map information in the temporal database.

           @return The map id "name@mapset"
        """
        return self.base.get_map_id()

    def build_id(self, name, mapset, layer=None):
        """!Convenient method to build the unique identifier

            Existing layer and mapset definitions in the name
            string will be reused

           @param name The name of the map
           @param mapset The mapset in which the map is located
           @param layer The layer of the vector map, use None in case no
                        layer exists

           @return the id of the map as "name(:layer)@mapset"
                   while layer is optional
        """

        # Check if the name includes any mapset
        if name.find("@") >= 0:
            name, mapset = name.split("@")

        # Check for layer number in map name
        if name.find(":") >= 0:
            name, layer = name.split(":")

        if layer is not None:
            return "%s:%s@%s" % (name, layer, mapset)
        else:
            return "%s@%s" % (name, mapset)

    def get_layer(self):
        """!Return the layer of the map
            @return the layer of the map or None in case no layer is defined
        """
        return self.base.get_layer()

    def print_info(self):
        """!Print information about this object in human readable style"""

        if self.get_type() == "raster":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print " +-------------------- Raster Dataset ----------------------------------------+"
        if self.get_type() == "raster3d":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print " +-------------------- 3D Raster Dataset -------------------------------------+"
        if self.get_type() == "vector":
            #                1         2         3         4         5         6         7
            #      0123456789012345678901234567890123456789012345678901234567890123456789012345678
            print " +-------------------- Vector Dataset ----------------------------------------+"
        print " |                                                                            |"
        self.base.print_info()
        self.temporal_extent.print_info()
        if self.is_topology_build():
            self.print_topology_info()
        self.spatial_extent.print_info()
        self.metadata.print_info()
        datasets = self.get_registered_datasets()
        count = 0
        string = ""
        if datasets is not None:
            for ds in datasets:
                if count > 0 and count % 3 == 0:
                    string += "\n | ............................ "
                    count = 0
                if count == 0:
                    string += ds["id"]
                else:
                    string += ",%s" % ds["id"]
                count += 1
        print " | Registered datasets ........ " + string
        print " +----------------------------------------------------------------------------+"

    def print_shell_info(self):
        """!Print information about this object in shell style"""
        self.base.print_shell_info()
        self.temporal_extent.print_shell_info()
        self.spatial_extent.print_shell_info()
        self.metadata.print_shell_info()
        datasets = self.get_registered_datasets()
        count = 0
        string = ""
        if datasets is not None:
            for ds in datasets:
                if count == 0:
                    string += ds["id"]
                else:
                    string += ",%s" % ds["id"]
                count += 1
            print "registered_datasets=" + string

        if self.is_topology_build():
            self.print_topology_shell_info()

    def insert(self, dbif=None, execute=True):
        """!Insert the map content into the database from the internal
           structure

           This functions assures that the timestamp is written to the
           grass file system based database in addition to the temporal
           database entry.

           @param dbif The database interface to be used
           @param execute If True the SQL statements will be executed.
                          If False the prepared SQL statements are
                          returned and must be executed by the caller.
           @return The SQL insert statement in case execute=False, or an
                   empty string otherwise
        """
        self.write_timestamp_to_grass()
        return AbstractDataset.insert(self, dbif, execute)

    def update(self, dbif=None, execute=True):
        """!Update the map content in the database from the internal structure
           excluding None variables

           This functions assures that the timestamp is written to the
           grass file system based database in addition to the temporal
           database entry.

           @param dbif The database interface to be used
           @param execute If True the SQL statements will be executed.
                          If False the prepared SQL statements are
                          returned and must be executed by the caller.
           @return The SQL insert statement in case execute=False, or an
                   empty string otherwise
        """
        self.write_timestamp_to_grass()
        return AbstractDataset.update(self, dbif, execute)

    def update_all(self, dbif=None, execute=True):
        """!Update the map content in the database from the internal structure
           including None variables

           This functions assures that the timestamp is written to the
           grass file system based database in addition to the temporal
           database entry.

           @param dbif The database interface to be used
           @param execute If True the SQL statements will be executed.
                          If False the prepared SQL statements are
                          returned and must be executed by the caller.
            @return The SQL insert statement in case execute=False, or an
                    empty string otherwise
        """
        self.write_timestamp_to_grass()
        return AbstractDataset.update_all(self, dbif, execute)

    def set_absolute_time(self, start_time, end_time=None, timezone=None):
        """!Set the absolute time with start time and end time

            The end time is optional and must be set to None in case of time
            instance.

            This method only modifies this object and does not commit
            the modifications to the temporal database.

           @param start_time a datetime object specifying the start time of the
                             map
           @param end_time a datetime object specifying the end time of the
                           map, None in case or time instance
           @param timezone Thee timezone of the map (not used)
        """
        if start_time and not isinstance(start_time, datetime):
            if self.get_layer() is not None:
                core.fatal(_("Start time must be of type datetime for %(type)s"
                             " map <%(id)s> with layer: %(l)s") % {
                             'type': self.get_type(), 'id': self.get_map_id(), 
                             'l': self.get_layer()})
            else:
                core.fatal(_("Start time must be of type datetime for "
                             "%(type)s map <%(id)s>") % {
                             'type': self.get_type(), 'id': self.get_map_id()})

        if end_time and not isinstance(end_time, datetime):
            if self.get_layer():
                core.fatal(_("End time must be of type datetime for %(type)s "
                             "map <%(id)s> with layer: %(l)s") % {
                             'type': self.get_type(), 'id': self.get_map_id(),
                             'l': self.get_layer()})
            else:
                core.fatal(_("End time must be of type datetime for "
                             "%(type)s map <%(id)s>") % {
                             'type': self.get_type(), 'id': self.get_map_id()})

        if start_time is not None and end_time is not None:
            if start_time > end_time:
                if self.get_layer():
                    core.fatal(_("End time must be greater than start time for"
                                 " %(type)s map <%(id)s> with layer: %(l)s") % {
                                 'type': self.get_type(),
                                 'id': self.get_map_id(),
                                 'l': self.get_layer()})
                else:
                    core.fatal(_("End time must be greater than start time "
                                 "for %(type)s map <%(id)s>") % {
                                 'type': self.get_type(),
                                 'id': self.get_map_id()})
            else:
                # Do not create an interval in case start and end time are
                # equal
                if start_time == end_time:
                    end_time = None

        self.base.set_ttype("absolute")
        self.absolute_time.set_start_time(start_time)
        self.absolute_time.set_end_time(end_time)
        self.absolute_time.set_timezone(timezone)

        return True

    def update_absolute_time(self, start_time, end_time=None,
                             timezone=None, dbif=None):
        """!Update the absolute time

           The end time is optional and must be set to None in case of time
           instance.

           This functions assures that the timestamp is written to the
           grass file system based database in addition to the temporal
           database entry.

           @param start_time a datetime object specifying the start time of
                  the map
           @param end_time a datetime object specifying the end time of the
                  map, None in case or time instance
           @param timezone Thee timezone of the map (not used)
           @param dbif The database interface to be used
        """
        dbif, connected = init_dbif(dbif)

        self.set_absolute_time(start_time, end_time, timezone)
        self.absolute_time.update_all(dbif)
        self.base.update(dbif)

        if connected:
            dbif.close()

        self.write_timestamp_to_grass()

    def set_relative_time(self, start_time, end_time, unit):
        """!Set the relative time interval

            The end time is optional and must be set to None in case of time
            instance.

            This method only modifies this object and does not commit
            the modifications to the temporal database.

           @param start_time An integer value
           @param end_time An integer value, None in case or time instance
           @param unit The unit of the relative time. Supported units:
                       year(s), month(s), day(s), hour(s), minute(s), second(s)

           @return True for success and False otherwise

        """

        if not self.check_relative_time_unit(unit):
            if self.get_layer() is not None:
                core.error(_("Unsupported relative time unit type for %(type)s"
                             " map <%(id)s> with layer %(l)s: %(u)s") % {
                             'type': self.get_type(), 'id': self.get_id(),
                             'l': self.get_layer(), 'u': unit})
            else:
                core.error(_("Unsupported relative time unit type for %(type)s"
                             " map <%(id)s>: %(u)s") % {
                             'type': self.get_type(), 'id': self.get_id(),
                             'u': unit})
            return False

        if start_time is not None and end_time is not None:
            if int(start_time) > int(end_time):
                if self.get_layer() is not None:
                    core.error(_("End time must be greater than start time for"
                                 " %(type)s map <%(id)s> with layer %(l)s") % \
                                 {'type': self.get_type(), 'id': self.get_id(),
                                  'l': self.get_layer()})
                else:
                    core.error(_("End time must be greater than start time for"
                                 " %(type)s map <%(id)s>") % {
                                 'type': self.get_type(), 'id': self.get_id()})
                return False
            else:
                # Do not create an interval in case start and end time are
                # equal
                if start_time == end_time:
                    end_time = None

        self.base.set_ttype("relative")

        self.relative_time.set_unit(unit)
        self.relative_time.set_start_time(int(start_time))
        if end_time is not None:
            self.relative_time.set_end_time(int(end_time))
        else:
            self.relative_time.set_end_time(None)

        return True

    def update_relative_time(self, start_time, end_time, unit, dbif=None):
        """!Update the relative time interval

           The end time is optional and must be set to None in case of time
           instance.

           This functions assures that the timestamp is written to the
           grass file system based database in addition to the temporal
           database entry.

           @param start_time An integer value
           @param end_time An integer value, None in case or time instance
           @param unit The relative time unit
           @param dbif The database interface to be used
        """
        dbif, connected = init_dbif(dbif)

        if self.set_relative_time(start_time, end_time, unit):
            self.relative_time.update_all(dbif)
            self.base.update(dbif)

        if connected:
            dbif.close()

        self.write_timestamp_to_grass()

    def temporal_buffer(self, increment, update=False, dbif=None):
        """!Create a temporal buffer based on an increment

           For absolute time the increment must be a string of type "integer
           unit"
           Unit can be year, years, month, months, day, days, hour, hours,
           minute, minutes, day or days.

           @param increment This is the increment, a string in case of absolute
                            time or an integer in case of relative time
           @param update Perform an immediate database update to store the
                         modified temporal extent, otherwise only this object
                         will be modified

           Usage:

           @code

           >>> import grass.temporal as tgis
           >>> maps = []
           >>> for i in range(5):
           ...   map = tgis.RasterDataset(None)
           ...   if i%2 == 0:
           ...       check = map.set_relative_time(i, i + 1, 'years')
           ...   else:
           ...       check = map.set_relative_time(i, None, 'years')
           ...   map.temporal_buffer(3)
           ...   maps.append(map)
           >>> for map in maps:
           ...   map.temporal_extent.print_info()
            +-------------------- Relative time -----------------------------------------+
            | Start time:................. -3
            | End time:................... 4
            | Relative time unit:......... years
            +-------------------- Relative time -----------------------------------------+
            | Start time:................. -2
            | End time:................... 4
            | Relative time unit:......... years
            +-------------------- Relative time -----------------------------------------+
            | Start time:................. -1
            | End time:................... 6
            | Relative time unit:......... years
            +-------------------- Relative time -----------------------------------------+
            | Start time:................. 0
            | End time:................... 6
            | Relative time unit:......... years
            +-------------------- Relative time -----------------------------------------+
            | Start time:................. 1
            | End time:................... 8
            | Relative time unit:......... years
           >>> maps = []
           >>> for i in range(1,5):
           ...   map = tgis.RasterDataset(None)
           ...   if i%2 == 0:
           ...       check = map.set_absolute_time(datetime(2001,i,1), datetime(2001, i + 1, 1))
           ...   else:
           ...       check = map.set_absolute_time(datetime(2001,i,1),  None)
           ...   map.temporal_buffer("7 days")
           ...   maps.append(map)
           >>> for map in maps:
           ...   map.temporal_extent.print_info()
            +-------------------- Absolute time -----------------------------------------+
            | Start time:................. 2000-12-25 00:00:00
            | End time:................... 2001-01-08 00:00:00
            +-------------------- Absolute time -----------------------------------------+
            | Start time:................. 2001-01-25 00:00:00
            | End time:................... 2001-03-08 00:00:00
            +-------------------- Absolute time -----------------------------------------+
            | Start time:................. 2001-02-22 00:00:00
            | End time:................... 2001-03-08 00:00:00
            +-------------------- Absolute time -----------------------------------------+
            | Start time:................. 2001-03-25 00:00:00
            | End time:................... 2001-05-08 00:00:00

           @endcode
        """

        if self.is_time_absolute():
            start, end, tz = self.get_absolute_time()

            new_start = decrement_datetime_by_string(start, increment)
            if end == None:
                new_end = increment_datetime_by_string(start, increment)
            else:
                new_end = increment_datetime_by_string(end, increment)

            if update:
                self.update_absolute_time(new_start, new_end, tz, dbif=dbif)
            else:
                self.set_absolute_time(new_start, new_end, tz)
        else:
            start, end, unit = self.get_relative_time()
            new_start = start - increment
            if end == None:
                new_end = start + increment
            else:
                new_end = end + increment

            if update:
                self.update_relative_time(new_start, new_end, unit, dbif=dbif)
            else:
                self.set_relative_time(new_start, new_end, unit)

    def set_spatial_extent(self, north, south, east, west, top=0, bottom=0):
        """!Set the spatial extent of the map

            This method only modifies this object and does not commit
            the modifications to the temporal database.

           @param north The northern edge
           @param south The southern edge
           @param east The eastern edge
           @param west The western edge
           @param top The top edge
           @param bottom The bottom edge
        """
        self.spatial_extent.set_spatial_extent(
            north, south, east, west, top, bottom)

    def check_for_correct_time(self):
        """!Check for correct time"""
        if self.is_time_absolute():
            start, end, tz = self.get_absolute_time()
        else:
            start, end, unit = self.get_relative_time()

        if start is not None:
            if end is not None:
                if start >= end:
                    if self.get_layer() is not None:
                        core.error(_("Map <%(id)s> with layer %(layer)s has "
                                     "incorrect time interval, start time is "
                                     "greater than end time") % {
                                     'id': self.get_map_id(),
                                     'layer': self.get_layer()})
                    else:
                        core.error(_("Map <%s> has incorrect time interval, "
                                     "start time is greater than end time") % \
                                   (self.get_map_id()))
                    return False
        else:
            core.error(_("Map <%s> has incorrect start time") %
                       (self.get_map_id()))
            return False

        return True

    def delete(self, dbif=None, update=True, execute=True):
        """!Delete a map entry from database if it exists

            Remove dependent entries:
            * Remove the map entry in each space time dataset in which this map
              is registered
            * Remove the space time dataset register table

           @param dbif The database interface to be used
           @param update Call for each unregister statement the update from
                          registered maps of the space time dataset.
                          This can slow down the un-registration process
                          significantly.
           @param execute If True the SQL DELETE and DROP table statements will
                           be executed.
                           If False the prepared SQL statements are
                           returned and must be executed by the caller.

           @return The SQL statements if execute=False, else an empty string,
                   None in case of a failure
        """

        dbif, connected = init_dbif(dbif)
        statement = ""

        if self.is_in_db(dbif):

            # SELECT all needed information from the database
            self.metadata.select(dbif)

            # First we unregister from all dependent space time datasets
            statement += self.unregister(
                dbif=dbif, update=update, execute=False)

            # Remove the strds register table
            if self.get_stds_register() is not None:
                statement += "DROP TABLE " + self.get_stds_register() + ";\n"

            # Commented because of performance issue calling g.message thousend times
            #core.verbose(_("Delete %s dataset <%s> from temporal database")
            #             % (self.get_type(), self.get_id()))

            # Delete yourself from the database, trigger functions will 
            # take care of dependencies
            statement += self.base.get_delete_statement()

        if execute:
            dbif.execute_transaction(statement)

        # Remove the timestamp from the file system
        self.remove_timestamp_from_grass()

        self.reset(None)

        if connected:
            dbif.close()

        if execute:
            return ""

        return statement

    def unregister(self, dbif=None, update=True, execute=True):
        """! Remove the map entry in each space time dataset in which this map
           is registered

           @param dbif The database interface to be used
           @param update Call for each unregister statement the update from
                         registered maps of the space time dataset. This can
                         slow down the un-registration process significantly.
           @param execute If True the SQL DELETE and DROP table statements
                          will be executed.
                          If False the prepared SQL statements are
                          returned and must be executed by the caller.

           @return The SQL statements if execute=False, else an empty string
        """

        # Commented because of performance issue calling g.message thousend times
        #if self.get_layer() is not None:
        #    core.verbose(_("Unregister %(type)s map <%(map)s> with "
        #                   "layer %(layer)s from space time datasets" % \
        #                 {'type':self.get_type(), 'map':self.get_map_id(), 
        #                  'layer':self.get_layer()}))
        #else:
        #    core.verbose(_("Unregister %(type)s map <%(map)s> "
        #                   "from space time datasets"
        #                 % {'type':self.get_type(), 'map':self.get_map_id()}))

        statement = ""
        dbif, connected = init_dbif(dbif)

        # Get all datasets in which this map is registered
        rows = self.get_registered_datasets(dbif)

        # For each stds in which the map is registered
        if rows is not None:
            for row in rows:
                # Create a space time dataset object to remove the map
                # from its register
                stds = self.get_new_stds_instance(row["id"])
                stds.metadata.select(dbif)
                statement += stds.unregister_map(self, dbif, False)
                # Take care to update the space time dataset after
                # the map has been unregistered
                if update == True and execute == True:
                    stds.update_from_registered_maps(dbif)

        if execute:
            dbif.execute_transaction(statement)

        if connected:
            dbif.close()

        if execute:
            return ""

        return statement

    def get_registered_datasets(self, dbif=None):
        """!Return all space time dataset ids in which this map is registered
           as dictionary like rows with column "id" or None if this map is not
           registered in any space time dataset.

           @param dbif The database interface to be used
           @return The SQL rows with the ids of all space time datasets in
                   which this map is registered
        """
        dbif, connected = init_dbif(dbif)

        rows = None

        try:
            if self.get_stds_register() is not None:
                # Select all stds tables in which this map is registered
                sql = "SELECT id FROM " + self.get_stds_register()
                dbif.cursor.execute(sql)
                rows = dbif.cursor.fetchall()
        except:
            core.error(_("Unable to select space time dataset register table "
                         "<%s>") % (self.get_stds_register()))

        if connected:
            dbif.close()

        return rows

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
