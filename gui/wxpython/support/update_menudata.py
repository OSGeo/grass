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

def parseModules():
    """!Parse modules' interface"""
    modules = dict()
    
    # list of modules to be ignored
    ignore =  [ 'g.mapsets_picker.py',
                'v.type_wrapper.py' ]
    
    count = len(globalvar.grassCmd['all'])
    i = 0
    for module in globalvar.grassCmd['all']:
        i += 1
        if i % 10 == 0:
            print '   %d/%d' % (i, count)
        if module in ignore:
            continue
        try:
            interface = menuform.GUI().ParseInterface(cmd = [module])
        except IOError, e:
            print >> sys.stderr, e
            continue
        modules[interface.name] = { 'label'   : interface.label,
                                    'desc'    : interface.description,
                                    'keywords': interface.keywords }
    
    return modules

def updateData(data, modules):
    """!Update menu data tree"""
    for node in data.tree.getiterator():
        if node.tag != 'menuitem':
            continue
        
        item = dict()
        for child in node.getchildren():
            item[child.tag] = child.text
        
        if not item.has_key('command'):
            continue
        
        module = item['command'].split(' ')[0]
        if not modules.has_key(module):
            print 'WARNING: \'%s\' not found in modules' % item['command']
            continue
        
        if modules[module]['label']:
            desc = modules[module]['label']
        else:
            desc = modules[module]['desc']
        node.find('help').text = desc
        node.find('keywords').text = ','.join(modules[module]['keywords'])
        
def writeData(data):
    """!Write updated menudata.xml"""
    file = os.path.join('..', 'xml', 'menudata.xml')
    data.tree.write(file)

def main(argv = None):
    if argv is None:
        argv = sys.argv

    if len(argv) != 1:
        print >> sys.stderr, __doc__
        return 1
    
    print "Step 1: parse modules..."
    modules = dict()
    modules = parseModules()
    print "Step 2: read menu data..."
    data = menudata.Data()
    print "Step 3: update menu data..."
    updateData(data, modules)
    print "Step 4: write menu data (menudata.xml)..."
    writeData(data)
    
    return 0

if __name__ == '__main__':
    if os.getenv("GISBASE") is None:
        print >> sys.stderr, "You must be in GRASS GIS to run this program."
        sys.exit(1)
    
    sys.path.append('../gui_modules')
    import menudata
    import menuform
    import globalvar
    
    sys.exit(main())
