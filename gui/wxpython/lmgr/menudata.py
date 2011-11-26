"""!
@package lmrg.menudata

@brief Complex list for menu entries for wxGUI

Classes:
 - menudata::MenuData

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
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys

from core.globalvar import ETCWXDIR
from core.menudata  import MenuData

class ManagerData(MenuData):
    def __init__(self, filename = None):
        if not filename:
            gisbase = os.getenv('GISBASE')
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
                    modules[module] = { 'desc': description,
                                        'keywords' : keywords }
                    if len(keywords) < 1:
                        print >> sys.stderr, "WARNING: Module <%s> has no keywords" % module
                
        return modules
