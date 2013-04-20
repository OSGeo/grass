"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in Python scripts.


(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from space_time_datasets import *
from factory import *

###############################################################################

def tlist_grouped(type, group_type = False):
    """!List of temporal elements grouped by mapsets.

    Returns a dictionary where the keys are mapset 
    names and the values are lists of space time datasets in that
    mapset. Example:

    @code
    >>> tgis.tlist_grouped('strds')['PERMANENT']
    ['precipitation', 'temperature']
    @endcode
    
    @param type element type (strds, str3ds, stvds)
    @param group_type TBD

    @return directory of mapsets/elements
    """
    result = {}
    
    mapset = None
    if type == 'stds':
        types = ['strds', 'str3ds', 'stvds']
    else:
        types = [type]
    for type in types:
        try:
            tlist_result = tlist(type)
        except core.ScriptError, e:
            warning(e)
            continue

        for line in tlist_result:
            try:
                name, mapset = line.split('@')
            except ValueError:
                warning(_("Invalid element '%s'") % line)
                continue

            if mapset not in result:
                if group_type:
                    result[mapset] = {}
                else:
                    result[mapset] = []

            if group_type:
                if type in result[mapset]:
                    result[mapset][type].append(name)
                else:        
                    result[mapset][type] = [name, ]
            else:
                result[mapset].append(name)

    return result

###############################################################################

def tlist(type):
    """!Return a list of space time datasets of absolute and relative time
     
    @param type element type (strds, str3ds, stvds)

    @return a list of space time dataset ids
    """
    id = None
    sp = dataset_factory(type, id)

    dbif = SQLDatabaseInterfaceConnection()
    dbif.connect()

    output = []
    temporal_type = ["absolute", 'relative']
    for type in temporal_type:
        # Table name
        if type == "absolute":
            table = sp.get_type() + "_view_abs_time"
        else:
            table = sp.get_type() + "_view_rel_time"

        # Create the sql selection statement
        sql = "SELECT id FROM " + table
        sql += " ORDER BY id"

        dbif.cursor.execute(sql)
        rows = dbif.cursor.fetchall()

        # Append the ids of the space time datasets
        for row in rows:
            for col in row:
                output.append(str(col))
    dbif.close()

    return output
