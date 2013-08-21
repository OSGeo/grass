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
    mapset = core.gisenv()["MAPSET"]

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

def open_new_space_time_dataset(name, type, temporaltype, title, descr, semantic,
                              dbif=None, overwrite=False, dry=False):
    """!Create a new space time dataset of a specific type

       This function is sensitive to the settings in grass.core.overwrite to
       overwrite existing space time datasets.

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

    #Get the current mapset to create the id of the space time dataset

    mapset = core.gisenv()["MAPSET"]

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
        core.fatal(_("Unkown type: %s") % (type))

    dbif, connected = init_dbif(dbif)

    if sp.is_in_db(dbif) and overwrite is False:
        if connected:
            dbif.close()
        core.fatal(_("Space time %(sp)s dataset <%(name)s> is already in the"
                      " database. Use the overwrite flag.") % {
                      'sp': sp.get_new_map_instance(None).get_type(),
                      'name': name})
        return None

    if sp.is_in_db(dbif) and overwrite is True:
        core.warning(_("Overwrite space time %(sp)s dataset <%(name)s> and "
                       "unregister all maps.") % {
                       'sp': sp.get_new_map_instance(None).get_type(),
                       'name': name})
        if not dry:
            sp.delete(dbif)
        sp = sp.get_new_instance(id)

    core.verbose(_("Create new space time %s dataset.") %
                   sp.get_new_map_instance(None).get_type())

    sp.set_initial_values(temporal_type=temporaltype, semantic_type=semantic,
                          title=title, description=descr)
    if not dry:
        sp.insert(dbif)

    if connected:
        dbif.close()

    return sp
