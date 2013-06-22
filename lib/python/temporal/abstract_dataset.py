# -*- coding: utf-8 -*-
"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

@code

>>> import grass.temporal as tgis
>>> ad = AbstractDataset()

@endcode

(C) 2011-2012 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import uuid
import copy
from abc import ABCMeta, abstractmethod
from temporal_extent import *
from spatial_extent import *
from metadata import *
from temporal_topology_dataset_connector import *
from spatial_topology_dataset_connector import *


class ImplementationError(Exception):
    """!Exception raised for the calling of methods that should be implemented in
       sub classes.
    """
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return repr(self.msg)
    
###############################################################################

class AbstractDataset(SpatialTopologyDatasetConnector, TemporalTopologyDatasetConnector):
    """!This is the base class for all datasets 
       (raster, vector, raster3d, strds, stvds, str3ds)"""
    
    __metaclass__ = ABCMeta
    
    def __init__(self):
        SpatialTopologyDatasetConnector.__init__(self)
        TemporalTopologyDatasetConnector.__init__(self)
        
    def reset_topology(self):
        """!Reset any information about temporal topology"""
        self.reset_spatial_topology()
        self.reset_temporal_topology()
        
    def get_number_of_relations(self):      
        """! Return a dictionary in which the keys are the relation names and the value
        are the number of relations.
        
        The following relations are available:
        
        Spatial relations
        - equivalent
        - overlap
        - in
        - contain
        - meet
        - cover
        - covered
        
        Temporal relations
        - equal
        - follows
        - precedes
        - overlaps
        - overlapped
        - during (including starts, finishes)
        - contains (including started, finished)
        - starts
        - started
        - finishes
        - finished
       
        To access topological information the spatial, temporal or booth topologies must be build first
        using the SpatioTemporalTopologyBuilder.
        
        @return the dictionary with relations as keys and number as values or None in case the topology  wasn't build
        """
        if self.is_temporal_topology_build() and not self.is_spatial_topology_build():
            return self.get_number_of_temporal_relations()
        elif self.is_spatial_topology_build() and not self.is_temporal_topology_build():
            self.get_number_of_spatial_relations()
        else:
            return  self.get_number_of_temporal_relations() + \
                    self.get_number_of_spatial_relations()
            
        return None

    def set_topology_build_true(self):
        """!Use this method when the spatio-temporal topology was build"""
        self.set_spatial_topology_build_true()
        self.set_temporal_topology_build_true()
        

    def set_topology_build_false(self):
        """!Use this method when the spatio-temporal topology was not build"""
        self.set_spatial_topology_build_false()
        self.set_temporal_topology_build_false()

    def is_topology_build(self):
        """!Check if the spatial and temporal topology was build
        
           @return A dictionary with "spatial" and "temporal" as keys that have boolen values
        """
        d = {}
        d["spatial"] = self.is_spatial_topology_build()
        d["temporal"] = self.is_temporal_topology_build()
        
        return d
        

    def print_topology_info(self):
        if self.is_temporal_topology_build():
            self.print_temporal_topology_info()
        if self.is_spatial_topology_build():
            self.print_spatial_topology_info()
            
    def print_topology_shell_info(self):
        if self.is_temporal_topology_build():
            self.print_temporal_topology_shell_info()
        if self.is_spatial_topology_build():
            self.print_spatial_topology_shell_info()
            
    @abstractmethod
    def reset(self, ident):
        """!Reset the internal structure and set the identifier
        
            This method creates the dataset specific internal objects
            that store the base information, the spatial and temporal extent
            and the metadata. It must be implemented in the dataset
            specific subclasses. This is the code for the 
            vector dataset:
            
            self.base = VectorBase(ident=ident)
            self.absolute_time = VectorAbsoluteTime(ident=ident)
            self.relative_time = VectorRelativeTime(ident=ident)
            self.spatial_extent = VectorSpatialExtent(ident=ident)
            self.metadata = VectorMetadata(ident=ident)
        
           @param ident The identifier of the dataset that  "name@mapset" or in case of vector maps "name:layer@mapset"
        """

    @abstractmethod
    def get_type(self):
        """!Return the type of this class as string
           
           The type can be "vect", "rast", "rast3d", "stvds", "strds" or "str3ds" 
           
           @return "vect", "rast", "rast3d", "stvds", "strds" or "str3ds"
        """

    @abstractmethod
    def get_new_instance(self, ident):
        """!Return a new instance with the type of this class

           @param ident The identifier of the new dataset instance
           @return A new instance with the type of this object
        """

    @abstractmethod
    def spatial_overlapping(self, dataset):
        """!Return True if the spatial extents overlap
        
           @param dataset The abstract dataset to check spatial overlapping
           @return True if self and the provided dataset spatial overlap
        """

    @abstractmethod
    def spatial_intersection(self, dataset):
        """!Return the spatial intersection as spatial_extent 
           object or None in case no intersection was found.
           
           @param dataset The abstract dataset to intersect with
           @return The intersection spatial extent
        """

    @abstractmethod
    def spatial_union(self, dataset):
        """!Return the spatial union as spatial_extent 
           object or None in case the extents does not overlap or meet.
       
           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
    
    @abstractmethod
    def spatial_disjoint_union(self, dataset):
        """!Return the spatial union as spatial_extent object.
       
           @param dataset The abstract dataset to create a union with
           @return The union spatial extent
        """
    
    @abstractmethod
    def spatial_relation(self, dataset):
        """!Return the spatial relationship between self and dataset
        
           @param dataset The abstract dataset to compute the spatial relation with self
           @return The spatial relationship as string
        """

    @abstractmethod
    def print_info(self):
        """!Print information about this class in human readable style"""

    @abstractmethod
    def print_shell_info(self):
        """!Print information about this class in shell style"""

    @abstractmethod
    def print_self(self):
        """!Print the content of the internal structure to stdout"""

    def set_id(self, ident):
        """!Set the identifier of the dataset"""
        self.base.set_id(ident)
        self.temporal_extent.set_id(ident)
        self.spatial_extent.set_id(ident)
        self.metadata.set_id(ident)

    def get_id(self):
        """!Return the unique identifier of the dataset
           @return The id of the dataset "name(:layer)@mapset" as string
        """
        return self.base.get_id()

    def get_name(self):
        """!Return the name
           @return The name of the dataset as string
        """
        return self.base.get_name()

    def get_mapset(self):
        """!Return the mapset
           @return The mapset in which the dataset was created as string
        """
        return self.base.get_mapset()

    def get_temporal_extent_as_tuple(self):
        """!Returns a tuple of the valid start and end time
        
           Start and end time can be either of type datetime or of type integer,
           depending on the temporal type.
           
           @return A tuple of (start_time, end_time)
        """
        start = self.temporal_extent.get_start_time()
        end = self.temporal_extent.get_end_time()
        return (start, end)

    def get_absolute_time(self):
        """!Returns the start time, the end 
           time and the timezone of the map as tuple
           
           @attention: The timezone is currently not used.
           
           The start time is of type datetime.
           
           The end time is of type datetime in case of interval time, 
           or None on case of a time instance.
           
           @return A tuple of (start_time, end_time, timezone)
        """

        start = self.absolute_time.get_start_time()
        end = self.absolute_time.get_end_time()
        tz = self.absolute_time.get_timezone()

        return (start, end, tz)

    def get_relative_time(self):
        """!Returns the start time, the end 
           time and the temporal unit of the dataset as tuple
           
           The start time is of type integer.
           
           The end time is of type integer in case of interval time, 
           or None on case of a time instance.
           
           @return A tuple of (start_time, end_time, unit)
        """

        start = self.relative_time.get_start_time()
        end = self.relative_time.get_end_time()
        unit = self.relative_time.get_unit()

        return (start, end, unit)

    def get_relative_time_unit(self):
        """!Returns the relative time unit
           @return The relative time unit as string, None if not present
        """
        return self.relative_time.get_unit()

    def check_relative_time_unit(self, unit):
        """!Check if unit is of type  year(s), month(s), day(s), hour(s), 
           minute(s) or second(s)

           @param unit The unit string
           @return True if success, False otherwise
        """
        # Check unit
        units = ["year", "years", "month", "months", "day", "days", "hour", 
                 "hours", "minute", "minutes", "second", "seconds"]
        if unit not in units:
            return False
        return True

    def get_temporal_type(self):
        """!Return the temporal type of this dataset
        
           The temporal type can be absolute or relative
           
           @return The temporal type of the dataset as string
        """
        return self.base.get_ttype()

    def get_spatial_extent_as_tuple(self):
        """!Return the spatial extent as tuple
        
           Top and bottom are set to 0 in case of a two dimensional spatial extent.
           
           @return A the spatial extent as tuple (north, south, east, west, top, bottom) 
        """
        return self.spatial_extent.get_spatial_extent_as_tuple()

    def select(self, dbif=None):
        """!Select temporal dataset entry from database and fill 
           the internal structure
           
           The content of every dataset is stored in the temporal database.
           This method must be used to fill this object with the content 
           from the temporal database.
           
           @param dbif The database interface to be used
        """

        dbif, connected = init_dbif(dbif)

        self.base.select(dbif)
        self.temporal_extent.select(dbif)
        self.spatial_extent.select(dbif)
        self.metadata.select(dbif)

        if connected:
            dbif.close()

    def is_in_db(self, dbif=None):
        """!Check if the dataset is registered in the database

           @param dbif The database interface to be used
           @return True if the dataset is registered in the database
        """
        return self.base.is_in_db(dbif)

    def delete(self):
        """!Delete dataset from database if it exists"""
        raise ImplementationError("This method must be implemented in the subclasses")

    def insert(self, dbif=None, execute=True):
        """!Insert dataset into database

           @param dbif The database interface to be used
           @param execute If True the SQL statements will be executed.
                           If False the prepared SQL statements are returned 
                           and must be executed by the caller.
            @return The SQL insert statement in case execute=False, or an empty string otherwise
        """

        dbif, connected = init_dbif(dbif)

        # Build the INSERT SQL statement
        statement = self.base.get_insert_statement_mogrified(dbif)
        statement += self.temporal_extent.get_insert_statement_mogrified(dbif)
        statement += self.spatial_extent.get_insert_statement_mogrified(dbif)
        statement += self.metadata.get_insert_statement_mogrified(dbif)
        
        if execute:
            dbif.execute_transaction(statement)
            if connected:
                dbif.close()
            return ""

        if connected:
            dbif.close()
        return statement

    def update(self, dbif=None, execute=True, ident=None):
        """!Update the dataset entry in the database from the internal structure
           excluding None variables

           @param dbif The database interface to be used
           @param execute If True the SQL statements will be executed.
                           If False the prepared SQL statements are returned 
                           and must be executed by the caller.
           @param ident The identifier to be updated, useful for renaming
           @return The SQL update statement in case execute=False, or an empty string otherwise
        """

        dbif, connected = init_dbif(dbif)

        # Build the UPDATE SQL statement
        statement = self.base.get_update_statement_mogrified(dbif, ident)
        statement += self.temporal_extent.get_update_statement_mogrified(dbif, 
                                                                         ident)
        statement += self.spatial_extent.get_update_statement_mogrified(dbif, 
                                                                        ident)
        statement += self.metadata.get_update_statement_mogrified(dbif, ident)

        if execute:
            dbif.execute_transaction(statement)
            if connected:
                dbif.close()
            return ""

        if connected:
            dbif.close()
        return statement

    def update_all(self, dbif=None, execute=True, ident=None):
        """!Update the dataset entry in the database from the internal structure
           and include None variables.

           @param dbif The database interface to be used
           @param execute If True the SQL statements will be executed.
                           If False the prepared SQL statements are returned 
                           and must be executed by the caller.
           @param ident The identifier to be updated, useful for renaming
           @return The SQL update statement in case execute=False, or an empty string otherwise
        """

        dbif, connected = init_dbif(dbif)

        # Build the UPDATE SQL statement
        statement = self.base.get_update_all_statement_mogrified(dbif, ident)
        statement += self.temporal_extent.get_update_all_statement_mogrified(dbif, 
                                                                             ident)
        statement += self.spatial_extent.get_update_all_statement_mogrified(
            dbif, ident)
        statement += self.metadata.get_update_all_statement_mogrified(dbif, ident)

        if execute:
            dbif.execute_transaction(statement)
            if connected:
                dbif.close()
            return ""

        if connected:
            dbif.close()
        return statement

    def is_time_absolute(self):
        """!Return True in case the temporal type is absolute
        
            @return True if temporal type is absolute, False otherwise
        """
        if "temporal_type" in self.base.D:
            return self.base.get_ttype() == "absolute"
        else:
            return None

    def is_time_relative(self):
        """!Return True in case the temporal type is relative
        
            @return True if temporal type is relative, False otherwise
        """
        if "temporal_type" in self.base.D:
            return self.base.get_ttype() == "relative"
        else:
            return None
    
    def _get_temporal_extent(self):
        """!Return the temporal extent of the correct internal type
        """
        if self.is_time_absolute():
            return self.absolute_time
        if self.is_time_relative():
            return self.relative_time
        return None
    
    temporal_extent = property(fget=_get_temporal_extent)

    def temporal_relation(self, dataset):
        """!Return the temporal relation of self and the provided dataset
        
            @return The temporal relation as string
        """
        return self.temporal_extent.temporal_relation(dataset.temporal_extent)
    
    def temporal_intersection(self, dataset):
        """!Intersect self with the provided dataset and
           return a new temporal extent with the new start and end time
           
           @param dataset The abstract dataset to temporal intersect with
           @return The new temporal extent with start and end time, 
                   or None in case of no intersection
        """
        return self.temporal_extent.intersect(dataset.temporal_extent)
        
    def temporal_union(self, dataset):
        """!Creates a union with the provided dataset and
           return a new temporal extent with the new start and end time.
           
           @param dataset The abstract dataset to create temporal union with
           @return The new temporal extent with start and end time, 
                   or None in case of no intersection
        """
        return self.temporal_extent.union(dataset.temporal_extent)
        
    def temporal_disjoint_union(self, dataset):
        """!Creates a union with the provided dataset and
           return a new temporal extent with the new start and end time.
           
           @param dataset The abstract dataset to create temporal union with
           @return The new temporal extent with start and end time
        """
        return self.temporal_extent.disjoint_union(dataset.temporal_extent)
        
###############################################################################

class AbstractDatasetComparisonKeyStartTime(object):
    """!This comparison key can be used to sort lists of abstract datasets 
       by start time

        Example:

        # Return all maps in a space time raster dataset as map objects
        map_list = strds.get_registered_maps_as_objects()

        # Sort the maps in the list by start time
        sorted_map_list = sorted(
            map_list, key=AbstractDatasetComparisonKeyStartTime)
    """
    def __init__(self, obj, *args):
        self.obj = obj

    def __lt__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return startA < startB

    def __gt__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return startA > startB

    def __eq__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return startA == startB

    def __le__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return startA <= startB

    def __ge__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return startA >= startB

    def __ne__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return startA != startB

###############################################################################

class AbstractDatasetComparisonKeyEndTime(object):
    """!This comparison key can be used to sort lists of abstract datasets 
       by end time

        Example:

        # Return all maps in a space time raster dataset as map objects
        map_list = strds.get_registered_maps_as_objects()

        # Sort the maps in the list by end time
        sorted_map_list = sorted(
            map_list, key=AbstractDatasetComparisonKeyEndTime)
    """
    def __init__(self, obj, *args):
        self.obj = obj

    def __lt__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return endA < endB

    def __gt__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return endA > endB

    def __eq__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return endA == endB

    def __le__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return endA <= endB

    def __ge__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return endA >= endB

    def __ne__(self, other):
        startA, endA = self.obj.get_temporal_extent_as_tuple()
        startB, endB = other.obj.get_temporal_extent_as_tuple()
        return endA != endB

###############################################################################
        
if __name__ == "__main__":
    import doctest
    doctest.testmod()
