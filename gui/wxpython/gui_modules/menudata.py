"""
@package menudata.py

@brief Complex list for main menu entries for GRASS wxPython GUI.

Classes:
 - Data

COPYRIGHT:  (C) 2007-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton (Arizona State University)
@author Yann Chemin <yann.chemin gmail.com>
@author Martin Landa <landa.martin gmail.com>
@author Glynn Clements
"""

import os
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

class Data:
    '''Data object that returns menu descriptions to be used in wxgui.py.'''
    def __init__(self):
	filename = os.getenv('GISBASE') + '/etc/wxpython/xml/menudata.xml'
	self.tree = etree.parse(filename)

    def getMenuItem(self, mi):
	if mi.tag == 'separator':
	    return ('', '', '', '')
	elif mi.tag == 'menuitem':
	    label   = _(mi.find('label').text)
	    help    = _(mi.find('help').text)
	    handler = mi.find('handler').text
	    gcmd    = mi.find('command')
	    if gcmd != None:
		gcmd = gcmd.text
	    else:
		gcmd = ""
	    return (label, help, handler, gcmd)
	elif mi.tag == 'menu':
	    return self.getMenu(mi)
	else:
	    raise Exception()

    def getMenu(self, m):
	label = _(m.find('label').text)
	items = m.find('items')
	return (label, tuple(map(self.getMenuItem, items)))

    def getMenuBar(self, mb):
	return tuple(map(self.getMenu, mb.findall('menu')))

    def getMenuData(self, md):
	return list(map(self.getMenuBar, md.findall('menubar')))

    def GetMenu(self):
	return self.getMenuData(self.tree.getroot())

    def PrintStrings(self, fh):
	fh.write('menustrings = [\n')
	for node in self.tree.getiterator():
	    if node.tag in ['label', 'help']:
		fh.write('     _(%r),\n' % node.text)
	fh.write('    \'\']\n')

if __name__ == "__main__":
    import sys
    Data().PrintStrings(sys.stdout)
