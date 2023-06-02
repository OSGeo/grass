"""
@package main_window.notebook

@brief Custom AuiNotebook class and class for undocked AuiNotebook frame

Classes:
 - notebook::MapPageFrame
 - notebook::MapNotebook

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova <lindakladivova gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

import os

import wx
import wx.lib.agw.aui as aui

from core import globalvar
from gui_core.wrap import SimpleTabArt


class MapPageFrame(wx.Frame):
    """Frame for independent map display window."""

    def __init__(self, parent, mapdisplay, size, pos, title):
        wx.Frame.__init__(self, parent=parent, size=size, pos=pos, title=title)
        self.mapdisplay = mapdisplay
        self.mapdisplay.Reparent(self)

        self._layout()

        # set system icon
        self.SetIcon(
            wx.Icon(
                os.path.join(globalvar.ICONDIR, "grass_map.ico"), wx.BITMAP_TYPE_ICO
            )
        )

        self.mapdisplay.onFocus.emit()
        self.Bind(wx.EVT_CLOSE, self.OnClose)

        self._show()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.mapdisplay, proportion=1, flag=wx.EXPAND)
        self.SetSizer(sizer)
        self.CentreOnParent()

    def _show(self):
        """Show frame and contained mapdisplay panel"""
        self.mapdisplay.Show()
        self.Show()

    def SetDockingCallback(self, function):
        """Set docking callback on reparented mapdisplay panel"""
        self.mapdisplay.SetDockingCallback(function)

    def OnClose(self, event):
        """Close frame and associated layer notebook page."""
        self.mapdisplay.OnCloseWindow(event=None, askIfSaveWorkspace=True)


class MapNotebook(aui.AuiNotebook):
    """Map notebook class. Overrides some AuiNotebook classes.
    Takes into consideration the dock/undock functionality.
    """

    def __init__(
        self,
        parent,
        agwStyle=aui.AUI_NB_DEFAULT_STYLE | aui.AUI_NB_TAB_EXTERNAL_MOVE | wx.NO_BORDER,
    ):
        self.parent = parent
        super().__init__(parent=self.parent, id=wx.ID_ANY, agwStyle=agwStyle)

        self.SetArtProvider(SimpleTabArt())

        # bindings
        self.Bind(
            aui.EVT_AUINOTEBOOK_PAGE_CHANGED,
            lambda evt: self.GetCurrentPage().onFocus.emit(),
        )
        self.Bind(aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.OnClose)

    def UndockMapDisplay(self, page):
        """Undock active map display to independent MapFrame object"""
        index = self.GetPageIndex(page)
        text = self.GetPageText(index)
        original_size = page.GetSize()
        original_pos = page.GetPosition()
        self.RemovePage(index)
        frame = MapPageFrame(
            parent=self.parent,
            mapdisplay=page,
            size=original_size,
            pos=original_pos,
            title=text,
        )
        frame.SetDockingCallback(self.DockMapDisplay)

    def DockMapDisplay(self, page):
        """Dock independent MapFrame object back to Aui.Notebook"""
        frame = page.GetParent()
        page.Reparent(self)
        page.SetDockingCallback(self.UndockMapDisplay)
        self.AddPage(page, frame.GetTitle())
        frame.Destroy()

    def AddPage(self, *args, **kwargs):
        """Overrides Aui.Notebook AddPage method.
        Adds page to notebook and makes it current"""
        super().AddPage(*args, **kwargs)
        self.SetSelection(self.GetPageCount() - 1)

    def SetSelectionToMapPage(self, page):
        """Decides whether to set selection to a MapNotebook page
        or an undocked independent frame"""
        self.SetSelection(self.GetPageIndex(page))

        if not page.IsDocked():
            frame = page.GetParent()
            wx.CallLater(500, lambda: frame.Raise() if frame else None)

    def DeleteMapPage(self, page):
        """Decides whether to delete a MapNotebook page
        or close an undocked independent frame"""
        if page.IsDocked():
            self.DeletePage(self.GetPageIndex(page))
        else:
            frame = page.GetParent()
            frame.Destroy()

    def SetMapPageText(self, page, text):
        """Decides whether sets title to MapNotebook page
        or an undocked independent frame"""
        if page.IsDocked():
            self.SetPageText(page_idx=self.GetPageIndex(page), text=text)
        else:
            frame = page.GetParent()
            frame.SetTitle(text)
            wx.CallLater(500, frame.Raise)

    def OnClose(self, event):
        """Page of map notebook is being closed"""
        display = self.GetCurrentPage()
        display.OnCloseWindow(event=None, askIfSaveWorkspace=True)
        event.Veto()
