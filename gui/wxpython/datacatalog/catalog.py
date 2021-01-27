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
"""

import wx
import os

from core.debug import Debug
from datacatalog.tree import DataCatalogTree
from datacatalog.toolbars import DataCatalogToolbar
from gui_core.infobar import InfoBar
from datacatalog.infomanager import DataCatalogInfoManager

from grass.pydispatch.signal import Signal

from grass.grassdb.checks import is_current_mapset_in_demolocation


class DataCatalog(wx.Panel):
    """Data catalog panel"""

    def __init__(self, parent, giface=None, id=wx.ID_ANY,
                 title=_("Data catalog"), name='catalog', **kwargs):
        """Panel constructor  """
        self.showNotification = Signal('DataCatalog.showNotification')
        self.parent = parent
        self.baseTitle = title
        self.giface = giface
        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)
        self.SetName("DataCatalog")

        Debug.msg(1, "DataCatalog.__init__()")

        # toolbar
        self.toolbar = DataCatalogToolbar(parent=self)

        # tree with layers
        self.tree = DataCatalogTree(self, giface=giface)
        self.tree.showNotification.connect(self.showNotification)

        # infobar for data catalog
        self.infoBar = InfoBar(self)

        # infobar manager for data catalog
        self.infoManager = DataCatalogInfoManager(infobar=self.infoBar,
                                                  giface=self.giface)
        self.tree.showImportDataInfo.connect(self.showImportDataInfo)

        # some layout
        self._layout()

        # show data structure infobar for first-time user with proper layout
        if is_current_mapset_in_demolocation():
            wx.CallLater(2000, self.showDataStructureInfo)

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.toolbar, proportion=0, flag=wx.EXPAND)
        sizer.Add(self.infoBar, proportion=0, flag=wx.EXPAND)
        sizer.Add(self.tree.GetControl(), proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        self.Fit()

        self.Layout()

    def showDataStructureInfo(self):
        self.infoManager.ShowDataStructureInfo(self.OnCreateLocation)

    def showImportDataInfo(self):
        self.infoManager.ShowImportDataInfo(self.OnImportOgrLayers, self.OnImportGdalLayers)

    def LoadItems(self):
        self.tree.ReloadTreeItems()

    def OnReloadTree(self, event):
        """Reload whole tree"""
        self.LoadItems()

    def OnReloadCurrentMapset(self, event):
        """Reload current mapset tree only"""
        self.tree.ReloadCurrentMapset()

    def OnAddGrassDB(self, event):
        """Add grass database"""
        dlg = wx.DirDialog(self, _("Choose GRASS data directory:"),
                           os.getcwd(), wx.DD_DEFAULT_STYLE)
        if dlg.ShowModal() == wx.ID_OK:
            grassdatabase = dlg.GetPath()
            grassdb_node = self.tree.InsertGrassDb(name=grassdatabase)

            # Offer to create a new location
            if grassdb_node and not os.listdir(grassdatabase):
                message = _("Do you want to create a location?")
                dlg2 = wx.MessageDialog(self,
                                        message=message,
                                        caption=_("Create location?"),
                                        style=wx.YES_NO | wx.YES_DEFAULT |
                                        wx.ICON_QUESTION)
                if dlg2.ShowModal() == wx.ID_YES:
                    self.tree.CreateLocation(grassdb_node)
                dlg2.Destroy()
        dlg.Destroy()

    def OnCreateMapset(self, event):
        """Create new mapset in current location"""
        db_node, loc_node, mapset_node = self.tree.GetCurrentDbLocationMapsetNode()
        self.tree.CreateMapset(db_node, loc_node)

    def OnImportOgrLayers(self, event, cmd=None):
        """Convert multiple OGR layers to GRASS vector map layers"""
        from modules.import_export import OgrImportDialog
        dlg = OgrImportDialog(parent=self, giface=self.giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnImportGdalLayers(self, event, cmd=None):
        """Convert multiple GDAL layers to GRASS raster map layers"""
        from modules.import_export import GdalImportDialog
        dlg = GdalImportDialog(parent=self, giface=self.giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnCreateLocation(self, event):
        """Create new location"""
        db_node, loc_node, mapset_node = self.tree.GetCurrentDbLocationMapsetNode()
        self.tree.CreateLocation(db_node)

    def OnDownloadLocation(self, event):
        """Download location to current grass database"""
        db_node, loc_node, mapset_node = self.tree.GetCurrentDbLocationMapsetNode()
        self.tree.DownloadLocation(db_node)

    def SetRestriction(self, restrict):
        """Allow editing other mapsets or restrict editing to current mapset"""
        self.tree.SetRestriction(restrict)

    def Filter(self, text, element=None):
        self.tree.Filter(text=text, element=element)
