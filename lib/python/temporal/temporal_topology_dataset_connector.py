# -*- coding: utf-8 -*-
"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

>>> import grass.temporal as tgis
>>> tmr = tgis.TemporalTopologyDatasetConnector()

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import copy

class TemporalTopologyDatasetConnector(object):
    """!This class implements a temporal topology access structure to connect temporal related datasets

       This object will be set up by temporal topology creation method provided by the 
       SpatioTemporalTopologyBuilder.

       If correctly initialize the calls next() and prev() 
       let the user walk temporally forward and backward in time.

       The following temporal relations with access methods are supported:
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
        
        >>> import grass.temporal as tgis
        >>> tgis.init()
        >>> map = tgis.RasterDataset("a@P")
        >>> tmr = tgis.TemporalTopologyDatasetConnector()
        >>> tmr.set_next(map)
        >>> tmr.set_prev(map)
        >>> tmr.append_equal(map)
        >>> tmr.append_follows(map)
        >>> tmr.append_precedes(map)
        >>> tmr.append_overlapped(map)
        >>> tmr.append_overlaps(map)
        >>> tmr.append_during(map)
        >>> tmr.append_contains(map)
        >>> tmr.append_starts(map)
        >>> tmr.append_started(map)
        >>> tmr.append_finishes(map)
        >>> tmr.append_finished(map)
        >>> tmr.print_temporal_topology_info()
         +-------------------- Temporal Topology -------------------------------------+
         | Next: ...................... a@P
         | Previous: .................. a@P
         | Equal:...................... a@P
         | Follows: ................... a@P
         | Precedes: .................. a@P
         | Overlaps: .................. a@P
         | Overlapped: ................ a@P
         | During: .................... a@P
         | Contains: .................. a@P
         | Starts:.. .................. a@P
         | Started:. .................. a@P
         | Finishes:................... a@P
         | Finished:................... a@P
        >>> tmr.print_temporal_topology_shell_info()
        next=a@P
        prev=a@P
        equal=a@P
        follows=a@P
        precedes=a@P
        overlaps=a@P
        overlapped=a@P
        during=a@P
        contains=a@P
        starts=a@P
        started=a@P
        finishes=a@P
        finished=a@P
        >>> rlist = tmr.get_temporal_relations()
        >>> if "FINISHED" in rlist.keys():
        ...    print rlist["FINISHED"][0].get_id()
        a@P

        @endcode
    """

    def __init__(self):
        self.reset_temporal_topology()

    def reset_temporal_topology(self):
        """!Reset any information about temporal topology"""
        self._temporal_topology = {}
        self._has_temporal_topology = False
        
    def get_temporal_relations(self):
        """!Return the dictionary of temporal relationships
        
            Keys are the temporal relationships in upper case,
            values are abstract map objects.
            
            @return The temporal relations dictionary
        """
        return copy.copy(self._temporal_topology)
        
    def get_number_of_temporal_relations(self):
        """! Return a dictionary in which the keys are the relation names and the value
        are the number of relations.
        
        The following relations are available:
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
        
        To access topological information the temporal topology must be build first
        using the SpatioTemporalTopologyBuilder.
        
        @return the dictionary with relations as keys and number as values or None in case the topology wasn't build
        """
        if self._has_temporal_topology == False:
            return None
    
        relations = {}
        try:
            relations["equal"] = len(self._temporal_topology["EQUAL"]) 
        except:
            relations["equal"] = 0
        try: 
            relations["follows"] = len(self._temporal_topology["FOLLOWS"]) 
        except: 
            relations["follows"] = 0
        try: 
            relations["precedes"] = len(self._temporal_topology["PRECEDES"])
        except: 
            relations["precedes"] = 0
        try: 
            relations["overlaps"] = len(self._temporal_topology["OVERLAPS"])
        except: 
            relations["overlaps"] = 0
        try: 
            relations["overlapped"] = len(self._temporal_topology["OVERLAPPED"])
        except: 
            relations["overlapped"] = 0
        try: 
            relations["during"] = len(self._temporal_topology["DURING"])
        except: 
            relations["during"] = 0
        try: 
            relations["contains"] = len(self._temporal_topology["CONTAINS"])
        except: 
            relations["contains"] = 0
        try: 
            relations["starts"] = len(self._temporal_topology["STARTS"])
        except: 
            relations["starts"] = 0
        try:    
            relations["started"] = len(self._temporal_topology["STARTED"])
        except: 
            relations["started"] = 0
        try: 
            relations["finishes"] = len(self._temporal_topology["FINISHES"])
        except: 
            relations["finishes"] = 0
        try: 
            relations["finished"] = len(self._temporal_topology["FINISHED"])
        except: 
            relations["finished"] = 0
            
        return relations

    def set_temporal_topology_build_true(self):
        """!Same as name"""
        self._has_temporal_topology = True

    def set_temporal_topology_build_false(self):
        """!Same as name"""
        self._has_temporal_topology = False

    def is_temporal_topology_build(self):
        """!Check if the temporal topology was build"""
        return self._has_temporal_topology

    def set_next(self, map):
        """!Set the map that is temporally as closest located after this map.

           Temporally located means that the start time of the "next" map is
           temporally located AFTER the start time of this map, but temporally
           near than other maps of the same dataset.

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        self._temporal_topology["NEXT"] = map

    def set_prev(self, map):
        """!Set the map that is temporally as closest located before this map.

           Temporally located means that the start time of the "previous" map is
           temporally located BEFORE the start time of this map, but temporally
           near than other maps of the same dataset.

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        self._temporal_topology["PREV"] = map

    def next(self):
        """!Return the map with a start time temporally located after
           the start time of this map, but temporal closer than other maps

           @return A map object or None
        """
        if "NEXT" not in self._temporal_topology:
            return None
        return self._temporal_topology["NEXT"]

    def prev(self):
        """!Return the map with a start time temporally located before
           the start time of this map, but temporal closer than other maps

           @return A map object or None
        """
        if "PREV" not in self._temporal_topology:
            return None
        return self._temporal_topology["PREV"]

    def append_equal(self, map):
        """!Append a map with equivalent temporal extent as this map

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "EQUAL" not in self._temporal_topology:
            self._temporal_topology["EQUAL"] = []
        self._temporal_topology["EQUAL"].append(map)

    def get_equal(self):
        """!Return a list of map objects with equivalent temporal extent as this map

           @return A list of map objects or None
        """
        if "EQUAL" not in self._temporal_topology:
            return None
        return self._temporal_topology["EQUAL"]

    def append_starts(self, map):
        """!Append a map that this map temporally starts with

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "STARTS" not in self._temporal_topology:
            self._temporal_topology["STARTS"] = []
        self._temporal_topology["STARTS"].append(map)

    def get_starts(self):
        """!Return a list of map objects that this map temporally starts with

           @return A list of map objects or None
        """
        if "STARTS" not in self._temporal_topology:
            return None
        return self._temporal_topology["STARTS"]

    def append_started(self, map):
        """!Append a map that this map temporally started with

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "STARTED" not in self._temporal_topology:
            self._temporal_topology["STARTED"] = []
        self._temporal_topology["STARTED"].append(map)

    def get_started(self):
        """!Return a list of map objects that this map temporally started with

           @return A list of map objects or None
        """
        if "STARTED" not in self._temporal_topology:
            return None
        return self._temporal_topology["STARTED"]

    def append_finishes(self, map):
        """!Append a map that this map temporally finishes with

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "FINISHES" not in self._temporal_topology:
            self._temporal_topology["FINISHES"] = []
        self._temporal_topology["FINISHES"].append(map)

    def get_finishes(self):
        """!Return a list of map objects that this map temporally finishes with

           @return A list of map objects or None
        """
        if "FINISHES" not in self._temporal_topology:
            return None
        return self._temporal_topology["FINISHES"]

    def append_finished(self, map):
        """!Append a map that this map temporally finished with

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "FINISHED" not in self._temporal_topology:
            self._temporal_topology["FINISHED"] = []
        self._temporal_topology["FINISHED"].append(map)

    def get_finished(self):
        """!Return a list of map objects that this map temporally finished with

           @return A list of map objects or None
        """
        if "FINISHED" not in self._temporal_topology:
            return None
        return self._temporal_topology["FINISHED"]

    def append_overlaps(self, map):
        """!Append a map that this map temporally overlaps

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "OVERLAPS" not in self._temporal_topology:
            self._temporal_topology["OVERLAPS"] = []
        self._temporal_topology["OVERLAPS"].append(map)

    def get_overlaps(self):
        """!Return a list of map objects that this map temporally overlaps

           @return A list of map objects or None
        """
        if "OVERLAPS" not in self._temporal_topology:
            return None
        return self._temporal_topology["OVERLAPS"]

    def append_overlapped(self, map):
        """!Append a map that this map temporally overlapped

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "OVERLAPPED" not in self._temporal_topology:
            self._temporal_topology["OVERLAPPED"] = []
        self._temporal_topology["OVERLAPPED"].append(map)

    def get_overlapped(self):
        """!Return a list of map objects that this map temporally overlapped

           @return A list of map objects or None
        """
        if "OVERLAPPED" not in self._temporal_topology:
            return None
        return self._temporal_topology["OVERLAPPED"]

    def append_follows(self, map):
        """!Append a map that this map temporally follows

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "FOLLOWS" not in self._temporal_topology:
            self._temporal_topology["FOLLOWS"] = []
        self._temporal_topology["FOLLOWS"].append(map)

    def get_follows(self):
        """!Return a list of map objects that this map temporally follows

           @return A list of map objects or None
        """
        if "FOLLOWS" not in self._temporal_topology:
            return None
        return self._temporal_topology["FOLLOWS"]

    def append_precedes(self, map):
        """!Append a map that this map temporally precedes

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "PRECEDES" not in self._temporal_topology:
            self._temporal_topology["PRECEDES"] = []
        self._temporal_topology["PRECEDES"].append(map)

    def get_precedes(self):
        """!Return a list of map objects that this map temporally precedes

           @return A list of map objects or None
        """
        if "PRECEDES" not in self._temporal_topology:
            return None
        return self._temporal_topology["PRECEDES"]

    def append_during(self, map):
        """!Append a map that this map is temporally located during
           This includes temporal relationships starts and finishes

           @param map This object should be of type 
                        AbstractMapDataset or derived classes
        """
        if "DURING" not in self._temporal_topology:
            self._temporal_topology["DURING"] = []
        self._temporal_topology["DURING"].append(map)

    def get_during(self):
        """!Return a list of map objects that this map is temporally located during
           This includes temporally relationships starts and finishes

           @return A list of map objects or None
        """
        if "DURING" not in self._temporal_topology:
            return None
        return self._temporal_topology["DURING"]

    def append_contains(self, map):
        """!Append a map that this map temporally contains
           This includes temporal relationships started and finished

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "CONTAINS" not in self._temporal_topology:
            self._temporal_topology["CONTAINS"] = []
        self._temporal_topology["CONTAINS"].append(map)

    def get_contains(self):
        """!Return a list of map objects that this map temporally contains
           This includes temporal relationships started and finished

           @return A list of map objects or None
        """
        if "CONTAINS" not in self._temporal_topology:
            return None
        return self._temporal_topology["CONTAINS"]

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

    def print_temporal_topology_info(self):
        """!Print information about this class in human readable style"""
        
        print " +-------------------- Temporal Topology -------------------------------------+"
        #          0123456789012345678901234567890
        if self.next() is not None:
            print " | Next: ...................... " + str(self.next().get_id())
        if self.prev() is not None:
            print " | Previous: .................. " + str(self.prev().get_id())
        if self.equal is not None:
            print " | Equal:...................... " + \
                self._generate_map_list_string(self.equal)
        if self.follows is not None:
            print " | Follows: ................... " + \
                self._generate_map_list_string(self.follows)
        if self.precedes is not None:
            print " | Precedes: .................. " + \
                self._generate_map_list_string(self.precedes)
        if self.overlaps is not None:
            print " | Overlaps: .................. " + \
                self._generate_map_list_string(self.overlaps)
        if self.overlapped is not None:
            print " | Overlapped: ................ " + \
                self._generate_map_list_string(self.overlapped)
        if self.during is not None:
            print " | During: .................... " + \
                self._generate_map_list_string(self.during)
        if self.contains is not None:
            print " | Contains: .................. " + \
                self._generate_map_list_string(self.contains)
        if self.starts is not None:
            print " | Starts:.. .................. " + \
                self._generate_map_list_string(self.starts)
        if self.started is not None:
            print " | Started:. .................. " + \
                self._generate_map_list_string(self.started)
        if self.finishes is not None:
            print " | Finishes:................... " + \
                self._generate_map_list_string(self.finishes)
        if self.finished is not None:
            print " | Finished:................... " + \
                self._generate_map_list_string(self.finished)

    def print_temporal_topology_shell_info(self):
        """!Print information about this class in shell style"""
        
        if self.next() is not None:
            print "next=" + self.next().get_id()
        if self.prev() is not None:
            print "prev=" + self.prev().get_id()
        if self.equal is not None:
            print "equal=" + self._generate_map_list_string(self.equal, False)
        if self.follows is not None:
            print "follows=" + self._generate_map_list_string(self.follows, False)
        if self.precedes is not None:
            print "precedes=" + self._generate_map_list_string(
                self.precedes, False)
        if self.overlaps is not None:
            print "overlaps=" + self._generate_map_list_string(
                self.overlaps, False)
        if self.overlapped is not None:
            print "overlapped=" + \
                self._generate_map_list_string(self.overlapped, False)
        if self.during is not None:
            print "during=" + self._generate_map_list_string(self.during, False)
        if self.contains is not None:
            print "contains=" + self._generate_map_list_string(
                self.contains, False)
        if self.starts is not None:
            print "starts=" + \
                self._generate_map_list_string(self.starts)
        if self.started is not None:
            print "started=" + \
                self._generate_map_list_string(self.started)
        if self.finishes is not None:
            print "finishes=" + \
                self._generate_map_list_string(self.finishes)
        if self.finished is not None:
            print "finished=" + \
                self._generate_map_list_string(self.finished)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()