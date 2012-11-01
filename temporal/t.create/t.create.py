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
#% description: Creates a space time dataset.
#% keywords: temporal
#% keywords: map management
#% keywords: create
#%end

#%option G_OPT_STDS_OUTPUT
#%end

#%option G_OPT_STDS_TYPE
#% description: The output type of the space time dataset
#%end

#%option G_OPT_T_TYPE
#%end

#%option
#% key: semantictype
#% type: string
#% description: Semantic type of the space time dataset
#% required: yes
#% multiple: no
#% options: min,max,sum,mean
#% answer: mean
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

    tgis.init()
    
    #Get the current mapset to create the id of the space time dataset

    mapset = grass.gisenv()["MAPSET"]
    id = name + "@" + mapset

    sp = tgis.dataset_factory(type, id)

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    if sp.is_in_db(dbif) and grass.overwrite() == False:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> is already in the database. "
                      "Use the overwrite flag.") %
                    (sp.get_new_map_instance(None).get_type(), name))

    if sp.is_in_db(dbif) and grass.overwrite() == True:
        grass.info(_("Overwrite space time %s dataset <%s> "
                     "and unregister all maps.") %
                   (sp.get_new_map_instance(None).get_type(), name))
        sp.delete(dbif)
        sp = sp.get_new_instance(id)

    grass.verbose(_("Create space time %s dataset.") %
                  sp.get_new_map_instance(None).get_type())

    sp.set_initial_values(temporal_type=temporaltype, semantic_type=semantic,
                          title=title, description=descr)
    sp.insert(dbif)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
