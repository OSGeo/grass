"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related spatial extent functions to be used in Python scripts and tgis packages.

Usage:

@code

>>> import grass.temporal as tgis
>>> tgis.init()
>>> extent = tgis.RasterSpatialExtent( 
... ident="raster@PERMANENT", north=90, south=90, east=180, west=180,
... top=100, bottom=-20)
>>> extent = tgis.Raster3DSpatialExtent( 
... ident="raster3d@PERMANENT", north=90, south=90, east=180, west=180,
... top=100, bottom=-20)
>>> extent = tgis.VectorSpatialExtent( 
... ident="vector@PERMANENT", north=90, south=90, east=180, west=180,
... top=100, bottom=-20)
>>> extent = tgis.STRDSSpatialExtent( 
... ident="strds@PERMANENT", north=90, south=90, east=180, west=180,
... top=100, bottom=-20)
>>> extent = tgis.STR3DSSpatialExtent( 
... ident="str3ds@PERMANENT", north=90, south=90, east=180, west=180,
... top=100, bottom=-20)
>>> extent = tgis.STVDSSpatialExtent( 
... ident="stvds@PERMANENT", north=90, south=90, east=180, west=180,
... top=100, bottom=-20)

@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from base import *


class SpatialExtent(SQLDatabaseInterface):
    """!This is the spatial extent base class for all maps and space time datasets
        
        This class implements a three dimensional axis aligned bounding box
        and functions to compute topological relationships
        
        Usage:
        
        @code
        
        >>> init()
        >>> extent = SpatialExtent(table="raster_spatial_extent", 
        ... ident="soil@PERMANENT", north=90, south=90, east=180, west=180,
        ... top=100, bottom=-20)
        >>> extent.id
        'soil@PERMANENT'
        >>> extent.north
        90.0
        >>> extent.south
        90.0
        >>> extent.east
        180.0
        >>> extent.west
        180.0
        >>> extent.top
        100.0
        >>> extent.bottom
        -20.0
        >>> extent.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 90.0
         | South:...................... 90.0
         | East:.. .................... 180.0
         | West:....................... 180.0
         | Top:........................ 100.0
         | Bottom:..................... -20.0
        >>> extent.print_shell_info()
        north=90.0
        south=90.0
        east=180.0
        west=180.0
        top=100.0
        bottom=-20.0
        
        @endcode
    """
    def __init__(self, table=None, ident=None, north=None, south=None, 
                 east=None, west=None, top=None, bottom=None, proj="XY"):

        SQLDatabaseInterface.__init__(self, table, ident)
        self.set_id(ident)
        self.set_spatial_extent(north, south, east, west, top, bottom)
        self.set_projection(proj)

    def overlapping_2d(self, extent):
        """!Return True if this (A) and the provided spatial extent (B) overlaps
        in two dimensional space. 
        Code is lend from wind_overlap.c in lib/gis
        
        Overlapping includes the spatial relations:
        
        - contain
        - in
        - cover
        - covered
        - equivalent
        
         @code
         
         >>> A = SpatialExtent(north=80, south=20, east=60, west=10)
         >>> B = SpatialExtent(north=80, south=20, east=60, west=10)
         >>> A.overlapping_2d(B)
         True
         
         @endcode
        
        @param extent The spatial extent to check overlapping with
        @return True or False
        """

        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute "
                         "overlapping_2d for spatial extents"))
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
        """!Return True if this (A) and the provided spatial 
        extent (B) overlaps in three dimensional space.
        
        Overlapping includes the spatial relations:
        
        - contain
        - in
        - cover
        - covered
        - equivalent
            
         Usage:
         
         @code
         
         >>> A = SpatialExtent(north=80, south=20, east=60, west=10, bottom=-50, top=50)
         >>> B = SpatialExtent(north=80, south=20, east=60, west=10, bottom=-50, top=50)
         >>> A.overlapping(B)
         True
         
         @endcode
         
         @param extent The spatial extent to check overlapping with
         @return True or False
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
        """!Return the two dimensional intersection as spatial_extent 
           object or None in case no intersection was found.
       
        @param extent The spatial extent to intersect with
        @return The intersection spatial extent
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

        new = SpatialExtent(north=nN, south=nS, east=nE, west=nW,
                             top=0, bottom=0, proj=self.get_projection())
        return new

    def intersect(self, extent):
        """!Return the three dimensional intersection as spatial_extent 
        object or None in case no intersection was found.
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> B = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> C = A.intersect(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 20.0
         | East:.. .................... 60.0
         | West:....................... 10.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> C = A.intersect(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 40.0
         | South:...................... 30.0
         | East:.. .................... 60.0
         | West:....................... 10.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=30, 
        ... bottom=-50, top=50)
        >>> C = A.intersect(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 40.0
         | South:...................... 30.0
         | East:.. .................... 60.0
         | West:....................... 30.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=30, 
        ... bottom=-30, top=50)
        >>> C = A.intersect(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 40.0
         | South:...................... 30.0
         | East:.. .................... 60.0
         | West:....................... 30.0
         | Top:........................ 50.0
         | Bottom:..................... -30.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=30, 
        ... bottom=-30, top=30)
        >>> C = A.intersect(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 40.0
         | South:...................... 30.0
         | East:.. .................... 60.0
         | West:....................... 30.0
         | Top:........................ 30.0
         | Bottom:..................... -30.0
         
         @endcode
         
         
         @param extent The spatial extent to intersect with
         @return The intersection spatial extent
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
        
    def union_2d(self, extent):
        """!Return the two dimensional union as spatial_extent 
           object or None in case the extents does not overlap or meet.
       
        @param extent The spatial extent to create a union with
        @return The union spatial extent
        """
        if not self.overlapping_2d(extent) and not self.meet_2d(extent):
            return None
        
        return self.disjoint_union_2d(extent)
    
    def disjoint_union_2d(self, extent):
        """!Return the two dimensional union as spatial_extent.
       
        @param extent The spatial extent to create a union with
        @return The union spatial extent
        """
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

        if W > eW:
            nW = eW
        if E < eE:
            nE = eE
        if N < eN:
            nN = eN
        if S > eS:
            nS = eS

        new = SpatialExtent(north=nN, south=nS, east=nE, west=nW,
                             top=0, bottom=0, proj=self.get_projection())
        return new

    def union(self, extent):
        """!Return the three dimensional union as spatial_extent 
           object or None in case the extents does not overlap or meet.
       
        @param extent The spatial extent to create a union with
        @return The union spatial extent
        """
        if not self.overlapping(extent) and not self.meet(extent):
            return None
        
        return self.disjoint_union(extent)
    
    def disjoint_union(self, extent):
        """!Return the three dimensional union as spatial_extent .
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> B = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> C = A.disjoint_union(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 20.0
         | East:.. .................... 60.0
         | West:....................... 10.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> C = A.disjoint_union(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 20.0
         | East:.. .................... 60.0
         | West:....................... 10.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=30, 
        ... bottom=-50, top=50)
        >>> C = A.disjoint_union(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 20.0
         | East:.. .................... 60.0
         | West:....................... 10.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=30, 
        ... bottom=-30, top=50)
        >>> C = A.disjoint_union(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 20.0
         | East:.. .................... 60.0
         | West:....................... 10.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> B = SpatialExtent(north=40, south=30, east=60, west=30, 
        ... bottom=-30, top=30)
        >>> C = A.disjoint_union(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 80.0
         | South:...................... 20.0
         | East:.. .................... 60.0
         | West:....................... 10.0
         | Top:........................ 50.0
         | Bottom:..................... -50.0
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> B = SpatialExtent(north=90, south=80, east=70, west=20, 
        ... bottom=-30, top=60)
        >>> C = A.disjoint_union(B)
        >>> C.print_info()
         +-------------------- Spatial extent ----------------------------------------+
         | North:...................... 90.0
         | South:...................... 20.0
         | East:.. .................... 70.0
         | West:....................... 10.0
         | Top:........................ 60.0
         | Bottom:..................... -50.0
         
         @endcode
         
         @param extent The spatial extent to create a disjoint union with
         @return The union spatial extent
        """

        new = self.disjoint_union_2d(extent)

        eT = extent.get_top()
        eB = extent.get_bottom()

        T = self.get_top()
        B = self.get_bottom()

        nT = T
        nB = B

        if B > eB:
            nB = eB
        if T < eT:
            nT = eT

        new.set_top(nT)
        new.set_bottom(nB)

        return new
    
    def is_in_2d(self, extent):
        """!Return True if this extent (A) is located in the provided spatial
        extent (B) in two dimensions.
        
        @verbatim
         _____
        |A _  |
        | |_| |
        |_____|B 
        
        @endverbatim
        
        @param extent The spatial extent
        @return True or False
        """
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute "
                         "is_in_2d for spatial extents"))
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
        """!Return True if this extent (A) is located in the provided spatial
        extent (B) in three dimensions.
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=79, south=21, east=59, west=11, 
        ... bottom=-49, top=49)
        >>> B = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> A.is_in(B)
        True
        >>> B.is_in(A)
        False
        
        @endcode
        
        @param extent The spatial extent
        @return True or False
        """
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
        """!Return True if this extent (A) contains the provided spatial
        extent (B) in two dimensions.
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10)
        >>> B = SpatialExtent(north=79, south=21, east=59, west=11)
        >>> A.contain_2d(B)
        True
        >>> B.contain_2d(A)
        False
        
        @endcode
        
        @param extent The spatial extent
        @return True or False
        """
        return extent.is_in_2d(self)

    def contain(self, extent):        
        """!Return True if this extent (A) contains the provided spatial
        extent (B) in three dimensions.
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> B = SpatialExtent(north=79, south=21, east=59, west=11, 
        ... bottom=-49, top=49)
        >>> A.contain(B)
        True
        >>> B.contain(A)
        False
        
        @endcode
        
        @param extent The spatial extent
        @return True or False
        """
        return extent.is_in(self)

    def equivalent_2d(self, extent):
        """!Return True if this extent (A) is equal to the provided spatial
        extent (B) in two dimensions.
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10)
        >>> B = SpatialExtent(north=80, south=20, east=60, west=10)
        >>> A.equivalent_2d(B)
        True
        >>> B.equivalent_2d(A)
        True
        
        @endcode
        
        @param extent The spatial extent
        @return True or False
        """
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute "
                         "equivalent_2d for spatial extents"))
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
        """!Return True if this extent (A) is equal to the provided spatial
        extent (B) in three dimensions.
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> B = SpatialExtent(north=80, south=20, east=60, west=10, 
        ... bottom=-50, top=50)
        >>> A.equivalent(B)
        True
        >>> B.equivalent(A)
        True
        
        @endcode
        
        @param extent The spatial extent
        @return True or False
        """

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
        """!Return True if this extent (A) covers the provided spatial
        extent (B) in two dimensions.
           
        @verbatim
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
        
        @endverbatim
        
        The following cases are excluded:
        
        - contain
        - in
        - equivalent

        @param extent The spatial extent
        @return True or False
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
        if E <= eW:
            return False
        if W >= eE:
            return False
        if N <= eS:
            return False
        if S >= eN:
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
        """!Return True if this extent covers the provided spatial
        extent in three dimensions.
        
        The following cases are excluded:
        
        - contain
        - in
        - equivalent

        @param extent The spatial extent
        @return True or False
        """
        if self.get_projection() != extent.get_projection():
            core.error(_("Projections are different. Unable to compute "
                         "cover for spatial extents"))
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
        """!Return True if this extent is covered by the provided spatial
        extent in two dimensions.
        
        The following cases are excluded:
        
        - contain
        - in
        - equivalent

        @param extent The spatial extent
        @return True or False
        """

        return extent.cover_2d(self)

    def covered(self, extent):
        """!Return True if this extent is covered by the provided spatial
        extent in three dimensions.
        
        The following cases are excluded:
        
        - contain
        - in
        - equivalent

        @param extent The spatial extent
        @return True or False
        """

        return extent.cover(self)

    def overlap_2d(self, extent):
        """!Return True if this extent (A) overlaps with the provided spatial
        extent (B) in two dimensions.
        Code is lend from wind_overlap.c in lib/gis
        
        @verbatim
         _____
        |A  __|__
        |  |  | B|
        |__|__|  |
           |_____|
           
        @endverbatim
        
        The following cases are excluded:
        
        - contain
        - in
        - cover
        - covered
        - equivalent
           
        @param extent The spatial extent
        @return True or False
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
        """!Return True if this extent overlaps with the provided spatial
        extent in three dimensions.

        The following cases are excluded:
        
        - contain
        - in
        - cover
        - covered
        - equivalent
           
        @param extent The spatial extent
        @return True or False
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

    def meet_2d(self, extent):
        """!Return True if this extent (A) meets with the provided spatial
        extent (B) in two dimensions.
        
        @verbatim
          _____ _____ 
         |  A  |  B  |
         |_____|     |
               |_____|
          _____ _____
         |  B  |  A  |
         |     |     |
         |_____|_____|
           ___
          | A |
          |   |
          |___| 
         |  B  |
         |     |  
         |_____|  
          _____
         |  B  |
         |     |
         |_____|_
           |  A  |
           |     |
           |_____|
             
         @endverbatim
           
        @param extent The spatial extent
        @return True or False
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

    def meet(self, extent):
        """!Return True if this extent meets with the provided spatial
        extent in three dimensions.
           
        @param extent The spatial extent
        @return True or False
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
        """!Return True if this extent (A) is disjoint with the provided spatial
        extent (B) in three dimensions.

        @verbatim
          _____
         |  A  |
         |_____|
         _______
        |   B   |
        |_______|

         @endverbatim

        @param extent The spatial extent
        @return True or False
        """

        if self.is_in_2d(extent):
            return False

        if self.contain_2d(extent):
            return False

        if self.cover_2d(extent):
            return False

        if self.covered_2d(extent):
            return False

        if self.equivalent_2d(extent):
            return False

        if self.overlapping_2d(extent):
            return False

        if  self.meet_2d(extent):
            return False

        return True

    def disjoint(self, extent):
        """!Return True if this extent is disjoint with the provided spatial
        extent in three dimensions.
           
        @param extent The spatial extent
        @return True or False
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

        if self.overlapping(extent):
            return False

        if  self.meet(extent):
            return False
            
        return True

    def spatial_relation_2d(self, extent):
        """!Returns the two dimensional spatial relation between this
        extent and the provided spatial extent in two dimensions.

        Spatial relations are:
        
        - disjoint
        - meet
        - overlap
        - cover
        - covered
        - in
        - contain
        - equivalent
        
        Usage: see self.spatial_relation()
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
        """!Returns the two dimensional spatial relation between this
        extent and the provided spatial extent in three dimensions.

        Spatial relations are:
        
        - disjoint
        - meet
        - overlap
        - cover
        - covered
        - in
        - contain
        - equivalent
            
        
        Usage:
        
        @code
        
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, bottom=-50, top=50)
        >>> B = SpatialExtent(north=80, south=20, east=60, west=10, bottom=-50, top=50)
        >>> A.spatial_relation(B)
        'equivalent'
        >>> B.spatial_relation(A)
        'equivalent'
        >>> B = SpatialExtent(north=70, south=20, east=60, west=10, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'cover'
        >>> A.spatial_relation(B)
        'cover'
        >>> B = SpatialExtent(north=70, south=30, east=60, west=10, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'cover'
        >>> A.spatial_relation(B)
        'cover'
        >>> B.spatial_relation_2d(A)
        'covered'
        >>> B.spatial_relation(A)
        'covered'
        >>> B = SpatialExtent(north=70, south=30, east=50, west=10, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'cover'
        >>> B.spatial_relation_2d(A)
        'covered'
        >>> A.spatial_relation(B)
        'cover'
        >>> B = SpatialExtent(north=70, south=30, east=50, west=20, bottom=-50, top=50)
        >>> B.spatial_relation(A)
        'covered'
        >>> B = SpatialExtent(north=70, south=30, east=50, west=20, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'contain'
        >>> A.spatial_relation(B)
        'cover'
        >>> B = SpatialExtent(north=70, south=30, east=50, west=20, bottom=-40, top=50)
        >>> A.spatial_relation(B)
        'cover'
        >>> B = SpatialExtent(north=70, south=30, east=50, west=20, bottom=-40, top=40)
        >>> A.spatial_relation(B)
        'contain'
        >>> B.spatial_relation(A)
        'in'
        >>> B = SpatialExtent(north=90, south=30, east=50, west=20, bottom=-40, top=40)
        >>> A.spatial_relation_2d(B)
        'overlap'
        >>> A.spatial_relation(B)
        'overlap'
        >>> B = SpatialExtent(north=90, south=5, east=70, west=5, bottom=-40, top=40)
        >>> A.spatial_relation_2d(B)
        'in'
        >>> A.spatial_relation(B)
        'overlap'
        >>> B = SpatialExtent(north=90, south=5, east=70, west=5, bottom=-40, top=60)
        >>> A.spatial_relation(B)
        'overlap'
        >>> B = SpatialExtent(north=90, south=5, east=70, west=5, bottom=-60, top=60)
        >>> A.spatial_relation(B)
        'in'
        >>> A = SpatialExtent(north=80, south=60, east=60, west=10, bottom=-50, top=50)
        >>> B = SpatialExtent(north=60, south=20, east=60, west=10, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'meet'
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=60, south=40, east=60, west=10, bottom=-50, top=50)
        >>> B = SpatialExtent(north=80, south=60, east=60, west=10, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'meet'
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=40, bottom=-50, top=50)
        >>> B = SpatialExtent(north=80, south=40, east=40, west=20, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'meet'
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=40, west=20, bottom=-50, top=50)
        >>> B = SpatialExtent(north=90, south=30, east=60, west=40, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'meet'
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=40, west=20, bottom=-50, top=50)
        >>> B = SpatialExtent(north=70, south=50, east=60, west=40, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'meet'
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=40, west=20, bottom=-50, top=50)
        >>> B = SpatialExtent(north=60, south=20, east=60, west=40, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'meet'
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=40, west=20, bottom=-50, top=50)
        >>> B = SpatialExtent(north=40, south=20, east=60, west=40, bottom=-50, top=50)
        >>> A.spatial_relation_2d(B)
        'disjoint'
        >>> A.spatial_relation(B)
        'disjoint'
        >>> A = SpatialExtent(north=80, south=40, east=40, west=20, bottom=-50, top=50)
        >>> B = SpatialExtent(north=60, south=20, east=60, west=40, bottom=-60, top=60)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=40, west=20, bottom=-50, top=50)
        >>> B = SpatialExtent(north=90, south=30, east=60, west=40, bottom=-40, top=40)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=0, top=50)
        >>> B = SpatialExtent(north=80, south=40, east=60, west=20, bottom=-50, top=0)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=0, top=50)
        >>> B = SpatialExtent(north=80, south=50, east=60, west=30, bottom=-50, top=0)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=0, top=50)
        >>> B = SpatialExtent(north=70, south=50, east=50, west=30, bottom=-50, top=0)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=0, top=50)
        >>> B = SpatialExtent(north=90, south=30, east=70, west=10, bottom=-50, top=0)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=0, top=50)
        >>> B = SpatialExtent(north=70, south=30, east=50, west=10, bottom=-50, top=0)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=-50, top=0)
        >>> B = SpatialExtent(north=80, south=40, east=60, west=20, bottom=0, top=50)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=-50, top=0)
        >>> B = SpatialExtent(north=80, south=50, east=60, west=30, bottom=0, top=50)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=-50, top=0)
        >>> B = SpatialExtent(north=70, south=50, east=50, west=30, bottom=0, top=50)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=-50, top=0)
        >>> B = SpatialExtent(north=90, south=30, east=70, west=10, bottom=0, top=50)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=40, east=60, west=20, bottom=-50, top=0)
        >>> B = SpatialExtent(north=70, south=30, east=50, west=10, bottom=0, top=50)
        >>> A.spatial_relation(B)
        'meet'
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, bottom=-50, top=50)
        >>> B = SpatialExtent(north=90, south=81, east=60, west=10, bottom=-50, top=50)
        >>> A.spatial_relation(B)
        'disjoint'
        >>> A = SpatialExtent(north=80, south=20, east=60, west=10, bottom=-50, top=50)
        >>> B = SpatialExtent(north=90, south=80, east=60, west=10, bottom=-50, top=50)
        >>> A.spatial_relation(B)
        'meet'
        
        @endcode
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
        """!Set the three dimensional spatial extent"""

        self.set_north(north)
        self.set_south(south)
        self.set_east(east)
        self.set_west(west)
        self.set_top(top)
        self.set_bottom(bottom)

    def set_projection(self, proj):
        """!Set the projection of the spatial extent it should be XY or LL.
           As default the projection is XY
        """
        if proj is None or (proj != "XY" and proj != "LL"):
            self.D["proj"] = "XY"
        else:
            self.D["proj"] = proj

    def set_spatial_extent_2d(self, north, south, east, west):
        """!Set the two dimensional spatial extent"""

        self.set_north(north)
        self.set_south(south)
        self.set_east(east)
        self.set_west(west)

    def set_id(self, ident):
        """!Convenient method to set the unique identifier (primary key)"""
        self.ident = ident
        self.D["id"] = ident

    def set_north(self, north):
        """!Set the northern edge of the map"""
        if north is not None:
            self.D["north"] = float(north)
        else:
            self.D["north"] = None

    def set_south(self, south):
        """!Set the southern edge of the map"""
        if south is not None:
            self.D["south"] = float(south)
        else:
            self.D["south"] = None

    def set_west(self, west):
        """!Set the western edge of the map"""
        if west is not None:
            self.D["west"] = float(west)
        else:
            self.D["west"] = None

    def set_east(self, east):
        """!Set the eastern edge of the map"""
        if east is not None:
            self.D["east"] = float(east)
        else:
            self.D["east"] = None

    def set_top(self, top):
        """!Set the top edge of the map"""
        if top is not None:
            self.D["top"] = float(top)
        else:
            self.D["top"] = None

    def set_bottom(self, bottom):
        """!Set the bottom edge of the map"""
        if bottom is not None:
            self.D["bottom"] = float(bottom)
        else:
            self.D["bottom"] = None

    def get_id(self):
        """!Convenient method to get the unique identifier (primary key)
           @return None if not found
        """
        if "id" in self.D:
            return self.D["id"]
        else:
            return None

    def get_projection(self):
        """!Get the projection of the spatial extent"""
        return self.D["proj"]

    def get_volume(self):
        """!Compute the volume of the extent, in case z is zero 
           (top == bottom or top - bottom = 1) the area is returned"""

        if self.get_projection() == "LL":
            core.error(_("Volume computation is not supported "
                         "for LL projections"))

        area = self.get_area()

        bbox = self.get_spatial_extent_as_tuple()

        z = abs(bbox[4] - bbox[5])

        if z == 0:
            z = 1.0

        return area * z

    def get_area(self):
        """!Compute the area of the extent, extent in z direction is ignored"""

        if self.get_projection() == "LL":
            core.error(_("Area computation is not supported "
                         "for LL projections"))

        bbox = self.get_spatial_extent_as_tuple()

        y = abs(bbox[0] - bbox[1])
        x = abs(bbox[2] - bbox[3])

        return x * y

    def get_spatial_extent_as_tuple(self):
        """!Return a tuple (north, south, east, west, top, bottom) 
           of the spatial extent"""

        return (
            self.north, self.south, self.east, self.west,
            self.top, self.bottom)

    def get_spatial_extent_as_tuple_2d(self):
        """!Return a tuple (north, south, east, west,) of the 2d spatial extent
        """
        return (self.north, self.south, self.east, self.west)

    def get_north(self):
        """!Get the northern edge of the map
           @return None if not found"""
        if "north" in self.D:
            return self.D["north"]
        else:
            return None

    def get_south(self):
        """!Get the southern edge of the map
           @return None if not found"""
        if "south" in self.D:
            return self.D["south"]
        else:
            return None

    def get_east(self):
        """!Get the eastern edge of the map
           @return None if not found"""
        if "east" in self.D:
            return self.D["east"]
        else:
            return None

    def get_west(self):
        """!Get the western edge of the map
           @return None if not found"""
        if "west" in self.D:
            return self.D["west"]
        else:
            return None

    def get_top(self):
        """!Get the top edge of the map
           @return None if not found"""
        if "top" in self.D:
            return self.D["top"]
        else:
            return None

    def get_bottom(self):
        """!Get the bottom edge of the map
           @return None if not found"""
        if "bottom" in self.D:
            return self.D["bottom"]
        else:
            return None
    
    id = property(fget=get_id, fset=set_id)
    north = property(fget=get_north, fset=set_north)
    south = property(fget=get_south, fset=set_south)
    east = property(fget=get_east, fset=set_east)
    west = property(fget=get_west, fset=set_west)
    top = property(fget=get_top, fset=set_top)
    bottom= property(fget=get_bottom, fset=set_bottom)

    def print_info(self):
        """!Print information about this class in human readable style"""
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

class RasterSpatialExtent(SpatialExtent):
    def __init__(self, ident=None, north=None, south=None, east=None, 
                 west=None, top=None, bottom=None):
        SpatialExtent.__init__(self, "raster_spatial_extent",
                                ident, north, south, east, west, top, bottom)

class Raster3DSpatialExtent(SpatialExtent):
    def __init__(self, ident=None, north=None, south=None, east=None, 
                 west=None, top=None, bottom=None):
        SpatialExtent.__init__(self, "raster3d_spatial_extent",
                                ident, north, south, east, west, top, bottom)

class VectorSpatialExtent(SpatialExtent):
    def __init__(self, ident=None, north=None, south=None, east=None, 
                 west=None, top=None, bottom=None):
        SpatialExtent.__init__(self, "vector_spatial_extent",
                                ident, north, south, east, west, top, bottom)

class STRDSSpatialExtent(SpatialExtent):
    def __init__(self, ident=None, north=None, south=None, east=None, 
                 west=None, top=None, bottom=None):
        SpatialExtent.__init__(self, "strds_spatial_extent",
                                ident, north, south, east, west, top, bottom)

class STR3DSSpatialExtent(SpatialExtent):
    def __init__(self, ident=None, north=None, south=None, east=None, 
                 west=None, top=None, bottom=None):
        SpatialExtent.__init__(self, "str3ds_spatial_extent",
                                ident, north, south, east, west, top, bottom)

class STVDSSpatialExtent(SpatialExtent):
    def __init__(self, ident=None, north=None, south=None, east=None, 
                 west=None, top=None, bottom=None):
        SpatialExtent.__init__(self, "stvds_spatial_extent",
                                ident, north, south, east, west, top, bottom)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
