# -*- coding: utf-8 -*-
"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

>>> import grass.temporal as tgis
>>> tmr = tgis.AbstractTemporalDataset()

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from abstract_dataset import *
from datetime_math import *


class AbstractTemporalDataset(AbstractDataset):
    """!This class implements a temporal topology access structure for an abstract dataset

       This object will be set up by temporal topology creation method provided by the 
       TemporallyTopologyBuilder.

       If correctly initialize the calls next() and prev() 
       let the user walk temporally forward and backward in time.

       The following temporal relations with access methods are supported:
       * equal
       * follows
       * precedes
       * overlaps
       * overlapped
       * during (including starts, finishes)
       * contains (including started, finished)
       * starts
       * started
       * finishes
       * finished


       @code:
       # We have build the temporal topology and we know the first map
       start = first
       while start:

           # Print all maps this map temporally contains
           dlist = start.get_contains()
           for map in dlist:
               map.print_info()

           start = start.next()
         @endcode  
        
        Usage:
        
        @code
        
        >>> tmr = AbstractTemporalDataset()
        >>> tmr.print_topology_info()
         +-------------------- Temporal Topology -------------------------------------+
        >>> tmr.print_topology_shell_info()
        
        @endcode
    """

    def __init__(self):
        AbstractDataset.__init__(self)
        self.reset_topology()

    def reset_topology(self):
        """!Reset any information about temporal topology"""
        self._topology = {}
        self._has_topology = False
        
    def get_number_of_relations(self):      
        """! Return a dictionary in which the keys are the relation names and the value
        are the number of relations.
        
        The following relations are available:
        * equal
        * follows
        * precedes
        * overlaps
        * overlapped
        * during (including starts, finishes)
        * contains (including started, finished)
        * starts
        * started
        * finishes
        * finished
        
        To access topological information the temporal topology must be build first
        using the TemporalTopologyBuilder.
        
        @return the dictionary with relations as keys and number as values or None in case the topology wasn't build
        """
        if self._has_topology == False:
            return None
    
        relations = {}
        try:
            relations["equal"] = len(self._topology["EQUAL"]) 
        except:
            relations["equal"] = 0
        try: 
            relations["follows"] = len(self._topology["FOLLOWS"]) 
        except: 
            relations["follows"] = 0
        try: 
            relations["precedes"] = len(self._topology["PRECEDES"])
        except: 
            relations["precedes"] = 0
        try: 
            relations["overlaps"] = len(self._topology["OVERLAPS"])
        except: 
            relations["overlaps"] = 0
        try: 
            relations["overlapped"] = len(self._topology["OVERLAPPED"])
        except: 
            relations["overlapped"] = 0
        try: 
            relations["during"] = len(self._topology["DURING"])
        except: 
            relations["during"] = 0
        try: 
            relations["contains"] = len(self._topology["CONTAINS"])
        except: 
            relations["contains"] = 0
        try: 
            relations["starts"] = len(self._topology["STARTS"])
        except: 
            relations["starts"] = 0
        try:    
            relations["started"] = len(self._topology["STARTED"])
        except: 
            relations["started"] = 0
        try: 
            relations["finishes"] = len(self._topology["FINISHES"])
        except: 
            relations["finishes"] = 0
        try: 
            relations["finished"] = len(self._topology["FINISHED"])
        except: 
            relations["finished"] = 0
            
        return relations

    def set_topology_build_true(self):
        """!Same as name"""
        self._has_topology = True

    def set_topology_build_false(self):
        """!Same as name"""
        self._has_topology = False

    def is_topology_build(self):
        """!Check if the temporal topology was build"""
        return self._has_topology

    def set_next(self, map_):
        """!Set the map that is temporally as closest located after this map.

           Temporally located means that the start time of the "next" map is
           temporally located AFTER the start time of this map, but temporally
           near than other maps of the same dataset.

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        self._topology["NEXT"] = map_

    def set_prev(self, map_):
        """!Set the map that is temporally as closest located before this map.

           Temporally located means that the start time of the "previous" map is
           temporally located BEFORE the start time of this map, but temporally
           near than other maps of the same dataset.

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        self._topology["PREV"] = map_

    def next(self):
        """!Return the map with a start time temporally located after
           the start time of this map, but temporal closer than other maps

           @return A map object or None
        """
        if "NEXT" not in self._topology:
            return None
        return self._topology["NEXT"]

    def prev(self):
        """!Return the map with a start time temporally located before
           the start time of this map, but temporal closer than other maps

           @return A map object or None
        """
        if "PREV" not in self._topology:
            return None
        return self._topology["PREV"]

    def append_equal(self, map_):
        """!Append a map with equivalent temporal extent as this map

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "EQUAL" not in self._topology:
            self._topology["EQUAL"] = []
        self._topology["EQUAL"].append(map_)

    def get_equal(self):
        """!Return a list of map objects with equivalent temporal extent as this map

           @return A list of map objects or None
        """
        if "EQUAL" not in self._topology:
            return None
        return self._topology["EQUAL"]

    def append_starts(self, map_):
        """!Append a map that this map temporally starts with

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "STARTS" not in self._topology:
            self._topology["STARTS"] = []
        self._topology["STARTS"].append(map_)

    def get_starts(self):
        """!Return a list of map objects that this map temporally starts with

           @return A list of map objects or None
        """
        if "STARTS" not in self._topology:
            return None
        return self._topology["STARTS"]

    def append_started(self, map_):
        """!Append a map that this map temporally started with

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "STARTED" not in self._topology:
            self._topology["STARTED"] = []
        self._topology["STARTED"].append(map_)

    def get_started(self):
        """!Return a list of map objects that this map temporally started with

           @return A list of map objects or None
        """
        if "STARTED" not in self._topology:
            return None
        return self._topology["STARTED"]

    def append_finishes(self, map_):
        """!Append a map that this map temporally finishes with

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "FINISHES" not in self._topology:
            self._topology["FINISHES"] = []
        self._topology["FINISHES"].append(map_)

    def get_finishes(self):
        """!Return a list of map objects that this map temporally finishes with

           @return A list of map objects or None
        """
        if "FINISHES" not in self._topology:
            return None
        return self._topology["FINISHES"]

    def append_finished(self, map_):
        """!Append a map that this map temporally finished with

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "FINISHED" not in self._topology:
            self._topology["FINISHED"] = []
        self._topology["FINISHED"].append(map_)

    def get_finished(self):
        """!Return a list of map objects that this map temporally finished with

           @return A list of map objects or None
        """
        if "FINISHED" not in self._topology:
            return None
        return self._topology["FINISHED"]

    def append_overlaps(self, map_):
        """!Append a map that this map temporally overlaps

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "OVERLAPS" not in self._topology:
            self._topology["OVERLAPS"] = []
        self._topology["OVERLAPS"].append(map_)

    def get_overlaps(self):
        """!Return a list of map objects that this map temporally overlaps

           @return A list of map objects or None
        """
        if "OVERLAPS" not in self._topology:
            return None
        return self._topology["OVERLAPS"]

    def append_overlapped(self, map_):
        """!Append a map that this map temporally overlapped

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "OVERLAPPED" not in self._topology:
            self._topology["OVERLAPPED"] = []
        self._topology["OVERLAPPED"].append(map_)

    def get_overlapped(self):
        """!Return a list of map objects that this map temporally overlapped

           @return A list of map objects or None
        """
        if "OVERLAPPED" not in self._topology:
            return None
        return self._topology["OVERLAPPED"]

    def append_follows(self, map_):
        """!Append a map that this map temporally follows

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "FOLLOWS" not in self._topology:
            self._topology["FOLLOWS"] = []
        self._topology["FOLLOWS"].append(map_)

    def get_follows(self):
        """!Return a list of map objects that this map temporally follows

           @return A list of map objects or None
        """
        if "FOLLOWS" not in self._topology:
            return None
        return self._topology["FOLLOWS"]

    def append_precedes(self, map_):
        """!Append a map that this map temporally precedes

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "PRECEDES" not in self._topology:
            self._topology["PRECEDES"] = []
        self._topology["PRECEDES"].append(map_)

    def get_precedes(self):
        """!Return a list of map objects that this map temporally precedes

           @return A list of map objects or None
        """
        if "PRECEDES" not in self._topology:
            return None
        return self._topology["PRECEDES"]

    def append_during(self, map_):
        """!Append a map that this map is temporally located during
           This includes temporal relationships starts and finishes

           @param map_: This object should be of type 
                        AbstractMapDataset or derived classes
        """
        if "DURING" not in self._topology:
            self._topology["DURING"] = []
        self._topology["DURING"].append(map_)

    def get_during(self):
        """!Return a list of map objects that this map is temporally located during
           This includes temporally relationships starts and finishes

           @return A list of map objects or None
        """
        if "DURING" not in self._topology:
            return None
        return self._topology["DURING"]

    def append_contains(self, map_):
        """!Append a map that this map temporally contains
           This includes temporal relationships started and finished

           @param map_: This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "CONTAINS" not in self._topology:
            self._topology["CONTAINS"] = []
        self._topology["CONTAINS"].append(map_)

    def get_contains(self):
        """!Return a list of map objects that this map temporally contains
           This includes temporal relationships started and finished

           @return A list of map objects or None
        """
        if "CONTAINS" not in self._topology:
            return None
        return self._topology["CONTAINS"]

    def _generate_map_list_string(self, map_list, line_wrap=True):
        count = 0
        string = ""
        for map_ in map_list:
            if line_wrap and count > 0 and count % 3 == 0:
                string += "\n | ............................ "
                count = 0
            if count == 0:
                string += map_.get_id()
            else:
                string += ",%s" % map_.get_id()
            count += 1

        return string
    
    # Set the properties
    equal = property(fget=get_equal, 
                                       fset=append_equal)
    follows = property(fget=get_follows, 
                                    fset=append_follows)
    precedes = property(fget=get_precedes, 
                                     fset=append_precedes)
    overlaps = property(fget=get_overlaps, 
                                     fset=append_overlaps)
    overlapped = property(fget=get_overlapped, 
                                       fset=append_overlapped)
    during = property(fget=get_during, 
                                   fset=append_during)
    contains = property(fget=get_contains, 
                                     fset=append_contains)
    starts = property(fget=get_starts, 
                                     fset=append_starts)
    started = property(fget=get_started, 
                                     fset=append_started)
    finishes = property(fget=get_finishes, 
                                     fset=append_finishes)
    finished = property(fget=get_finished, 
                                     fset=append_finished)

    def print_topology_info(self):
        """!Print information about this class in human readable style"""
        _next = self.next()
        _prev = self.prev()
        _equal = self.get_equal()
        _follows = self.get_follows()
        _precedes = self.get_precedes()
        _overlaps = self.get_overlaps()
        _overlapped = self.get_overlapped()
        _during = self.get_during()
        _contains = self.get_contains()
        _starts = self.get_starts()
        _started = self.get_started()
        _finishes = self.get_finishes()
        _finished = self.get_finished()
        
        print " +-------------------- Temporal Topology -------------------------------------+"
        #          0123456789012345678901234567890
        if _next is not None:
            print " | Next: ...................... " + str(_next.get_id())
        if _prev is not None:
            print " | Previous: .................. " + str(_prev.get_id())
        if _equal is not None:
            print " | Equal:...................... " + \
                self._generate_map_list_string(_equal)
        if _follows is not None:
            print " | Follows: ................... " + \
                self._generate_map_list_string(_follows)
        if _precedes is not None:
            print " | Precedes: .................. " + \
                self._generate_map_list_string(_precedes)
        if _overlaps is not None:
            print " | Overlaps: .................. " + \
                self._generate_map_list_string(_overlaps)
        if _overlapped is not None:
            print " | Overlapped: ................ " + \
                self._generate_map_list_string(_overlapped)
        if _during is not None:
            print " | During: .................... " + \
                self._generate_map_list_string(_during)
        if _contains is not None:
            print " | Contains: .................. " + \
                self._generate_map_list_string(_contains)
        if _starts is not None:
            print " | Starts:.. .................. " + \
                self._generate_map_list_string(_starts)
        if _started is not None:
            print " | Started:. .................. " + \
                self._generate_map_list_string(_started)
        if _finishes is not None:
            print " | Finishes:................... " + \
                self._generate_map_list_string(_finishes)
        if _finished is not None:
            print " | Finished:................... " + \
                self._generate_map_list_string(_finished)

    def print_topology_shell_info(self):
        """!Print information about this class in shell style"""

        _next = self.next()
        _prev = self.prev()
        _equal = self.get_equal()
        _follows = self.get_follows()
        _precedes = self.get_precedes()
        _overlaps = self.get_overlaps()
        _overlapped = self.get_overlapped()
        _during = self.get_during()
        _contains = self.get_contains()
        _starts = self.get_starts()
        _started = self.get_started()
        _finishes = self.get_finishes()
        _finished = self.get_finished()
        
        if _next is not None:
            print "next=" + _next.get_id()
        if _prev is not None:
            print "prev=" + _prev.get_id()
        if _equal is not None:
            print "equal=" + self._generate_map_list_string(_equal, False)
        if _follows is not None:
            print "follows=" + self._generate_map_list_string(_follows, False)
        if _precedes is not None:
            print "precedes=" + self._generate_map_list_string(
                _precedes, False)
        if _overlaps is not None:
            print "overlaps=" + self._generate_map_list_string(
                _overlaps, False)
        if _overlapped is not None:
            print "overlapped=" + \
                self._generate_map_list_string(_overlapped, False)
        if _during is not None:
            print "during=" + self._generate_map_list_string(_during, False)
        if _contains is not None:
            print "contains=" + self._generate_map_list_string(
                _contains, False)
        if _starts is not None:
            print "starts=" + \
                self._generate_map_list_string(_starts)
        if _started is not None:
            print "started=" + \
                self._generate_map_list_string(_started)
        if _finishes is not None:
            print "finishes=" + \
                self._generate_map_list_string(_finishes)
        if _finished is not None:
            print "finished=" + \
                self._generate_map_list_string(_finished)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()