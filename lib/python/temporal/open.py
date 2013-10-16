"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.

Usage:

@code
import grass.temporal as tgis

tgis.register_maps_in_space_time_dataset(type, name, maps)

...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *
from factory import *

###############################################################################

def open_old_space_time_dataset(name, type, dbif=None):
    """!This function opens an existing space time dataset and return the
       created and intialized object of the specified type.

       This function will raise a ScriptError in case the type is wrong,
       or the space time dataset was not found.

       @param name The name of the space time dataset, if the name does not
                    contain the mapset (name@mapset) then the current mapset
                    will be used to identifiy the space time dataset
       @param type The type of the space time dataset (strd, str3ds, stvds,
                                                       raster, vector, raster3d)
       @param dbif The optional database interface to be used

    """
    mapset = get_current_mapset()

    # Check if the dataset name contains the mapset as well
    if name.find("@") < 0:
        id = name + "@" + mapset
    else:
        id = name

    if type == "strds" or type == "rast" or type == "raster":
        sp = dataset_factory("strds", id)
    elif type == "str3ds" or type == "rast3d" or type == "raster3d":
        sp = dataset_factory("str3ds", id)
    elif type == "stvds" or type == "vect" or type == "vector":
        sp = dataset_factory("stvds", id)
    else:
        core.fatal(_("Unkown type: %s") % (type))

    dbif, connected = init_dbif(dbif)

    if not sp.is_in_db(dbif):
        dbif.close()
        core.fatal(_("Space time %(sp)s dataset <%(name)s> no found") %
                     {'sp': sp.get_new_map_instance(None).get_type(),
                      'name': name})

    # Read content from temporal database
    sp.select(dbif)
    if connected:
        dbif.close()

    return sp

###############################################################################

def check_new_space_time_dataset(name, type, dbif=None, overwrite=False):
    """!Check if a new space time dataset of a specific type can be created

       @param name The name of the new space time dataset
       @param type The type of the new space time dataset (strd, str3ds, stvds,
                                                      raster, vector, raster3d)
       @param dbif The temporal database interface to be used
       @param overwrite Flag to allow overwriting

       @return A space time dataset object that must be filled with
               content before insertion in the temporal database

       This function will raise a ScriptError in case of an error.
    """

    #Get the current mapset to create the id of the space time dataset

    mapset = get_current_mapset()

    if name.find("@") < 0:
        id = name + "@" + mapset
    else:
        n, m = name.split("@")
        if mapset != m:
            core.fatal(_("Space time datasets can only be created in the "
                         "current mapset"))
        id = name

    if type == "strds" or type == "rast" or type == "raster":
        sp = dataset_factory("strds", id)
    elif type == "str3ds" or type == "rast3d" or type == "raster3d":
        sp = dataset_factory("str3ds", id)
    elif type == "stvds" or type == "vect" or type == "vector":
        sp = dataset_factory("stvds", id)
    else:
        core.error(_("Unkown type: %s") % (type))
        return None

    dbif, connected = init_dbif(dbif)

    if sp.is_in_db(dbif) and overwrite is False:
        core.fatal(_("Space time %(sp)s dataset <%(name)s> is already in the"
                      " database. Use the overwrite flag.") % {
                      'sp': sp.get_new_map_instance(None).get_type(),
                      'name': name})
    if connected:
        dbif.close()

    return sp

###############################################################################

def open_new_space_time_dataset(name, type, temporaltype, title, descr, semantic,
                              dbif=None, overwrite=False):
    """!Create a new space time dataset of a specific type

       @param name The name of the new space time dataset
       @param type The type of the new space time dataset (strd, str3ds, stvds,
                                                      raster, vector, raster3d)
       @param temporaltype The temporal type (relative or absolute)
       @param title The title
       @param descr The dataset description
       @param semantic Semantical information
       @param dbif The temporal database interface to be used
       @param overwrite Flag to allow overwriting
       @param dry Do not create the space time dataset in the temporal database,
                  make a dry run with including all checks

       @return The new created space time dataset

       This function will raise a ScriptError in case of an error.
    """
    dbif, connected = init_dbif(dbif)
    sp =  check_new_space_time_dataset(name, type, dbif, overwrite)

    if sp.is_in_db(dbif):
        core.warning(_("Overwrite space time %(sp)s dataset <%(name)s> and "
                       "unregister all maps.") % {
                       'sp': sp.get_new_map_instance(None).get_type(),
                       'name': name})
        id = sp.get_id()
        sp.delete(dbif)
        sp = sp.get_new_instance(id)

    core.verbose(_("Create new space time %s dataset.") %
                   sp.get_new_map_instance(None).get_type())

    sp.set_initial_values(temporal_type=temporaltype, semantic_type=semantic,
                          title=title, description=descr)

    sp.insert(dbif)

    if connected:
        dbif.close()

    return sp

############################################################################

def check_new_map_dataset(name, layer=None, type="raster", 
                          overwrite=False, dbif=None):
    """!Check if a new map dataset of a specific type can be created in
        the temporal database

       @param name The name of the new map dataset
       @param layer The layer of the new map dataset
       @param type The type of the new map dataset (raster, vector, raster3d)
       @param dbif The temporal database interface to be used
       @param overwrite Flag to allow overwriting

       @return A map dataset object

       This function will raise a ScriptError in case of an error.
    """
    mapset = get_current_mapset()

    dbif, connected = init_dbif(dbif)
    map_id = AbstractMapDataset.build_id(name, mapset, layer)

    new_map = dataset_factory(type, map_id)
    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        if not overwrite:
            if connected:
                dbif.close()
            core.fatal(_("Map <%s> is already in temporal database,"
                         " use overwrite flag to overwrite") % (map_id))

    if connected:
        dbif.close()

    return new_map

############################################################################

def open_new_map_dataset(name, layer=None, type="raster",
                         temporal_extent=None, overwrite=False,
                         dbif=None):
    """!Create a new map dataset object of a specific type that can be
        registered in the temporal database

       @param name The name of the new map dataset
       @param layer The layer of the new map dataset
       @param type The type of the new map dataset (raster, vector, raster3d)
       @param dbif The temporal database interface to be used
       @param overwrite Flag to allow overwriting

       @return A map dataset object

       This function will raise a ScriptError in case of an error.
    """

    mapset = get_current_mapset()

    dbif, connected = init_dbif(dbif)
    new_map = check_new_map_dataset(name, layer, "raster", overwrite, dbif)

    # Check if new map is in the temporal database
    if new_map.is_in_db(dbif):
        # Remove the existing temporal database entry
        map_id = new_map.get_id()
        new_map.delete(dbif)
        new_map = new_map.get_new_instance(map_id)

    if temporal_extent:
        new_map.set_temporal_extent(temporal_extent)

    if connected:
        dbif.close()

    return new_map
