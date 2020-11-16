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
import webbrowser

from core.debug import Debug
from datacatalog.tree import DataCatalogTree
from datacatalog.toolbars import DataCatalogToolbar
from gui_core.infobar import InfoManager

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
        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)
        self.SetName("DataCatalog")

        Debug.msg(1, "DataCatalog.__init__()")

        # toolbar
        self.toolbar = DataCatalogToolbar(parent=self)

        # tree with layers
        self.tree = DataCatalogTree(self, giface=giface)
        self.tree.showNotification.connect(self.showNotification)

        # some layout
        self._layout()

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.toolbar, proportion=0,
                  flag=wx.EXPAND)
        sizer.Add(self.tree.GetControl(), proportion=1,
                  flag=wx.EXPAND)

        # infobar instance
        self.infoManager = InfoManager(guiparent=self, sizer=sizer)

        # Show infobar for first-time user
        if is_current_mapset_in_demolocation:
            buttons1 = []
            buttons2 = []
            buttons1.append(("Learn More", self._onLearnMore))
            buttons2.append(("Create new Location", self.OnCreateLocation))
            self.infoManager.ShowInfoBar1(buttons1)
            self.infoManager.ShowInfoBar2(buttons2)
            sizer = self.infoManager.sizer

        self.SetAutoLayout(True)
        self.SetSizer(sizer)

        self.Layout()

    def _onLearnMore(self, event):
        webbrowser.open("https://grass.osgeo.org/grass79/manuals/grass_database.html")

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
