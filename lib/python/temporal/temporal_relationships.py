"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

@code
import grass.temporal as tgis

tgis.print_temporal_relations(maps)
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from abstract_map_dataset import *
from datetime_math import *
import grass.lib.vector as vector
import grass.lib.gis as gis
from ctypes import *

###############################################################################

class TemporalTopologyBuilder(object):
    """!This class is designed to build the temporal topology 
       of temporally related abstract dataset objects.
       
       The abstract dataset objects must be provided as a single list, or in two lists.

        Example:
        @code
        # We have a space time raster dataset and build a map list
        # from all registered maps ordered by start time
        maps = strds.get_registered_maps_as_objects()

        # Now lets build the temporal topology of the maps in the list
        
        identical = False
        tb = TemporalTopologyBuilder()
        
        tb.build(maps)

        dbif, connected = init_dbif(None)
            
        for _map in tb:
            _map.select(dbif)
            _map.print_info()

        # Using the next and previous methods, we can iterate over the
        # topological related maps in this way

        _first = tb.get_first()

        while _first:
            _first.print_topology_info()
            _first = _first.next()

        # Dictionary like accessed
        _map = tb["name@mapset"]
        @endcode

    """
    def __init__(self):
        self._reset()
        # 0001-01-01 00:00:00
        self._timeref = datetime(1,1,1)

    def _reset(self):
        self._store = {}
        self._first = None
        self._iteratable = False

    def _set_first(self, first):
        self._first = first
        self._insert(first)

    def _detect_first(self):
        if len(self) > 0:
            prev_ = self._store.values()[0]
            while prev_ is not None:
                self._first = prev_
                prev_ = prev_.prev()

    def _insert(self, t):
        self._store[t.get_id()] = t

    def get_first(self):
        """!Return the first map with the earliest start time

           @return The map with the earliest start time
        """
        return self._first

    def _build_internal_iteratable(self, maps):
        """!Build an iteratable temporal topology structure for all maps in 
           the list and store the maps internally

           Basically the "next" and "prev" relations will be set in the 
           temporal topology structure of each map
           The maps will be added to the object, so they can be 
           accessed using the iterator of this class

           @param maps: A sorted (by start_time)list of abstract_dataset 
                        objects with initiated temporal extent
        """
        self._build_iteratable(maps)

        for _map in maps:
            self._insert(_map)

        # Detect the first map
        self._detect_first()

    def _build_iteratable(self, maps):
        """!Build an iteratable temporal topology structure for 
           all maps in the list

           Basically the "next" and "prev" relations will be set in 
           the temporal topology structure of each map.

           @param maps: A sorted (by start_time)list of abstract_dataset 
                        objects with initiated temporal extent
        """
#        for i in xrange(len(maps)):
#            offset = i + 1
#            for j in xrange(offset, len(maps)):
#                # Get the temporal relationship
#                relation = maps[j].temporal_relation(maps[i])
#
#                # Build the next reference
#                if relation != "equal" and relation != "started":
#                    maps[i].set_next(maps[j])
#                    break

        # First we need to order the map list chronologically 
        sorted_maps = sorted(
            maps, key=AbstractDatasetComparisonKeyStartTime)
            
        for i in xrange(len(sorted_maps) - 1):
            sorted_maps[i].set_next(sorted_maps[i + 1])

        for map_ in sorted_maps:
            next_ = map_.next()
            if next_:
                next_.set_prev(map_)
            map_.set_topology_build_true()
        
    def _map_to_rect(self, tree, map_):
        """Use the temporal extent of a map to create and return a RTree rectange"""
        rect = vector.RTreeAllocRect(tree)
        
        start, end = map_.get_valid_time()
        
        if not end:
            end = start
        
        if map_.is_time_absolute():
            start = time_delta_to_relative_time(start - self._timeref)
            end = time_delta_to_relative_time(end - self._timeref)
                
        vector.RTreeSetRect1D(rect, tree, float(start), float(end))
        
        return rect
        
    def _build_1d_rtree(self, maps):
        """Build and return the one dimensional R*-Tree"""

        tree = vector.RTreeCreateTree(-1, 0, 4)

        for i in xrange(len(maps)):
            
            rect = self._map_to_rect(tree, maps[i])
            vector.RTreeInsertRect(rect, i + 1, tree)
            
        return tree

    def build(self, mapsA, mapsB=None):
        """!Build the temporal topology structure between 
           one or two unordered lists of abstract dataset objects

           This method builds the temporal topology from mapsA to 
           mapsB and vice verse. The temporal topology structure of each map, 
           defined in class temporal_map_relations,
           will be reseted and rebuild for mapsA and mapsB.

           After building the temporal topology the modified 
           map objects of mapsA can be accessed
           in the same way as a dictionary using there id. 
           The implemented iterator assures
           the chronological iteration over the mapsA.

           @param mapsA: A list of abstract_dataset 
                         objects with initiated temporal extent
           @param mapsB: An optional list of abstract_dataset 
                         objects with initiated temporal extent
        """

        identical = False
        if mapsA == mapsB:
            identical = True
            
        if mapsB == None:
            mapsB = mapsA
            idetnical = True

        for map_ in mapsA:
            map_.reset_topology()

        if not identical:
            for map_ in mapsB:
                map_.reset_topology()

        tree = self. _build_1d_rtree(mapsA)
            
        for j in xrange(len(mapsB)):            
        
            list_ = gis.ilist()
            rect = self._map_to_rect(tree, mapsB[j])
            num = vector.RTreeSearch2(tree, rect, byref(list_))
            vector.RTreeFreeRect(rect)

            for k in xrange(list_.n_values):
                i = list_.value[k] - 1

                # Get the temporal relationship
                relation = mapsB[j].temporal_relation(mapsA[i])
                
                if relation == "equal":
                    if mapsB[j] != mapsA[i]:
                        if not mapsB[j].get_equal() or \
                        (mapsB[j].get_equal() and \
                        mapsA[i] not in mapsB[j].get_equal()):
                            mapsB[j].append_equal(mapsA[i])
                        if not mapsA[i].get_equal() or \
                        (mapsA[i].get_equal() and \
                        mapsB[j] not in mapsA[i].get_equal()):
                            mapsA[i].append_equal(mapsB[j])
                elif relation == "follows":
                    if not mapsB[j].get_follows() or \
                       (mapsB[j].get_follows() and \
                       mapsA[i] not in mapsB[j].get_follows()):
                        mapsB[j].append_follows(mapsA[i])
                    if not mapsA[i].get_precedes() or \
                       (mapsA[i].get_precedes() and
                       mapsB[j] not in mapsA[i].get_precedes()):
                        mapsA[i].append_precedes(mapsB[j])
                elif relation == "precedes":
                    if not mapsB[j].get_precedes() or \
                       (mapsB[j].get_precedes() and \
                       mapsA[i] not in mapsB[j].get_precedes()):
                        mapsB[j].append_precedes(mapsA[i])
                    if not mapsA[i].get_follows() or \
                       (mapsA[i].get_follows() and \
                       mapsB[j] not in mapsA[i].get_follows()):
                        mapsA[i].append_follows(mapsB[j])
                elif relation == "during" or relation == "starts" or \
                     relation == "finishes":
                    if not mapsB[j].get_during() or \
                       (mapsB[j].get_during() and \
                       mapsA[i] not in mapsB[j].get_during()):
                        mapsB[j].append_during(mapsA[i])
                    if not mapsA[i].get_contains() or \
                       (mapsA[i].get_contains() and \
                       mapsB[j] not in mapsA[i].get_contains()):
                        mapsA[i].append_contains(mapsB[j])
                    if relation == "starts":
                        if not mapsB[j].get_starts() or \
                        (mapsB[j].get_starts() and \
                        mapsA[i] not in mapsB[j].get_starts()):
                            mapsB[j].append_starts(mapsA[i])
                        if not mapsA[i].get_started() or \
                        (mapsA[i].get_started() and \
                        mapsB[j] not in mapsA[i].get_started()):
                            mapsA[i].append_started(mapsB[j])
                    if relation == "finishes":
                        if not mapsB[j].get_finishes() or \
                        (mapsB[j].get_finishes() and \
                        mapsA[i] not in mapsB[j].get_finishes()):
                            mapsB[j].append_finishes(mapsA[i])
                        if not mapsA[i].get_finished() or \
                        (mapsA[i].get_finished() and \
                        mapsB[j] not in mapsA[i].get_finished()):
                            mapsA[i].append_finished(mapsB[j])
                elif relation == "contains" or relation == "started" or \
                     relation == "finished":
                    if not mapsB[j].get_contains() or \
                       (mapsB[j].get_contains() and \
                       mapsA[i] not in mapsB[j].get_contains()):
                        mapsB[j].append_contains(mapsA[i])
                    if not mapsA[i].get_during() or \
                       (mapsA[i].get_during() and \
                       mapsB[j] not in mapsA[i].get_during()):
                        mapsA[i].append_during(mapsB[j])
                    if relation == "started":
                        if not mapsB[j].get_started() or \
                        (mapsB[j].get_started() and \
                        mapsA[i] not in mapsB[j].get_started()):
                            mapsB[j].append_started(mapsA[i])
                        if not mapsA[i].get_starts() or \
                        (mapsA[i].get_starts() and \
                        mapsB[j] not in mapsA[i].get_starts()):
                            mapsA[i].append_starts(mapsB[j])
                    if relation == "finished":
                        if not mapsB[j].get_finished() or \
                        (mapsB[j].get_finished() and \
                        mapsA[i] not in mapsB[j].get_finished()):
                            mapsB[j].append_finished(mapsA[i])
                        if not mapsA[i].get_finishes() or \
                        (mapsA[i].get_finishes() and \
                        mapsB[j] not in mapsA[i].get_finishes()):
                            mapsA[i].append_finishes(mapsB[j])
                elif relation == "overlaps":
                    if not mapsB[j].get_overlaps() or \
                       (mapsB[j].get_overlaps() and \
                       mapsA[i] not in mapsB[j].get_overlaps()):
                        mapsB[j].append_overlaps(mapsA[i])
                    if not mapsA[i].get_overlapped() or \
                       (mapsA[i].get_overlapped() and \
                       mapsB[j] not in mapsA[i].get_overlapped()):
                        mapsA[i].append_overlapped(mapsB[j])
                elif relation == "overlapped":
                    if not mapsB[j].get_overlapped() or \
                       (mapsB[j].get_overlapped() and \
                       mapsA[i] not in mapsB[j].get_overlapped()):
                        mapsB[j].append_overlapped(mapsA[i])
                    if not mapsA[i].get_overlaps() or \
                       (mapsA[i].get_overlaps() and \
                       mapsB[j] not in mapsA[i].get_overlaps()):
                        mapsA[i].append_overlaps(mapsB[j])

        self._build_internal_iteratable(mapsA)
        if not identical and mapsB != None:
            self._build_iteratable(mapsB)
        
        vector.RTreeDestroyTree(tree)

    def __iter__(self):
        start_ = self._first
        while start_ is not None:
            yield start_
            start_ = start_.next()

    def __getitem__(self, index):
        return self._store[index.get_id()]

    def __len__(self):
        return len(self._store)

    def __contains__(self, _map):
        return _map in self._store.values()

###############################################################################

def print_temporal_topology_relationships(maps1, maps2=None, dbif=None):
    """!Print the temporal relationships of the 
       map lists maps1 and maps2 to stdout.

        @param maps1: A list of abstract_dataset 
                      objects with initiated temporal extent
        @param maps2: An optional list of abstract_dataset 
                      objects with initiated temporal extent
        @param dbif: The database interface to be used
    """
                    
    tb = TemporalTopologyBuilder()

    tb.build(maps1, maps2)

    dbif, connected = init_dbif(dbif)
        
    for _map in tb:
        _map.select(dbif)
        _map.print_info()
        
    if connected:
        dbif.close()
        
    return

###############################################################################

def count_temporal_topology_relationships(maps1, maps2=None, dbif=None):
    """!Count the temporal relations of a single list of maps or between two lists of maps


        @param maps1: A list of abstract_dataset 
                      objects with initiated temporal extent
        @param maps2: A list of abstract_dataset 
                      objects with initiated temporal extent
        @param dbif: The database interface to be used
        @return A dictionary with counted temporal relationships
    """

    
    tb = TemporalTopologyBuilder()
    tb.build(maps1, maps2)

    dbif, connected = init_dbif(dbif)
    
    relations = None
        
    for _map in tb:
        if relations != None:
            r = _map.get_number_of_relations()
            for k in r.keys():
                relations[k] += r[k]
        else:
            relations = _map.get_number_of_relations()

    if connected:
        dbif.close()
        
    return relations

###############################################################################

def create_temporal_relation_sql_where_statement(
                        start, end, use_start=True, use_during=False,
                        use_overlap=False, use_contain=False, use_equal=False,
                        use_follows=False, use_precedes=False):
    """!Create a SQL WHERE statement for temporal relation selection of maps in space time datasets

        @param start: The start time
        @param end: The end time
        @param use_start: Select maps of which the start time is located in the selection granule
                         @verbatim
                         map    :        s
                         granule:  s-----------------e

                         map    :        s--------------------e
                         granule:  s-----------------e

                         map    :        s--------e
                         granule:  s-----------------e
                         @endverbatim

        @param use_during: during: Select maps which are temporal during the selection granule
                         @verbatim
                         map    :     s-----------e
                         granule:  s-----------------e
                         @endverbatim

        @param use_overlap: Select maps which temporal overlap the selection granule
                         @verbatim
                         map    :     s-----------e
                         granule:        s-----------------e

                         map    :     s-----------e
                         granule:  s----------e
                         @endverbatim

        @param use_contain: Select maps which temporally contain the selection granule
                         @verbatim
                         map    :  s-----------------e
                         granule:     s-----------e
                         @endverbatim

        @param use_equal: Select maps which temporally equal to the selection granule
                         @verbatim
                         map    :  s-----------e
                         granule:  s-----------e
                         @endverbatim

        @param use_follows: Select maps which temporally follow the selection granule
                         @verbatim
                         map    :              s-----------e
                         granule:  s-----------e
                         @endverbatim

        @param use_precedes: Select maps which temporally precedes the selection granule
                         @verbatim
                         map    :  s-----------e
                         granule:              s-----------e
                         @endverbatim

        Usage:
        
        @code
        
        >>> # Relative time
        >>> start = 1
        >>> end = 2
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False)
        >>> create_temporal_relation_sql_where_statement(start, end)
        '((start_time >= 1 and start_time < 2) )'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=True)
        '((start_time >= 1 and start_time < 2) )'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_during=True)
        '(((start_time > 1 and end_time < 2) OR (start_time >= 1 and end_time < 2) OR (start_time > 1 and end_time <= 2)))'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_overlap=True)
        '(((start_time < 1 and end_time > 1 and end_time < 2) OR (start_time < 2 and start_time > 1 and end_time > 2)))'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_contain=True)
        '(((start_time < 1 and end_time > 2) OR (start_time <= 1 and end_time > 2) OR (start_time < 1 and end_time >= 2)))'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_equal=True)
        '((start_time = 1 and end_time = 2))'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_follows=True)
        '((start_time = 2))'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_precedes=True)
        '((end_time = 1))'
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=True, use_during=True, use_overlap=True, use_contain=True,
        ... use_equal=True, use_follows=True, use_precedes=True)
        '((start_time >= 1 and start_time < 2)  OR ((start_time > 1 and end_time < 2) OR (start_time >= 1 and end_time < 2) OR (start_time > 1 and end_time <= 2)) OR ((start_time < 1 and end_time > 1 and end_time < 2) OR (start_time < 2 and start_time > 1 and end_time > 2)) OR ((start_time < 1 and end_time > 2) OR (start_time <= 1 and end_time > 2) OR (start_time < 1 and end_time >= 2)) OR (start_time = 1 and end_time = 2) OR (start_time = 2) OR (end_time = 1))'

        >>> # Absolute time
        >>> start = datetime(2001, 1, 1, 12, 30)
        >>> end = datetime(2001, 3, 31, 14, 30)
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False)
        >>> create_temporal_relation_sql_where_statement(start, end)
        "((start_time >= '2001-01-01 12:30:00' and start_time < '2001-03-31 14:30:00') )"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=True)
        "((start_time >= '2001-01-01 12:30:00' and start_time < '2001-03-31 14:30:00') )"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_during=True)
        "(((start_time > '2001-01-01 12:30:00' and end_time < '2001-03-31 14:30:00') OR (start_time >= '2001-01-01 12:30:00' and end_time < '2001-03-31 14:30:00') OR (start_time > '2001-01-01 12:30:00' and end_time <= '2001-03-31 14:30:00')))"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_overlap=True)
        "(((start_time < '2001-01-01 12:30:00' and end_time > '2001-01-01 12:30:00' and end_time < '2001-03-31 14:30:00') OR (start_time < '2001-03-31 14:30:00' and start_time > '2001-01-01 12:30:00' and end_time > '2001-03-31 14:30:00')))"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_contain=True)
        "(((start_time < '2001-01-01 12:30:00' and end_time > '2001-03-31 14:30:00') OR (start_time <= '2001-01-01 12:30:00' and end_time > '2001-03-31 14:30:00') OR (start_time < '2001-01-01 12:30:00' and end_time >= '2001-03-31 14:30:00')))"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_equal=True)
        "((start_time = '2001-01-01 12:30:00' and end_time = '2001-03-31 14:30:00'))"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_follows=True)
        "((start_time = '2001-03-31 14:30:00'))"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=False, use_precedes=True)
        "((end_time = '2001-01-01 12:30:00'))"
        >>> create_temporal_relation_sql_where_statement(start, end, 
        ... use_start=True, use_during=True, use_overlap=True, use_contain=True,
        ... use_equal=True, use_follows=True, use_precedes=True)
        "((start_time >= '2001-01-01 12:30:00' and start_time < '2001-03-31 14:30:00')  OR ((start_time > '2001-01-01 12:30:00' and end_time < '2001-03-31 14:30:00') OR (start_time >= '2001-01-01 12:30:00' and end_time < '2001-03-31 14:30:00') OR (start_time > '2001-01-01 12:30:00' and end_time <= '2001-03-31 14:30:00')) OR ((start_time < '2001-01-01 12:30:00' and end_time > '2001-01-01 12:30:00' and end_time < '2001-03-31 14:30:00') OR (start_time < '2001-03-31 14:30:00' and start_time > '2001-01-01 12:30:00' and end_time > '2001-03-31 14:30:00')) OR ((start_time < '2001-01-01 12:30:00' and end_time > '2001-03-31 14:30:00') OR (start_time <= '2001-01-01 12:30:00' and end_time > '2001-03-31 14:30:00') OR (start_time < '2001-01-01 12:30:00' and end_time >= '2001-03-31 14:30:00')) OR (start_time = '2001-01-01 12:30:00' and end_time = '2001-03-31 14:30:00') OR (start_time = '2001-03-31 14:30:00') OR (end_time = '2001-01-01 12:30:00'))"
        
        @endcode
        """

    where = "("

    if use_start:
        if isinstance(start, datetime):
            where += "(start_time >= '%s' and start_time < '%s') " % (start, end)
        else:
            where += "(start_time >= %i and start_time < %i) " % (start, end)

    if use_during:
        if use_start:
            where += " OR "
            
        if isinstance(start, datetime):
            where += "((start_time > '%s' and end_time < '%s') OR " % (start, end)
            where += "(start_time >= '%s' and end_time < '%s') OR " % (start, end)
            where += "(start_time > '%s' and end_time <= '%s'))" % (start, end)
        else:
            where += "((start_time > %i and end_time < %i) OR " % (start, end)
            where += "(start_time >= %i and end_time < %i) OR " % (start, end)
            where += "(start_time > %i and end_time <= %i))" % (start, end)

    if use_overlap:
        if use_start or use_during:
            where += " OR "

        if isinstance(start, datetime):
            where += "((start_time < '%s' and end_time > '%s' and end_time < '%s') OR " % (start, start, end)
            where += "(start_time < '%s' and start_time > '%s' and end_time > '%s'))" % (end, start, end)
        else:
            where += "((start_time < %i and end_time > %i and end_time < %i) OR " % (start, start, end)
            where += "(start_time < %i and start_time > %i and end_time > %i))" % (end, start, end)

    if use_contain:
        if use_start or use_during or use_overlap:
            where += " OR "

        if isinstance(start, datetime):
            where += "((start_time < '%s' and end_time > '%s') OR " % (start, end)
            where += "(start_time <= '%s' and end_time > '%s') OR " % (start, end)
            where += "(start_time < '%s' and end_time >= '%s'))" % (start, end)
        else:
            where += "((start_time < %i and end_time > %i) OR " % (start, end)
            where += "(start_time <= %i and end_time > %i) OR " % (start, end)
            where += "(start_time < %i and end_time >= %i))" % (start, end)

    if use_equal:
        if use_start or use_during or use_overlap or use_contain:
            where += " OR "

        if isinstance(start, datetime):
            where += "(start_time = '%s' and end_time = '%s')" % (start, end)
        else:
            where += "(start_time = %i and end_time = %i)" % (start, end)

    if use_follows:
        if use_start or use_during or use_overlap or use_contain or use_equal:
            where += " OR "

        if isinstance(start, datetime):
            where += "(start_time = '%s')" % (end)
        else:
            where += "(start_time = %i)" % (end)

    if use_precedes:
        if use_start or use_during or use_overlap or use_contain or use_equal \
           or use_follows:
            where += " OR "

        if isinstance(start, datetime):
            where += "(end_time = '%s')" % (start)
        else:
            where += "(end_time = %i)" % (start)

    where += ")"

    # Catch empty where statement
    if where == "()":
        where = None

    return where