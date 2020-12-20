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

from grass.grassdb.checks import is_current_subproject_in_demoproject


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
        # some layout
        self._layout()

        # show data structure infobar for first-time user with proper layout
        if is_current_subproject_in_demoproject():
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
        self.infoManager.ShowDataStructureInfo(self.OnCreateProject)

    def LoadItems(self):
        self.tree.ReloadTreeItems()

    def OnReloadTree(self, event):
        """Reload whole tree"""
        self.LoadItems()

    def OnReloadCurrentSubproject(self, event):
        """Reload current subproject tree only"""
        self.tree.ReloadCurrentSubproject()

    def OnAddGrassDB(self, event):
        """Add grass database"""
        dlg = wx.DirDialog(self, _("Choose GRASS data directory:"),
                           os.getcwd(), wx.DD_DEFAULT_STYLE)
        if dlg.ShowModal() == wx.ID_OK:
            grassdatabase = dlg.GetPath()
            self.tree.InsertGrassDb(name=grassdatabase)
        dlg.Destroy()

    def OnCreateSubproject(self, event):
        """Create new subproject in current project"""
        db_node, loc_node, subproject_node = self.tree.GetCurrentDbProjectSubprojectNode()
        self.tree.CreateSubproject(db_node, loc_node)

    def OnCreateProject(self, event):
        """Create new project"""
        db_node, loc_node, subproject_node = self.tree.GetCurrentDbProjectSubprojectNode()
        self.tree.CreateProject(db_node)

    def OnDownloadProject(self, event):
        """Download project to current grass database"""
        db_node, loc_node, subproject_node = self.tree.GetCurrentDbProjectSubprojectNode()
        self.tree.DownloadProject(db_node)

    def SetRestriction(self, restrict):
        """Allow editing other subprojects or restrict editing to current subproject"""
        self.tree.SetRestriction(restrict)

    def Filter(self, text):
        self.tree.Filter(text=text)
