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
from gui_core.toolbars import BaseToolbar
from gui_core.wrap import StaticText, TextCtrl
from icons.icon import MetaIcon

icons = {
    'reloadTree': MetaIcon(
        img='redraw',
        label=_("Reload GRASS projects")),
    'reloadSubproject': MetaIcon(
        img='reload',
        label=_("Reload current GRASS subproject only")),
    'unlocked': MetaIcon(
        img='edit',
        label=_("Restrict edits to the current subproject only")),
    'locked': MetaIcon(
        img='edit',
        label=_("Allow edits outside of the current subproject")),
    'addGrassDB': MetaIcon(
        img='grassdb-add',
        label=_("Add existing or create new database")),
    'addSubproject': MetaIcon(
        img='subproject-add',
        label=_("Create new subproject in current project")),
    'addProject': MetaIcon(
        img='project-add',
        label=_("Create new project in current GRASS database")),
    'downloadProject': MetaIcon(
        img='project-download',
        label=_("Download sample project to current GRASS database"))
}


class DataCatalogToolbar(BaseToolbar):
    """Main data catalog toolbar
    """

    def __init__(self, parent):
        """Main toolbar constructor
        """

        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())
        self.filter = TextCtrl(parent=self)
        self.filter.SetSize((120, self.filter.GetBestSize()[1]))
        self.filter.Bind(wx.EVT_TEXT,
                         lambda event: self.parent.Filter(
                         self.filter.GetValue()))
        self.AddControl(StaticText(self, label=_("Search:")))
        self.AddControl(self.filter)
        help = _("Type to search database by map type or name. "
                 "Use prefix 'r:', 'v:' and 'r3:'"
                 "to show only raster, vector or 3D raster data, respectively. "
                 "Use Python regular expressions to refine your search.")
        self.SetToolShortHelp(self.filter.GetId(), help)
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Returns toolbar data (name, icon, handler)"""
        # BaseIcons are a set of often used icons. It is possible
        # to reuse icons in ./trunk/gui/icons/grass or add new ones there.
        return self._getToolbarData((("reloadTree", icons["reloadTree"],
                                      self.parent.OnReloadTree),
                                     ("reloadSubproject", icons["reloadSubproject"],
                                      self.parent.OnReloadCurrentSubproject),
                                     ("lock", icons['locked'],
                                      self.OnSetRestriction, wx.ITEM_CHECK),
                                     ("addGrassDB", icons['addGrassDB'],
                                      self.parent.OnAddGrassDB),
                                     ("addProject", icons['addProject'],
                                      self.parent.OnCreateProject),
                                     ("downloadProject", icons['downloadProject'],
                                      self.parent.OnDownloadProject),
                                     ("addSubproject", icons['addSubproject'],
                                      self.parent.OnCreateSubproject)
                                     ))

    def OnSetRestriction(self, event):
        if self.GetToolState(self.lock):
            self.SetToolNormalBitmap(self.lock, icons['unlocked'].GetBitmap())
            self.SetToolShortHelp(self.lock, icons['unlocked'].GetLabel())
            self.parent.SetRestriction(restrict=False)
        else:
            self.SetToolNormalBitmap(self.lock, icons['locked'].GetBitmap())
            self.SetToolShortHelp(self.lock, icons['locked'].GetLabel())
            self.parent.SetRestriction(restrict=True)
