#!/usr/bin/env python3

############################################################################
#
# MODULE:       g.extension.all
#
# AUTHOR(S):    Martin Landa <landa.martin gmail.com>
#
# PURPOSE:      Rebuilds or removes locally installed GRASS Addons extensions
#
# COPYRIGHT:    (C) 2011-2013 by Martin Landa, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% label: Rebuilds or removes all locally installed GRASS Addons extensions.
#% description: By default only extensions built against different GIS Library are rebuilt.
#% keyword: general
#% keyword: installation
#% keyword: extensions
#%end
#%option
#% key: operation
#% type: string
#% description: Operation to be performed
#% required: no
#% options: rebuild,remove
#% answer: rebuild
#%end
#%flag
#% key: f
#% label: Force operation (required for removal)
#% end
from __future__ import print_function
import os
import sys

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree  # Python <= 2.4

import grass.script as gscript
from grass.exceptions import CalledModuleError


def get_extensions():
    addon_base = os.getenv('GRASS_ADDON_BASE')
    if not addon_base:
        gscript.fatal(_("%s not defined") % "GRASS_ADDON_BASE")
    fXML = os.path.join(addon_base, 'modules.xml')
    if not os.path.exists(fXML):
        return []

    # read XML file
    fo = open(fXML, 'r')
    try:
        tree = etree.fromstring(fo.read())
    except Exception as e:
        gscript.error(_("Unable to parse metadata file: %s") % e)
        fo.close()
        return []

    fo.close()

    libgis_rev = gscript.version()['libgis_revision']
    ret = list()
    for tnode in tree.findall('task'):
        gnode = tnode.find('libgis')
        if gnode is not None and \
                gnode.get('revision', '') != libgis_rev:
            ret.append(tnode.get('name'))

    return ret


def main():
    remove = options['operation'] == 'remove'
    if remove or flags['f']:
        extensions = gscript.read_command(
            'g.extension',
            quiet=True,
            flags='a').splitlines()
    else:
        extensions = get_extensions()

    if not extensions:
        if remove:
            gscript.info(_("No extension found. Nothing to remove."))
        else:
            gscript.info(
                _("Nothing to rebuild. Rebuilding process can be forced with -f flag."))
        return 0

    if remove and not flags['f']:
        gscript.message(_("List of extensions to be removed:"))
        print(os.linesep.join(extensions))
        gscript.message(
            _("You must use the force flag (-f) to actually remove them. Exiting."))
        return 0

    for ext in extensions:
        gscript.message('-' * 60)
        if remove:
            gscript.message(_("Removing extension <%s>...") % ext)
        else:
            gscript.message(_("Reinstalling extension <%s>...") % ext)
        gscript.message('-' * 60)
        if remove:
            operation = 'remove'
            operation_flags = 'f'
        else:
            operation = 'add'
            operation_flags = ''
        try:
            gscript.run_command('g.extension', flags=operation_flags,
                                extension=ext, operation=operation)
        except CalledModuleError:
            gscript.error(_("Unable to process extension:%s") % ext)

    return 0

if __name__ == "__main__":
    options, flags = gscript.parser()
    sys.exit(main())
