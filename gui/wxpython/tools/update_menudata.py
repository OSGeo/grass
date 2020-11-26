"""
@brief Support script for wxGUI - only for developers needs. Updates
menudata.xml file.

Parse all GRASS modules in the search path ('bin' & 'script') and
updates: - description (i.e. help) - keywords

Prints warning for missing modules.

(C) 2008-2010 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

Usage: python support/update_menudata.py [-d]

 -d - dry run (prints diff, file is not updated)

@author Martin Landa <landa.martin gmail.com>
"""

from __future__ import print_function

import os
import sys
import tempfile
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree  # Python <= 2.4

from grass.script import core as grass
from grass.script import task as gtask

from lmgr.menudata import LayerManagerMenuData
from core.globalvar import grassCmd


def parseModules():
    """Parse modules' interface"""
    modules = dict()

    # list of modules to be ignored
    ignore = ['g.mapsets_picker.py',
              'v.type_wrapper.py',
              'g.parser',
              'vcolors']

    count = len(grassCmd)
    i = 0
    for module in grassCmd:
        i += 1
        if i % 10 == 0:
            grass.info('* %d/%d' % (i, count))
        if module in ignore:
            continue
        try:
            interface = gtask.parse_interface(module)
        except Exception as e:
            grass.error(module + ': ' + str(e))
            continue
        modules[interface.name] = {'label': interface.label,
                                   'desc': interface.description,
                                   'keywords': interface.keywords}

    return modules


def updateData(data, modules):
    """Update menu data tree"""
    # list of modules to be ignored
    ignore = ['v.type_wrapper.py',
              'vcolors']

    menu_modules = list()
    for node in data.tree.iter():
        if node.tag != 'menuitem':
            continue

        item = dict()
        for child in node:
            item[child.tag] = child.text

        if 'command' not in item:
            continue

        if item['command'] in ignore:
            continue

        module = item['command'].split(' ')[0]
        if module not in modules:
            grass.warning("'%s' not found in modules" % item['command'])
            continue

        if modules[module]['label']:
            desc = modules[module]['label']
        else:
            desc = modules[module]['desc']
        if node.find('handler').text == 'OnMenuCmd':
            node.find('help').text = desc

        if 'keywords' not in modules[module]:
            grass.warning('%s: keywords missing' % module)
        else:
            if node.find('keywords') is None:
                node.insert(2, etree.Element('keywords'))
                grass.warning("Adding tag 'keywords' to '%s'" % module)
            node.find('keywords').text = ','.join(modules[module]['keywords'])

        menu_modules.append(item['command'])

    for module in modules.keys():
        if module not in menu_modules:
            grass.warning("'%s' not available from the menu" % module)


def writeData(data, file=None):
    """Write updated menudata.xml"""
    if file is None:
        file = os.path.join('xml', 'menudata.xml')

    try:
        data.tree.write(file)
    except IOError:
        print("'%s' not found."
              " Please run the script from 'gui/wxpython'." % file,
              file=sys.stderr)
        return

    try:
        f = open(file, 'a')
        try:
            f.write('\n')
        finally:
            f.close()
    except IOError:
        print("ERROR: Unable to write to menudata file.",
              file=sys.stderr)


def main(argv=None):
    if argv is None:
        argv = sys.argv

    if len(argv) > 1 and argv[1] == '-d':
        printDiff = True
    else:
        printDiff = False

    if len(argv) > 1 and argv[1] == '-h':
        print(sys.stderr, __doc__, file=sys.stderr)
        return 1

    nuldev = open(os.devnull, 'w+')
    grass.info("Step 1: running make...")
    grass.call(['make'], stderr=nuldev)
    grass.info("Step 2: parsing modules...")
    modules = dict()
    modules = parseModules()
    grass.info("Step 3: reading menu data...")
    data = LayerManagerMenuData()
    grass.info("Step 4: updating menu data...")
    updateData(data, modules)

    if printDiff:
        tempFile = tempfile.NamedTemporaryFile()
        grass.info("Step 5: diff menu data...")
        writeData(data, tempFile.name)

        grass.call(['diff', '-u',
                    os.path.join('xml', 'menudata.xml'),
                    tempFile.name], stderr=nuldev)
    else:
        grass.info("Step 5: writing menu data (menudata.xml)...")
        writeData(data)

    return 0

if __name__ == '__main__':
    if os.getenv("GISBASE") is None:
        sys.exit("You must be in GRASS GIS to run this program.")

    sys.exit(main())
