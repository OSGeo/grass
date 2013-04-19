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

###############################################################################

def create_space_time_dataset(name, type, temporaltype, title, descr, semantic,
                              dbif=None, overwrite=False):
    """!Create a new space time dataset
    
       This function is sensitive to the settings in grass.core.overwrite to
       overwrute existing space time datasets.
    
       @param name The name of the new space time dataset
       @param type The type (strds, stvds, str3ds) of the new space time dataset
       @param temporaltype The temporal type (relative or absolute)
       @param title The title
       @param descr The dataset description
       @param semantic Semantical information
       @param dbif The temporal database interface to be used
       @param overwrite Flag to allow overwriting
       
       @return The new created space time dataset
       
       This function will raise a ScriptError in case of an error.
    """
    
    #Get the current mapset to create the id of the space time dataset

    mapset = core.gisenv()["MAPSET"]
    id = name + "@" + mapset

    sp = dataset_factory(type, id)

    dbif, connected = init_dbif(dbif)

    if sp.is_in_db(dbif) and overwrite == False:
        if connected:
            dbif.close()
        core.fatal(_("Space time %s dataset <%s> is already in the database. "
                      "Use the overwrite flag.") %
                    (sp.get_new_map_instance(None).get_type(), name))
        return None

    if sp.is_in_db(dbif) and overwrite == True:
        core.warning(_("Overwrite space time %s dataset <%s> "
                     "and unregister all maps.") %
                   (sp.get_new_map_instance(None).get_type(), name))
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
