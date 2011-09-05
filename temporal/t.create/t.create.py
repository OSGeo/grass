#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.create
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Create a space-time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Create a space-time dataset
#% keywords: spacetime dataset
#% keywords: create
#%end

#%option
#% key: name
#% type: string
#% description: Name of the new space-time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: granularity
#% type: string
#% description: The granularity of the new space-time dataset (NNN day, NNN week, NNN month)
#% required: yes
#% multiple: no
#%end

#%option
#% key: semantictype
#% type: string
#% description: The semantic type of the space-time dataset
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
#% options: strds
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
#% description: Title of the new space-time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: description
#% type: string
#% description: Description of the new space-time dataset
#% required: yes
#% multiple: no
#%end

import sys
import os
import getpass
import subprocess
import grass.script as grass
############################################################################

def main():

    # Get the options
    name = options["name"]
    type = options["type"]
    temporaltype = options["temporaltype"]
    title = options["title"]
    descr = options["description"]
    semantic = options["semantictype"]
    gran = options["granularity"]

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

