"""!
@package menudata.py

@brief Complex list for menu entries for wxGUI.

Classes:
 - MenuData
 - ManagerData
 - ModelerData
 - PsMapData

Usage:
@code
python menudata.py [action] [manager|modeler]
@endcode

where <i>action</i>:
 - strings (default)
 - tree
 - commands
 - dump

(C) 2007-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Yann Chemin <yann.chemin gmail.com>
@author Martin Landa <landa.martin gmail.com>
@author Glynn Clements
@author Anna Kratochvilova <anna.kratochvilova fsv.cvut.cz>
"""

import os
import sys
import pprint
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

if not os.getenv("GISBASE"):
    sys.exit("GRASS is not running. Exiting...")

etcwxdir = os.path.join(os.getenv("GISBASE"), "etc", "gui", "wxpython")

class MenuData:
    """!Abstract menu data class"""
    def __init__(self, filename):
	self.tree = etree.parse(filename)

    def _getMenuItem(self, mi):
        """!Get menu item

        @param mi menu item instance
        """
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
	    return self._getMenu(mi)
	else:
	    raise Exception(_("Unknow tag"))

    def _getMenu(self, m):
        """!Get menu

        @param m menu

        @return label, menu items
        """
	label = _(m.find('label').text)
	items = m.find('items')
	return (label, tuple(map(self._getMenuItem, items)))
    
    def _getMenuBar(self, mb):
        """!Get menu bar

        @param mb menu bar instance
        
        @return menu items
        """
	return tuple(map(self._getMenu, mb.findall('menu')))

    def _getMenuData(self, md):
        """!Get menu data

        @param md menu data instace
        
        @return menu data
        """
	return list(map(self._getMenuBar, md.findall('menubar')))

    def GetMenu(self):
        """!Get menu

        @return menu data
        """
	return self._getMenuData(self.tree.getroot())

    def PrintStrings(self, fh):
        """!Print menu strings to file (used for localization)

        @param fh file descriptor"""
        className = str(self.__class__).split('.', 1)[1]
	fh.write('menustrings_%s = [\n' % className)
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
                fh.write('- %s\n' % label.replace('&', ''))
                self._PrintTreeItems(fh, level + 1, items)
        
    def _PrintTreeItems(self, fh, level, menuData):
        """!Print menu tree items to file (used by PrintTree)

        @param fh file descriptor
        @param level menu level
        @param menuData menu data to print out"""
        for eachItem in menuData:
            if len(eachItem) == 2:
                if eachItem[0]:
                    fh.write('%s - %s\n' % (' ' * level, eachItem[0]))
                self._PrintTreeItems(fh, level + 1, eachItem[1])
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
                self._PrintCommandsItems(fh, level + 1, items,
                                         menuItems, itemSep, menuSep)
        
    def _PrintCommandsItems(self, fh, level, menuData,
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
                self._PrintCommandsItems(fh, level + 1, eachItem[1],
                                          menuItems, itemSep, menuSep)
            else:
                try:
                    del menuItems[level]
                except IndexError:
                    pass
                
                if eachItem[3]:
                    fh.write('%s%s' % (eachItem[3], itemSep))
                    fh.write(menuSep.join(map(lambda x: x.replace('&', ''), menuItems)))
                    fh.write('%s%s' % (menuSep, eachItem[0]))
                    fh.write('\n')

class ManagerData(MenuData):
    def __init__(self, filename = None):
        if not filename:
            gisbase = os.getenv('GISBASE')
            global etcwxdir
	    filename = os.path.join(etcwxdir, 'xml', 'menudata.xml')
        
        MenuData.__init__(self, filename)
        
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
                        if child.text:
                            keywords = child.text.split(',')
                    
                if module:
                    modules[module] = { 'desc': description,
                                        'keywords' : keywords }
                    if len(keywords) < 1:
                        print >> sys.stderr, "WARNING: Module <%s> has no keywords" % module
                
        return modules

class ModelerData(MenuData):
    def __init__(self, filename = None):
        if not filename:
            gisbase = os.getenv('GISBASE')
            global etcwxdir
	    filename = os.path.join(etcwxdir, 'xml', 'menudata_modeler.xml')
        
        MenuData.__init__(self, filename)

class PsMapData(MenuData):
    def __init__(self, path = None):
        """!Menu for Hardcopy Map Output Utility (psmap.py)
        
        @path path to XML to be read (None for menudata_psmap.xml)
        """
        if not path:
            gisbase = os.getenv('GISBASE')
            global etcwxdir
        path = os.path.join(etcwxdir, 'xml', 'menudata_psmap.xml')
        
        MenuData.__init__(self, path)

if __name__ == "__main__":
    import sys

    # i18N
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

    action = 'strings'
    menu   = 'manager'
    
    for arg in sys.argv:
        if arg in ('strings', 'tree', 'commands', 'dump'):
            action =  arg
        elif arg in ('manager', 'modeler'):
            menu = arg
    
    if menu == 'manager':
        data = ManagerData()
    else:
        data = ModelerData()
    
    if action == 'strings':
        data.PrintStrings(sys.stdout)
    elif action == 'tree':
        data.PrintTree(sys.stdout)
    elif action == 'commands':
        data.PrintCommands(sys.stdout)
    elif action == 'dump':
	pprint.pprint(data.GetMenu(), stream = sys.stdout, indent = 2)
    
    sys.exit(0)
