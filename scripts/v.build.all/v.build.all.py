#!/usr/bin/env python3
############################################################################
#
# MODULE:	v.build.all
# AUTHOR(S):	Glynn Clements, Radim Blazek
# PURPOSE:	Build all vectors in current mapset
# COPYRIGHT:	(C) 2004, 2008-2009 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Rebuilds topology on all vector maps in the current mapset.
#% keyword: vector
#% keyword: topology
#%end

import sys
from grass.script import core as grass
from grass.exceptions import CalledModuleError


def main():
    env = grass.gisenv()
    mapset = env['MAPSET']
    ret = 0

    vectors = grass.list_grouped('vect')[mapset]
    num_vectors = len(vectors)

    if grass.verbosity() < 2:
        quiet = True
    else:
        quiet = False

    i = 1
    for vect in vectors:
        map = "%s@%s" % (vect, mapset)
        grass.message(_("%s\nBuilding topology for vector map <%s> (%d of %d)...\n%s") %
                      ('-' * 80, map, i, num_vectors, '-' * 80))
        grass.verbose(_("v.build map=%s") % map)
        try:
            grass.run_command("v.build", map=map, quiet=quiet)
        except CalledModuleError:
            grass.error(_("Building topology for vector map <%s> failed") % map)
            ret = 1
        i += 1

    return ret


if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
