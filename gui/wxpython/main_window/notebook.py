"""
@package main_window.notebook

@brief Custom AuiNotebook class and class for undocked AuiNotebook frame

Classes:
 - notebook::MainPageFrame
 - notebook::MainNotebook

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova <lindakladivova gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

import os

import wx
from wx.lib.agw import aui

from core import globalvar
from gui_core.wrap import SimpleTabArt
from mapdisp.frame import MapPanel


class MainPageFrame(wx.Frame):
    """Frame for independent window."""

    def __init__(self, parent, panel, size, pos, title, icon="grass", menu=None):
        wx.Frame.__init__(self, parent=parent, size=size, pos=pos, title=title)
        self.panel = panel
        self.panel.Reparent(self)

        self._layout()

        # set system icon
        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, icon + ".ico"), wx.BITMAP_TYPE_ICO)
        )

        if menu is not None:
            self.SetMenuBar(menu)

        self.panel.onFocus.emit()
        self.Bind(wx.EVT_CLOSE, self.OnClose)

        self._show()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.panel, proportion=1, flag=wx.EXPAND)
        self.SetSizer(sizer)
        self.CentreOnParent()

    def _show(self):
        """Show frame and contained panel"""
        self.panel.Show()
        self.Show()

    def SetDockingCallback(self, function):
        """Set docking callback on reparented panel"""
        self.panel.SetDockingCallback(function)

    def OnClose(self, event):
        """Close frame and associated layer notebook page."""
        if isinstance(self.panel, MapPanel):
            self.panel.OnCloseWindow(event=None, askIfSaveWorkspace=True)
        else:
            self.panel.OnCloseWindow(event=None)


class MainNotebook(aui.AuiNotebook):
    """Main notebook class. Overrides some AuiNotebook classes.
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
        self.Bind(aui.EVT_AUINOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.OnClose)

        # remember number of items in the menu
        self._menuCount = self.parent.menubar.GetMenuCount()

    def OnPageChanged(self, event):
        page = self.GetCurrentPage()
        page.onFocus.emit()

        # set up menu
        mbar = self.parent.menubar
        if page.HasMenu():
            # add new (or replace if exists) additional menu item related to this page
            menu, menuName = page.GetMenu()
            if mbar.GetMenuCount() == self._menuCount:
                appendMenu = mbar.Insert
            else:
                appendMenu = mbar.Replace
            appendMenu(self._menuCount - 1, menu, menuName)
        elif mbar.GetMenuCount() > self._menuCount:
            # remove additional menu item
            mbar.Remove(self._menuCount - 1)

    def UndockPage(self, page):
        """Undock active page to independent MainFrame object"""
        index = self.GetPageIndex(page)
        text = self.GetPageText(index)
        original_size = page.GetSize()
        original_pos = page.GetPosition()
        icon = "grass_map" if isinstance(page, MapPanel) else "grass"
        if page.HasMenu():
            menu, _ = page.GetMenu()
        else:
            menu = None
        self.RemovePage(index)
        frame = MainPageFrame(
            parent=self.parent,
            panel=page,
            size=original_size,
            pos=original_pos,
            title=text,
            icon=icon,
            menu=menu,
        )
        frame.SetDockingCallback(self.DockPage)

    def DockPage(self, page):
        """Dock independent MainFrame object back to Aui.Notebook"""
        frame = page.GetParent()
        page.Reparent(self)
        page.SetDockingCallback(self.UndockPage)
        self.AddPage(page, frame.GetTitle())
        if frame.GetMenuBar():
            # avoid destroying menu if defined
            frame.SetMenuBar(None)
        frame.Destroy()

    def AddPage(self, *args, **kwargs):
        """Overrides Aui.Notebook AddPage method.
        Adds page to notebook and makes it current"""
        super().AddPage(*args, **kwargs)
        self.SetSelection(self.GetPageCount() - 1)

    def SetSelectionToMainPage(self, page):
        """Decides whether to set selection to a MainNotebook page
        or an undocked independent frame"""
        self.SetSelection(self.GetPageIndex(page))

        if not page.IsDocked():
            frame = page.GetParent()
            wx.CallLater(500, lambda: frame.Raise() if frame else None)

    def DeleteMainPage(self, page):
        """Decides whether to delete a MainNotebook page
        or close an undocked independent frame"""
        if page.IsDocked():
            self.DeletePage(self.GetPageIndex(page))
        else:
            frame = page.GetParent()
            frame.Destroy()

    def SetMainPageText(self, page, text):
        """Decides whether sets title to MainNotebook page
        or an undocked independent frame"""
        if page.IsDocked():
            self.SetPageText(page_idx=self.GetPageIndex(page), text=text)
        else:
            frame = page.GetParent()
            frame.SetTitle(text)
            wx.CallLater(500, frame.Raise)

    def OnClose(self, event):
        """Page of map notebook is being closed"""
        page = self.GetCurrentPage()
        if isinstance(page, MapPanel):
            page.OnCloseWindow(event=None, askIfSaveWorkspace=True)
        else:
            page.OnCloseWindow(event=None)
        event.Veto()
