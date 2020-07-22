"""
@package datacatalog::frame

@brief Data catalog frame class

Classes:
 - datacatalog::DataCatalogFrame

(C) 2014-2018 by Tereza Fiedlerova, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Tereza Fiedlerova (original author)
@author Martin Landa <landa.martin gmail.com> (various improvements)
"""

import os
import sys

import wx

from core.globalvar import ICONDIR
from core.gcmd import RunCommand, GError, GMessage
from datacatalog.tree import DataCatalogTree
from datacatalog.toolbars import DataCatalogToolbar
from gui_core.wrap import Button
from grass.script import gisenv
from startup.guiutils import create_mapset_interactively


class DataCatalogFrame(wx.Frame):
    """Frame for testing purposes only."""

    def __init__(self, parent, giface=None):
        wx.Frame.__init__(self, parent=parent,
                          title=_('GRASS GIS Data Catalog'))
        self.SetName("DataCatalog")
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    ICONDIR,
                    'grass.ico'),
                wx.BITMAP_TYPE_ICO))

        self._giface = giface
        self.panel = wx.Panel(self)

        self.toolbar = DataCatalogToolbar(parent=self)
        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform != 'darwin':
            self.SetToolBar(self.toolbar)

        # tree
        self.tree = DataCatalogTree(parent=self.panel, giface=self._giface)
        self.tree.InitTreeItems()
        self.tree.UpdateCurrentDbLocationMapsetNode()
        self.tree.ExpandCurrentMapset()
        self.tree.changeMapset.connect(lambda mapset:
                                       self.ChangeDbLocationMapset(
                                               location=None,
                                               mapset=mapset))
        self.tree.changeLocation.connect(lambda mapset, location, dbase:
                                         self.ChangeDbLocationMapset(
                                                 dbase=dbase,
                                                 location=location,
                                                 mapset=mapset))

        # buttons
        self.btnClose = Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btnClose.SetToolTip(_("Close GRASS GIS Data Catalog"))
        self.btnClose.SetDefault()

        # events
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self._layout()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.tree, proportion=1, flag=wx.EXPAND)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(self.btnClose)

        sizer.Add(btnSizer, proportion=0,
                  flag=wx.ALL | wx.EXPAND,
                  border=5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        self.SetMinSize((400, 500))

    def OnCloseWindow(self, event):
        """Cancel button pressed"""
        if not isinstance(event, wx.CloseEvent):
            self.Destroy()

        event.Skip()

    def OnReloadTree(self, event):
        """Reload whole tree"""
        self.tree.ReloadTreeItems()
        self.tree.ExpandCurrentMapset()

    def OnReloadCurrentMapset(self, event):
        """Reload current mapset tree only"""
        self.tree.ReloadCurrentMapset()

    def OnAddGrassDB(self, event):
        """Add an existing grass database"""
        dlg = wx.DirDialog(self, _("Choose GRASS data directory:"),
                           os.getcwd(), wx.DD_DEFAULT_STYLE)
        if dlg.ShowModal() == wx.ID_OK:
            grassdatabase = dlg.GetPath()
            self.tree.InsertGrassDb(name=grassdatabase)

        dlg.Destroy()

    def OnCreateMapset(self, event):
        """Create new mapset in current location"""
        genv = gisenv()
        gisdbase = genv['GISDBASE']
        location = genv['LOCATION_NAME']
        try:
            mapset = create_mapset_interactively(self, gisdbase, location)
            if mapset:
                db = self.tree._model.SearchNodes(name=gisdbase,
                                                  type='grassdb')
                loc = self.tree._model.SearchNodes(parent=db[0],
                                                   name=location,
                                                   type='location')
                self.tree.InsertMapset(name=mapset,
                                       location_node=loc[0])
                self.tree.ReloadTreeItems()
        except Exception as e:
            GError(parent=self,
                   message=_("Unable to create new mapset: %s") % e,
                   showTraceback=False)

    def SetRestriction(self, restrict):
        """Allow editing other mapsets or restrict editing to current mapset"""
        self.tree.SetRestriction(restrict)

    def ChangeDbLocationMapset(self, mapset, location=None, dbase=None):
        """Change mapset, location or db"""
        if dbase:
            if RunCommand('g.mapset', parent=self,
                          location=location,
                          mapset=mapset,
                          dbase=dbase) == 0:
                GMessage(parent=self,
                         message=_("Current GRASS database is <%(dbase)s>.\n"
                                   "Current location is <%(loc)s>.\n"
                                   "Current mapset is <%(mapset)s>."
                                   ) %
                         {'dbase': dbase, 'loc': location, 'mapset': mapset})
        elif location:
            if RunCommand('g.mapset', parent=self,
                          location=location,
                          mapset=mapset) == 0:
                GMessage(parent=self,
                         message=_("Current location is <%(loc)s>.\n"
                                   "Current mapset is <%(mapset)s>.") %
                         {'loc': location, 'mapset': mapset})
        else:
            if RunCommand('g.mapset',
                          parent=self,
                          mapset=mapset) == 0:
                GMessage(parent=self,
                         message=_("Current mapset is <%s>.") % mapset)

    def Filter(self, text):
        self.tree.Filter(text=text)
