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
from icons.icon import MetaIcon

from core.debug import Debug
from datacatalog.tree import DataCatalogTree
from datacatalog.toolbars import DataCatalogToolbar
from gui_core.infobar import InfoBar
from datacatalog.infomanager import DataCatalogInfoManager

from grass.pydispatch.signal import Signal

from grass.grassdb.checks import is_current_mapset_in_demolocation

icons = {
    'addLocation': MetaIcon(
        img='location-add',
        label=_("Create new location in current GRASS database"))
}


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
        self.infoManager = DataCatalogInfoManager(self,
                                                  infobar=self.infoBar,
                                                  giface=self.giface)
        # some layout
        self._layout()

    def _layout(self):
        """Do layout"""
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.Add(self.toolbar, proportion=0, flag=wx.EXPAND)
        self.sizer.Add(self.tree.GetControl(), proportion=1, flag=wx.EXPAND)

        # Show first infobar for first-time user
        if is_current_mapset_in_demolocation():
            num_items = self.sizer.GetItemCount()
            self.sizer.Insert(num_items - 1, self.infoBar, wx.SizerFlags().Expand())
            buttons1 = [("Create new Location", self.OnCreateLocation),
                        ("Learn More", self.infoManager._onLearnMore)]
            self.infoManager.ShowDataStructureInfo(buttons1)

        self.SetAutoLayout(True)
        self.SetSizer(self.sizer)
        self.Fit()

        self.Layout()

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
            self.tree.InsertGrassDb(name=grassdatabase)
        dlg.Destroy()

    def OnCreateMapset(self, event):
        """Create new mapset in current location"""
        db_node, loc_node, mapset_node = self.tree.GetCurrentDbLocationMapsetNode()
        self.tree.CreateMapset(db_node, loc_node)

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

    def Filter(self, text):
        self.tree.Filter(text=text)
