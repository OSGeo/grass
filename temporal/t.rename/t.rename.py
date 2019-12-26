#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rename
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Renames a space time dataset
# COPYRIGHT:    (C) 2011-2017 by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

#%module
#% description: Renames a space time dataset
#% keyword: temporal
#% keyword: map management
#% keyword: rename
#% keyword: time
#%end

#%option G_OPT_STDS_INPUT
#%end

#%option G_OPT_STDS_OUTPUT
#%end

#%option G_OPT_STDS_TYPE
#% guidependency: input
#% guisection: Required
#%end

import grass.script as grass

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    type = options["type"]

    # Make sure the temporal database exists
    tgis.init()

    #Get the current mapset to create the id of the space time dataset
    mapset = grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        old_id = input
    else:
        old_id = input + "@" + mapset

    if output.find("@") >= 0:
        new_id = output
    else:
        new_id = output + "@" + mapset
        
    # Do not overwrite yourself
    if new_id == old_id:
        return
        

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds = tgis.dataset_factory(type, old_id)

    if new_id.split("@")[1] != mapset:
        grass.fatal(_("Space time %s dataset <%s> can not be renamed. "
                      "Mapset of the new identifier differs from the current "
                      "mapset.") % (stds.get_new_map_instance(None).get_type(), 
                                    old_id))
        
    if stds.is_in_db(dbif=dbif) == False:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> not found") % (
            stds.get_new_map_instance(None).get_type(), old_id))

    # Check if the new id is in the database
    new_stds = tgis.dataset_factory(type, new_id)

    if new_stds.is_in_db(dbif=dbif) == True and grass.overwrite() == False:
        dbif.close()
        grass.fatal(_("Unable to rename Space time %s dataset <%s>. Name <%s> "
                      "is in use, please use the overwrite flag.") % (
            stds.get_new_map_instance(None).get_type(), old_id, new_id))
    
    # Remove an already existing space time dataset
    if new_stds.is_in_db(dbif=dbif) == True:
        new_stds.delete(dbif=dbif)
        
    stds.select(dbif=dbif)
    stds.rename(ident=new_id, dbif=dbif)
    stds.update_command_string(dbif=dbif)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
