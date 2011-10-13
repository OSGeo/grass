#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.support
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Modify the metadata of a space time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Modify the metadata of a space time dataset
#% keywords: spacetime
#% keywords: dataset
#% keywords: create
#%end

#%option
#% key: input
#% type: string
#% description: Name of the space time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: granularity
#% type: string
#% description: The granularity of the space time dataset (NNN day, NNN week, NNN month)
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
#% key: title
#% type: string
#% description: Title of the space time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: description
#% type: string
#% description: Description of the space time dataset
#% required: yes
#% multiple: no
#%end

#%flag
#% key: u
#% description: Update metadata information, temporal and spatial extent from registered maps
#%end

import grass.temporal as tgis
import grass.script as grass

############################################################################

def main():

    # Get the options
    name = options["input"]
    type = options["type"]
    title = options["title"]
    descr = options["description"]
    semantic = options["semantictype"]
    gran = options["granularity"]
    update = flags["u"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    #Get the current mapset to create the id of the space time dataset
    mapset =  grass.gisenv()["MAPSET"]
    id = name + "@" + mapset

    if type == "strds":
        sp = tgis.space_time_raster_dataset(id)
    if type == "str3ds":
        sp = tgis.space_time_raster3d_dataset(id)
    if type == "stvds":
        sp = tgis.space_time_vector_dataset(id)

    if sp.is_in_db() == False:
        grass.fatal(_("%s dataset <%s> not found in temporal database") % (ds.get_type(), name))

    sp.select()
    # Temporal type can not be changed
    ttype= sp.get_temporal_type()
    sp.set_initial_values(granularity=gran, temporal_type=ttype, semantic_type=semantic, title=title, description=descr)
    # Update only non-null entries
    sp.update()

    if update:
        sp.update_from_registered_maps()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

