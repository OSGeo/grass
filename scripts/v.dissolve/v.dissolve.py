#!/usr/bin/env python3
############################################################################
#
# MODULE:       v.dissolve
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago University,
#                 New Zealand
#               Markus Neteler for column support
#               Converted to Python by Glynn Clements
# PURPOSE:      Dissolve common boundaries between areas with common cat
#                 (frontend to v.extract -d)
# COPYRIGHT:    (c) 2006-2014 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Dissolves boundaries between adjacent areas sharing a common category number or attribute.
#% keyword: vector
#% keyword: dissolve
#% keyword: area
#% keyword: line
#%end
#%option G_OPT_V_INPUT
#%end
#%option G_OPT_V_FIELD
#% label: Layer number or name.
#% required: no
#%end
#%option G_OPT_DB_COLUMN
#% description: Name of attribute column used to dissolve common boundaries
#%end
#%option G_OPT_V_OUTPUT
#%end
import sys
import os
import atexit

import grass.script as grass
from grass.exceptions import CalledModuleError


def cleanup():
    nuldev = open(os.devnull, 'w')
    grass.run_command(
        'g.remove', flags='f', type='vector', name='%s_%s' %
        (output, tmp), quiet=True, stderr=nuldev)


def main():
    global output, tmp

    input = options['input']
    output = options['output']
    layer = options['layer']
    column = options['column']

    # setup temporary file
    tmp = str(os.getpid())

    # does map exist?
    if not grass.find_file(input, element='vector')['file']:
        grass.fatal(_("Vector map <%s> not found") % input)

    if not column:
        grass.warning(
            _("No '%s' option specified. Dissolving based on category values from layer <%s>.") %
            ("column", layer))
        grass.run_command('v.extract', flags='d', input=input,
                          output=output, type='area', layer=layer)
    else:
        if int(layer) == -1:
            grass.warning(_("Invalid layer number (%d). "
                            "Parameter '%s' specified, assuming layer '1'.") %
                          (int(layer), 'column'))
            layer = '1'
        try:
            coltype = grass.vector_columns(input, layer)[column]
        except KeyError:
            grass.fatal(_('Column <%s> not found') % column)

        if coltype['type'] not in ('INTEGER', 'SMALLINT', 'CHARACTER', 'TEXT'):
            grass.fatal(_("Key column must be of type integer or string"))

        f = grass.vector_layer_db(input, layer)

        table = f['table']

        tmpfile = '%s_%s' % (output, tmp)

        try:
            grass.run_command('v.reclass', input=input, output=tmpfile,
                              layer=layer, column=column)
            grass.run_command('v.extract', flags='d', input=tmpfile,
                              output=output, type='area', layer=layer)
        except CalledModuleError as e:
            grass.fatal(_("Final extraction steps failed."
                          " Check above error messages and"
                          " see following details:\n%s") % e)

    # write cmd history:
    grass.vector_history(output)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
