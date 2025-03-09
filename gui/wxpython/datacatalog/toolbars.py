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
from gui_core.wrap import SearchCtrl
from icons.icon import MetaIcon

icons = {
    "reloadTree": MetaIcon(img="redraw", label=_("Reload GRASS projects")),
    "reloadMapset": MetaIcon(img="reload", label=_("Reload current GRASS mapset only")),
    "unlocked": MetaIcon(
        img="edit", label=_("Restrict edits to the current mapset only")
    ),
    "locked": MetaIcon(
        img="edit", label=_("Allow edits outside of the current mapset")
    ),
    "addGrassDB": MetaIcon(
        img="grassdb-add", label=_("Add existing or create new database")
    ),
    "addMapset": MetaIcon(
        img="mapset-add", label=_("Create new mapset in current project")
    ),
    "addLocation": MetaIcon(
        img="location-add",
        label=_("Create new project (location) in current GRASS database"),
    ),
    "downloadLocation": MetaIcon(
        img="location-download",
        label=_("Download sample project (location) to current GRASS database"),
    ),
    "importRaster": MetaIcon(
        img="raster-import", label=_("Import raster data  [r.import]")
    ),
    "importVector": MetaIcon(
        img="vector-import", label=_("Import vector data  [v.import]")
    ),
    "importLayer": MetaIcon(
        img="layer-import", label=_("Select another import option")
    ),
}


class DataCatalogSearch(SearchCtrl):
    def __init__(self, parent, filter_function):
        super().__init__(parent)
        self.filter_function = filter_function
        self.filter_element = None
        self.SetDescriptiveText(_("Search"))
        self.ShowCancelButton(True)
        self.Bind(
            wx.EVT_TEXT,
            lambda event: self.filter_function(self.GetValue(), self.filter_element),
        )
        self.Bind(wx.EVT_SEARCHCTRL_CANCEL_BTN, lambda evt: self.filter_function(""))
        filterMenu = wx.Menu()
        item = filterMenu.AppendRadioItem(-1, "All")
        self.Bind(wx.EVT_MENU, self.OnFilterMenu, item)
        item = filterMenu.AppendRadioItem(-1, "Raster maps")
        self.Bind(wx.EVT_MENU, self.OnFilterMenu, item)
        item = filterMenu.AppendRadioItem(-1, "Vector maps")
        self.Bind(wx.EVT_MENU, self.OnFilterMenu, item)
        item = filterMenu.AppendRadioItem(-1, "3D raster maps")
        self.Bind(wx.EVT_MENU, self.OnFilterMenu, item)
        self.SetMenu(filterMenu)
        helpTip = _(
            "Type to search database by map type or name. "
            "Use Python regular expressions to refine your search."
        )
        self.SetToolTip(helpTip)

    def OnFilterMenu(self, event):
        """Decide the element to filter by"""
        filterMenu = self.GetMenu().GetMenuItems()
        self.filter_element = None
        if filterMenu[1].IsChecked():
            self.filter_element = "raster"
        elif filterMenu[2].IsChecked():
            self.filter_element = "vector"
        elif filterMenu[3].IsChecked():
            self.filter_element = "raster_3d"
        # trigger filter on change
        if self.GetValue():
            self.filter_function(self.GetValue(), self.filter_element)


class DataCatalogToolbar(BaseToolbar):
    """Main data catalog toolbar"""

    def __init__(self, parent):
        """Main toolbar constructor"""

        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Returns toolbar data (name, icon, handler)"""
        # BaseIcons are a set of often used icons. It is possible
        # to reuse icons in gui/icons/grass or add new ones there.
        return self._getToolbarData(
            (
                (
                    ("reloadTree", icons["reloadTree"].label),
                    icons["reloadTree"],
                    self.parent.OnReloadTree,
                ),
                (
                    ("reloadMapset", icons["reloadMapset"].label),
                    icons["reloadMapset"],
                    self.parent.OnReloadCurrentMapset,
                ),
                (
                    ("lock", icons["locked"].label),
                    icons["locked"],
                    self.OnSetRestriction,
                    wx.ITEM_CHECK,
                ),
                (
                    ("addGrassDB", icons["addGrassDB"].label),
                    icons["addGrassDB"],
                    self.parent.OnAddGrassDB,
                ),
                (
                    ("addLocation", icons["addLocation"].label),
                    icons["addLocation"],
                    self.parent.OnCreateLocation,
                ),
                (
                    ("downloadLocation", icons["downloadLocation"].label),
                    icons["downloadLocation"],
                    self.parent.OnDownloadLocation,
                ),
                (
                    ("addMapset", icons["addMapset"].label),
                    icons["addMapset"],
                    self.parent.OnCreateMapset,
                ),
                (
                    ("importRaster", icons["importRaster"].label),
                    icons["importRaster"],
                    self.parent.OnImportGdalLayers,
                ),
                (
                    ("importVector", icons["importVector"].label),
                    icons["importVector"],
                    self.parent.OnImportOgrLayers,
                ),
                (
                    ("importLayer", icons["importLayer"].label),
                    icons["importLayer"],
                    self.parent.OnImportMenu,
                ),
            )
        )

    def OnSetRestriction(self, event):
        if self.GetToolState(self.lock):
            self.SetToolNormalBitmap(self.lock, icons["unlocked"].GetBitmap())
            self.SetToolShortHelp(self.lock, icons["unlocked"].GetLabel())
            self.parent.SetRestriction(restrict=False)
        else:
            self.SetToolNormalBitmap(self.lock, icons["locked"].GetBitmap())
            self.SetToolShortHelp(self.lock, icons["locked"].GetLabel())
            self.parent.SetRestriction(restrict=True)
