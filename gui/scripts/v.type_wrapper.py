#!/usr/bin/env python
############################################################################
#
# MODULE:       v.type_wrapper.py (v.type wrapper script)
#
# AUTHOR(S):    Hamish Bowman  (Otago University, New Zealand)
#               (original contributor, v.type.sh)
#               Pythonized by Martin Landa <landa.martin gmail.com> (2008/08)
#
# PURPOSE:      Supply v.type options in a GUI compatible way
#
# COPYRIGHT:    (C) 2007-2008 by Hamish Bowman, and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Change the type of geometry elements.
#% keywords: vector, geometry
#%end

#%option
#% key: input
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name of input vector map
#% gisprompt: old,vector,vector
#%end

#%option
#% key: output
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name for output vector map
#% gisprompt: new,vector,vector
#%end

#%option
#% key: type
#% type: string
#% required: no
#% multiple: no
#% options: point to centroid,point to kernel,centroid to point,centroid to kernel,kernel to point,kernel to centroid,line to boundary,line to face,boundary to line,boundary to face,face to line,face to boundary
#% description: Conversion
#% answer: boundary to line
#%end

import sys
from grass.script import core as grass

def main():
    if options['type'] == "point to centroid":
        type_cnv = "point,centroid"
    elif options['type'] == "point to kernel":
        type_cnv = "point,kernel"
    elif options['type'] == "centroid to point":
        type_cnv = "centroid,point"
    elif options['type'] == "centroid to kernel":
        type_cnv = "centroid,kernel"
    elif options['type'] == "kernel to point":
        type_cnv = "kernel,point"
    elif options['type'] == "kernel to centroid":
        type_cnv = "kernel,centroid"
    elif options['type'] == "line to boundary":
        type_cnv = "line,boundary"
    elif options['type'] == "line to face":
        type_cnv = "line,face"
    elif options['type'] == "boundary to line":
        type_cnv = "boundary,line"
    elif options['type'] == "boundary to face":
        type_cnv = "boundary,face"
    elif options['type'] == "face to line":
        type_cnv = "face,line"
    elif options['type'] == "face to boundary":
        type_cnv = "face,boundary"
    
    options.pop('type')
    grass.exec_command("v.type", type = type_cnv, **options)

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
