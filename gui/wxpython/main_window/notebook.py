"""
@package frame.notebook

@brief Classes for main window statusbar management

Classes:
 - notebook::MapPageFrame
 - notebook::MapNotebook

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova <lindakladivova gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

import wx
import wx.lib.agw.aui as aui

from gui_core.wrap import SimpleTabArt


class MapPageFrame(wx.Frame):
    """Frame for independent map display window."""

    def __init__(self, parent, mapdisplay, title):
        wx.Frame.__init__(self, parent=parent, title=title)
        self.mapdisplay = mapdisplay
        self.SetSize(mapdisplay.GetSize())
        self.sizer = wx.BoxSizer(wx.HORIZONTAL)
        self.SetSizerAndFit(self.sizer)
        self.Bind(wx.EVT_CLOSE, self.OnClose)

    def closeFrameNoEvent(self):
        """Close frame without generating OnClose event."""
        self.Unbind(wx.EVT_CLOSE)
        self.Close()
        self.Bind(wx.EVT_CLOSE, self.OnClose)

    def OnClose(self, event):
        """Close frame and associated layer notebook page."""
        self.mapdisplay.OnCloseWindow(event=None, askIfSaveWorkspace=True)


class MapNotebook(aui.AuiNotebook):
    """Map notebook class. Overloades some AuiNotebook classes.
    Takes into consideration the dock/undock functionality.
    """

    def __init__(
        self,
        parent,
        agwStyle=aui.AUI_NB_DEFAULT_STYLE | aui.AUI_NB_TAB_EXTERNAL_MOVE | wx.NO_BORDER,
    ):
        self.parent = parent
        super().__init__(parent=self.parent, id=wx.ID_ANY, agwStyle=agwStyle)
        client_size = self.parent.GetClientSize()
        self.SetPosition(wx.Point(client_size.x, client_size.y))
        self.SetSize(430, 200)

        self.SetArtProvider(SimpleTabArt())

        # bindings
        self.Bind(
            aui.EVT_AUINOTEBOOK_PAGE_CHANGED,
            lambda evt: self.GetCurrentPage().onFocus.emit(),
        )
        self.Bind(aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.OnClose)

    def UndockMapDisplay(self, page):
        """Undock active map display to independent MapFrame object"""
        idx = self.GetPageIndex(page)
        text = self.GetPageText(idx)
        self.RemovePage(idx)
        fr = MapPageFrame(parent=self.parent, mapdisplay=page, title=text)
        page.Reparent(fr)
        page.SetDockingCallback(self.DockMapDisplay)
        fr.sizer.Add(page, proportion=1, flag=wx.EXPAND)
        fr.Show()
        page.Show()
        page.onFocus.emit()

    def DockMapDisplay(self, page):
        """Dock independent MapFrame object back to Aui.Notebook"""
        fr = page.GetParent()
        page.Reparent(self)
        page.SetDockingCallback(self.UndockMapDisplay)
        self.AddPage(page, fr.GetTitle())
        fr.closeFrameNoEvent()

    def AddPage(self, *args, **kwargs):
        """Overrides Aui.Notebook AddPage method. Adds page to notebook and make it current"""
        super().AddPage(*args, **kwargs)
        self.SetSelection(self.GetPageCount() - 1)

    def SetSelectionToPage(self, page):
        """Overrides Aui.Notebook SetSelectionToPage method.
        Decides whether to set selection to a MapNotebook page or an undocked independent frame"""
        try:
            super().SetSelection(self.GetPageIndex(page))
        except Exception:
            pass

        if not page.IsDocked():
            wx.CallLater(500, page.GetParent().Raise)

    def DeletePage(self, page):
        """Overrides Aui.Notebook DeletePage method.
        Decides whether to destroy a MapNotebook page or an undocked independent frame"""
        try:
            super().DeletePage(self.GetPageIndex(page))
        except Exception:
            pass

        if not page.IsDocked():
            page.Destroy()

    def SetPageText(self, page, name):
        """Overrides Aui.Notebook SetPageText method.
        Decides whether sets title to MapNotebook page or an undocked independent frame"""
        try:
            super().SetPageText(page_idx=self.GetPageIndex(page), text=name)
        except Exception:
            pass

        if not page.IsDocked():
            frame = page.GetParent()
            frame.SetTitle(name)
            wx.CallLater(500, frame.Raise)

    def OnClose(self, event):
        """Page of map notebook is being closed"""
        display = self.GetCurrentPage()
        display.OnCloseWindow(event=None, askIfSaveWorkspace=True)
        event.Veto()
