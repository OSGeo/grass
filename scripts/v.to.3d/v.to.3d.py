#!/usr/bin/env python
############################################################################
#
# MODULE:       v.to.3d
#
# AUTHOR:       Martin Landa, CTU in Prague, Czech Republic
#
# PURPOSE:      Transform 2D vectors to 3D vectors
#                 (frontend to v.transform)
#
# COPYRIGHT:    (c) 2008 Martin Landa, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#% description: Performs transformation of 2D vector features to 3D.
#% keywords: vector, transformation, 3D
#%End

#%Option
#% key: input
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name of input vector map
#% gisprompt: old,vector,vector
#%End

#%Option
#% key: output
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name for output vector map
#% gisprompt: new,vector,vector
#%End

#%Option
#% key: height
#% type: double
#% required: no
#% multiple: no
#% description: Fixed height for 3D vector features
#%End

#%Option
#% key: layer
#% type: integer
#% required: no
#% multiple: no
#% label: Layer number
#% description: A single vector map can be connected to multiple database tables. This number determines which table to use.
#% answer: 1
#% gisprompt: old_layer,layer,layer
#%End

#%Option
#% key: column
#% type: string
#% required: no
#% multiple: no
#% key_desc: name
#% description: Name of attribute column used for height
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#%End

import sys
import os
import grass

def main():
    input = options['input']
    output = options['output']
    layer = options['layer']
    column = options['column']
    height = options['height']
    
    map3d = int(grass.parse_key_val(grass.read_command('v.info', map = input, flags = 't'))['map3d'])
    
    if map3d == 1:
        grass.fatal("Vector map <%s> is 3D" % input)
        
    if height:
        if column:
            grass.fatal("Either 'height' or 'column' parameter have to be used")
        # fixed height
        grass.run_command('v.transform', input = input, output = output,
                          zshift = height)
    else:
        if not column:
            grass.fatal("Either 'height' or 'column' parameter have to be used")
        # attribute height, check column type
        try:
            coltype = grass.vector_columns(map = input, layer = layer)[column]
        except KeyError:
            grass.fatal("Column <%s> not found" % column)
    
        if coltype not in ('INTEGER',
                           'DOUBLE PRECISION'):
            grass.fatal("Column type must be numeric")
        
        table = grass.vector_db(map = input, layer = layer)[1]
        
        columns = "zshift:%s" % column
        grass.run_command('v.transform', input = input, output = output,
                          layer = layer, columns = columns,
                          table = table)
    
    return 0
                             
if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
