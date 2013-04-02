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

from core.globalvar import ETCWXDIR
from core.menutree  import MenuTreeModelBuilder

class LayerManagerMenuData(MenuTreeModelBuilder):
    def __init__(self, filename = None):
        if not filename:
            filename = os.path.join(ETCWXDIR, 'xml', 'menudata.xml')
        
        MenuTreeModelBuilder.__init__(self, filename)
