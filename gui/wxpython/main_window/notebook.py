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
        super().__init__(
            parent=self.parent, id=wx.ID_ANY, agwStyle=agwStyle
        )
        client_size =  self.parent.GetClientSize()
        print(client_size)
        self.SetPosition(wx.Point(client_size.x, client_size.y))
        self.SetSize(430, 200)

        self.SetArtProvider(SimpleTabArt())

        # the list of docked map displays
        self.dock_dict = {}
        print(self.dock_dict)

        # the list of all map displays and their display indexes
        self.map_dict = {}
        print(self.map_dict)

        # bindings
        self.Bind(
            aui.EVT_AUINOTEBOOK_PAGE_CHANGED,
            lambda evt: self.GetCurrentPage().onFocus.emit(),
        )
        self.Bind(aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.OnClose)

    def _extendDockDict(self, index, frame):
        """Note map display indexes and their frames"""
        print("_extendDockDict")
        print(self.dock_dict)
        self.dock_dict[index] = frame
        print(self.dock_dict)

    def _removeFromDockDict(self, frame):
        """Note map display indexes and their frames"""
        print("_removeFromDockDict")
        print(self.dock_dict)
        self.dock_dict = {key:val for key, val in self.dock_dict.items() if val != frame}
        print(self.dock_dict)

    def UndockMapDisplay(self, page):
        """Undock active map display to independent MapFrame object"""
        idx = self.GetPageIndex(page)
        text = self.GetPageText(idx)
        self.RemovePage(idx)
        fr = MapPageFrame(parent=self.parent, mapdisplay=page, title=text)
        self._extendDockDict(page.GetId(), fr)
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
        self._removeFromDockDict(fr)
        page.SetDockingCallback(self.UndockMapDisplay)
        super().AddPage(page=page, caption=fr.GetTitle())
        fr.closeFrameNoEvent()

    def AddPage(self, display_index,**kwargs):
        """Add page to notebook and make it current"""
        super().AddPage(**kwargs)
        self.SetSelection(self.GetPageCount() - 1)
        mapdisplay = kwargs["page"]
        self.map_dict[mapdisplay] = display_index
        print("AddPage")

    def SetSelection(self, index):
        """Select either a MapNotebook page or an undocked independent frame"""
        try:
            print("SetSelection")
            if index in self.dock_dict:
                print("SetSelection - undocked")
                self.dock_dict[index].Raise()
            else:
                print("SetSelection - normally docked")
                super().SetSelection(index)
        except Exception:
            pass

    def DeletePage(self, index):
        """Destroy either a MapNotebook page or an undocked independent frame"""
        try:
            print("DeletePage")
            if index in self.dock_dict:
                print("DeletePage - undocked")
                self.dock_dict[index].Destroy()
                self._removeFromDockDict(self.dock_dict[index])
            else:
                print("DeletePage - normally docked")
                super().DeletePage(index)
        except Exception:
            pass

    def GetDisplayIndex(self, page):
        print("GetDisplayIndex")
        print(self.map_dict)
        if page in self.map_dict:
            return self.map_dict[page]

    def OnClose(self, event):
        """Page of map notebook is being closed"""
        display = self.GetCurrentPage()
        display.OnCloseWindow(event=None, askIfSaveWorkspace=True)
        event.Veto()