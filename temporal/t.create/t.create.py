#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.create
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Create a space time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Create a space time dataset
#% keywords: spacetime
#% keywords: dataset
#% keywords: create
#%end

#%option
#% key: output
#% type: string
#% description: Name of the new space time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: semantictype
#% type: string
#% description: The semantic type of the space time dataset
#% required: yes
#% multiple: no
#% options: event, const, continuous
#% answer: event
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is strds
#% required: no
#% options: strds, str3ds, stvds
#% answer: strds
#%end

#%option
#% key: temporaltype
#% type: string
#% description: The temporal type of the space time dataset, default is absolute
#% required: no
#% options: absolute,relative
#% answer: absolute
#%end

#%option
#% key: title
#% type: string
#% description: Title of the new space time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: description
#% type: string
#% description: Description of the new space time dataset
#% required: yes
#% multiple: no
#%end

import grass.temporal as tgis
import grass.script as grass

############################################################################

def main():

    # Get the options
    name = options["output"]
    type = options["type"]
    temporaltype = options["temporaltype"]
    title = options["title"]
    descr = options["description"]
    semantic = options["semantictype"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    #Get the current mapset to create the id of the space time dataset

    mapset =  grass.gisenv()["MAPSET"]
    id = name + "@" + mapset

    sp = tgis.dataset_factory(type, id)

    dbif = tgis.sql_database_interface()
    dbif.connect()

    if sp.is_in_db(dbif) and grass.overwrite() == False:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> is already in the database. Use the overwrite flag.") % name)

    if sp.is_in_db(dbif) and grass.overwrite() == True:
        grass.info(_("Overwrite space time %s dataset <%s> and unregister all maps.") % (sp.get_new_map_instance(None).get_type(), name))
        sp.delete(dbif)
        sp = sp.get_new_instance(id)

    grass.verbose(_("Create space time %s dataset.") % sp.get_new_map_instance(None).get_type())

    sp.set_initial_values(temporal_type=temporaltype, semantic_type=semantic, title=title, description=descr)
    sp.insert(dbif)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

