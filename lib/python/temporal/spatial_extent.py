"""!@package grass.script.tgis_spatial_extent

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related spatial extent functions to be used in Python scripts and tgis packages.

Usage:

@code
from grass.script import tgis_spatial_extent as grass

extent = grass.raster_spatial_extent()
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
    """This is the spatial extent base class for all maps and spacetime datasets"""
    def __init__(self, table=None, ident=None, north=None, south=None, east=None, west=None, top=None, bottom=None, proj="XY"):

	sql_database_interface.__init__(self, table, ident)
        self.set_id(ident)
        self.set_spatial_extent(north, south, east, west, top, bottom)
	self.set_projection(proj)
        
    def overlap_2d(self, extent):
        """Return True if the 2d extents overlap. Code is lend from wind_overlap.c in lib/gis"""  
        
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute overlap_2d for spatial extents"))
        
        N = extent.get_north()
        S = extent.get_south()
        E = extent.get_east()
        W = extent.get_west()
        
        if(self.get_north() <= S):
            return False
        
        if(self.get_south() >= N):
            return False
        
        # Adjust the east and west in case of LL projection
        if self.get_proj() == "LL":
            while E < self.get_west():
                E += 360.0
                W += 360.0

            while W > self.get_east():
                E -= 360.0
                W -= 360.0
            
        if self.get_east() <= W:
            return False
        
        if self.get_west() >= E:
            return False
        
        return True

    def overlap(self, extent):
        """Return True if the extents overlap."""  
        
        if self.overlap_2d(extent) == False:
            return False
            
        T = extent.get_top()
        B = extent.get_bottom()
        
        if self.get_top() <= B:
            return False
        
        if self.get_bottom() >= T:
            return False
        
        return True

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
	"""Set the sourthern edge of the map"""
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
