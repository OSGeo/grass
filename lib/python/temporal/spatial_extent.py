"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related spatial extent functions to be used in Python scripts and tgis packages.

Usage:

@code
import grass.temporal as tgis

extent = tgis.raster_spatial_extent()
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from base import *

class spatial_extent(sql_database_interface):
    """!This is the spatial extent base class for all maps and space time datasets"""
    def __init__(self, table=None, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None, proj="XY"):

	sql_database_interface.__init__(self, table, ident)
        self.set_id(ident)
        self.set_spatial_extent(north, south, east, west, top, bottom)
	self.set_projection(proj)
        
    def overlapping_2d(self, extent):
        """!Return True if the two dimensional extents overlap. Code is lend from wind_overlap.c in lib/gis
        
	   Overlapping includes the spatial relations:
	   * contain
	   * in
	   * cover
	   * covered
	   * equivalent
	"""  
        
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute overlapping_2d for spatial extents"))
            return False
        
        N = extent.get_north()
        S = extent.get_south()
        E = extent.get_east()
        W = extent.get_west()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while E < self.get_west():
                E += 360.0
                W += 360.0

            while W > self.get_east():
                E -= 360.0
                W -= 360.0
                
        if(self.get_north() <= S):
            return False
        
        if(self.get_south() >= N):
            return False
            
        if self.get_east() <= W:
            return False
        
        if self.get_west() >= E:
            return False
        
        return True

    def overlapping(self, extent):
        """!Return True if the three dimensional extents overlap
        
	   Overlapping includes the spatial relations:
	   * contain
	   * in
	   * cover
	   * covered
	   * equivalent
        """  
        
        if not self.overlapping_2d(extent):
            return False
            
        T = extent.get_top()
        B = extent.get_bottom()
        
        if self.get_top() <= B:
            return False
        
        if self.get_bottom() >= T:
            return False
        
        return True

    def intersect_2d(self, extent):
	"""!Return the two dimensional intersection as spatial_extent object or None
	   in case no intersection was found.
	"""
	
	if not self.overlapping_2d(extent):
	    return None
	    
	eN = extent.get_north()
        eS = extent.get_south()
        eE = extent.get_east()
        eW = extent.get_west()
        
	N = self.get_north()
        S = self.get_south()
        E = self.get_east()
        W = self.get_west()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while eE < W:
                eE += 360.0
                eW += 360.0

            while eW > E:
                eE -= 360.0
                eW -= 360.0	
                
        # Compute the extent
        nN = N
        nS = S
        nE = E
        nW = W
        
        if W < eW:
	    nW = eW
	if E > eE:
	    nE = eE
	if N > eN:
	    nN = eN
	if S < eS:
	    nS = eS
	
	
	new = spatial_extent(north=nN, south=nS, east=nE, west=nW, top=0, bottom=0, proj=self.get_projection())
	return new

    def intersect(self, extent):
	"""!Return the three dimensional intersection as spatial_extent object or None
	   in case no intersection was found.
	"""
	
	if not self.overlapping(extent):
	    return None
	    
	new = self.intersect_2d(extent)
	
	eT = extent.get_top()
	eB = extent.get_bottom()
	
	T = self.get_top()
	B = self.get_bottom()
	
	nT = T
	nB = B
	
	if B < eB:
	    nB = eB
	if T > eT:
	    nT = eT
	
	new.set_top(nT)
	new.set_bottom(nB)
	
	return new

    def is_in_2d(self, extent):
	"""Check two dimensional if the self is located in extent 
	
         _____	
	|A _  |
	| |_| |
	|_____|
	
	"""
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute is_in_2d for spatial extents"))
            return False
            
	eN = extent.get_north()
        eS = extent.get_south()
        eE = extent.get_east()
        eW = extent.get_west()
        
	N = self.get_north()
        S = self.get_south()
        E = self.get_east()
        W = self.get_west()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while eE < W:
                eE += 360.0
                eW += 360.0

            while eW > E:
                eE -= 360.0
                eW -= 360.0	
               
        if W <= eW:
	    return False
	if E >= eE:
	    return False
	if N >= eN:
	    return False
	if S <= eS:
	    return False
	
	return True
	
    def is_in(self, extent):
	"""Check three dimensional if the self is located in extent """
	if not self.is_in_2d(extent):
	    return False
	
	eT = extent.get_top()
	eB = extent.get_bottom()
	
	T = self.get_top()
	B = self.get_bottom()
	
	if B <= eB:
	    return False
	if T >= eT:
	    return False
	    
	return True

    def contain_2d(self, extent):
	"""Check two dimensional if self contains extent """
	return extent.is_in_2d(self)
	
    def contain(self, extent):
	"""Check three dimensional if self contains extent """
	return extent.is_in(self)
        	
    def equivalent_2d(self, extent):
	"""Check two dimensional if self is equivalent to extent """

        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute equivalent_2d for spatial extents"))
            return False
            
	eN = extent.get_north()
        eS = extent.get_south()
        eE = extent.get_east()
        eW = extent.get_west()
        
	N = self.get_north()
        S = self.get_south()
        E = self.get_east()
        W = self.get_west()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while eE < W:
                eE += 360.0
                eW += 360.0

            while eW > E:
                eE -= 360.0
                eW -= 360.0	
               
        if W != eW:
	    return False
	if E != eE:
	    return False
	if N != eN:
	    return False
	if S != eS:
	    return False
	
	return True
	
    def equivalent(self, extent):
	"""Check three dimensional if self is equivalent to extent """

	if not self.equivalent_2d(extent):
	    return False
	
	eT = extent.get_top()
	eB = extent.get_bottom()
	
	T = self.get_top()
	B = self.get_bottom()
	
	if B != eB:
	    return False
	if T != eT:
	    return False
	    
	return True

    def cover_2d(self, extent):
        """Return True if two dimensional self covers extent 
            _____    _____    _____    _____
           |A  __|  |__  A|  |A | B|  |B | A|
           |  |B |  | B|  |  |  |__|  |__|  |
           |__|__|  |__|__|  |_____|  |_____|
           
            _____    _____    _____    _____
           |A|B| |  |A  __|  |A _  |  |__  A|
           | |_| |  |  |__|B | |B| | B|__|  |
           |_____|  |_____|  |_|_|_|  |_____|
           
            _____    _____    _____    _____
           |A|B  |  |_____|A |A|B|A|  |_____|A
           | |   |  |B    |  | | | |  |_____|B
           |_|___|  |_____|  |_|_|_|  |_____|A
                            
           The following cases are excluded:
	   * contain
	   * in
	   * equivalent
        """    
        
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute cover_2d for spatial extents"))
            return False
	    
	# Exclude equivalent_2d
        if self.equivalent_2d(extent):
	    return False
        
	eN = extent.get_north()
        eS = extent.get_south()
        eE = extent.get_east()
        eW = extent.get_west()
        
	N = self.get_north()
        S = self.get_south()
        E = self.get_east()
        W = self.get_west()
        
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while eE < W:
                eE += 360.0
                eW += 360.0

            while eW > E:
                eE -= 360.0
                eW -= 360.0	
                
	# Edges of extent located outside of self are not allowed 
        if E < eW:
	    return False
	if W > eE:
	    return False
	if N < eS:
	    return False
	if S > eN:
	    return False
         
        # First we check that at least one edge of extent meets an edge of self
        if W != eW and E != eE and N != eN and S != eS:
	    return False
	    
	# We check that at least one edge of extent is located in self
	edge_count = 0
	if W < eW and E > eW:
	    edge_count += 1
	if E > eE and W < eE:
	    edge_count += 1
	if N > eN and S < eN:
	    edge_count += 1
	if S < eS and N > eS:
	    edge_count += 1
	
	if edge_count == 0:
	    return False
	
	return True
	
    def cover(self, extent):
        """Return True if three dimensional self covers extent 
     
           The following cases are excluded:
	   * contain
	   * in
	   * equivalent
        """ 
        	    
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute cover for spatial extents"))
            return False
	    
	# Exclude equivalent_2d
        if self.equivalent_2d(extent):
	    return False
        
	eN = extent.get_north()
        eS = extent.get_south()
        eE = extent.get_east()
        eW = extent.get_west()
        
        eT = extent.get_top()
	eB = extent.get_bottom()
        
	N = self.get_north()
        S = self.get_south()
        E = self.get_east()
        W = self.get_west()
	
	T = self.get_top()
	B = self.get_bottom()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while eE < W:
                eE += 360.0
                eW += 360.0

            while eW > E:
                eE -= 360.0
                eW -= 360.0	
                
	# Edges of extent located outside of self are not allowed 
        if E <= eW:
	    return False
	if W >= eE:
	    return False
	if N <= eS:
	    return False
	if S >= eN:
	    return False
	if T <= eB:
	    return False
	if B >= eT:
	    return False
         
        # First we check that at least one edge of extent meets an edge of self
        if W != eW and E != eE and N != eN and S != eS and B != eB and T != eT:
	    return False
	    
	# We check that at least one edge of extent is located in self
	edge_count = 0
	if W < eW and E > eW:
	    edge_count += 1
	if E > eE and W < eE:
	    edge_count += 1
	if N > eN and S < eN:
	    edge_count += 1
	if S < eS and N > eS:
	    edge_count += 1
	if N > eN and S < eN:
	    edge_count += 1
	if S < eS and N > eS:
	    edge_count += 1
	if T > eT and B < eT:
	    edge_count += 1
	if B < eB and T > eB:
	    edge_count += 1
	
	if edge_count == 0:
	    return False
	
	return True
        
    def covered_2d(self, extent):
	"""Check two dimensional if self is covered by  extent """

	return extent.cover_2d(self)
	
    def covered(self, extent):
	"""Check three dimensional if self is covered by extent """
	
	return extent.cover(self)
        	
    def overlap_2d(self, extent):
        """Return True if the two dimensional extents overlap. Code is lend from wind_overlap.c in lib/gis
            _____
           |A  __|__
           |  |  | B|
           |__|__|  |
              |_____|
              
           The following cases are excluded:
	   * contain
	   * in
	   * cover
	   * covered
	   * equivalent
        """    
        
        if self.contain_2d(extent):
	    return False
	    
        if self.is_in_2d(extent):
	    return False
	    
        if self.cover_2d(extent):
	    return False
	    
        if self.covered_2d(extent):
	    return False
	    
        if self.equivalent_2d(extent):
	    return False
        
        N = extent.get_north()
        S = extent.get_south()
        E = extent.get_east()
        W = extent.get_west()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while E < self.get_west():
                E += 360.0
                W += 360.0

            while W > self.get_east():
                E -= 360.0
                W -= 360.0
                
        if(self.get_north() <= S):
            return False
        
        if(self.get_south() >= N):
            return False
        
        if self.get_east() <= W:
            return False
        
        if self.get_west() >= E:
            return False
        
        return True

    def overlap(self, extent):
        """Return True if the three dimensional extents overlap
        
           The following cases are excluded:
	   * contain
	   * in
	   * cover
	   * covered
	   * equivalent
        """   

        if self.is_in(extent):
	    return False

        if self.contain(extent):
	    return False

        if self.cover(extent):
	    return False

        if self.covered(extent):
	    return False

        if self.equivalent(extent):
	    return False
        
        N = extent.get_north()
        S = extent.get_south()
        E = extent.get_east()
        W = extent.get_west()
        T = extent.get_top()
        B = extent.get_bottom()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while E < self.get_west():
                E += 360.0
                W += 360.0

            while W > self.get_east():
                E -= 360.0
                W -= 360.0
                
        if(self.get_north() <= S):
            return False
        
        if(self.get_south() >= N):
            return False
            
        if self.get_east() <= W:
            return False
        
        if self.get_west() >= E:
            return False
        
        if self.get_top() <= B:
            return False
        
        if self.get_bottom() >= T:
            return False
        
        return True
        
    def meet_2d(self,extent):
	""" Check if self and extent meet each other in two dimensions
	  _____ _____    _____ _____
	 |  A  |  B  |  |  B  |  A  |
	 |_____|     |  |     |     |
	       |_____|  |_____|_____|
	       	       
	         ___
	        | A |
	        |   |
	        |___|    _____
	       |  B  |  |  B  |
	       |     |  |     |
	       |_____|  |_____|_
	                  |  A  |
	                  |     |
	                  |_____|
	                  
	"""
	
	eN = extent.get_north()
        eS = extent.get_south()
        eE = extent.get_east()
        eW = extent.get_west()
        
        eT = extent.get_top()
	eB = extent.get_bottom()
        
	N = self.get_north()
        S = self.get_south()
        E = self.get_east()
        W = self.get_west()
	        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while eE < W:
                eE += 360.0
                eW += 360.0

            while eW > E:
                eE -= 360.0
                eW -= 360.0	
                
        edge = None
        edge_count = 0
        
        if E == eW:
	    edge = "E"
	    edge_count += 1
        if W == eE:
	    edge = "W"
	    edge_count += 1
        if N == eS:
	    edge = "N"
	    edge_count += 1
        if S == eN:
	    edge = "S"
	    edge_count += 1
	
	# Meet a a single edge only
	if edge_count != 1:
	    return False
	
	# Check boundaries of the faces
	if edge == "E" or edge == "W":
	    if N < eS or S > eN:
		return False
		
	if edge == "N" or edge == "S":
	    if E < eW or W > eE:
		return False
	
	return True

    def meet(self,extent):
	""" Check if self and extent meet each other in three dimensions"""
	eN = extent.get_north()
        eS = extent.get_south()
        eE = extent.get_east()
        eW = extent.get_west()
        
        eT = extent.get_top()
	eB = extent.get_bottom()
        
	N = self.get_north()
        S = self.get_south()
        E = self.get_east()
        W = self.get_west()
	
	T = self.get_top()
	B = self.get_bottom()
        
        # Adjust the east and west in case of LL projection
        if self.get_projection() == "LL":
            while eE < W:
                eE += 360.0
                eW += 360.0

            while eW > E:
                eE -= 360.0
                eW -= 360.0	
                
        edge = None
        edge_count = 0
        
        if E == eW:
	    edge = "E"
	    edge_count += 1
        if W == eE:
	    edge = "W"
	    edge_count += 1
        if N == eS:
	    edge = "N"
	    edge_count += 1
        if S == eN:
	    edge = "S"
	    edge_count += 1
        if T == eB:
	    edge = "T"
	    edge_count += 1
        if B == eT:
	    edge = "B"
	    edge_count += 1	
	
	# Meet a single edge only
	if edge_count != 1:
	    return False
	
	# Check boundaries of the faces
	if edge == "E" or edge == "W":
	    if N < eS or S > eN:
		return False
	    if T < eB or B > eT:
		return False
		
	if edge == "N" or edge == "S":
	    if E < eW or W > eE:
		return False
	    if T < eB or B > eT:
		return False
		
	if edge == "T" or edge == "B":
	    if E < eW or W > eE:
		return False
	    if N < eS or S > eN:
		return False
	
	return True

    def disjoint_2d(self, extent):
        """Return True if the two dimensional extents are disjoint 
        """  
        
        if self.overlapping_2d(extent) or self.meet_2d(extent):
	    return False
	return True

    def disjoint(self, extent):
        """Return True if the three dimensional extents are disjoint 
        """  
        
        if self.overlapping(extent) or self.meet(extent):
	    return False
	return True
                
    def spatial_relation_2d(self, extent):
	"""Returns the two dimensional spatial relation between self and extent
	
	    Spatial relations are:
	    * disjoint
	    * meet
	    * overlap
	    * cover
	    * covered
	    * in
	    * contain
	    * equivalent
	"""
        
	if self.equivalent_2d(extent):
	    return "equivalent"
	if self.contain_2d(extent):
	    return "contain"
	if self.is_in_2d(extent):
	    return "in"
	if self.cover_2d(extent):
	    return "cover"
	if self.covered_2d(extent):
	    return "covered"
	if self.overlap_2d(extent):
	    return "overlap"
	if self.meet_2d(extent):
	    return "meet"
	if self.disjoint_2d(extent):
	    return "disjoint"
	    
        return "unknown"
        
    def spatial_relation(self, extent):
	"""Returns the three dimensional spatial relation between self and extent
	
	    Spatial relations are:
	    * disjoint
	    * meet
	    * overlap
	    * cover
	    * covered
	    * in
	    * contain
	    * equivalent
	"""
        
	if self.equivalent(extent):
	    return "equivalent"
	if self.contain(extent):
	    return "contain"
	if self.is_in(extent):
	    return "in"
	if self.cover(extent):
	    return "cover"
	if self.covered(extent):
	    return "covered"
	if self.overlap(extent):
	    return "overlap"
	if self.meet(extent):
	    return "meet"
	if self.disjoint(extent):
	    return "disjoint"
	    
        return "unknown"
        
    def set_spatial_extent(self, north, south, east, west, top, bottom):
        """Set the spatial extent"""

	self.set_north(north)
	self.set_south(south)
	self.set_east(east)
	self.set_west(west)
	self.set_top(top)
	self.set_bottom(bottom)        
        
    def set_projection(self, proj):
        """Set the projection of the spatial extent it should be XY or LL.
           As default the projection is XY
        """
        if proj == None or (proj != "XY" and proj != "LL"):
            self.D["proj"] = "XY"
        else:
            self.D["proj"] = proj

    def set_spatial_extent_2d(self, north, south, east, west):

	self.set_id(ident)
	self.set_north(north)
	self.set_south(south)
	self.set_east(east)
	self.set_west(west)
	self.set_top(0)
	self.set_bottom(0)

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_north(self, north):
	"""Set the northern edge of the map"""
	self.D["north"] = north

    def set_south(self, sourth):
	"""Set the southern edge of the map"""
	self.D["south"] = sourth

    def set_west(self, west):
	"""Set the western edge of the map"""
	self.D["west"] = west

    def set_east(self, east):
	"""Set the eastern edge of the map"""
	self.D["east"] = east

    def set_top(self, top):
	"""Set the top edge of the map"""
	self.D["top"] = top

    def set_bottom(self, bottom):
	"""Set the bottom edge of the map"""
	self.D["bottom"] = bottom

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_projection(self):
        """Get the projection of the spatial extent"""
        return self.D["proj"]
    
    def get_volume(self):
        """Compute the volume of the extent, in case z is zero (top == bottom or top - bottom = 1) the area is returned"""

        if self.get_projection() == "LL":
            core.error(_("Volume computation is not supported for LL projections"))
        
        area = self.get_area()
        
        bbox = self.get_spatial_extent()
        
        z = abs(bbox[4] - bbox[5])
        
        if z == 0:
            z = 1.0
            
        return area*z
       
    def get_area(self):
        """Compute the area of the extent, extent in z direction is ignored"""
        
        if self.get_projection() == "LL":
            core.error(_("Area computation is not supported for LL projections"))
        
        bbox = self.get_spatial_extent()
        
        y = abs(bbox[0] - bbox[1])
        x = abs(bbox[2] - bbox[3])
                    
        return x*y
    
    def get_spatial_extent(self):
        """Return a tuple (north, south, east, west, top, bottom) of the spatial extent"""
        
        return (self.get_north(), self.get_south, self.get_east(), self.get_west(), \
                self.get_top(), self.get_bottom())
                
    def get_spatial_extent_2d(self):
        """Return a tuple (north, south, east, west,) of the 2d spatial extent"""
        return (self.get_north(), self.get_south, self.get_east(), self.get_west())
    
    def get_north(self):
	"""Get the northern edge of the map
	   @return None if not found"""
	if self.D.has_key("north"):
	    return self.D["north"]
        else:
	    return None

    def get_south(self):
	"""Get the southern edge of the map
	   @return None if not found"""
	if self.D.has_key("south"):
	    return self.D["south"]
        else:
	    return None

    def get_east(self):
	"""Get the eastern edge of the map
	   @return None if not found"""
	if self.D.has_key("east"):
	    return self.D["east"]
        else:
	    return None

    def get_west(self):
	"""Get the western edge of the map
	   @return None if not found"""
	if self.D.has_key("west"):
	    return self.D["west"]
        else:
	    return None

    def get_top(self):
	"""Get the top edge of the map
	   @return None if not found"""
	if self.D.has_key("top"):
	    return self.D["top"]
        else:
	    return None

    def get_bottom(self):
	"""Get the bottom edge of the map
	   @return None if not found"""
	if self.D.has_key("bottom"):
	    return self.D["bottom"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " +-------------------- Spatial extent ----------------------------------------+"
        print " | North:...................... " + str(self.get_north())
        print " | South:...................... " + str(self.get_south())
        print " | East:.. .................... " + str(self.get_east())
        print " | West:....................... " + str(self.get_west())
        print " | Top:........................ " + str(self.get_top())
        print " | Bottom:..................... " + str(self.get_bottom())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "north=" + str(self.get_north())
        print "south=" + str(self.get_south())
        print "east=" + str(self.get_east())
        print "west=" + str(self.get_west())
        print "top=" + str(self.get_top())
        print "bottom=" + str(self.get_bottom())


###############################################################################

class raster_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "raster_spatial_extent", ident, north, south, east, west, top, bottom)

class raster3d_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "raster3d_spatial_extent", ident, north, south, east, west, top, bottom)

class vector_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "vector_spatial_extent", ident, north, south, east, west, top, bottom)

class strds_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "strds_spatial_extent", ident, north, south, east, west, top, bottom)

class str3ds_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "str3ds_spatial_extent", ident, north, south, east, west, top, bottom)

class stvds_spatial_extent(spatial_extent):
    def __init__(self, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None):
        spatial_extent.__init__(self, "stvds_spatial_extent", ident, north, south, east, west, top, bottom)
