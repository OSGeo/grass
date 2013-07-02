# -*- coding: utf-8 -*-
"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

>>> import grass.temporal as tgis
>>> tmr = tgis.SpatialTopologyDatasetConnector()

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import copy

class SpatialTopologyDatasetConnector(object):
    """!This class implements a spatial topology access structure to connect spatial related datasets

       This object will be set up by spatial topology creation method provided by the 
       SpatioTemporalTopologyBuilder.

       The following spatial relations with access methods are supported:
       - equivalent
       - overlap
       - in
       - contain
       - meet
       - cover
       - covered
            
        Usage:
        
        @code
        
        >>> import grass.temporal as tgis
        >>> tgis.init()
        >>> map = tgis.RasterDataset("a@P")
        >>> tmr = tgis.SpatialTopologyDatasetConnector()
        >>> tmr.append_equivalent(map)
        >>> tmr.append_overlap(map)
        >>> tmr.append_in(map)
        >>> tmr.append_contain(map)
        >>> tmr.append_meet(map)
        >>> tmr.append_cover(map)
        >>> tmr.append_covered(map)
        >>> tmr.print_spatial_topology_info()
         +-------------------- Spatial Topology --------------------------------------+
         | Equivalent: ................ a@P
         | Cover: ..................... a@P
         | Covered: ................... a@P
         | Overlap: ................... a@P
         | In: ........................ a@P
         | Contain: ................... a@P
         | Meet: ...................... a@P
        >>> tmr.print_spatial_topology_shell_info()
        equivalent=a@P
        cover=a@P
        covered=a@P
        overlap=a@P
        in=a@P
        contain=a@P
        meet=a@P
        >>> rlist = tmr.get_spatial_relations()
        >>> if "COVER" in rlist.keys():
        ...    print rlist["COVER"][0].get_id()
        a@P

        
        @endcode
    """

    def __init__(self):
        self.reset_spatial_topology()

    def reset_spatial_topology(self):
        """!Reset any information about temporal topology"""
        self._spatial_topology = {}
        self._has_spatial_topology = False
        
    def get_spatial_relations(self):
        """!Return the dictionary of spatial relationships
        
            Keys are the spatial relationships in upper case,
            values are abstract map objects.
            
            @return The spatial relations dictionary
        """
        return copy.copy(self._spatial_topology)
    
    def get_number_of_spatial_relations(self):
        """! Return a dictionary in which the keys are the relation names and the value
        are the number of relations.
        
        The following relations are available:
       - equivalent
       - overlap
       - in
       - contain
       - meet
       - cover
       - covered
        
        To access topological information the spatial topology must be build first
        using the SpatialTopologyBuilder.
        
        @return the dictionary with relations as keys and number as values or None in case the topology wasn't build
        """
        if self._has_spatial_topology == False:
            return None
    
        relations = {}
        try:
            relations["equivalent"] = len(self._spatial_topology["EQUIVALENT"]) 
        except:
            relations["equivalent"] = 0
        try: 
            relations["overlap"] = len(self._spatial_topology["OVERLAP"]) 
        except: 
            relations["overlap"] = 0
        try: 
            relations["in"] = len(self._spatial_topology["IN"])
        except: 
            relations["in"] = 0
        try: 
            relations["contain"] = len(self._spatial_topology["CONTAIN"])
        except: 
            relations["contain"] = 0
        try: 
            relations["meet"] = len(self._spatial_topology["MEET"])
        except: 
            relations["meet"] = 0
        try: 
            relations["cover"] = len(self._spatial_topology["COVER"])
        except: 
            relations["cover"] = 0
        try: 
            relations["covered"] = len(self._spatial_topology["COVERED"])
        except: 
            relations["covered"] = 0
            
        return relations

    def set_spatial_topology_build_true(self):
        """!Same as name"""
        self._has_spatial_topology = True

    def set_spatial_topology_build_false(self):
        """!Same as name"""
        self._has_spatial_topology = False

    def is_spatial_topology_build(self):
        """!Check if the temporal topology was build"""
        return self._has_spatial_topology

    def append_equivalent(self, map):
        """!Append a map with equivalent spatial extent as this map

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "EQUIVALENT" not in self._spatial_topology:
            self._spatial_topology["EQUIVALENT"] = []
        self._spatial_topology["EQUIVALENT"].append(map)

    def get_equivalent(self):
        """!Return a list of map objects with equivalent spatial extent as this map

           @return A list of map objects or None
        """
        if "EQUIVALENT" not in self._spatial_topology:
            return None
        return self._spatial_topology["EQUIVALENT"]

    def append_overlap(self, map):
        """!Append a map that this spatial overlap with this map

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "OVERLAP" not in self._spatial_topology:
            self._spatial_topology["OVERLAP"] = []
        self._spatial_topology["OVERLAP"].append(map)

    def get_overlap(self):
        """!Return a list of map objects that this map spatial overlap with

           @return A list of map objects or None
        """
        if "OVERLAP" not in self._spatial_topology:
            return None
        return self._spatial_topology["OVERLAP"]

    def append_in(self, map):
        """!Append a map that this is spatial in this map

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "IN" not in self._spatial_topology:
            self._spatial_topology["IN"] = []
        self._spatial_topology["IN"].append(map)

    def get_in(self):
        """!Return a list of map objects that are spatial in this map

           @return A list of map objects or None
        """
        if "IN" not in self._spatial_topology:
            return None
        return self._spatial_topology["IN"]

    def append_contain(self, map):
        """!Append a map that this map spatially contains

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "CONTAIN" not in self._spatial_topology:
            self._spatial_topology["CONTAIN"] = []
        self._spatial_topology["CONTAIN"].append(map)

    def get_contain(self):
        """!Return a list of map objects that this map contains

           @return A list of map objects or None
        """
        if "CONTAIN" not in self._spatial_topology:
            return None
        return self._spatial_topology["CONTAIN"]

    def append_meet(self, map):
        """!Append a map that spatially meet with this map

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "MEET" not in self._spatial_topology:
            self._spatial_topology["MEET"] = []
        self._spatial_topology["MEET"].append(map)

    def get_meet(self):
        """!Return a list of map objects that spatially meet with this map

           @return A list of map objects or None
        """
        if "MEET" not in self._spatial_topology:
            return None
        return self._spatial_topology["MEET"]

    def append_cover(self, map):
        """!Append a map that spatially cover this map

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "COVER" not in self._spatial_topology:
            self._spatial_topology["COVER"] = []
        self._spatial_topology["COVER"].append(map)

    def get_cover(self):
        """!Return a list of map objects that spatially cover this map

           @return A list of map objects or None
        """
        if "COVER" not in self._spatial_topology:
            return None
        return self._spatial_topology["COVER"]

    def append_covered(self, map):
        """!Append a map that is spatially covered by this map

           @param map This object should be of type AbstractMapDataset 
                        or derived classes
        """
        if "COVERED" not in self._spatial_topology:
            self._spatial_topology["COVERED"] = []
        self._spatial_topology["COVERED"].append(map)

    def get_covered(self):
        """!Return a list of map objects that are spatially covered by this map

           @return A list of map objects or None
        """
        if "COVERED" not in self._spatial_topology:
            return None
        return self._spatial_topology["COVERED"]


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
    equivalent = property(fget=get_equivalent, 
                                       fset=append_equivalent)
    cover = property(fget=get_cover, 
                                     fset=append_cover)
    covered = property(fget=get_covered, 
                                       fset=append_covered)
    overlap = property(fget=get_overlap, 
                                     fset=append_overlap)
    in_ = property(fget=get_in, 
                                     fset=append_in)
    contain = property(fget=get_contain, 
                                     fset=append_contain)
    meet = property(fget=get_meet, 
                                     fset=append_meet)

    def print_spatial_topology_info(self):
        """!Print information about this class in human readable style"""
        
        print " +-------------------- Spatial Topology --------------------------------------+"
        #          0123456789012345678901234567890
        if self.equivalent is not None:
            print " | Equivalent: ................ " + \
                self._generate_map_list_string(self.equivalent)
        if self.cover is not None:
            print " | Cover: ..................... " + \
                self._generate_map_list_string(self.cover)
        if self.covered is not None:
            print " | Covered: ................... " + \
                self._generate_map_list_string(self.covered)
        if self.overlap is not None:
            print " | Overlap: ................... " + \
                self._generate_map_list_string(self.overlap)
        if self.in_ is not None:
            print " | In: ........................ " + \
                self._generate_map_list_string(self.in_)
        if self.contain is not None:
            print " | Contain: ................... " + \
                self._generate_map_list_string(self.contain)
        if self.meet is not None:
            print " | Meet: ...................... " + \
                self._generate_map_list_string(self.meet)

    def print_spatial_topology_shell_info(self):
        """!Print information about this class in shell style"""

        if self.equivalent is not None:
            print "equivalent=" + self._generate_map_list_string(self.equivalent, False)
        if self.cover is not None:
            print "cover=" + self._generate_map_list_string(
                self.cover, False)
        if self.covered is not None:
            print "covered=" + \
                self._generate_map_list_string(self.covered, False)
        if self.overlap is not None:
            print "overlap=" + \
                self._generate_map_list_string(self.overlap)
        if self.in_ is not None:
            print "in=" + \
                self._generate_map_list_string(self.in_)
        if self.contain is not None:
            print "contain=" + \
                self._generate_map_list_string(self.contain)
        if self.meet is not None:
            print "meet=" + \
                self._generate_map_list_string(self.meet)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()