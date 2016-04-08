"""
@package datacatalog.toolbars

@brief Data Catalog toolbars

Classes:
 - toolbars::DataCatalogToolbar(BaseToolbar)

(C) 2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import wx
from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon
from core.utils import _

icons = {
    'reloadTree': MetaIcon(img='redraw', label=_("Reload GRASS locations")),
    'reloadMapset': MetaIcon(img='reload', label=_("Reload current GRASS mapset only"))
    }

class DataCatalogToolbar(BaseToolbar):
    """Main data catalog toolbar
    """
    def __init__(self, parent):
        """Main toolbar constructor
        """
        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Returns toolbar data (name, icon, handler)"""
        # BaseIcons are a set of often used icons. It is possible
        # to reuse icons in ./trunk/gui/icons/grass or add new ones there.
        return self._getToolbarData((("reloadTree", icons["reloadTree"],
                                      self.parent.OnReloadTree),
                                     ("reloadMapset", icons["reloadMapset"],
                                      self.parent.OnReloadCurrentMapset)
                                     ))
