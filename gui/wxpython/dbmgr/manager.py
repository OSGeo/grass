"""
@package dbmgr.manager

@brief GRASS Attribute Table Manager

This program is based on FileHunter, published in 'The wxPython Linux
Tutorial' on wxPython WIKI pages.

It also uses some functions at
http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/426407

List of classes:
 - manager::AttributeManager

(C) 2007-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Jachym Cepicky <jachym.cepicky gmail.com>
@author Martin Landa <landa.martin gmail.com>
@author Refactoring by Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
"""

import os

import wx
from core import globalvar

if globalvar.wxPythonPhoenix:
    try:
        import agw.flatnotebook as FN
    except ImportError:  # if it's not there locally, try the wxPython lib.
        import wx.lib.agw.flatnotebook as FN
else:
    import wx.lib.flatnotebook as FN

from core.gcmd import GMessage
from dbmgr.base import DbMgrBase
from gui_core.widgets import GNotebook
from gui_core.wrap import Button, ClearButton, CloseButton


class AttributeManager(wx.Frame, DbMgrBase):
    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        title=None,
        base_title=None,
        vectorName=None,
        item=None,
        log=None,
        selection=None,
        **kwargs,
    ):
        """GRASS Attribute Table Manager window

        :param parent: parent window
        :param id: window id
        :param title: full window title or None for default title
        :param base_title: the document independent part of title or None for default
        :param vectorName: name of vector map
        :param item: item from Layer Tree
        :param log: log window
        :param selection: name of page to be selected
        :param kwagrs: other wx.Frame's arguments
        """
        self.parent = parent
        try:
            mapdisplay = self.parent.GetMapDisplay()
        except:
            mapdisplay = None

        DbMgrBase.__init__(
            self,
            id=id,
            mapdisplay=mapdisplay,
            vectorName=vectorName,
            item=item,
            log=log,
            statusbar=self,
            **kwargs,
        )

        wx.Frame.__init__(self, parent, id, *kwargs)

        # title
        if not title:
            if not base_title:
                base_title = _("Attribute Table Manager")
            document = self.dbMgrData["vectName"]
            if not self.dbMgrData["editable"]:
                document = _("{document} (read-only)").format(document=document)
            title = "{document} - {tool_name}".format(
                document=document, tool_name=base_title
            )
        self.SetTitle(title)

        # icon
        self.SetIcon(
            wx.Icon(
                os.path.join(globalvar.ICONDIR, "grass_sql.ico"), wx.BITMAP_TYPE_ICO
            )
        )

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        if len(self.dbMgrData["mapDBInfo"].layers.keys()) == 0:
            GMessage(
                parent=self.parent,
                message=_(
                    "Database connection for vector map <%s> "
                    "is not defined in DB file. "
                    "You can define new connection in "
                    "'Manage layers' tab."
                )
                % self.dbMgrData["vectName"],
            )

        busy = wx.BusyInfo(
            _("Please wait, loading attribute data..."), parent=self.parent
        )
        wx.SafeYield()
        self.CreateStatusBar(number=1)

        self.notebook = GNotebook(self.panel, style=globalvar.FNPageDStyle)

        self.CreateDbMgrPage(parent=self, pageName="browse")

        self.notebook.AddPage(
            page=self.pages["browse"], text=_("Browse data"), name="browse"
        )
        self.pages["browse"].SetTabAreaColour(globalvar.FNPageColor)

        self.CreateDbMgrPage(parent=self, pageName="manageTable")

        self.notebook.AddPage(
            page=self.pages["manageTable"], text=_("Manage tables"), name="table"
        )
        self.pages["manageTable"].SetTabAreaColour(globalvar.FNPageColor)

        self.CreateDbMgrPage(parent=self, pageName="manageLayer")
        self.notebook.AddPage(
            page=self.pages["manageLayer"], text=_("Manage layers"), name="layers"
        )
        del busy

        if selection:
            wx.CallAfter(self.notebook.SetSelectionByName, selection)
        else:
            wx.CallAfter(self.notebook.SetSelection, 0)  # select browse tab

        # buttons
        self.btnClose = CloseButton(parent=self.panel)
        self.btnClose.SetToolTip(_("Close Attribute Table Manager"))
        self.btnReload = Button(parent=self.panel, id=wx.ID_REFRESH)
        self.btnReload.SetToolTip(_("Reload currently selected attribute data"))
        self.btnReset = ClearButton(parent=self.panel)
        self.btnReset.SetToolTip(
            _("Reload all attribute data (drop current selection)")
        )

        # bind closing to ESC
        self.Bind(wx.EVT_MENU, self.OnCloseWindow, id=wx.ID_CANCEL)
        accelTableList = [(wx.ACCEL_NORMAL, wx.WXK_ESCAPE, wx.ID_CANCEL)]
        accelTable = wx.AcceleratorTable(accelTableList)
        self.SetAcceleratorTable(accelTable)

        # events
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)
        self.btnReload.Bind(wx.EVT_BUTTON, self.OnReloadData)
        self.btnReset.Bind(wx.EVT_BUTTON, self.OnReloadDataAll)
        self.notebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        # do layout
        self._layout()

        # self.SetMinSize(self.GetBestSize())
        self.SetSize((700, 550))  # FIXME hard-coded size
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """Do layout"""
        # frame body
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        # buttons
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnReset, proportion=1, flag=wx.ALL, border=5)
        btnSizer.Add(self.btnReload, proportion=1, flag=wx.ALL, border=5)
        btnSizer.Add(self.btnClose, proportion=1, flag=wx.ALL, border=5)

        mainSizer.Add(self.notebook, proportion=1, flag=wx.EXPAND)
        mainSizer.Add(btnSizer, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(mainSizer)
        mainSizer.Fit(self.panel)
        self.Layout()

    def OnCloseWindow(self, event):
        """Cancel button pressed"""
        if self.parent and self.parent.GetName() == "LayerManager":
            # deregister ATM
            self.parent.dialogs["atm"].remove(self)

        if not isinstance(event, wx.CloseEvent):
            self.Destroy()

        event.Skip()

    def OnReloadData(self, event):
        """Reload data"""
        if self.pages["browse"]:
            self.pages["browse"].OnDataReload(event)  # TODO replace by signal

    def OnReloadDataAll(self, event):
        """Reload all data"""
        if self.pages["browse"]:
            self.pages["browse"].ResetPage()

    def OnPageChanged(self, event):
        """On page in ATM is changed"""
        try:
            if self.pages["browse"]:
                selPage = self.pages["browse"].selLayer
                id = self.pages["browse"].layerPage[selPage]["data"]
            else:
                id = None
        except KeyError:
            id = None

        if event.GetSelection() == self.notebook.GetPageIndexByName("browse") and id:
            win = self.FindWindowById(id)
            if win:
                self.log.write(_("Number of loaded records: %d") % win.GetItemCount())
            else:
                self.log.write("")
            self.btnReload.Enable()
            self.btnReset.Enable()
        else:
            self.log.write("")
            self.btnReload.Enable(False)
            self.btnReset.Enable(False)

        event.Skip()

    def OnTextEnter(self, event):
        pass

    def UpdateDialog(self, layer):
        """Updates dialog layout for given layer"""
        DbMgrBase.UpdateDialog(self, layer=layer)
        # set current page selection
        self.notebook.SetSelectionByName("layers")
