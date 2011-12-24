#!/usr/bin/env python

############################################################################
#
# MODULE:       g.extension.rebuild.all
#
# AUTHOR(S):   	Martin Landa <landa.martin gmail.com>
#
# PURPOSE:      Rebuild locally installed GRASS Addons extensions 
#
# COPYRIGHT:    (C) 2011 by Martin Landa, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Rebuilds all locally installed GRASS Addons extensions.
#% keywords: general
#% keywords: installation
#% keywords: extensions
#%end

import sys

import grass.script as grass

def main():
    extensions = grass.read_command('g.extension',
                                    quiet = True, flags = 'a').splitlines()
    if not extensions:
        grass.info(_("No extension installed. Nothing to rebuild."))
        return 0
    
    for ext in extensions:
        grass.message('-' * 60)
        grass.message(_("Reinstalling extension <%s>...") % ext)
        grass.message('-' * 60)
        grass.run_command('g.extension',
                          extension = ext)
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
