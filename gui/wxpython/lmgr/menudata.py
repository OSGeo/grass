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

from core.menutree  import MenuTreeModelBuilder
from core.toolboxes import getMenudataFile
from core.globalvar import ETCWXDIR
from core.gcmd import GError


class LayerManagerMenuData(MenuTreeModelBuilder):
    def __init__(self, filename=None):
        if filename:
            expandAddons = False
        else:
            expandAddons = True

        fallback = os.path.join(ETCWXDIR, 'xml', 'menudata.xml')
        if not filename:
            filename = getMenudataFile(userRootFile='main_menu.xml',
                                       newFile='menudata.xml',
                                       fallback=fallback)
        try:
            MenuTreeModelBuilder.__init__(self, filename, expandAddons=expandAddons)
        except (ValueError, AttributeError, TypeError):
            GError(_("Unable to parse user toolboxes XML files. "
                     "Default main menu will be loaded."))
            fallback = os.path.join(ETCWXDIR, 'xml', 'menudata.xml')
            MenuTreeModelBuilder.__init__(self, fallback)


class LayerManagerModuleTree(MenuTreeModelBuilder):
    def __init__(self, filename=None):
        if filename:
            expandAddons = False
        else:
            expandAddons = True

        fallback = os.path.join(ETCWXDIR, 'xml', 'module_tree_menudata.xml')
        if not filename:
            filename = getMenudataFile(userRootFile='module_tree.xml',
                                       newFile='module_tree_menudata.xml',
                                       fallback=fallback)
        # TODO: try-except useless?
        try:
            MenuTreeModelBuilder.__init__(self, filename, expandAddons=expandAddons)
        except (ValueError, AttributeError, TypeError):
            GError(_("Unable to parse user toolboxes XML files. "
                     "Default module tree will be loaded."))
            MenuTreeModelBuilder.__init__(self, fallback)
