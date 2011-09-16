"""!@package grass.script.tgis_base

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS basic functions and classes to be used in other
Python temporal gis packages.

This packages includes all base classes to stor basic information like id, name,
mapset creation and modification time.

Usage:

@code
from grass.script import tgis_core as grass

rbase = grass.raster_base(ident="soil")
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from tgis_core import *

class dataset_base(sql_database_interface):
    """This is the base class for all maps and spacetime datasets storing basic information"""
    def __init__(self, table=None, ident=None, name=None, mapset=None, creator=None, ctime=None,\
		    mtime=None, ttype=None, revision=1):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_name(name)
	self.set_mapset(mapset)
	self.set_creator(creator)
	self.set_ctime(ctime)
	self.set_mtime(mtime)
	self.set_ttype(ttype)
	self.set_revision(revision)

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_name(self, name):
	"""Set the name of the map"""
	self.D["name"] = name

    def set_mapset(self, mapset):
	"""Set the mapset of the map"""
	self.D["mapset"] = mapset

    def set_creator(self, creator):
	"""Set the creator of the map"""
	self.D["creator"] = creator

    def set_ctime(self, ctime=None):
	"""Set the creation time of the map, if nothing set the current time is used"""
	if ctime == None:
            self.D["creation_time"] = datetime.now()
	else:
            self.D["creation_time"] = ctime

    def set_mtime(self, mtime=None):
	"""Set the modification time of the map, if nothing set the current time is used"""
	if mtime == None:
            self.D["modification_time"] = datetime.now()
	else:
            self.D["modification_time"] = mtime

    def set_ttype(self, ttype):
	"""Set the temporal type of the map: absolute or relative, if nothing set absolute time will assumed"""
	if ttype == None or (ttype != "absolute" and ttype != "relative"):
	    self.D["temporal_type"] = "absolute"
        else:
	    self.D["temporal_type"] = ttype

    def set_revision(self, revision=1):
	"""Set the revision of the map: if nothing set revision 1 will assumed"""
	self.D["revision"] = revision

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_name(self):
	"""Get the name of the map
	   @return None if not found"""
	if self.D.has_key("name"):
	    return self.D["name"]
        else:
	    return None

    def get_mapset(self):
	"""Get the mapset of the map
	   @return None if not found"""
	if self.D.has_key("mapset"):
	    return self.D["mapset"]
        else:
	    return None

    def get_creator(self):
	"""Get the creator of the map
	   @return None if not found"""
	if self.D.has_key("creator"):
	    return self.D["creator"]
        else:
	    return None

    def get_ctime(self):
	"""Get the creation time of the map, datatype is datetime
	   @return None if not found"""
	if self.D.has_key("creation_time"):
	    return self.D["creation_time"]
        else:
	    return None

    def get_mtime(self):
	"""Get the modification time of the map, datatype is datetime
	   @return None if not found"""
	if self.D.has_key("modification_time"):
	    return self.D["modification_time"]
        else:
	    return None

    def get_ttype(self):
	"""Get the temporal type of the map
	   @return None if not found"""
	if self.D.has_key("temporal_type"):
	    return self.D["temporal_type"]
        else:
	    return None

    def get_revision(self):
	"""Get the revision of the map
	   @return None if not found"""
	if self.D.has_key("revision"):
	    return self.D["revision"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print "  Id: ........................ " + str(self.get_id())
        print "  Name: ...................... " + str(self.get_name())
        print "  Mapset: .................... " + str(self.get_mapset())
        print "  Creator: ................... " + str(self.get_creator())
        print "  Creation time: ............. " + str(self.get_ctime())
        print "  Modification time: ......... " + str(self.get_mtime())
        print "  Temporal type: ............. " + str(self.get_ttype())
        print "  Revision in database: ...... " + str(self.get_revision())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "id=" + str(self.get_id())
        print "name=" + str(self.get_name())
        print "mapset=" + str(self.get_mapset())
        print "creator=" + str(self.get_creator())
        print "creation_time=" + str(self.get_ctime())
        print "modification_time=" + str(self.get_mtime())
        print "temporal_type=" + str(self.get_ttype())
        print "revision=" + str(self.get_revision())

###############################################################################

class raster_base(dataset_base):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, "raster_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

class raster3d_base(dataset_base):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, "raster3d_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

class vector_base(dataset_base):
    def __init__(self, ident=None, name=None, mapset=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, "vector_base", ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

###############################################################################

class stds_base(dataset_base):
    def __init__(self, table=None, ident=None, name=None, mapset=None, semantic_type=None, creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        dataset_base.__init__(self, table, ident, name, mapset, creator, creation_time,\
	            modification_time, temporal_type, revision)

	self.set_semantic_type(semantic_type)

    def set_semantic_type(self, semantic_type):
	"""Set the semantic type of the space time dataset"""
	self.D["semantic_type"] = semantic_type

    def get_semantic_type(self):
	"""Get the semantic type of the space time dataset
	   @return None if not found"""
	if self.D.has_key("semantic_type"):
	    return self.D["semantic_type"]
        else:
	    return None

###############################################################################

class strds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None, semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "strds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

class str3ds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None, semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "str3ds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

class stvds_base(stds_base):
    def __init__(self, ident=None, name=None, mapset=None, semantic_type=None,  creator=None, creation_time=None,\
		    modification_time=None, temporal_type=None, revision=1):
        stds_base.__init__(self, "stvds_base", ident, name, mapset, semantic_type, creator, creation_time,\
	            modification_time, temporal_type, revision)

