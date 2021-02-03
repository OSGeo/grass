"""
@package datacatalog::frame

@brief Data catalog frame class

Classes:
 - datacatalog::DataCatalogFrame

(C) 2014-2020 by Tereza Fiedlerova, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Tereza Fiedlerova (original author)
@author Martin Landa <landa.martin gmail.com> (various improvements)
@author Anna Petrasova (simplify)
"""

import os
import wx

from core.globalvar import ICONDIR
from datacatalog.catalog import DataCatalog
from gui_core.wrap import Button


class DataCatalogFrame(wx.Frame):
    """Frame for testing purposes only."""

    def __init__(self, parent, giface=None, title=_('Data Catalog')):
        wx.Frame.__init__(self, parent=parent, title=title)
        self.SetName("DataCatalog")
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    ICONDIR,
                    'grass.ico'),
                wx.BITMAP_TYPE_ICO))

        self._giface = giface
        self.panel = wx.Panel(self)
        self.catalogpanel = DataCatalog(self.panel, giface=giface)

        # buttons
        self.btnClose = Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btnClose.SetToolTip(_("Close GRASS GIS Data Catalog"))
        self.btnClose.SetDefault()

        # events
        self.btnClose.Bind(wx.EVT_BUTTON, lambda evt: self.Close())

        self._layout()
        self.catalogpanel.LoadItems()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.catalogpanel, proportion=1, flag=wx.EXPAND)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(self.btnClose)

        sizer.Add(btnSizer, proportion=0,
                  flag=wx.ALL | wx.EXPAND,
                  border=5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        self.SetMinSize((450, 500))
