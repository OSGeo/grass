"""!
@package ps.menudata

@brief wxPsMap - menu entries

Classes:
 - menudata::PsMapMenuData

(C) 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os

from core import globalvar
from core.menutree import MenuTreeModelBuilder

class PsMapMenuData(MenuTreeModelBuilder):
    def __init__(self, path = None):
        """!Menu for Cartographic Composer (psmap.py)
        
        @param path path to XML to be read (None for menudata_psmap.xml)
        """
        if not path:
            path = os.path.join(globalvar.ETCWXDIR, 'xml', 'menudata_psmap.xml')
        
        MenuTreeModelBuilder.__init__(self, path)
