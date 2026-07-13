"""
@package datacatalog::catalog

@brief Data catalog

Classes:
 - datacatalog::DataCatalog

(C) 2014-2018 by Tereza Fiedlerova, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Tereza Fiedlerova
@author Linda Kladivova l.kladivova@seznam.cz
"""

import wx

from pathlib import Path

from core.debug import Debug
from datacatalog.tree import DataCatalogTree
from datacatalog.toolbars import DataCatalogToolbar, DataCatalogSearch
from gui_core.infobar import InfoBar
from datacatalog.infomanager import DataCatalogInfoManager
from gui_core.wrap import Menu
from gui_core.forms import GUI
from core.settings import UserSettings
from core.gcmd import GError

from grass.pydispatch.signal import Signal
from grass.script.utils import clock
from grass.script import gisenv

from grass.grassdb.manage import split_mapset_path
from grass.grassdb.checks import (
    get_reason_id_mapset_not_usable,
    is_fallback_session,
    is_first_time_user,
    is_location_valid,
    get_location_invalid_reason,
)


class DataCatalog(wx.Panel):
    """Data catalog panel"""

    def __init__(
        self,
        parent,
        giface=None,
        id=wx.ID_ANY,
        title=_("Data catalog"),
        name="catalog",
        **kwargs,
    ):
        """Panel constructor"""
        self.showNotification = Signal("DataCatalog.showNotification")
        self.parent = parent
        self.baseTitle = title
        self.giface = giface
        self._startLoadingTime = 0
        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)
        self.SetName("DataCatalog")

        Debug.msg(1, "DataCatalog.__init__()")

        # toolbar
        self.toolbar = DataCatalogToolbar(parent=self)

        # search
        self.search = DataCatalogSearch(parent=self, filter_function=self.Filter)

        # tree with layers
        self.tree = DataCatalogTree(self, giface=giface)
        self.tree.showNotification.connect(self.showNotification)

        # infobar for data catalog
        delay = 2000
        self.infoBar = InfoBar(self)
        self.giface.currentMapsetChanged.connect(self.dismissInfobar)

        # infobar manager for data catalog
        self.infoManager = DataCatalogInfoManager(
            infobar=self.infoBar, giface=self.giface
        )
        self.tree.showImportDataInfo.connect(self.showImportDataInfo)
        self.tree.loadingDone.connect(self._loadingDone)

        # some layout
        self._layout()

        # show infobar for first-time user if applicable
        if is_first_time_user():
            # show data structure infobar for first-time user
            wx.CallLater(delay, self.showDataStructureInfo)

        # show infobar if last used mapset is not usable
        if is_fallback_session():
            # get reason why last used mapset is not usable
            last_mapset_path = gisenv()["LAST_MAPSET_PATH"]
            self.reason_id = get_reason_id_mapset_not_usable(last_mapset_path)
            if self.reason_id in {"non-existent", "invalid", "different-owner"}:
                # show non-standard situation info
                wx.CallLater(delay, self.showFallbackSessionInfo)
            elif self.reason_id == "locked":
                # show info allowing to switch to locked mapset
                wx.CallLater(delay, self.showLockedMapsetInfo)

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.toolbar, proportion=0, flag=wx.EXPAND)
        sizer.Add(
            self.search,
            proportion=0,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=5,
        )
        sizer.Add(self.infoBar, proportion=0, flag=wx.EXPAND)
        sizer.Add(self.tree.GetControl(), proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        self.Fit()

        self.Layout()

    def showDataStructureInfo(self):
        self.infoManager.ShowDataStructureInfo(self.OnCreateLocation)

    def showLockedMapsetInfo(self):
        self.infoManager.ShowLockedMapsetInfo(self.OnSwitchToLastUsedMapset)

    def showFallbackSessionInfo(self):
        self.infoManager.ShowFallbackSessionInfo(self.reason_id)

    def showImportDataInfo(self):
        self.infoManager.ShowImportDataInfo(
            self.OnImportOgrLayers, self.OnImportGdalLayers
        )

    def LoadItems(self):
        """Reload tree - full or lazy - based on user settings"""
        self._startLoadingTime = clock()
        self.tree.ReloadTreeItems(full=False)

    def _loadingDone(self):
        """If loading took more time, suggest lazy loading"""
        if clock() - self._startLoadingTime > 5 and not self.tree._useLazyLoading():
            asked = UserSettings.Get(
                group="datacatalog", key="lazyLoading", subkey="asked"
            )
            if not asked:
                wx.CallAfter(
                    self.infoManager.ShowLazyLoadingOn,
                    setLazyLoadingOnHandler=self._saveLazyLoadingOnSettings,
                    doNotAskHandler=self._saveDontAskLazyLoadingSettings,
                )

    def _saveLazyLoadingOnSettings(self, event):
        """Turn on lazy loading in settings"""
        UserSettings.Set(
            group="datacatalog", key="lazyLoading", subkey="enabled", value=True
        )
        UserSettings.Set(
            group="datacatalog", key="lazyLoading", subkey="asked", value=True
        )
        self._saveLazyLoadingSettings()
        event.Skip()

    def _saveDontAskLazyLoadingSettings(self, event):
        """Save in settings that decision on lazy loading was done to not ask again"""
        UserSettings.Set(
            group="datacatalog", key="lazyLoading", subkey="asked", value=True
        )
        self._saveLazyLoadingSettings()
        event.Skip()

    def _saveLazyLoadingSettings(self):
        dcSettings = {}
        UserSettings.ReadSettingsFile(settings=dcSettings)
        if "datacatalog" not in dcSettings:
            dcSettings["datacatalog"] = UserSettings.Get(group="datacatalog")
        dcSettings["datacatalog"]["lazyLoading"] = UserSettings.Get(
            group="datacatalog", key="lazyLoading"
        )
        UserSettings.SaveToFile(dcSettings)

    def dismissInfobar(self):
        if self.infoBar.IsShown():
            self.infoBar.Dismiss()

    def OnReloadTree(self, event):
        """Reload whole tree"""
        self.tree.ReloadTreeItems(full=True)

    def OnReloadCurrentMapset(self, event):
        """Reload current mapset tree only"""
        self.tree.ReloadCurrentMapset()

    def OnCreateMapset(self, event):
        """Create new mapset in current location"""
        db_node, loc_node, mapset_node = self.tree.GetCurrentDbLocationMapsetNode()
        self.tree.CreateMapset(db_node, loc_node)

    def OnCreateLocation(self, event):
        """Create new location"""
        db_node, loc_node, mapset_node = self.tree.GetCurrentDbLocationMapsetNode()
        self.tree.CreateLocation(db_node)

    def OnAddProject(self, event):
        dlg = wx.DirDialog(
            self,
            _("Choose GRASS project:"),
            str(Path.cwd()),
            wx.DD_DEFAULT_STYLE,
        )
        if dlg.ShowModal() == wx.ID_OK:
            project = Path(dlg.GetPath())
            grassdatabase = None
            if is_location_valid(project):
                grassdatabase = project.parent
            elif is_location_valid(project.parent):
                grassdatabase = project.parent.parent
            else:
                for child in project.iterdir():
                    if is_location_valid(child):
                        grassdatabase = project
                        break
            if grassdatabase:
                self.tree.InsertGrassDb(name=str(grassdatabase))
            else:
                error = get_location_invalid_reason(project.parent, project.name)
                GError(parent=self.parent, message=error)

    def OnDownloadLocation(self, event):
        """Download location to current grass database"""
        db_node, loc_node, mapset_node = self.tree.GetCurrentDbLocationMapsetNode()
        self.tree.DownloadLocation(db_node)

    def OnSwitchToLastUsedMapset(self, event):
        """Switch to last used mapset"""
        last_mapset_path = gisenv()["LAST_MAPSET_PATH"]
        grassdb, location, mapset = split_mapset_path(last_mapset_path)
        self.tree.SwitchMapset(grassdb, location, mapset)

    def OnImportGdalLayers(self, event):
        """Convert multiple GDAL layers to GRASS raster map layers"""
        from modules.import_export import GdalImportDialog

        dlg = GdalImportDialog(parent=self, giface=self.giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnImportOgrLayers(self, event):
        """Convert multiple OGR layers to GRASS vector map layers"""
        from modules.import_export import OgrImportDialog

        dlg = OgrImportDialog(parent=self, giface=self.giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnLinkGdalLayers(self, event):
        """Link multiple GDAL layers to GRASS raster map layers"""
        from modules.import_export import GdalImportDialog

        dlg = GdalImportDialog(parent=self, giface=self.giface, link=True)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnLinkOgrLayers(self, event):
        """Links multiple OGR layers to GRASS vector map layers"""
        from modules.import_export import OgrImportDialog

        dlg = OgrImportDialog(parent=self, giface=self.giface, link=True)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnRasterOutputFormat(self, event):
        """Set raster output format handler"""
        from modules.import_export import GdalOutputDialog

        dlg = GdalOutputDialog(parent=self, ogr=False)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnVectorOutputFormat(self, event):
        """Set vector output format handler"""
        from modules.import_export import GdalOutputDialog

        dlg = GdalOutputDialog(parent=self, ogr=True)
        dlg.CentreOnScreen()
        dlg.Show()

    def GuiParseCommand(self, cmd):
        """Generic handler"""
        GUI(parent=self, giface=self.giface).ParseCommand(cmd=[cmd])

    def OnMoreOptions(self, event):
        self.giface.Help(entry="topic_import")

    def Filter(self, text, element=None):
        self.tree.Filter(text=text, element=element)

    def OnImportMenu(self, event):
        """Create popup menu for other import options"""
        # create submenu
        subMenu = Menu()

        subitem = wx.MenuItem(
            subMenu, wx.ID_ANY, _("Link external raster data  [r.external]")
        )
        subMenu.AppendItem(subitem)
        self.Bind(wx.EVT_MENU, self.OnLinkGdalLayers, subitem)

        subitem = wx.MenuItem(
            subMenu, wx.ID_ANY, _("Link external vector data  [v.external]")
        )
        subMenu.AppendItem(subitem)
        self.Bind(wx.EVT_MENU, self.OnLinkOgrLayers, subitem)

        subMenu.AppendSeparator()

        subitem = wx.MenuItem(
            subMenu, wx.ID_ANY, _("Set raster output format  [r.external.out]")
        )
        subMenu.AppendItem(subitem)
        self.Bind(wx.EVT_MENU, self.OnRasterOutputFormat, subitem)

        subitem = wx.MenuItem(
            subMenu, wx.ID_ANY, _("Set vector output format  [v.external.out]")
        )
        subMenu.AppendItem(subitem)
        self.Bind(wx.EVT_MENU, self.OnVectorOutputFormat, subitem)

        # create menu
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("Unpack GRASS raster map  [r.unpack]"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, lambda evt: self.GuiParseCommand("r.unpack"), item)

        item = wx.MenuItem(menu, wx.ID_ANY, _("Unpack GRASS vector map  [v.unpack]"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, lambda evt: self.GuiParseCommand("v.unpack"), item)

        menu.AppendSeparator()

        item = wx.MenuItem(
            menu, wx.ID_ANY, _("Create raster map from x,y,z data  [r.in.xyz]")
        )
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, lambda evt: self.GuiParseCommand("r.in.xyz"), item)

        item = wx.MenuItem(
            menu, wx.ID_ANY, _("Create vector map from x,y,z data  [v.in.ascii]")
        )
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, lambda evt: self.GuiParseCommand("v.in.ascii"), item)

        menu.AppendSeparator()
        menu.AppendMenu(wx.ID_ANY, _("Link external data"), subMenu)

        menu.AppendSeparator()
        item = wx.MenuItem(menu, wx.ID_ANY, _("More options..."))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnMoreOptions, item)

        self.PopupMenu(menu)
        menu.Destroy()
