"""!@package grass.script.tgis_metadata

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related metadata functions to be used in Python scripts and tgis packages.

Usage:

@code
from grass.script import tgis_metadata as grass

meta = grass.raster_metadata()
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from base import *

###############################################################################

class raster_metadata_base(sql_database_interface):
    """This is the raster metadata base class for raster and raster3d maps"""
    def __init__(self, table=None, ident=None, datatype=None, cols=None, rows=None, number_of_cells=None, nsres=None, ewres=None, min=None, max=None):

	sql_database_interface.__init__(self, table, ident)

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
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_datatype(self, datatype):
	"""Set the datatype"""
	self.D["datatype"] = datatype

    def set_cols(self, cols):
	"""Set the number of cols"""
	self.D["cols"] = cols

    def set_rows(self, rows):
	"""Set the number of rows"""
	self.D["rows"] = rows

    def set_number_of_cells(self, number_of_cells):
	"""Set the number of cells"""
	self.D["number_of_cells"] = number_of_cells

    def set_nsres(self, nsres):
	"""Set the north-south resolution"""
	self.D["nsres"] = nsres

    def set_ewres(self, ewres):
	"""Set the east-west resolution"""
	self.D["ewres"] = ewres

    def set_min(self, min):
	"""Set the minimum raster value"""
	self.D["min"] = min

    def set_max(self, max):
	"""Set the maximum raster value"""
	self.D["max"] = max

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_datatype(self):
	"""Get the map type 
	   @return None if not found"""
	if self.D.has_key("datatype"):
	    return self.D["datatype"]
        else:
	    return None

    def get_cols(self):
	"""Get number of cols 
	   @return None if not found"""
	if self.D.has_key("cols"):
	    return self.D["cols"]
        else:
	    return None

    def get_rows(self):
	"""Get number of rows
	   @return None if not found"""
	if self.D.has_key("rows"):
	    return self.D["rows"]
        else:
	    return None

    def get_number_of_cells(self):
	"""Get number of cells 
	   @return None if not found"""
	if self.D.has_key("number_of_cells"):
	    return self.D["number_of_cells"]
        else:
	    return None

    def get_nsres(self):
	"""Get the north-south resolution
	   @return None if not found"""
	if self.D.has_key("nsres"):
	    return self.D["nsres"]
        else:
	    return None

    def get_ewres(self):
	"""Get east-west resolution
	   @return None if not found"""
	if self.D.has_key("ewres"):
	    return self.D["ewres"]
        else:
	    return None

    def get_min(self):
	"""Get the minimum cell value 
	   @return None if not found"""
	if self.D.has_key("min"):
	    return self.D["min"]
        else:
	    return None

    def get_max(self):
	"""Get the maximum cell value 
	   @return None if not found"""
	if self.D.has_key("max"):
	    return self.D["max"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " | Datatype:................... " + str(self.get_datatype())
        print " | Number of columns:.......... " + str(self.get_cols())
        print " | Number of rows:............. " + str(self.get_rows())
        print " | Number of cells:............ " + str(self.get_number_of_cells())
        print " | North-South resolution:..... " + str(self.get_nsres())
        print " | East-west resolution:....... " + str(self.get_ewres())
        print " | Minimum value:.............. " + str(self.get_min())
        print " | Maximum value:.............. " + str(self.get_max())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "datatype=" + str(self.get_datatype())
        print "cols=" + str(self.get_cols())
        print "rows=" + str(self.get_rows())
        print "number_of_cells=" + str(self.get_number_of_cells())
        print "nsres=" + str(self.get_nsres())
        print "ewres=" + str(self.get_ewres())
        print "min=" + str(self.get_min())
        print "max=" + str(self.get_max())

###############################################################################

class raster_metadata(raster_metadata_base):
    """This is the raster metadata class"""
    def __init__(self, ident=None, strds_register=None, datatype=None, cols=None, rows=None, number_of_cells=None, nsres=None, ewres=None, min=None, max=None):

	raster_metadata_base.__init__(self, "raster_metadata", ident, datatype, cols, rows, number_of_cells, nsres, ewres, min, max)

	self.set_strds_register(strds_register)

    def set_strds_register(self, strds_register):
	"""Set the space time raster dataset register table name"""
	self.D["strds_register"] = strds_register

    def get_strds_register(self):
	"""Get the space time raster dataset register table name
	   @return None if not found"""
	if self.D.has_key("strds_register"):
	    return self.D["strds_register"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        print " | STRDS register table ....... " + str(self.get_strds_register())
        raster_metadata_base.print_info(self)

    def print_shell_info(self):
        """Print information about this class in shell style"""
        raster_metadata_base.print_shell_info(self)

###############################################################################

class raster3d_metadata(raster_metadata_base):
    """This is the raster3d metadata class"""
    def __init__(self, ident=None, str3ds_register=None, datatype=None, cols=None, rows=None, depths=None, number_of_cells=None, nsres=None, ewres=None, tbres=None, min=None, max=None):

	raster_metadata_base.__init__(self, "raster3d_metadata", ident, datatype, cols, rows, number_of_cells, nsres, ewres, min, max)

	self.set_str3ds_register(str3ds_register)
	self.set_tbres(tbres)
	self.set_depths(depths)

    def set_str3ds_register(self, str3ds_register):
	"""Set the space time raster3d dataset register table name"""
	self.D["str3ds_register"] = str3ds_register

    def set_depths(self, depths):
	"""Set the number of depths"""
	self.D["depths"] = depths

    def set_tbres(self, tbres):
	"""Set the top-bottom resolution"""
	self.D["tbres"] = tbres

    def get_str3ds_register(self):
	"""Get the space time raster3d dataset register table name
	   @return None if not found"""
	if self.D.has_key("str3ds_register"):
	    return self.D["str3ds_register"]
        else:
	    return None

    def get_depths(self):
	"""Get number of depths
	   @return None if not found"""
	if self.D.has_key("depths"):
	    return self.D["depths"]
        else:
	    return None

    def get_tbres(self):
	"""Get top-bottom resolution
	   @return None if not found"""
	if self.D.has_key("tbres"):
	    return self.D["tbres"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        print " | STR3DS register table ...... " + str(self.get_str3ds_register())
        raster_metadata_base.print_info(self)
        #      0123456789012345678901234567890
        print " | Number of depths:........... " + str(self.get_depths())
        print " | Top-Bottom resolution:...... " + str(self.get_tbres())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        raster_metadata_base.print_shell_info(self)
        print "depths=" + str(self.get_depths())
        print "tbres=" + str(self.get_tbres())
        
###############################################################################

class vector_metadata(sql_database_interface):
    """This is the vector metadata class"""
    def __init__(self, ident=None, stvds_register=None):

	sql_database_interface.__init__(self, "vector_metadata", ident)

	self.set_id(ident)
	self.set_stvds_register(stvds_register)

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_stvds_register(self, stvds_register):
	"""Set the space time vector dataset register table name"""
	self.D["stvds_register"] = stvds_register

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_stvds_register(self):
	"""Get the space time vector dataset register table name
	   @return None if not found"""
	if self.D.has_key("stvds_register"):
	    return self.D["stvds_register"]
        else:
	    return None


    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " +-------------------- Metadata information ----------------------------------+"
        print " | STVDS register table ....... " + str(self.get_stvds_register())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "stvds_register=" + str(self.get_stvds_register())

###############################################################################

class stds_metadata_base(sql_database_interface):
    """This is the space time dataset metadata base class for strds, stvds and str3ds datasets
       setting/getting the id, the title and the description
    """
    def __init__(self, table=None, ident=None, title=None, description=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_title(title)
	self.set_description(description)
        # No setter for this
        self.D["number_of_maps"] = None

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_title(self, title):
	"""Set the title"""
	self.D["title"] = title

    def set_description(self, description):
	"""Set the number of cols"""
	self.D["description"] = description

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_title(self):
	"""Get the title 
	   @return None if not found"""
	if self.D.has_key("title"):
	    return self.D["title"]
        else:
	    return None

    def get_description(self):
	"""Get description 
	   @return None if not found"""
	if self.D.has_key("description"):
	    return self.D["description"]
        else:
	    return None

    def get_number_of_maps(self):
	"""Get the number of registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("number_of_maps"):
	    return self.D["number_of_maps"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " | Number of registered maps:.. " + str(self.get_number_of_maps())
        print " | Title:"
        print " | " + str(self.get_title())
        print " | Description:"
        print " | " + str(self.get_description())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "number_of_maps=" + str(self.get_number_of_maps())

###############################################################################
    
class stds_raster_metadata_base(stds_metadata_base):
    """This is the space time dataset metadata base class for strds and str3ds datasets
       
       Most of the metadata values are set by triggers in the database when 
       new raster of voxel maps are added. Therefor only some set- an many get-functions 
       are available.
    """
    def __init__(self, table=None, ident=None, title=None, description=None):

	stds_metadata_base.__init__(self, table, ident, title, description)
        
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
	"""Get the minimal maximum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("max_min"):
	    return self.D["max_min"]
        else:
	    return None

    def get_min_min(self):
	"""Get the minimal minimum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("min_min"):
	    return self.D["min_min"]
        else:
	    return None

    def get_max_max(self):
	"""Get the maximal maximum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("max_max"):
	    return self.D["max_max"]
        else:
	    return None

    def get_min_max(self):
	"""Get the maximal minimum of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("min_max"):
	    return self.D["min_max"]
        else:
	    return None

    def get_nsres_min(self):
	"""Get the minimal north-south resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("nsres_min"):
	    return self.D["nsres_min"]
        else:
	    return None

    def get_nsres_max(self):
	"""Get the maximal north-south resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("nsres_max"):
	    return self.D["nsres_max"]
        else:
	    return None

    def get_ewres_min(self):
	"""Get the minimal east-west resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("ewres_min"):
	    return self.D["ewres_min"]
        else:
	    return None

    def get_ewres_max(self):
	"""Get the maximal east-west resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("ewres_max"):
	    return self.D["ewres_max"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        stds_metadata_base.print_info(self)
        #      0123456789012345678901234567890
        print " | North-South resolution min:. " + str(self.get_nsres_min())
        print " | North-South resolution max:. " + str(self.get_nsres_max())
        print " | East-west resolution min:... " + str(self.get_ewres_min())
        print " | East-west resolution max:... " + str(self.get_ewres_max())
        print " | Minimum value min:.......... " + str(self.get_min_min())
        print " | Minimum value max:.......... " + str(self.get_min_max())
        print " | Maximum value min:.......... " + str(self.get_max_min())
        print " | Maximum value max:.......... " + str(self.get_max_max())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        stds_metadata_base.print_shell_info(self)
        print "nsres_min=" + str(self.get_nsres_min())
        print "nsres_max=" + str(self.get_nsres_max())
        print "ewres_min=" + str(self.get_ewres_min())
        print "ewres_max=" + str(self.get_ewres_max())
        print "min_min=" + str(self.get_min_min())
        print "min_max=" + str(self.get_min_max())
        print "max_min=" + str(self.get_max_min())
        print "max_max=" + str(self.get_max_max())


###############################################################################

class strds_metadata(stds_raster_metadata_base):
    """This is the raster metadata class"""
    def __init__(self, ident=None, raster_register=None,  title=None, description=None):

	stds_raster_metadata_base.__init__(self, "strds_metadata", ident, title, description)

	self.set_raster_register(raster_register)

    def set_raster_register(self, raster_register):
	"""Set the raster map register table name"""
	self.D["raster_register"] = raster_register

    def get_raster_register(self):
	"""Get the raster map register table name
	   @return None if not found"""
	if self.D.has_key("raster_register"):
	    return self.D["raster_register"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        print " | Raster register table:...... " + str(self.get_raster_register())
        stds_raster_metadata_base.print_info(self)

    def print_shell_info(self):
        """Print information about this class in shell style"""
        stds_raster_metadata_base.print_shell_info(self)

###############################################################################

class str3ds_metadata(stds_raster_metadata_base):
    """This is the space time raster3d metadata class"""
    def __init__(self, ident=None, raster3d_register=None,  title=None, description=None):

	stds_raster_metadata_base.__init__(self, "str3ds_metadata", ident, title, description)

	self.set_raster3d_register(raster3d_register)
        self.D["tbres_min"] = None
        self.D["tbres_max"] = None

    def set_raster3d_register(self, raster3d_register):
	"""Set the raster map register table name"""
	self.D["raster3d_register"] = raster3d_register

    def get_raster3d_register(self):
	"""Get the raster3d map register table name
	   @return None if not found"""
	if self.D.has_key("raster3d_register"):
	    return self.D["raster3d_register"]
        else:
	    return None

    def get_tbres_min(self):
	"""Get the minimal top-bottom resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("tbres_min"):
	    return self.D["tbres_min"]
        else:
	    return None

    def get_tbres_max(self):
	"""Get the maximal top-bottom resolution of all registered maps, this value is set in the database
           automatically via SQL trigger, so no setter exists
	   @return None if not found"""
	if self.D.has_key("tbres_max"):
	    return self.D["tbres_max"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        print " | Raster3d register table:.... " + str(self.get_raster3d_register())
        stds_raster_metadata_base.print_info(self)
        #      0123456789012345678901234567890
        print " | Top-bottom resolution min:.. " + str(self.get_ewres_min())
        print " | Top-bottom resolution max:.. " + str(self.get_ewres_max())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        stds_raster_metadata_base.print_shell_info(self)
        print "tbres_min=" + str(self.get_tbres_min())
        print "tbres_max=" + str(self.get_tbres_max())

###############################################################################

class stvds_metadata(stds_metadata_base):
    """This is the raster metadata class"""
    def __init__(self, ident=None, vector_register=None,  title=None, description=None):

	stds_metadata_base.__init__(self, "stvds_metadata", ident, title, description)

	self.set_vector_register(vector_register)

    def set_vector_register(self, vector_register):
	"""Set the vector map register table name"""
	self.D["vector_register"] = vector_register

    def get_vector_register(self):
	"""Get the vector map register table name
	   @return None if not found"""
	if self.D.has_key("vector_register"):
	    return self.D["vector_register"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        print " +-------------------- Metadata information ----------------------------------+"
        #      0123456789012345678901234567890
        print " | Vector register table:...... " + str(self.get_vector_register())
        stds_metadata_base.print_info(self)

    def print_shell_info(self):
        """Print information about this class in shell style"""
        stds_metadata_base.print_shell_info(self)
