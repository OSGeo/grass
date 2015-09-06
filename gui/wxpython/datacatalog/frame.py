"""
@package datacatalog::frame

@brief Data catalog frame class

Classes:
 - datacatalog::DataCatalogTree
 - datacatalog::DataCatalogFrame

(C) 2014-2015 by Tereza Fiedlerova, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Tereza Fiedlerova
"""

import wx

from grass.script import core as gcore

from core.utils import _
from datacatalog.tree import DataCatalogTree

class DataCatalogFrame(wx.Frame):
    """Frame for testing purposes only."""
    def __init__(self, parent, giface=None):
        wx.Frame.__init__(self, parent=parent,
                          title=_('GRASS GIS Data Catalog (experimetal)'))

        self._giface = giface
        self.panel = wx.Panel(self)

        # tree
        self.tree = DataCatalogTree(parent=self.panel, giface=self._giface)
        self.tree.InitTreeItems()
        
        # buttons
        self.btnClose = wx.Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btnClose.SetToolTipString(_("Close GRASS GIS Data Catalog"))
        self.btnClose.SetDefault()

        # events
        
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        
        self._layout()
        
    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.tree, proportion=1, flag=wx.EXPAND)
        
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnClose)
        
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALL | wx.ALIGN_RIGHT,
                  border = 5)
        
        self.panel.SetSizerAndFit(sizer)
        sizer.SetSizeHints(self.panel)
        
        self.SetMinSize((400, 500))

    def OnCloseWindow(self, event):
        """Cancel button pressed"""
        if not isinstance(event, wx.CloseEvent):
            self.Destroy()
        
        event.Skip()
