"""
@brief Support script for wxGUI - only for developers needs. Updates
menudata.xml file.

Parse all GRASS modules in the search path ('bin' & 'script') and
updates: - description (i.e. help) - keywords

Prints warning for missing modules.

(C) 2008-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

Usage: python update_menudata.py

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

from grass.script import core as grass

def parseModules():
    """!Parse modules' interface"""
    modules = dict()
    
    # list of modules to be ignored
    ignore =  [ 'g.mapsets_picker.py',
                'v.type_wrapper.py',
                'g.parser',
                'vcolors' ]
    
    count = len(globalvar.grassCmd['all'])
    i = 0
    for module in globalvar.grassCmd['all']:
        i += 1
        if i % 10 == 0:
            grass.info('* %d/%d' % (i, count))
        if module in ignore:
            continue
        try:
            interface = menuform.GUI().ParseInterface(cmd = [module])
        except IOError, e:
            grass.error(e)
            continue
        modules[interface.name] = { 'label'   : interface.label,
                                    'desc'    : interface.description,
                                    'keywords': interface.keywords }
    
    return modules

def updateData(data, modules):
    """!Update menu data tree"""
    # list of modules to be ignored
    ignore =  [ 'v.type_wrapper.py',
                'vcolors' ]
    
    menu_modules = []
    for node in data.tree.getiterator():
        if node.tag != 'menuitem':
            continue

        item = dict()
        for child in node.getchildren():
            item[child.tag] = child.text
        
        if not item.has_key('command'):
            continue

        if item['command'] in ignore:
            continue
        
        module = item['command'].split(' ')[0]
        if not modules.has_key(module):
            grass.warning("'%s' not found in modules" % item['command'])
            continue
        
        if modules[module]['label']:
            desc = modules[module]['label']
        else:
            desc = modules[module]['desc']
        node.find('help').text = desc

        if not modules[module].has_key('keywords'):
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
    
def writeData(data):
    """!Write updated menudata.xml"""
    file = os.path.join('..', 'xml', 'menudata.xml')
    try:
        data.tree.write(file)
    except IOError:
        print >> sys.stderr, "'menudata.xml' not found. Please run the script from 'gui/wxpython/support'."
        

def main(argv = None):
    if argv is None:
        argv = sys.argv

    if len(argv) != 1:
        print >> sys.stderr, __doc__
        return 1
    
    grass.info("Step 1: parsing modules...")
    modules = dict()
    modules = parseModules()
    grass.info("Step 2: reading menu data...")
    data = menudata.Data()
    grass.info("Step 3: updating menu data...")
    updateData(data, modules)
    grass.info("Step 4: writing menu data (menudata.xml)...")
    writeData(data)
    
    return 0

if __name__ == '__main__':
    if os.getenv("GISBASE") is None:
        print >> sys.stderr, "You must be in GRASS GIS to run this program."
        sys.exit(1)
    
    sys.path.append(os.path.join(os.getenv("GISBASE"), 'etc', 'wxpython', 'gui_modules'))
    import menudata
    import menuform
    import globalvar
    
    sys.exit(main())
