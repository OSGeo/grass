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
from abstract_dataset import *
from datetime_math import *
import grass.lib.vector as vector
import grass.lib.gis as gis
from ctypes import *

###############################################################################

class SpatioTemporalTopologyBuilder(object):
    """!This class is designed to build the spatio-temporal topology
       of spatio-temporally related abstract dataset objects.

       The abstract dataset objects must be provided as a single list, or in two lists.

        Example:
        @code
        # We have a space time raster dataset and build a map list
        # from all registered maps ordered by start time
        maps = strds.get_registered_maps_as_objects()

        # Now lets build the temporal topology of the maps in the list

        tb = SpatioTemporalTopologyBuilder()

        tb.build(maps)

        dbif, connected = init_dbif(None)

        for map in tb:
            map.select(dbif)
            map.print_info()

        # Same can be done with the existing map list
        # But be aware that this is might not be temporally ordered
        for map in maps:
            map.select(dbf)
            map.print_info()

        # Using the next and previous methods, we can iterate over the
        # topological related maps in this way

        first = tb.get_first()

        while first:
            first.print_topology_info()
            first = first.next()

        # Dictionary like accessed
        map = tb["name@mapset"]

        >>> # Example with two lists of maps
        >>> import grass.temporal as tgis
        >>> import datetime
        >>> # Create two list of maps with equal time stamps
        >>> mapsA = []
        >>> mapsB = []
        >>> for i in range(4):
        ...     idA = "a%i@B"%(i)
        ...     mapA = tgis.RasterDataset(idA)
        ...     idB = "b%i@B"%(i)
        ...     mapB = tgis.RasterDataset(idB)
        ...     check = mapA.set_relative_time(i, i + 1, "months")
        ...     check = mapB.set_relative_time(i, i + 1, "months")
        ...     mapsA.append(mapA)
        ...     mapsB.append(mapB)
        >>> # Build the topology between the two map lists
        >>> tb = SpatioTemporalTopologyBuilder()
        >>> tb.build(mapsA, mapsB, None)
        >>> # Check relations of mapsA
        >>> for map in mapsA:
        ...     if map.get_equal():
        ...         relations = map.get_equal()
        ...         print "Map %s has equal relation to map %s"%(map.get_name(),
        ...               relations[0].get_name())
        Map a0 has equal relation to map b0
        Map a1 has equal relation to map b1
        Map a2 has equal relation to map b2
        Map a3 has equal relation to map b3
        >>> # Check relations of mapsB
        >>> for map in mapsB:
        ...     if map.get_equal():
        ...         relations = map.get_equal()
        ...         print "Map %s has equal relation to map %s"%(map.get_name(),
        ...               relations[0].get_name())
        Map b0 has equal relation to map a0
        Map b1 has equal relation to map a1
        Map b2 has equal relation to map a2
        Map b3 has equal relation to map a3


        >>> mapsA = []
        >>> mapsB = []
        >>> for i in range(4):
        ...     idA = "a%i@B"%(i)
        ...     mapA = tgis.RasterDataset(idA)
        ...     idB = "b%i@B"%(i)
        ...     mapB = tgis.RasterDataset(idB)
        ...     check = mapA.set_relative_time(i, i + 1, "months")
        ...     check = mapB.set_relative_time(i + 1, i + 2, "months")
        ...     mapsA.append(mapA)
        ...     mapsB.append(mapB)
        >>> # Build the topology between the two map lists
        >>> tb = SpatioTemporalTopologyBuilder()
        >>> tb.build(mapsA, mapsB, None)
        >>> # Check relations of mapsA
        >>> for map in mapsA:
        ...     print(map.get_temporal_extent_as_tuple())
        ...     m = map.get_temporal_relations()
        ...     for key in m.keys():
        ...         if key not in ["NEXT", "PREV"]:
        ...             print(key, m[key][0].get_temporal_extent_as_tuple())
        (0, 1)
        ('PRECEDES', (1, 2))
        (1, 2)
        ('PRECEDES', (2, 3))
        ('EQUAL', (1, 2))
        (2, 3)
        ('FOLLOWS', (1, 2))
        ('PRECEDES', (3, 4))
        ('EQUAL', (2, 3))
        (3, 4)
        ('FOLLOWS', (2, 3))
        ('EQUAL', (3, 4))
        ('PRECEDES', (4, 5))

        >>> mapsA = []
        >>> mapsB = []
        >>> for i in range(4):
        ...     idA = "a%i@B"%(i)
        ...     mapA = tgis.RasterDataset(idA)
        ...     idB = "b%i@B"%(i)
        ...     mapB = tgis.RasterDataset(idB)
        ...     start = datetime.datetime(2000 + i, 1, 1)
        ...     end = datetime.datetime(2000 + i + 1, 1, 1)
        ...     check = mapA.set_absolute_time(start, end)
        ...     start = datetime.datetime(2000 + i + 1, 1, 1)
        ...     end = datetime.datetime(2000 + i + 2, 1, 1)
        ...     check = mapB.set_absolute_time(start, end)
        ...     mapsA.append(mapA)
        ...     mapsB.append(mapB)
        >>> # Build the topology between the two map lists
        >>> tb = SpatioTemporalTopologyBuilder()
        >>> tb.build(mapsA, mapsB, None)
        >>> # Check relations of mapsA
        >>> for map in mapsA:
        ...     print(map.get_temporal_extent_as_tuple())
        ...     m = map.get_temporal_relations()
        ...     for key in m.keys():
        ...         if key not in ["NEXT", "PREV"]:
        ...             print(key, m[key][0].get_temporal_extent_as_tuple())
        (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2001, 1, 1, 0, 0))
        ('PRECEDES', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2002, 1, 1, 0, 0)))
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2002, 1, 1, 0, 0))
        ('PRECEDES', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('EQUAL', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2002, 1, 1, 0, 0)))
        (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0))
        ('FOLLOWS', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2002, 1, 1, 0, 0)))
        ('PRECEDES', (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('EQUAL', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0))
        ('FOLLOWS', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('EQUAL', (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('PRECEDES', (datetime.datetime(2004, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))

        >>> mapsA = []
        >>> mapsB = []
        >>> for i in range(4):
        ...     idA = "a%i@B"%(i)
        ...     mapA = tgis.RasterDataset(idA)
        ...     idB = "b%i@B"%(i)
        ...     mapB = tgis.RasterDataset(idB)
        ...     start = datetime.datetime(2000 + i, 1, 1)
        ...     end = datetime.datetime(2000 + i + 1, 1, 1)
        ...     check = mapA.set_absolute_time(start, end)
        ...     start = datetime.datetime(2000 + i, 1, 1)
        ...     end = datetime.datetime(2000 + i + 3, 1, 1)
        ...     check = mapB.set_absolute_time(start, end)
        ...     mapsA.append(mapA)
        ...     mapsB.append(mapB)
        >>> # Build the topology between the two map lists
        >>> tb = SpatioTemporalTopologyBuilder()
        >>> tb.build(mapsA, mapsB, None)
        >>> # Check relations of mapsA
        >>> for map in mapsA:
        ...     print(map.get_temporal_extent_as_tuple())
        ...     m = map.get_temporal_relations()
        ...     for key in m.keys():
        ...         if key not in ["NEXT", "PREV"]:
        ...             print(key, m[key][0].get_temporal_extent_as_tuple())
        (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2001, 1, 1, 0, 0))
        ('DURING', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('PRECEDES', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2002, 1, 1, 0, 0))
        ('DURING', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('PRECEDES', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))
        (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0))
        ('PRECEDES', (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2006, 1, 1, 0, 0)))
        ('FINISHES', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('DURING', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))
        (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0))
        ('FOLLOWS', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('DURING', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('FINISHES', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2006, 1, 1, 0, 0)))

        >>> mapsA = []
        >>> mapsB = []
        >>> for i in range(4):
        ...     idA = "a%i@B"%(i)
        ...     mapA = tgis.RasterDataset(idA)
        ...     idB = "b%i@B"%(i)
        ...     mapB = tgis.RasterDataset(idB)
        ...     start = datetime.datetime(2000 + i, 1, 1)
        ...     end = datetime.datetime(2000 + i + 2, 1, 1)
        ...     check = mapA.set_absolute_time(start, end)
        ...     start = datetime.datetime(2000 + i, 1, 1)
        ...     end = datetime.datetime(2000 + i + 3, 1, 1)
        ...     check = mapB.set_absolute_time(start, end)
        ...     mapsA.append(mapA)
        ...     mapsB.append(mapB)
        >>> # Build the topology between the two map lists
        >>> tb = SpatioTemporalTopologyBuilder()
        >>> tb.build(mapsA, mapsB, None)
        >>> # Check relations of mapsA
        >>> for map in mapsA:
        ...     print(map.get_temporal_extent_as_tuple())
        ...     m = map.get_temporal_relations()
        ...     for key in m.keys():
        ...         if key not in ["NEXT", "PREV"]:
        ...             print(key, m[key][0].get_temporal_extent_as_tuple())
        (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2002, 1, 1, 0, 0))
        ('OVERLAPS', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('DURING', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('PRECEDES', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))
        (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0))
        ('OVERLAPS', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))
        ('PRECEDES', (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2006, 1, 1, 0, 0)))
        ('FINISHES', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('DURING', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0))
        ('OVERLAPS', (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2006, 1, 1, 0, 0)))
        ('OVERLAPPED', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))
        ('FINISHES', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('DURING', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))
        (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0))
        ('OVERLAPPED', (datetime.datetime(2001, 1, 1, 0, 0), datetime.datetime(2004, 1, 1, 0, 0)))
        ('DURING', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))
        ('FINISHES', (datetime.datetime(2002, 1, 1, 0, 0), datetime.datetime(2005, 1, 1, 0, 0)))
        ('STARTS', (datetime.datetime(2003, 1, 1, 0, 0), datetime.datetime(2006, 1, 1, 0, 0)))
        ('FOLLOWS', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2003, 1, 1, 0, 0)))

        >>> mapsA = []
        >>> mapsB = []
        >>> for i in range(4):
        ...     idA = "a%i@B"%(i)
        ...     mapA = tgis.RasterDataset(idA)
        ...     idB = "b%i@B"%(i)
        ...     mapB = tgis.RasterDataset(idB)
        ...     start = datetime.datetime(2000, 1, 1, 0, 0, i)
        ...     end = datetime.datetime(2000, 1, 1, 0, 0, i + 2)
        ...     check = mapA.set_absolute_time(start, end)
        ...     start = datetime.datetime(2000, 1, 1, 0, 0, i + 1)
        ...     end = datetime.datetime(2000, 1, 1, 0, 0, i + 3)
        ...     check = mapB.set_absolute_time(start, end)
        ...     mapsA.append(mapA)
        ...     mapsB.append(mapB)
        >>> # Build the topology between the two map lists
        >>> tb = SpatioTemporalTopologyBuilder()
        >>> tb.build(mapsA, mapsB, None)
        >>> # Check relations of mapsA
        >>> for map in mapsA:
        ...     print(map.get_temporal_extent_as_tuple())
        ...     m = map.get_temporal_relations()
        ...     for key in m.keys():
        ...         if key not in ["NEXT", "PREV"]:
        ...             print(key, m[key][0].get_temporal_extent_as_tuple())
        (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2000, 1, 1, 0, 0, 2))
        ('OVERLAPS', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        ('PRECEDES', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3))
        ('OVERLAPS', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        ('PRECEDES', (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5)))
        ('EQUAL', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4))
        ('OVERLAPS', (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5)))
        ('OVERLAPPED', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        ('PRECEDES', (datetime.datetime(2000, 1, 1, 0, 0, 4), datetime.datetime(2000, 1, 1, 0, 0, 6)))
        ('EQUAL', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5))
        ('OVERLAPS', (datetime.datetime(2000, 1, 1, 0, 0, 4), datetime.datetime(2000, 1, 1, 0, 0, 6)))
        ('FOLLOWS', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        ('OVERLAPPED', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        ('EQUAL', (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5)))

        >>> mapsA = []
        >>> for i in range(4):
        ...     idA = "a%i@B"%(i)
        ...     mapA = tgis.RasterDataset(idA)
        ...     start = datetime.datetime(2000, 1, 1, 0, 0, i)
        ...     end = datetime.datetime(2000, 1, 1, 0, 0, i + 2)
        ...     check = mapA.set_absolute_time(start, end)
        ...     mapsA.append(mapA)
        >>> tb = SpatioTemporalTopologyBuilder()
        >>> tb.build(mapsA)
        >>> # Check relations of mapsA
        >>> for map in mapsA:
        ...     print(map.get_temporal_extent_as_tuple())
        ...     m = map.get_temporal_relations()
        ...     for key in m.keys():
        ...         if key not in ["NEXT", "PREV"]:
        ...             print(key, m[key][0].get_temporal_extent_as_tuple())
        (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2000, 1, 1, 0, 0, 2))
        ('OVERLAPS', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        ('PRECEDES', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        ('EQUAL', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2000, 1, 1, 0, 0, 2)))
        (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3))
        ('OVERLAPS', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        ('OVERLAPPED', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2000, 1, 1, 0, 0, 2)))
        ('PRECEDES', (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5)))
        ('EQUAL', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4))
        ('OVERLAPS', (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5)))
        ('OVERLAPPED', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        ('FOLLOWS', (datetime.datetime(2000, 1, 1, 0, 0), datetime.datetime(2000, 1, 1, 0, 0, 2)))
        ('EQUAL', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5))
        ('FOLLOWS', (datetime.datetime(2000, 1, 1, 0, 0, 1), datetime.datetime(2000, 1, 1, 0, 0, 3)))
        ('OVERLAPPED', (datetime.datetime(2000, 1, 1, 0, 0, 2), datetime.datetime(2000, 1, 1, 0, 0, 4)))
        ('EQUAL', (datetime.datetime(2000, 1, 1, 0, 0, 3), datetime.datetime(2000, 1, 1, 0, 0, 5)))

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

    def _build_internal_iteratable(self, maps, spatial):
        """!Build an iteratable temporal topology structure for all maps in
           the list and store the maps internally

           Basically the "next" and "prev" relations will be set in the
           temporal topology structure of each map
           The maps will be added to the object, so they can be
           accessed using the iterator of this class

           @param maps A sorted (by start_time)list of abstract_dataset
                        objects with initiated temporal extent
        """
        self._build_iteratable(maps, spatial)

        for _map in maps:
            self._insert(_map)

        # Detect the first map
        self._detect_first()

    def _build_iteratable(self, maps, spatial):
        """!Build an iteratable temporal topology structure for
           all maps in the list

           Basically the "next" and "prev" relations will be set in
           the temporal topology structure of each map.

           @param maps A sorted (by start_time)list of abstract_dataset
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
            map_.set_temporal_topology_build_true()
            if spatial is not None:
                map_.set_spatial_topology_build_true()

    def _map_to_rect(self, tree, map_, spatial=None):
        """!Use the spatio-temporal extent of a map to create and
           return a RTree rectange

           @param spatial This indicates if the spatial topology is created as well:
                          spatial can be None (no spatial topology), "2D" using west, east,
                          #south, north or "3D" using west, east, south, north, bottom, top
        """
        rect = vector.RTreeAllocRect(tree)

        start, end = map_.get_temporal_extent_as_tuple()

        if not end:
            end = start

        if map_.is_time_absolute():
            start = time_delta_to_relative_time_seconds(start - self._timeref)
            end = time_delta_to_relative_time_seconds(end - self._timeref)

        if spatial is None:
            vector.RTreeSetRect1D(rect, tree, float(start), float(end))
        elif spatial == "2D":
            north, south, east, west, top, bottom = map_.get_spatial_extent_as_tuple()
            vector.RTreeSetRect3D(rect, tree, west, east, south, north,
                                  float(start), float(end))
        elif spatial == "3D":
            north, south, east, west, top, bottom = map_.get_spatial_extent_as_tuple()
            vector.RTreeSetRect4D(rect, tree, west, east, south, north,
                                  bottom, top, float(start), float(end))

        return rect

    def _build_rtree(self, maps, spatial=None):
        """!Build and return the 1-4 dimensional R*-Tree


           @param spatial This indicates if the spatial topology is created as well:
                          spatial can be None (no spatial topology), "2D" using west, east,
                          south, north or "3D" using west, east, south, north, bottom, top
        """
        dim = 1
        if spatial == "2D":
            dim = 3
        if spatial == "3D":
            dim = 4

        tree = vector.RTreeCreateTree(-1, 0, dim)

        for i in xrange(len(maps)):

            rect = self._map_to_rect(tree, maps[i], spatial)
            vector.RTreeInsertRect(rect, i + 1, tree)

        return tree

    def build(self, mapsA, mapsB=None, spatial=None):
        """!Build the spatio-temporal topology structure between
           one or two unordered lists of abstract dataset objects

           This method builds the temporal or spatio-temporal topology from mapsA to
           mapsB and vice verse. The spatio-temporal topology structure of each map
           will be reseted and rebuild for mapsA and mapsB.

           After building the temporal or spatio-temporal topology the modified
           map objects of mapsA can be accessed
           in the same way as a dictionary using there id.
           The implemented iterator assures
           the chronological iteration over the mapsA.

           @param mapsA A list of abstract_dataset
                         objects with initiated spatio-temporal extent
           @param mapsB An optional list of abstract_dataset
                         objects with initiated spatio-temporal extent
           @param spatial This indicates if the spatial topology is created as well:
                          spatial can be None (no spatial topology), "2D" using west, east,
                          south, north or "3D" using west, east, south, north, bottom, top
        """

        identical = False
        if mapsA == mapsB:
            identical = True

        if mapsB == None:
            mapsB = mapsA
            identical = True

        for map_ in mapsA:
            map_.reset_topology()

        if not identical:
            for map_ in mapsB:
                map_.reset_topology()

        tree = self. _build_rtree(mapsA, spatial)

        list_ = gis.G_new_ilist()

        for j in xrange(len(mapsB)):

            rect = self._map_to_rect(tree, mapsB[j], spatial)
            vector.RTreeSearch2(tree, rect, list_)
            vector.RTreeFreeRect(rect)

            for k in xrange(list_.contents.n_values):
                i = list_.contents.value[k] - 1

                # Get the temporal relationship
                relation = mapsB[j].temporal_relation(mapsA[i])

                A = mapsA[i]
                B = mapsB[j]
                set_temoral_relationship(A, B, relation)

                if spatial is not None:
                    relation = mapsB[j].spatial_relation(mapsA[i])
                    set_spatial_relationship(A, B, relation)

        self._build_internal_iteratable(mapsA, spatial)
        if not identical and mapsB != None:
            self._build_iteratable(mapsB, spatial)

        gis.G_free_ilist(list_)

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

def set_temoral_relationship(A, B, relation):
    if relation == "equal" or relation == "equals":
        if not B.get_equal() or \
        (B.get_equal() and \
        A not in B.get_equal()):
            B.append_equal(A)
        if not A.get_equal() or \
        (A.get_equal() and \
        B not in A.get_equal()):
            A.append_equal(B)
    elif relation == "follows":
        if not B.get_follows() or \
            (B.get_follows() and \
            A not in B.get_follows()):
            B.append_follows(A)
        if not A.get_precedes() or \
            (A.get_precedes() and
            B not in A.get_precedes()):
            A.append_precedes(B)
    elif relation == "precedes":
        if not B.get_precedes() or \
            (B.get_precedes() and \
            A not in B.get_precedes()):
            B.append_precedes(A)
        if not A.get_follows() or \
            (A.get_follows() and \
            B not in A.get_follows()):
            A.append_follows(B)
    elif relation == "during" or relation == "starts" or \
            relation == "finishes":
        if not B.get_during() or \
            (B.get_during() and \
            A not in B.get_during()):
            B.append_during(A)
        if not A.get_contains() or \
            (A.get_contains() and \
            B not in A.get_contains()):
            A.append_contains(B)
        if relation == "starts":
            if not B.get_starts() or \
            (B.get_starts() and \
            A not in B.get_starts()):
                B.append_starts(A)
            if not A.get_started() or \
            (A.get_started() and \
            B not in A.get_started()):
                A.append_started(B)
        if relation == "finishes":
            if not B.get_finishes() or \
            (B.get_finishes() and \
            A not in B.get_finishes()):
                B.append_finishes(A)
            if not A.get_finished() or \
            (A.get_finished() and \
            B not in A.get_finished()):
                A.append_finished(B)
    elif relation == "contains" or relation == "started" or \
            relation == "finished":
        if not B.get_contains() or \
            (B.get_contains() and \
            A not in B.get_contains()):
            B.append_contains(A)
        if not A.get_during() or \
            (A.get_during() and \
            B not in A.get_during()):
            A.append_during(B)
        if relation == "started":
            if not B.get_started() or \
            (B.get_started() and \
            A not in B.get_started()):
                B.append_started(A)
            if not A.get_starts() or \
            (A.get_starts() and \
            B not in A.get_starts()):
                A.append_starts(B)
        if relation == "finished":
            if not B.get_finished() or \
            (B.get_finished() and \
            A not in B.get_finished()):
                B.append_finished(A)
            if not A.get_finishes() or \
            (A.get_finishes() and \
            B not in A.get_finishes()):
                A.append_finishes(B)
    elif relation == "overlaps":
        if not B.get_overlaps() or \
            (B.get_overlaps() and \
            A not in B.get_overlaps()):
            B.append_overlaps(A)
        if not A.get_overlapped() or \
            (A.get_overlapped() and \
            B not in A.get_overlapped()):
            A.append_overlapped(B)
    elif relation == "overlapped":
        if not B.get_overlapped() or \
            (B.get_overlapped() and \
            A not in B.get_overlapped()):
            B.append_overlapped(A)
        if not A.get_overlaps() or \
            (A.get_overlaps() and \
            B not in A.get_overlaps()):
            A.append_overlaps(B)

###############################################################################

def set_spatial_relationship(A, B, relation):

    if relation == "equivalent":
        if not B.get_equivalent() or \
        (B.get_equivalent() and \
        A not in B.get_equivalent()):
            B.append_equivalent(A)
        if not A.get_equivalent() or \
        (A.get_equivalent() and \
        B not in A.get_equivalent()):
            A.append_equivalent(B)
    elif relation == "overlap":
        if not B.get_overlap() or \
            (B.get_overlap() and \
            A not in B.get_overlap()):
            B.append_overlap(A)
        if not A.get_overlap() or \
            (A.get_overlap() and
            B not in A.get_overlap()):
            A.append_overlap(B)
    elif relation == "meet":
        if not B.get_meet() or \
            (B.get_meet() and \
            A not in B.get_meet()):
            B.append_meet(A)
        if not A.get_meet() or \
            (A.get_meet() and
            B not in A.get_meet()):
            A.append_meet(B)
    elif relation == "contain":
        if not B.get_contain() or \
            (B.get_contain() and \
            A not in B.get_contain()):
            B.append_contain(A)
        if not A.get_in() or \
            (A.get_in() and \
            B not in A.get_in()):
            A.append_in(B)
    elif relation == "in":
        if not B.get_in() or \
            (B.get_in() and \
            A not in B.get_in()):
            B.append_in(A)
        if not A.get_contain() or \
            (A.get_contain() and \
            B not in A.get_contain()):
            A.append_contain(B)
    elif relation == "cover":
        if not B.get_cover() or \
            (B.get_cover() and \
            A not in B.get_cover()):
            B.append_cover(A)
        if not A.get_covered() or \
            (A.get_covered() and \
            B not in A.get_covered()):
            A.append_covered(B)
    elif relation == "covered":
        if not B.get_covered() or \
            (B.get_covered() and \
            A not in B.get_covered()):
            B.append_covered(A)
        if not A.get_cover() or \
            (A.get_cover() and \
            B not in A.get_cover()):
            A.append_cover(B)

###############################################################################

def print_temporal_topology_relationships(maps1, maps2=None, dbif=None):
    """!Print the temporal relationships of the
       map lists maps1 and maps2 to stdout.

        @param maps1 A list of abstract_dataset
                      objects with initiated temporal extent
        @param maps2 An optional list of abstract_dataset
                      objects with initiated temporal extent
        @param dbif The database interface to be used
    """

    tb = SpatioTemporalTopologyBuilder()

    tb.build(maps1, maps2)

    dbif, connected = init_dbif(dbif)

    for _map in tb:
        _map.select(dbif)
        _map.print_info()

    if connected:
        dbif.close()

    return

###############################################################################

def print_spatio_temporal_topology_relationships(maps1, maps2=None, spatial="2D", dbif=None):
    """!Print the temporal relationships of the
       map lists maps1 and maps2 to stdout.

        @param maps1 A list of abstract_dataset
                      objects with initiated temporal extent
        @param maps2 An optional list of abstract_dataset
                      objects with initiated temporal extent
        @param spatial The dimension of the spatial extent to be used: "2D" using west, east,
                        south, north or "3D" using west, east, south, north, bottom, top
        @param dbif The database interface to be used
    """

    tb = SpatioTemporalTopologyBuilder()

    tb.build(maps1, maps2, spatial)

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


        @param maps1 A list of abstract_dataset
                      objects with initiated temporal extent
        @param maps2 A list of abstract_dataset
                      objects with initiated temporal extent
        @param dbif The database interface to be used
        @return A dictionary with counted temporal relationships
    """


    tb = SpatioTemporalTopologyBuilder()
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

        @param start The start time
        @param end The end time
        @param use_start Select maps of which the start time is located in the selection granule
                         @verbatim
                         map    :        s
                         granule:  s-----------------e

                         map    :        s--------------------e
                         granule:  s-----------------e

                         map    :        s--------e
                         granule:  s-----------------e
                         @endverbatim

        @param use_during Select maps which are temporal during the selection granule
                         @verbatim
                         map    :     s-----------e
                         granule:  s-----------------e
                         @endverbatim

        @param use_overlap Select maps which temporal overlap the selection granule
                         @verbatim
                         map    :     s-----------e
                         granule:        s-----------------e

                         map    :     s-----------e
                         granule:  s----------e
                         @endverbatim

        @param use_contain Select maps which temporally contain the selection granule
                         @verbatim
                         map    :  s-----------------e
                         granule:     s-----------e
                         @endverbatim

        @param use_equal Select maps which temporally equal to the selection granule
                         @verbatim
                         map    :  s-----------e
                         granule:  s-----------e
                         @endverbatim

        @param use_follows Select maps which temporally follow the selection granule
                         @verbatim
                         map    :              s-----------e
                         granule:  s-----------e
                         @endverbatim

        @param use_precedes Select maps which temporally precedes the selection granule
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

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
