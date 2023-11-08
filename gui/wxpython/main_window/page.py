"""
@package main_window.notebook

@brief Custom AuiNotebook class and class for undocked AuiNotebook frame

Classes:
 - page::MainPageBase

(C) 2023 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova <lindakladivova gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

from grass.pydispatch.signal import Signal


class MainPageBase:
    def __init__(self, dockable):
        self._mainnotebook = None

        self.canCloseCallback = None

        # distinquishes whether map panel is dockable (Single-Window)
        self._dockable = dockable

        # distinguishes whether map panel is docked or not
        self._docked = True

        # undock/dock bound method
        self._docking_callback = None

        # Emitted when switching map notebook tabs (Single-Window)
        self.onFocus = Signal("MainPage.onFocus")

        # Emitted when closing page by closing its window.
        self.closingPage = Signal("MainPage.closingPage")

        # Emitted when renaming page.
        self.renamingPage = Signal("MainPage.renamingPage")

    def _pgnumDict(self):
        """Get dictionary containg page index"""
        return {"mainnotebook": self._mainnotebook.GetPageIndex(self)}

    def SetUpPage(self, parent, notebook, can_close=None):
        self._mainnotebook = notebook

        def CanClosePage():
            return self._pgnumDict()

        # set callbacks
        self.canCloseCallback = CanClosePage if can_close is None else can_close
        self.SetDockingCallback(notebook.UndockPage)

        # bind various events
        self.closingPage.connect(parent._closePageNoEvent)
        self.renamingPage.connect(parent._renamePageNoEvent)

    def SetDockingCallback(self, function):
        """Sets docking bound method to dock or undock"""
        self._docking_callback = function

    def IsDocked(self):
        return self._docked

    def IsDockable(self):
        return self._dockable

    def OnDockUndock(self, event=None):
        """Dock or undock map display panel to independent MapFrame"""
        if self._docking_callback:
            self._docked = not self._docked
            self._docking_callback(self)

    def _onCloseWindow(self, event):
        """Close window"""
        if self.canCloseCallback:
            pgnum_dict = self.canCloseCallback()
            if pgnum_dict is not None:
                if self.IsDockable():
                    self.closingPage.emit(
                        pgnum_dict=pgnum_dict, is_docked=self.IsDocked()
                    )
                    if not self.IsDocked():
                        frame = self.GetParent()  # pylint: disable=E1101
                        frame.Destroy()
                else:
                    self.closingPage.emit(pgnum_dict=pgnum_dict)
                # Destroy is called when notebook page is deleted
        else:
            self.parent.Destroy()  # pylint: disable=E1101

    def RenamePage(self, title):
        """Rename page or change frame title"""
        if self.canCloseCallback:
            pgnum_dict = self._pgnumDict()
            if pgnum_dict is not None:
                if self.IsDockable():
                    self.renamingPage.emit(
                        pgnum_dict=pgnum_dict, is_docked=self.IsDocked(), text=title
                    )
                    if not self.IsDocked():
                        self.GetParent().SetTitle(title)  # pylint: disable=E1101
        else:
            self.GetParent().SetTitle(title)  # pylint: disable=E1101
