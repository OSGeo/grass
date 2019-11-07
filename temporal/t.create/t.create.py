#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.create
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Create a space time dataset
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
#% description: Creates a space time dataset.
#% keyword: temporal
#% keyword: map management
#% keyword: create
#% keyword: time
#%end

#%option G_OPT_STDS_OUTPUT
#%end

#%option G_OPT_STDS_TYPE
#% description: Type of the output space time dataset
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

import grass.script as grass

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    name = options["output"]
    type = options["type"]
    temporaltype = options["temporaltype"]
    title = options["title"]
    descr = options["description"]
    semantic = options["semantictype"]

    # Make sure the temporal database exists
    tgis.init()
    
    tgis.open_new_stds(name, type, temporaltype, title, descr, 
                                     semantic, None, grass.overwrite())

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
