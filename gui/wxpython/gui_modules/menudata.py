"""!
@package menudata.py

@brief Complex list for main menu entries for GRASS wxPython GUI.

Classes:
 - Data
 - MenuTree

Usage:
@code
python menudata.py action [file]
@endcode

where <i>action</i>:
 - strings (default)
 - tree
 - commands

(C) 2007-2009 by the GRASS Development Team
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
    '''!Data object that returns menu descriptions to be used in wxgui.py.'''
    def __init__(self, gisbase=None):
        if not gisbase:
            gisbase = os.getenv('GISBASE')
	filename = os.path.join(gisbase, 'etc', 'wxpython', 'xml', 'menudata.xml')
	self.tree = etree.parse(filename)

    def getMenuItem(self, mi):
	if mi.tag == 'separator':
	    return ('', '', '', '', '')
	elif mi.tag == 'menuitem':
	    label    = _(mi.find('label').text)
	    help     = _(mi.find('help').text)
	    handler  = mi.find('handler').text
	    gcmd     = mi.find('command')  # optional
            keywords = mi.find('keywords') # optional
            shortcut = mi.find('shortcut') # optional
	    if gcmd != None:
		gcmd = gcmd.text
	    else:
		gcmd = ""
            if keywords != None:
                keywords = keywords.text
            else:
                keywords = ""
            if shortcut != None:
                shortcut = shortcut.text
            else:
                shortcut = ""
	    return (label, help, handler, gcmd, keywords, shortcut)
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
        """!Print menu strings to file (used for localization)

        @param fh file descriptor"""
	fh.write('menustrings = [\n')
	for node in self.tree.getiterator():
	    if node.tag in ['label', 'help']:
		fh.write('     _(%r),\n' % node.text)
	fh.write('    \'\']\n')

    def PrintTree(self, fh):
        """!Print menu tree to file

        @param fh file descriptor"""
        level = 0
        for eachMenuData in self.GetMenu():
            for label, items in eachMenuData:
                fh.write('- %s\n' % label)
                self.__PrintTreeItems(fh, level + 1, items)

    def __PrintTreeItems(self, fh, level, menuData):
        """!Print menu tree items to file (used by PrintTree)

        @param fh file descriptor
        @param level menu level
        @param menuData menu data to print out"""
        for eachItem in menuData:
            if len(eachItem) == 2:
                if eachItem[0]:
                    fh.write('%s - %s\n' % (' ' * level, eachItem[0]))
                self.__PrintTreeItems(fh, level + 1, eachItem[1])
            else:
                if eachItem[0]:
                    fh.write('%s - %s\n' % (' ' * level, eachItem[0]))
    
    def PrintCommands(self, fh, itemSep = ' | ', menuSep = ' > '):
        """!Print commands list (command | menu item > menu item)

        @param fh file descriptor
        """
        level = 0
        for eachMenuData in self.GetMenu():
            for label, items in eachMenuData:
                menuItems = [label, ]
                self.__PrintCommandsItems(fh, level + 1, items,
                                          menuItems, itemSep, menuSep)
        
    def __PrintCommandsItems(self, fh, level, menuData,
                             menuItems, itemSep, menuSep):
        """!Print commands item (used by PrintCommands)

        @param fh file descriptor
        @param menuItems list of menu items
        """
        for eachItem in menuData:
            if len(eachItem) == 2:
                if eachItem[0]:
                    try:
                        menuItems[level] = eachItem[0]
                    except IndexError:
                        menuItems.append(eachItem[0])
                self.__PrintCommandsItems(fh, level + 1, eachItem[1],
                                          menuItems, itemSep, menuSep)
            else:
                try:
                    del menuItems[level]
                except IndexError:
                    pass
                
                if eachItem[3]:
                    fh.write('%s%s' % (eachItem[3], itemSep))
                    fh.write(menuSep.join(menuItems))
                    fh.write('%s%s' % (menuSep, eachItem[0]))
                    fh.write('\n')
        
    def GetModules(self):
        """!Create dictionary of modules used to search module by
        keywords, description, etc."""
        modules = dict()
        
        for node in self.tree.getiterator():
            if node.tag == 'menuitem':
                module = description = ''
                keywords = []
                for child in node.getchildren():
                    if child.tag == 'help':
                        description = child.text
                    if child.tag == 'command':
                        module = child.text
                    if child.tag == 'keywords':
                        keywords = child.text.split(',')
                
                if module:
                    modules[module] = { 'desc': description,
                                        'keywords' : keywords }
        
        return modules

if __name__ == "__main__":
    import sys

    # i18N
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

    file = None
    if len(sys.argv) == 1:
        action = 'strings'
    elif len(sys.argv) > 1:
        action = sys.argv[1]
    if len(sys.argv) > 2:
        file = sys.argv[2]

    if file:
        data = Data(file)
    else:
        data = Data()
    
    if action == 'strings':
        data.PrintStrings(sys.stdout)
    elif action == 'tree':
        data.PrintTree(sys.stdout)
    elif action == 'commands':
        data.PrintCommands(sys.stdout)
    
    sys.exit(0)
