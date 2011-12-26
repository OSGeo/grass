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
#% label: Rebuilds all locally installed GRASS Addons extensions.
#% description: By default only extensions built against different GIS Library are rebuilt.
#% keywords: general
#% keywords: installation
#% keywords: extensions
#%end
#%flag
#% key: f
#% description: Force to rebuild all extensions
#% end

import os
import sys

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

import grass.script as grass

def get_extensions():
    addon_base = os.getenv('GRASS_ADDON_BASE')
    if not addon_base:
        grass.fatal(_("%s not defined") % "GRASS_ADDON_BASE")
    fXML = os.path.join(addon_base, 'modules.xml')
    if not os.path.exists(fXML):
        return []

    # read XML file
    fo = open(fXML, 'r')
    try:
        tree = etree.fromstring(fo.read())
    except StandardError, e:
        grass.error(_("Unable to parse metadata file: %s") % e)
        fo.close()
        return []
    
    fo.close()
    
    libgis_rev = grass.version()['libgis_revision']
    ret = list()
    for tnode in tree.findall('task'):
        gnode = tnode.find('libgis')
        if gnode is not None and \
                gnode.get('revision', '') != libgis_rev:
            ret.append(tnode.get('name'))
    
    return ret

def main():
    if flags['f']:
        extensions = grass.read_command('g.extension',
                                        quiet = True, flags = 'a').splitlines()
    else:
        extensions = get_extensions()
    
    if not extensions:
        grass.info(_("Nothing to rebuild."))
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
