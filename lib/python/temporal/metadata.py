"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related metadata functions to be used in Python scripts and tgis packages.

Usage:

@code

>>> import grass.temporal as tgis
>>> tgis.init()
>>> meta = tgis.RasterMetadata()
>>> meta = tgis.Raster3DMetadata()
>>> meta = tgis.VectorMetadata()
>>> meta = tgis.STRDSMetadata()
>>> meta = tgis.STR3DSMetadata()
>>> meta = tgis.STVDSMetadata()

@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from base import *

###############################################################################


class RasterMetadataBase(SQLDatabaseInterface):
    """!This is the metadata base class for time stamped raster and raster3d maps
    
        Usage:
        
        @code
        
        >>> init()
        >>> meta = RasterMetadataBase(table="metadata", ident="soil@PERMANENT",
        ... datatype="CELL", cols=100, rows=100, number_of_cells=10000, nsres=0.1,
        ... ewres=0.1, min=0, max=100)
        >>> meta.datatype
        'CELL'
        >>> meta.cols
        100
        >>> meta.rows
        100
        >>> meta.number_of_cells
        10000
        >>> meta.nsres
        0.1
        >>> meta.ewres
        0.1
        >>> meta.min
        0.0
        >>> meta.max
        100.0
        >>> meta.print_info()
         | Datatype:................... CELL
         | Number of columns:.......... 100
         | Number of rows:............. 100
         | Number of cells:............ 10000
         | North-South resolution:..... 0.1
         | East-west resolution:....... 0.1
         | Minimum value:.............. 0.0
         | Maximum value:.............. 100.0
        >>> meta.print_shell_info()
        datatype=CELL
        cols=100
        rows=100
        number_of_cells=10000
        nsres=0.1
        ewres=0.1
        min=0.0
        max=100.0
        
        @endcode
    """
    def __init__(self, table=None, ident=None, datatype=None, cols=None, 
		rows=None, number_of_cells=None, nsres=None, ewres=None, 
		min=None, max=None):

        SQLDatabaseInterface.__init__(self, table, ident)

        self.set_id(ident)
        self.set_datatype(datatype)
        self.set_cols(cols)
        self.set_rows(rows)
        self.set_number_of_cells(number_of_cells)
        self.set_nsres(nsres)
        self.set_ewres(ewres)
        self.set_min(min)
        self.set_max(max)

    def set_id(self, ident):
        """!Convenient method to set the unique identifier (primary key)"""
        self.ident = ident
        self.D["id"] = ident

    def set_datatype(self, datatype):
        """!Set the datatype"""
        self.D["datatype"] = datatype

    def set_cols(self, cols):
        """!Set the number of cols"""
        if cols is not None:
            self.D["cols"] = int(cols)
        else:
            self.D["cols"] = None

    def set_rows(self, rows):
        """!Set the number of rows"""
        if rows is not None:
            self.D["rows"] = int(rows)
        else:
            self.D["rows"] = None

    def set_number_of_cells(self, number_of_cells):
        """!Set the number of cells"""
        if number_of_cells is not None:
            self.D["number_of_cells"] = int(number_of_cells)
        else:
            self.D["number_of_cells"] = None

    def set_nsres(self, nsres):
        """!Set the north-south resolution"""
        if nsres is not None:
            self.D["nsres"] = float(nsres)
        else:
            self.D["nsres"] = None

    def set_ewres(self, ewres):
        """!Set the east-west resolution"""
        if ewres is not None:
            self.D["ewres"] = float(ewres)
        else:
            self.D["ewres"] = None

    def set_min(self, min):
        """!Set the minimum raster value"""
        if min is not None:
            self.D["min"] = float(min)
        else:
            self.D["min"] = None

    def set_max(self, max):
        """!Set the maximum raster value"""
        if max is not None:
            self.D["max"] = float(max)
        else:
            self.D["max"] = None

    def get_id(self):
        """!Convenient method to get the unique identifier (primary key)
           @return None if not found
        """
        if "id" in self.D:
            return self.D["id"]
        else:
            return None

    def get_datatype(self):
        """!Get the map type
           @return None if not found"""
        if "datatype" in self.D:
            return self.D["datatype"]
        else:
            return None

    def get_cols(self):
        """!Get number of cols
           @return None if not found"""
        if "cols" in self.D:
            return self.D["cols"]
        else:
            return None

    def get_rows(self):
        """!Get number of rows
           @return None if not found"""
        if "rows" in self.D:
            return self.D["rows"]
        else:
            return None

    def get_number_of_cells(self):
        """!Get number of cells
           @return None if not found"""
        if "number_of_cells" in self.D:
            return self.D["number_of_cells"]
        else:
            return None

    def get_nsres(self):
        """!Get the north-south resolution
           @return None if not found"""
        if "nsres" in self.D:
            return self.D["nsres"]
        else:
            return None

    def get_ewres(self):
        """!Get east-west resolution
           @return None if not found"""
        if "ewres" in self.D:
            return self.D["ewres"]
        else:
            return None

    def get_min(self):
        """!Get the minimum cell value
           @return None if not found"""
        if "min" in self.D:
            return self.D["min"]
        else:
            return None

    def get_max(self):
        """!Get the maximum cell value
           @return None if not found"""
        if "max" in self.D:
            return self.D["max"]
        else:
            return None
    
    # Properties
    datatype = property(fget=get_datatype, fset=set_datatype)
    cols = property(fget=get_cols, fset=set_cols)
    rows = property(fget=get_rows, fset=set_rows)
    number_of_cells = property(fget=get_number_of_cells, fset=set_number_of_cells)
    nsres = property(fget=get_nsres, fset=set_nsres)
    ewres = property(fget=get_ewres, fset=set_ewres)
    min = property(fget=get_min, fset=set_min)
    max = property(fget=get_max, fset=set_max)
    
    def print_info(self):
        """!Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " | Datatype:................... " + str(self.get_datatype())
        print " | Number of columns:.......... " + str(self.get_cols())
        print " | Number of rows:............. " + str(self.get_rows())
        print " | Number of cells:............ " + str(
            self.get_number_of_cells())
        print " | North-South resolution:..... " + str(self.get_nsres())
        print " | East-west resolution:....... " + str(self.get_ewres())
        print " | Minimum value:.............. " + str(self.get_min())
        print " | Maximum value:.............. " + str(self.get_max())

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        print "datatype=" + str(self.get_datatype())
        print "cols=" + str(self.get_cols())
        print "rows=" + str(self.get_rows())
        print "number_of_cells=" + str(self.get_number_of_cells())
        print "nsres=" + str(self.get_nsres())
        print "ewres=" + str(self.get_ewres())
        print "min=" + str(self.get_min())
        print "max=" + str(self.get_max())

###############################################################################


class RasterMetadata(RasterMetadataBase):
    """!This is the raster metadata class
       
        This class is the interface to the raster_metadata table in the
        temporal database that stores the metadata of all registered raster maps.
        
        The metadata includes the datatype, number of cols, rows and cells and
        the north-south and east west resolution of the map. Additionally the 
        minimum and maximum values and the name of the space time raster dataset
        register table is stored.
       
        Usage:
        
        @code
       
        >>> init()
        >>> meta = RasterMetadata(ident="soil@PERMANENT",
        ... datatype="CELL", cols=100, rows=100, number_of_cells=10000, nsres=0.1,
        ... ewres=0.1, min=0, max=100)
        >>> meta.datatype
        'CELL'
        >>> meta.cols
        100
        >>> meta.rows
        100
        >>> meta.number_of_cells
        10000
        >>> meta.nsres
        0.1
        >>> meta.ewres
        0.1
        >>> meta.min
        0.0
        >>> meta.max
        100.0
        >>> meta.strds_register
        >>> meta.print_info()
         +-------------------- Metadata information ----------------------------------+
         | Datatype:................... CELL
         | Number of columns:.......... 100
         | Number of rows:............. 100
         | Number of cells:............ 10000
         | North-South resolution:..... 0.1
         | East-west resolution:....... 0.1
         | Minimum value:.............. 0.0
         | Maximum value:.............. 100.0
         | STRDS register table ....... None
        >>> meta.print_shell_info()
        datatype=CELL
        cols=100
        rows=100
        number_of_cells=10000
        nsres=0.1
        ewres=0.1
        min=0.0
        max=100.0
        strds_register=None
        
        @endcode
    """
    def __init__(self, ident=None, strds_register=None, datatype=None, 
		 cols=None, rows=None, number_of_cells=None, nsres=None, 
		 ewres=None, min=None, max=None):

        RasterMetadataBase.__init__(self, "raster_metadata", ident, datatype,
                                      cols, rows, number_of_cells, nsres, 
                                      ewres, min, max)

        self.set_strds_register(strds_register)

    def set_strds_register(self, strds_register):
        """!Set the space time raster dataset register table name"""
        self.D["strds_register"] = strds_register

    def get_strds_register(self):
        """!Get the space time raster dataset register table name
           @return None if not found"""
        if "strds_register" in self.D:
            return self.D["strds_register"]
        else:
            return None
        
    strds_register = property(fget=get_strds_register, fset=set_strds_register)

    def print_info(self):
        """!Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        RasterMetadataBase.print_info(self)
        print " | STRDS register table ....... " + str(
            self.get_strds_register())

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        RasterMetadataBase.print_shell_info(self)
        print "strds_register=" + str(self.get_strds_register())

###############################################################################


class Raster3DMetadata(RasterMetadataBase):
    """!This is the raster3d metadata class
       
        This class is the interface to the raster3d_metadata table in the
        temporal database that stores the metadata of all registered 
        3D raster maps.
        
        The metadata includes all raster metadata variables and additional
        the number of depths, the top-bottom resolution and the space time 3D
        raster dataset register table is stored.
       
        Usage:
        
        @code
       
        >>> init()
        >>> meta = Raster3DMetadata(ident="soil@PERMANENT",
        ... datatype="FCELL", cols=100, rows=100, depths=100,
        ... number_of_cells=1000000, nsres=0.1, ewres=0.1, tbres=0.1,
        ... min=0, max=100)
        >>> meta.datatype
        'FCELL'
        >>> meta.cols
        100
        >>> meta.rows
        100
        >>> meta.depths
        100
        >>> meta.number_of_cells
        1000000
        >>> meta.nsres
        0.1
        >>> meta.ewres
        0.1
        >>> meta.tbres
        0.1
        >>> meta.min
        0.0
        >>> meta.max
        100.0
        >>> meta.str3ds_register
        >>> meta.print_info()
         +-------------------- Metadata information ----------------------------------+
         | Datatype:................... FCELL
         | Number of columns:.......... 100
         | Number of rows:............. 100
         | Number of cells:............ 1000000
         | North-South resolution:..... 0.1
         | East-west resolution:....... 0.1
         | Minimum value:.............. 0.0
         | Maximum value:.............. 100.0
         | Number of depths:........... 100
         | Top-Bottom resolution:...... 0.1
         | STR3DS register table ...... None
        >>> meta.print_shell_info()
        datatype=FCELL
        cols=100
        rows=100
        number_of_cells=1000000
        nsres=0.1
        ewres=0.1
        min=0.0
        max=100.0
        str3ds_register=None
        depths=100
        tbres=0.1
        
        @endcode
    """
    def __init__(self, ident=None, str3ds_register=None, datatype=None, 
		 cols=None, rows=None, depths=None, number_of_cells=None, 
		 nsres=None, ewres=None, tbres=None, min=None, max=None):

        RasterMetadataBase.__init__(self, "raster3d_metadata", ident, 
				datatype, cols, rows, number_of_cells, nsres, 
				ewres, min, max)

        self.set_str3ds_register(str3ds_register)
        self.set_tbres(tbres)
        self.set_depths(depths)

    def set_str3ds_register(self, str3ds_register):
        """!Set the space time raster3d dataset register table name"""
        self.D["str3ds_register"] = str3ds_register

    def set_depths(self, depths):
        """!Set the number of depths"""
        if depths is not None:
            self.D["depths"] = int(depths)
        else:
            self.D["depths"] = None

    def set_tbres(self, tbres):
        """!Set the top-bottom resolution"""
        if tbres is not None:
            self.D["tbres"] = float(tbres)
        else:
            self.D["tbres"] = None

    def get_str3ds_register(self):
        """!Get the space time raster3d dataset register table name
           @return None if not found"""
        if "str3ds_register" in self.D:
            return self.D["str3ds_register"]
        else:
            return None

    def get_depths(self):
        """!Get number of depths
           @return None if not found"""
        if "depths" in self.D:
            return self.D["depths"]
        else:
            return None

    def get_tbres(self):
        """!Get top-bottom resolution
           @return None if not found"""
        if "tbres" in self.D:
            return self.D["tbres"]
        else:
            return None

    depths = property(fget=get_depths, fset=set_depths)
    tbres = property(fget=get_tbres, fset=set_tbres)
    str3ds_register = property(fget=get_str3ds_register, fset=set_str3ds_register)
    
    def print_info(self):
        """!Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        RasterMetadataBase.print_info(self)
        #      0123456789012345678901234567890
        print " | Number of depths:........... " + str(self.get_depths())
        print " | Top-Bottom resolution:...... " + str(self.get_tbres())
        print " | STR3DS register table ...... " + str(
                                                self.get_str3ds_register())

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        RasterMetadataBase.print_shell_info(self)
        print "str3ds_register=" + str(self.get_str3ds_register())
        print "depths=" + str(self.get_depths())
        print "tbres=" + str(self.get_tbres())

###############################################################################


class VectorMetadata(SQLDatabaseInterface):
    """!This is the vector metadata class
        
        This class is the interface to the vector_metadata table in the
        temporal database that stores the metadata of all registered 
        vector maps.
        
        Usage:
        
        @code
       
        >>> init()
        >>> meta = VectorMetadata(ident="lidar@PERMANENT", is_3d=True, 
        ... number_of_points=1, number_of_lines=2, number_of_boundaries=3,
        ... number_of_centroids=4, number_of_faces=5, number_of_kernels=6, 
        ... number_of_primitives=7, number_of_nodes=8, number_of_areas=9,
        ... number_of_islands=10, number_of_holes=11, number_of_volumes=12)
        >>> meta.id
        'lidar@PERMANENT'
        >>> meta.is_3d
        True
        >>> meta.number_of_points
        1
        >>> meta.number_of_lines
        2
        >>> meta.number_of_boundaries
        3
        >>> meta.number_of_centroids
        4
        >>> meta.number_of_faces
        5
        >>> meta.number_of_kernels
        6
        >>> meta.number_of_primitives
        7
        >>> meta.number_of_nodes
        8
        >>> meta.number_of_areas
        9
        >>> meta.number_of_islands
        10
        >>> meta.number_of_holes
        11
        >>> meta.number_of_volumes
        12
        >>> meta.print_info()
         +-------------------- Metadata information ----------------------------------+
         | STVDS register table ....... None
         | Is map 3d .................. True
         | Number of points ........... 1
         | Number of lines ............ 2
         | Number of boundaries ....... 3
         | Number of centroids ........ 4
         | Number of faces ............ 5
         | Number of kernels .......... 6
         | Number of primitives ....... 7
         | Number of nodes ............ 8
         | Number of areas ............ 9
         | Number of islands .......... 10
         | Number of holes ............ 11
         | Number of volumes .......... 12
        >>> meta.print_shell_info()
        stvds_register=None
        is_3d=True
        points=1
        lines=2
        boundaries=3
        centroids=4
        faces=5
        kernels=6
        primitives=7
        nodes=8
        areas=9
        islands=10
        holes=11
        volumes=12
        
        @endcode
    """
    def __init__(
        self, ident=None, stvds_register=None, is_3d=False, 
        number_of_points=None, number_of_lines=None, number_of_boundaries=None,
        number_of_centroids=None, number_of_faces=None, number_of_kernels=None, 
        number_of_primitives=None, number_of_nodes=None, number_of_areas=None,
        number_of_islands=None, number_of_holes=None, number_of_volumes=None):

        SQLDatabaseInterface.__init__(self, "vector_metadata", ident)

        self.set_id(ident)
        self.set_stvds_register(stvds_register)
        self.set_3d_info(is_3d)
        self.set_number_of_points(number_of_points)
        self.set_number_of_lines(number_of_lines)
        self.set_number_of_boundaries(number_of_boundaries)
        self.set_number_of_centroids(number_of_centroids)
        self.set_number_of_faces(number_of_faces)
        self.set_number_of_kernels(number_of_kernels)
        self.set_number_of_primitives(number_of_primitives)
        self.set_number_of_nodes(number_of_nodes)
        self.set_number_of_areas(number_of_areas)
        self.set_number_of_islands(number_of_islands)
        self.set_number_of_holes(number_of_holes)
        self.set_number_of_volumes(number_of_volumes)

    def set_id(self, ident):
        """!Convenient method to set the unique identifier (primary key)"""
        self.ident = ident
        self.D["id"] = ident

    def set_stvds_register(self, stvds_register):
        """!Set the space time vector dataset register table name"""
        self.D["stvds_register"] = stvds_register

    def set_3d_info(self, is_3d):
        """!Set True if the vector map is three dimensional"""
        self.D["is_3d"] = is_3d

    def set_number_of_points(self, number_of_points):
        """!Set the number of points of the vector map"""
        self.D["points"] = number_of_points

    def set_number_of_lines(self, number_of_lines):
        """!Set the number of lines of the vector map"""
        self.D["lines"] = number_of_lines

    def set_number_of_boundaries(self, number_of_boundaries):
        """!Set the number of boundaries of the vector map"""
        self.D["boundaries"] = number_of_boundaries

    def set_number_of_centroids(self, number_of_centroids):
        """!Set the number of centroids of the vector map"""
        self.D["centroids"] = number_of_centroids

    def set_number_of_faces(self, number_of_faces):
        """!Set the number of faces of the vector map"""
        self.D["faces"] = number_of_faces

    def set_number_of_kernels(self, number_of_kernels):
        """!Set the number of kernels of the vector map"""
        self.D["kernels"] = number_of_kernels

    def set_number_of_primitives(self, number_of_primitives):
        """!Set the number of primitives of the vector map"""
        self.D["primitives"] = number_of_primitives

    def set_number_of_nodes(self, number_of_nodes):
        """!Set the number of nodes of the vector map"""
        self.D["nodes"] = number_of_nodes

    def set_number_of_areas(self, number_of_areas):
        """!Set the number of areas of the vector map"""
        self.D["areas"] = number_of_areas

    def set_number_of_islands(self, number_of_islands):
        """!Set the number of islands of the vector map"""
        self.D["islands"] = number_of_islands

    def set_number_of_holes(self, number_of_holes):
        """!Set the number of holes of the vector map"""
        self.D["holes"] = number_of_holes

    def set_number_of_volumes(self, number_of_volumes):
        """!Set the number of volumes of the vector map"""
        self.D["volumes"] = number_of_volumes

    def get_id(self):
        """!Convenient method to get the unique identifier (primary key)
           @return None if not found
        """
        if "id" in self.D:
            return self.D["id"]
        else:
            return None

    def get_stvds_register(self):
        """!Get the space time vector dataset register table name
           @return None if not found"""
        if "stvds_register" in self.D:
            return self.D["stvds_register"]
        else:
            return None

    def get_3d_info(self):
        """!Return True if the map is three dimensional, 
           False if not and None if not info was found"""
        if "is_3d" in self.D:
            return self.D["is_3d"]
        else:
            return None

    def get_number_of_points(self):
        """!Get the number of points of the vector map
           @return None if not found"""
        if "points" in self.D:
            return self.D["points"]
        else:
            return None

    def get_number_of_lines(self):
        """!Get the number of lines of the vector map
           @return None if not found"""
        if "lines" in self.D:
            return self.D["lines"]
        else:
            return None

    def get_number_of_boundaries(self):
        """!Get the number of boundaries of the vector map
           @return None if not found"""
        if "boundaries" in self.D:
            return self.D["boundaries"]
        else:
            return None

    def get_number_of_centroids(self):
        """!Get the number of centroids of the vector map
           @return None if not found"""
        if "centroids" in self.D:
            return self.D["centroids"]
        else:
            return None

    def get_number_of_faces(self):
        """!Get the number of faces of the vector map
           @return None if not found"""
        if "faces" in self.D:
            return self.D["faces"]
        else:
            return None

    def get_number_of_kernels(self):
        """!Get the number of kernels of the vector map
           @return None if not found"""
        if "kernels" in self.D:
            return self.D["kernels"]
        else:
            return None

    def get_number_of_primitives(self):
        """!Get the number of primitives of the vector map
           @return None if not found"""
        if "primitives" in self.D:
            return self.D["primitives"]
        else:
            return None

    def get_number_of_nodes(self):
        """!Get the number of nodes of the vector map
           @return None if not found"""
        if "nodes" in self.D:
            return self.D["nodes"]
        else:
            return None

    def get_number_of_areas(self):
        """!Get the number of areas of the vector map
           @return None if not found"""
        if "areas" in self.D:
            return self.D["areas"]
        else:
            return None

    def get_number_of_islands(self):
        """!Get the number of islands of the vector map
           @return None if not found"""
        if "islands" in self.D:
            return self.D["islands"]
        else:
            return None

    def get_number_of_holes(self):
        """!Get the number of holes of the vector map
           @return None if not found"""
        if "holes" in self.D:
            return self.D["holes"]
        else:
            return None

    def get_number_of_volumes(self):
        """!Get the number of volumes of the vector map
           @return None if not found"""
        if "volumes" in self.D:
            return self.D["volumes"]
        else:
            return None
    
    # Set the properties
    id  = property(fget=get_id, fset=set_id)
    stvds_register  = property(fget=get_stvds_register, 
                               fset=set_stvds_register)
    is_3d  = property(fget=get_3d_info, fset=set_3d_info)
    number_of_points = property(fget=get_number_of_points, 
                                fset=set_number_of_points)
    number_of_lines = property(fget=get_number_of_lines, 
                               fset=set_number_of_lines)
    number_of_boundaries = property(fget=get_number_of_boundaries, 
                                    fset=set_number_of_boundaries)
    number_of_centroids = property(fget=get_number_of_centroids, 
                                   fset=set_number_of_centroids)
    number_of_faces = property(fget=get_number_of_faces, 
                               fset=set_number_of_faces)
    number_of_kernels = property(fget=get_number_of_kernels, 
                                 fset=set_number_of_kernels)
    number_of_primitives = property(fget=get_number_of_primitives, 
                                    fset=set_number_of_primitives)
    number_of_nodes = property(fget=get_number_of_nodes, 
                               fset=set_number_of_nodes)
    number_of_areas = property(fget=get_number_of_areas, 
                               fset=set_number_of_areas)
    number_of_islands = property(fget=get_number_of_islands, 
                                 fset=set_number_of_islands)
    number_of_holes = property(fget=get_number_of_holes, 
                               fset=set_number_of_holes)
    number_of_volumes = property(fget=get_number_of_volumes, 
                                 fset=set_number_of_volumes)
        
    def print_info(self):
        """!Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " +-------------------- Metadata information ----------------------------------+"
        print " | STVDS register table ....... " + str(
            self.get_stvds_register())
        print " | Is map 3d .................. " + str(self.get_3d_info())
        print " | Number of points ........... " + str(self.get_number_of_points())
        print " | Number of lines ............ " + str(self.get_number_of_lines())
        print " | Number of boundaries ....... " + str(self.get_number_of_boundaries())
        print " | Number of centroids ........ " + str(self.get_number_of_centroids())
        print " | Number of faces ............ " + str(self.get_number_of_faces())
        print " | Number of kernels .......... " + str(self.get_number_of_kernels())
        print " | Number of primitives ....... " + str(self.get_number_of_primitives())
        print " | Number of nodes ............ " + str(self.get_number_of_nodes())
        print " | Number of areas ............ " + str(self.get_number_of_areas())
        print " | Number of islands .......... " + str(self.get_number_of_islands())
        print " | Number of holes ............ " + str(self.get_number_of_holes())
        print " | Number of volumes .......... " + str(self.get_number_of_volumes())

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        print "stvds_register=" + str(self.get_stvds_register())
        print "is_3d=" + str(self.get_3d_info())
        print "points=" + str(self.get_number_of_points())
        print "lines=" + str(self.get_number_of_lines())
        print "boundaries=" + str(self.get_number_of_boundaries())
        print "centroids=" + str(self.get_number_of_centroids())
        print "faces=" + str(self.get_number_of_faces())
        print "kernels=" + str(self.get_number_of_kernels())
        print "primitives=" + str(self.get_number_of_primitives())
        print "nodes=" + str(self.get_number_of_nodes())
        print "areas=" + str(self.get_number_of_areas())
        print "islands=" + str(self.get_number_of_islands())
        print "holes=" + str(self.get_number_of_holes())
        print "volumes=" + str(self.get_number_of_volumes())

###############################################################################


class STDSMetadataBase(SQLDatabaseInterface):
    """!This is the space time dataset metadata base class for 
       strds, stvds and str3ds datasets
       setting/getting the id, the title and the description
       
        Usage:
        
        @code
        
        >>> init()
        >>> meta = STDSMetadataBase(ident="soils@PERMANENT",
        ... title="Soils", description="Soils 1950 - 2010")
        >>> meta.id
        'soils@PERMANENT'
        >>> meta.title
        'Soils'
        >>> meta.description
        'Soils 1950 - 2010'
        >>> meta.number_of_maps
        >>> meta.print_info()
         | Number of registered maps:.. None
         | Title:
         | Soils
         | Description:
         | Soils 1950 - 2010
         | Commands of creation:
        >>> meta.print_shell_info()
        number_of_maps=None
        
        @endcode
    """
    def __init__(self, table=None, ident=None, title=None, description=None, command=None):

        SQLDatabaseInterface.__init__(self, table, ident)

        self.set_id(ident)
        self.set_title(title)
        self.set_description(description)
        self.set_command(command)
        # No setter for this
        self.D["number_of_maps"] = None

    def set_id(self, ident):
        """!Convenient method to set the unique identifier (primary key)"""
        self.ident = ident
        self.D["id"] = ident

    def set_title(self, title):
        """!Set the title"""
        self.D["title"] = title

    def set_description(self, description):
        """!Set the number of cols"""
        self.D["description"] = description

    def set_command(self, command):
        """!Set the number of cols"""
        self.D["command"] = command
        
    def get_id(self):
        """!Convenient method to get the unique identifier (primary key)
           @return None if not found
        """
        if "id" in self.D:
            return self.D["id"]
        else:
            return None

    def get_title(self):
        """!Get the title
           @return None if not found"""
        if "title" in self.D:
            return self.D["title"]
        else:
            return None

    def get_description(self):
        """!Get description
           @return None if not found"""
        if "description" in self.D:
            return self.D["description"]
        else:
            return None

    def get_command(self):
        """!Get command
           @return None if not found"""
        if "command" in self.D:
            return self.D["command"]
        else:
            return None
            
    def get_number_of_maps(self):
        """!Get the number of registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "number_of_maps" in self.D:
            return self.D["number_of_maps"]
        else:
            return None
        
    id  = property(fget=get_id, fset=set_id)
    title  = property(fget=get_title, fset=set_title)
    description  = property(fget=get_description, fset=set_description)
    number_of_maps  = property(fget=get_number_of_maps)
    
    def print_info(self):
        """!Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " | Number of registered maps:.. " + str(
            self.get_number_of_maps())
        print " |"
        print " | Title:"
        print " | " + str(self.get_title())
        print " | Description:"
        print " | " + str(self.get_description())
        print " | Command history:"
        command = self.get_command()
        if command:
            for token in command.split("\n"):
                print " | " + str(token)

    def print_history(self):
        """!Print history information about this class in human readable 
            shell style
        """
        #      0123456789012345678901234567890
        print "# Title:"
        print "# " + str(self.get_title())
        print "# Description:"
        print "# " + str(self.get_description())
        print "# Command history:"
        command = self.get_command()
        
        if command:
            tokens = command.split("\n")
            print_list = []
            for token in tokens:
                token = str(token).rstrip().lstrip()
                if len(token) > 1:
                    print_list.append(token)
                
            count = 0
            for token in print_list:
                count += 1
                if len(token) > 1:
                    if token[0] == "#":
                        print token
                    elif count < len(print_list):
                        print token + " \\"
                    else:
                        print token 
                        
    def print_shell_info(self):
        """!Print information about this class in shell style"""
        print "number_of_maps=" + str(self.get_number_of_maps())

###############################################################################


class STDSRasterMetadataBase(STDSMetadataBase):
    """!This is the space time dataset metadata base 
        class for strds and str3ds datasets

        Most of the metadata values are set by SQL scripts in the database when
        new maps are added. Therefor only some set- an many 
        get-functions are available.
        
       
        Usage:
        
        @code
        >>> init()
        >>> meta = STDSRasterMetadataBase(ident="soils@PERMANENT",
        ... title="Soils", description="Soils 1950 - 2010")
        >>> meta.id
        'soils@PERMANENT'
        >>> meta.title
        'Soils'
        >>> meta.description
        'Soils 1950 - 2010'
        >>> meta.number_of_maps
        >>> meta.min_max
        >>> meta.max_max
        >>> meta.min_min
        >>> meta.max_min
        >>> meta.nsres_min
        >>> meta.nsres_max
        >>> meta.ewres_min
        >>> meta.ewres_max
        >>> meta.print_info()
         | Number of registered maps:.. None
         | Title:
         | Soils
         | Description:
         | Soils 1950 - 2010
         | Commands of creation:
         | North-South resolution min:. None
         | North-South resolution max:. None
         | East-west resolution min:... None
         | East-west resolution max:... None
         | Minimum value min:.......... None
         | Minimum value max:.......... None
         | Maximum value min:.......... None
         | Maximum value max:.......... None
        >>> meta.print_shell_info()
        number_of_maps=None
        nsres_min=None
        nsres_max=None
        ewres_min=None
        ewres_max=None
        min_min=None
        min_max=None
        max_min=None
        max_max=None
        
        @endcode
    """
    def __init__(self, table=None, ident=None, title=None, description=None):

        STDSMetadataBase.__init__(self, table, ident, title, description)

        # Initialize the dict to select all values from the db
        self.D["min_max"] = None
        self.D["max_max"] = None
        self.D["min_min"] = None
        self.D["max_min"] = None
        self.D["nsres_min"] = None
        self.D["nsres_max"] = None
        self.D["ewres_min"] = None
        self.D["ewres_max"] = None

    def get_max_min(self):
        """!Get the minimal maximum of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "max_min" in self.D:
            return self.D["max_min"]
        else:
            return None

    def get_min_min(self):
        """!Get the minimal minimum of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "min_min" in self.D:
            return self.D["min_min"]
        else:
            return None

    def get_max_max(self):
        """!Get the maximal maximum of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "max_max" in self.D:
            return self.D["max_max"]
        else:
            return None

    def get_min_max(self):
        """!Get the maximal minimum of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "min_max" in self.D:
            return self.D["min_max"]
        else:
            return None

    def get_nsres_min(self):
        """!Get the minimal north-south resolution of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "nsres_min" in self.D:
            return self.D["nsres_min"]
        else:
            return None

    def get_nsres_max(self):
        """!Get the maximal north-south resolution of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "nsres_max" in self.D:
            return self.D["nsres_max"]
        else:
            return None

    def get_ewres_min(self):
        """!Get the minimal east-west resolution of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "ewres_min" in self.D:
            return self.D["ewres_min"]
        else:
            return None

    def get_ewres_max(self):
        """!Get the maximal east-west resolution of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "ewres_max" in self.D:
            return self.D["ewres_max"]
        else:
            return None
    
    nsres_min = property(fget=get_nsres_min)
    nsres_max = property(fget=get_nsres_max)
    ewres_min = property(fget=get_ewres_min)
    ewres_max = property(fget=get_ewres_max)
    min_min = property(fget=get_min_min)
    min_max = property(fget=get_min_max)
    max_min = property(fget=get_max_min)
    max_max = property(fget=get_max_max)
    
    def print_info(self):
        """!Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " | North-South resolution min:. " + str(self.get_nsres_min())
        print " | North-South resolution max:. " + str(self.get_nsres_max())
        print " | East-west resolution min:... " + str(self.get_ewres_min())
        print " | East-west resolution max:... " + str(self.get_ewres_max())
        print " | Minimum value min:.......... " + str(self.get_min_min())
        print " | Minimum value max:.......... " + str(self.get_min_max())
        print " | Maximum value min:.......... " + str(self.get_max_min())
        print " | Maximum value max:.......... " + str(self.get_max_max())
        STDSMetadataBase.print_info(self)

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        STDSMetadataBase.print_shell_info(self)
        print "nsres_min=" + str(self.get_nsres_min())
        print "nsres_max=" + str(self.get_nsres_max())
        print "ewres_min=" + str(self.get_ewres_min())
        print "ewres_max=" + str(self.get_ewres_max())
        print "min_min=" + str(self.get_min_min())
        print "min_max=" + str(self.get_min_max())
        print "max_min=" + str(self.get_max_min())
        print "max_max=" + str(self.get_max_max())


###############################################################################

class STRDSMetadata(STDSRasterMetadataBase):
    """!This is the raster metadata class
        
        This class is the interface to the strds_metadata table in the
        temporal database that stores the metadata of all registered 
        space time raster datasets
        
        Most of the metadata values are set by SQL scripts in the database when
        new raster maps are added. Therefor only some set- an many 
        get-functions are available.
        
        Usage:
        
        @code
        
        >>> init()
        >>> meta = STRDSMetadata(ident="soils@PERMANENT",
        ... title="Soils", description="Soils 1950 - 2010")
        >>> meta.id
        'soils@PERMANENT'
        >>> meta.title
        'Soils'
        >>> meta.description
        'Soils 1950 - 2010'
        >>> meta.number_of_maps
        >>> meta.min_max
        >>> meta.max_max
        >>> meta.min_min
        >>> meta.max_min
        >>> meta.nsres_min
        >>> meta.nsres_max
        >>> meta.ewres_min
        >>> meta.ewres_max
        >>> meta.raster_register
        >>> meta.print_info()
         +-------------------- Metadata information ----------------------------------+
         | Number of registered maps:.. None
         | Title:
         | Soils
         | Description:
         | Soils 1950 - 2010
         | Commands of creation:
         | North-South resolution min:. None
         | North-South resolution max:. None
         | East-west resolution min:... None
         | East-west resolution max:... None
         | Minimum value min:.......... None
         | Minimum value max:.......... None
         | Maximum value min:.......... None
         | Maximum value max:.......... None
         | Raster register table:...... None
        >>> meta.print_shell_info()
        number_of_maps=None
        nsres_min=None
        nsres_max=None
        ewres_min=None
        ewres_max=None
        min_min=None
        min_max=None
        max_min=None
        max_max=None
        raster_register=None
        
        @endcode
    """
    def __init__(self, ident=None, raster_register=None, title=None, description=None):

        STDSRasterMetadataBase.__init__(
            self, "strds_metadata", ident, title, description)

        self.set_raster_register(raster_register)

    def set_raster_register(self, raster_register):
        """!Set the raster map register table name"""
        self.D["raster_register"] = raster_register

    def get_raster_register(self):
        """!Get the raster map register table name
           @return None if not found"""
        if "raster_register" in self.D:
            return self.D["raster_register"]
        else:
            return None
    
    raster_register = property(fget=get_raster_register, 
                               fset=set_raster_register)

    def print_info(self):
        """!Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        print " | Raster register table:...... " + str(
            self.get_raster_register())
        STDSRasterMetadataBase.print_info(self)

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        STDSRasterMetadataBase.print_shell_info(self)
        print "raster_register=" + str(self.get_raster_register())

###############################################################################


class STR3DSMetadata(STDSRasterMetadataBase):
    """!This is the space time 3D raster metadata class
    
        This class is the interface to the str3ds_metadata table in the
        temporal database that stores the metadata of all registered 
        space time 3D raster datasets
        
        Most of the metadata values are set by SQL scripts in the database when
        new 3D raster maps are added. Therefor only some set- an many 
        get-functions are available.
        
        Usage:
        
        @code
        
        >>> init()
        >>> meta = STR3DSMetadata(ident="soils@PERMANENT",
        ... title="Soils", description="Soils 1950 - 2010")
        >>> meta.id
        'soils@PERMANENT'
        >>> meta.title
        'Soils'
        >>> meta.description
        'Soils 1950 - 2010'
        >>> meta.number_of_maps
        >>> meta.min_max
        >>> meta.max_max
        >>> meta.min_min
        >>> meta.max_min
        >>> meta.nsres_min
        >>> meta.nsres_max
        >>> meta.ewres_min
        >>> meta.ewres_max
        >>> meta.tbres_min
        >>> meta.tbres_max
        >>> meta.raster3d_register
        >>> meta.print_info()
         +-------------------- Metadata information ----------------------------------+
         | Number of registered maps:.. None
         | Title:
         | Soils
         | Description:
         | Soils 1950 - 2010
         | Commands of creation:
         | North-South resolution min:. None
         | North-South resolution max:. None
         | East-west resolution min:... None
         | East-west resolution max:... None
         | Minimum value min:.......... None
         | Minimum value max:.......... None
         | Maximum value min:.......... None
         | Maximum value max:.......... None
         | Top-bottom resolution min:.. None
         | Top-bottom resolution max:.. None
         | 3D raster register table:... None
        >>> meta.print_shell_info()
        number_of_maps=None
        nsres_min=None
        nsres_max=None
        ewres_min=None
        ewres_max=None
        min_min=None
        min_max=None
        max_min=None
        max_max=None
        tbres_min=None
        tbres_max=None
        raster3d_register=None
        
        @endcode
        """
    def __init__(self, ident=None, raster3d_register=None, title=None, description=None):

        STDSRasterMetadataBase.__init__(
            self, "str3ds_metadata", ident, title, description)

        self.set_raster3d_register(raster3d_register)
        self.D["tbres_min"] = None
        self.D["tbres_max"] = None

    def set_raster3d_register(self, raster3d_register):
        """!Set the raster map register table name"""
        self.D["raster3d_register"] = raster3d_register

    def get_raster3d_register(self):
        """!Get the raster3d map register table name
           @return None if not found"""
        if "raster3d_register" in self.D:
            return self.D["raster3d_register"]
        else:
            return None

    def get_tbres_min(self):
        """!Get the minimal top-bottom resolution of all registered maps,
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "tbres_min" in self.D:
            return self.D["tbres_min"]
        else:
            return None

    def get_tbres_max(self):
        """!Get the maximal top-bottom resolution of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "tbres_max" in self.D:
            return self.D["tbres_max"]
        else:
            return None

    raster3d_register = property(fget=get_raster3d_register, 
                               fset=set_raster3d_register)
    tbres_min = property(fget=get_tbres_min)
    tbres_max = property(fget=get_tbres_max)
    
    def print_info(self):
        """!Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        #      0123456789012345678901234567890
        print " | 3D raster register table:... " + str(
            self.get_raster3d_register())
        print " | Top-bottom resolution min:.. " + str(self.get_ewres_min())
        print " | Top-bottom resolution max:.. " + str(self.get_ewres_max())
        STDSRasterMetadataBase.print_info(self)

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        STDSRasterMetadataBase.print_shell_info(self)
        print "tbres_min=" + str(self.get_tbres_min())
        print "tbres_max=" + str(self.get_tbres_max())
        print "raster3d_register=" + str(self.get_raster3d_register())

###############################################################################

class STVDSMetadata(STDSMetadataBase):
    """!This is the space time vector dataset metadata class
        
       This class is the interface to the stvds_metadata table in the
       temporal database that stores the metadata of all registered 
       space time vector datasets
        
       Most of the metadata values are set by SQL scripts in the database when
       new vector maps are added. Therefor only some set- an many get-functions
       are available.
        
        Usage:
        
        @code
        
        >>> init()
        >>> meta = STVDSMetadata(ident="lidars@PERMANENT",
        ... title="LIDARS", description="LIDARS 2008 - 2010")
        >>> meta.id
        'lidars@PERMANENT'
        >>> meta.title
        'LIDARS'
        >>> meta.description
        'LIDARS 2008 - 2010'
        >>> meta.number_of_maps
        >>> meta.number_of_points
        >>> meta.number_of_lines
        >>> meta.number_of_boundaries
        >>> meta.number_of_centroids
        >>> meta.number_of_faces
        >>> meta.number_of_kernels
        >>> meta.number_of_primitives
        >>> meta.number_of_nodes
        >>> meta.number_of_areas
        >>> meta.number_of_islands
        >>> meta.number_of_holes
        >>> meta.number_of_volumes
        >>> meta.print_info()
         +-------------------- Metadata information ----------------------------------+
         | Number of registered maps:.. None
         | Title:
         | LIDARS
         | Description:
         | LIDARS 2008 - 2010
         | Commands of creation:
         | Vector register table:...... None
         | Number of points ........... None
         | Number of lines ............ None
         | Number of boundaries ....... None
         | Number of centroids ........ None
         | Number of faces ............ None
         | Number of kernels .......... None
         | Number of primitives ....... None
         | Number of nodes ............ None
         | Number of areas ............ None
         | Number of islands .......... None
         | Number of holes ............ None
         | Number of volumes .......... None
        >>> meta.print_shell_info()
        number_of_maps=None
        vector_register=None
        points=None
        lines=None
        boundaries=None
        centroids=None
        faces=None
        kernels=None
        primitives=None
        nodes=None
        areas=None
        islands=None
        holes=None
        volumes=None
        
        @endcode
    """
    def __init__(
        self, ident=None, vector_register=None, title=None, description=None):

        STDSMetadataBase.__init__(
            self, "stvds_metadata", ident, title, description)

        self.set_vector_register(vector_register)
        self.D["points"] = None
        self.D["lines"] = None
        self.D["boundaries"] = None
        self.D["centroids"] = None
        self.D["faces"] = None
        self.D["kernels"] = None
        self.D["primitives"] = None
        self.D["nodes"] = None
        self.D["areas"] = None
        self.D["islands"] = None
        self.D["holes"] = None
        self.D["volumes"] = None
        
    def set_vector_register(self, vector_register):
        """!Set the vector map register table name"""
        self.D["vector_register"] = vector_register

    def get_vector_register(self):
        """!Get the vector map register table name
           @return None if not found"""
        if "vector_register" in self.D:
            return self.D["vector_register"]
        else:
            return None

    def get_number_of_points(self):
        """!Get the number of points of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "points" in self.D:
            return self.D["points"]
        else:
            return None

    def get_number_of_lines(self):
        """!Get the number of lines of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "lines" in self.D:
            return self.D["lines"]
        else:
            return None

    def get_number_of_boundaries(self):
        """!Get the number of boundaries of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "boundaries" in self.D:
            return self.D["boundaries"]
        else:
            return None

    def get_number_of_centroids(self):
        """!Get the number of centroids of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "centroids" in self.D:
            return self.D["centroids"]
        else:
            return None

    def get_number_of_faces(self):
        """!Get the number of faces of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "faces" in self.D:
            return self.D["faces"]
        else:
            return None

    def get_number_of_kernels(self):
        """!Get the number of kernels of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "kernels" in self.D:
            return self.D["kernels"]
        else:
            return None

    def get_number_of_primitives(self):
        """!Get the number of primitives of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "primitives" in self.D:
            return self.D["primitives"]
        else:
            return None

    def get_number_of_nodes(self):
        """!Get the number of nodes of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "nodes" in self.D:
            return self.D["nodes"]
        else:
            return None

    def get_number_of_areas(self):
        """!Get the number of areas of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "areas" in self.D:
            return self.D["areas"]
        else:
            return None

    def get_number_of_islands(self):
        """!Get the number of islands of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "islands" in self.D:
            return self.D["islands"]
        else:
            return None

    def get_number_of_holes(self):
        """!Get the number of holes of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "holes" in self.D:
            return self.D["holes"]
        else:
            return None

    def get_number_of_volumes(self):
        """!Get the number of volumes of all registered maps, 
           this value is set in the database
           automatically via SQL, so no setter exists
           @return None if not found"""
        if "volumes" in self.D:
            return self.D["volumes"]
        else:
            return None
    
    # Set the properties
    vector_register  = property(fget=get_vector_register,
                                fset=set_vector_register)
    number_of_points = property(fget=get_number_of_points)
    number_of_lines = property(fget=get_number_of_lines)
    number_of_boundaries = property(fget=get_number_of_boundaries)
    number_of_centroids = property(fget=get_number_of_centroids)
    number_of_faces = property(fget=get_number_of_faces)
    number_of_kernels = property(fget=get_number_of_kernels)
    number_of_primitives = property(fget=get_number_of_primitives)
    number_of_nodes = property(fget=get_number_of_nodes)
    number_of_areas = property(fget=get_number_of_areas)
    number_of_islands = property(fget=get_number_of_islands)
    number_of_holes = property(fget=get_number_of_holes)
    number_of_volumes = property(fget=get_number_of_volumes)

    def print_info(self):
        """!Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        print " | Vector register table:...... " + str(
            self.get_vector_register())
        print " | Number of points ........... " + str(self.number_of_points)
        print " | Number of lines ............ " + str(self.number_of_lines)
        print " | Number of boundaries ....... " + str(self.number_of_boundaries)
        print " | Number of centroids ........ " + str(self.number_of_centroids)
        print " | Number of faces ............ " + str(self.number_of_faces)
        print " | Number of kernels .......... " + str(self.number_of_kernels)
        print " | Number of primitives ....... " + str(self.number_of_primitives)
        print " | Number of nodes ............ " + str(self.number_of_nodes)
        print " | Number of areas ............ " + str(self.number_of_areas)
        print " | Number of islands .......... " + str(self.number_of_islands)
        print " | Number of holes ............ " + str(self.number_of_holes)
        print " | Number of volumes .......... " + str(self.number_of_volumes)
        STDSMetadataBase.print_info(self)

    def print_shell_info(self):
        """!Print information about this class in shell style"""
        STDSMetadataBase.print_shell_info(self)
        print "vector_register=" + str(self.get_vector_register())
        print "points=" + str(self.get_number_of_points())
        print "lines=" + str(self.get_number_of_lines())
        print "boundaries=" + str(self.get_number_of_boundaries())
        print "centroids=" + str(self.get_number_of_centroids())
        print "faces=" + str(self.get_number_of_faces())
        print "kernels=" + str(self.get_number_of_kernels())
        print "primitives=" + str(self.get_number_of_primitives())
        print "nodes=" + str(self.get_number_of_nodes())
        print "areas=" + str(self.get_number_of_areas())
        print "islands=" + str(self.get_number_of_islands())
        print "holes=" + str(self.get_number_of_holes())
        print "volumes=" + str(self.get_number_of_volumes())

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
