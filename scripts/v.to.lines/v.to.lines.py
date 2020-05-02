#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       v.to.lines (former v.polytoline)
# AUTHOR(S):    Luca Delucchi
#               point support added by Markus Neteler
#
# PURPOSE:      Converts polygons and points to lines
# COPYRIGHT:    (C) 2013-2014 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
# TODO
#    support centroids (treat as points)?
#############################################################################

#%module
#% description: Converts vector polygons or points to lines.
#% keyword: vector
#% keyword: geometry
#% keyword: area
#% keyword: line
#% keyword: point
#%end

#%option G_OPT_V_INPUT
#%end

#%option G_OPT_V_OUTPUT
#%end

#%option
#% key: method
#% type: string
#% description: Method used for point interpolation
#% options: delaunay
#% answer: delaunay
#% guisection: Area
#%end

import grass.script as grass
from grass.script.utils import decode
from grass.exceptions import CalledModuleError
import os


def main():
    # Get the options
    input = options["input"]
    input_name = input.split('@')[0]
    output = options["output"]
    method = options["method"]
    min_cat = None
    max_cat = None
    point = None
    overwrite = grass.overwrite()

    quiet = True

    if grass.verbosity() > 2:
        quiet = False

    in_info = grass.vector_info(input)
    # check for wild mixture of vector types
    if in_info['points'] > 0 and in_info['boundaries'] > 0:
        grass.fatal(_("The input vector map contains both polygons and points,"
                      " cannot handle mixed types"))

    pid = os.getpid()
    # process points via triangulation, then exit
    if in_info['points'] > 0:
        point = True
        layer = 1  # hardcoded for now
        out_temp = '{inp}_point_tmp_{pid}'.format(inp=input_name, pid=pid)
        if method == 'delaunay':
            grass.message(_("Processing point data (%d points found)...") % in_info['points'])
            grass.run_command('v.delaunay', input=input, layer=layer,
                              output=out_temp, quiet=quiet)

        grass.run_command('v.db.addtable', map=out_temp, quiet=True)
        input = out_temp
        in_info = grass.vector_info(input)

    # process areas
    if in_info['areas'] == 0 and in_info['boundaries'] == 0:
        grass.fatal(_("The input vector map does not contain polygons"))

    out_type = '{inp}_type_{pid}'.format(inp=input_name, pid=pid)
    input_tmp = '{inp}_tmp_{pid}'.format(inp=input_name, pid=pid)
    remove_names = "%s,%s" % (out_type, input_tmp)
    grass.message(_("Processing area data (%d areas found)...") % in_info['areas'])

    try:
        grass.run_command('v.category', layer="2", type='boundary',
                          option='add', input=input, out=input_tmp,
                          quiet=quiet)
    except CalledModuleError:
        grass.run_command('g.remove', flags='f', type='vector',
                          name=input_tmp, quiet=quiet)
        grass.fatal(_("Error creating layer 2"))
    try:
        grass.run_command('v.db.addtable', map=input_tmp, layer="2",
                          quiet=quiet)
    except CalledModuleError:
        grass.run_command('g.remove', flags='f', type='vector',
                          name=input_tmp, quiet=quiet)
        grass.fatal(_("Error creating new table for layer 2"))
    try:
        grass.run_command('v.to.db', map=input_tmp, option="sides",
                          columns="left,right", layer="2", quiet=quiet)
    except CalledModuleError:
        grass.run_command('g.remove', flags='f', type='vector',
                          name=input_tmp, quiet=quiet)
        grass.fatal(_("Error populating new table for layer 2"))
    try:
        grass.run_command('v.type', input=input_tmp, output=out_type,
                          from_type='boundary', to_type='line',
                          quiet=quiet, layer="2")
    except CalledModuleError:
        grass.run_command('g.remove', flags='f', type='vector',
                          name=remove_names, quiet=quiet)
        grass.fatal(_("Error converting polygon to line"))
    report = grass.read_command('v.category', flags='g', input=out_type,
                                option='report', quiet=quiet)
    report = decode(report).split('\n')
    for r in report:
        if r.find('centroid') != -1:
            min_cat = report[0].split()[-2]
            max_cat = report[0].split()[-1]
            break
    if min_cat and max_cat:
        try:
            grass.run_command('v.edit', map=out_type, tool='delete',
                              type='centroid', layer=2, quiet=quiet,
                              cats='{mi}-{ma}'.format(mi=min_cat, ma=max_cat))
        except CalledModuleError:
            grass.run_command('g.remove', flags='f', type='vector',
                              name=remove_names, quiet=quiet)
            grass.fatal(_("Error removing centroids"))

    try:
        try:
            # TODO: fix magic numbers for layer here and there
            grass.run_command('v.db.droptable', map=out_type, layer=1,
                              flags='f', quiet=True)
        except CalledModuleError:
            grass.run_command('g.remove', flags='f', type='vector',
                              name=remove_names, quiet=quiet)
            grass.fatal(_("Error removing table from layer 1"))
    # TODO: when this except is happaning, it seems that never, so it seems wrong
    except:
        grass.warning(_("No table for layer %d" % 1))
    try:
        grass.run_command('v.category', input=out_type, option='transfer',
                          output=output, layer="2,1", quiet=quiet,
                          overwrite=overwrite)
    except CalledModuleError:
        grass.run_command('g.remove', flags='f', type='vector',
                          name=remove_names, quiet=quiet)
        grass.fatal(_("Error adding categories"))
    grass.run_command('g.remove', flags='f', type='vector',
                      name=remove_names, quiet=quiet)
    if point:
        grass.run_command('g.remove', flags='f', type='vector',
                          name=out_temp, quiet=quiet)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
