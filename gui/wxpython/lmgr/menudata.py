"""!
@package lmgr.menudata

@brief wxGUI Layer Manager - menu data

Classes:
 - menudata::LayerManagerMenuData


(C) 2007-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

from core.globalvar import ETCWXDIR
from core.menudata  import MenuData

class LayerManagerMenuData(MenuData):
    def __init__(self, filename = None):
        if not filename:
            global etcwxdir
            filename = os.path.join(ETCWXDIR, 'xml', 'menudata.xml')
        
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
                    modules[module] = { 'description': description,
                                        'keywords' : keywords }
                    if len(keywords) < 1:
                        print >> sys.stderr, "WARNING: Module <%s> has no keywords" % module
                
        return modules
