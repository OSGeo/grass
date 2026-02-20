"""
@package jupyter_notebook.frame

@brief Frame for integration of Jupyter Notebook to multi-window GUI.

Classes:
 - frame::JupyterFrame

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

from pathlib import Path

import wx

from core import globalvar
from jupyter_notebook.panel import JupyterPanel, JupyterBrowserPanel


class JupyterFrame(wx.Frame):
    """Main window for the Jupyter Notebook interface in multi-window GUI.

    Supports both integrated (embedded WebView) and browser (external) modes.
    """

    def __init__(
        self,
        parent,
        giface,
        action="integrated",
        storage=None,
        create_template=False,
        id=wx.ID_ANY,
        title=_("Jupyter Notebook"),
        **kwargs,
    ):
        super().__init__(parent=parent, id=id, title=title, **kwargs)

        self.SetName("JupyterFrame")

        icon_path = Path(globalvar.ICONDIR) / "grass.ico"
        self.SetIcon(wx.Icon(str(icon_path), wx.BITMAP_TYPE_ICO))

        self.statusbar = self.CreateStatusBar(number=1)
        self.panel = None

        # Try integrated mode first if requested
        if action == "integrated":
            try:
                self.panel = JupyterPanel(
                    parent=self,
                    giface=giface,
                    storage=storage,
                    create_template=create_template,
                    statusbar=self.statusbar,
                    dockable=False,
                )

                # Setup environment and load notebooks
                if not self.panel.SetUpEnvironment():
                    self.panel.Destroy()
                    self.panel = None
                    self.Close()
                    return

            except NotImplementedError as e:
                # WebView.New() raised NotImplementedError - not functional
                if self.panel:
                    self.panel.Destroy()
                    self.panel = None

                response = wx.MessageBox(
                    _(
                        "Integrated mode failed: wx.html2.WebView is not functional on this system.\n"
                        "Error: {}\n\n"
                        "Would you like to open Jupyter Notebook in your external browser instead?"
                    ).format(str(e)),
                    _("WebView Not Supported"),
                    wx.ICON_ERROR | wx.YES_NO,
                )

                if response == wx.YES:
                    action = "browser"
                else:
                    self.Close()
                    return

        # Browser mode
        if action == "browser":
            self.panel = JupyterBrowserPanel(
                parent=self,
                giface=giface,
                storage=storage,
                create_template=create_template,
                statusbar=self.statusbar,
                dockable=False,
            )

            # Setup environment and open in browser
            if not self.panel.SetUpEnvironment():
                self.panel.Destroy()
                self.panel = None
                self.Close()
                return

        self._layout()

    def _layout(self):
        if self.panel:
            sizer = wx.BoxSizer(wx.VERTICAL)
            sizer.Add(self.panel, 1, wx.EXPAND)
            self.SetSizer(sizer)

            self.SetSize((800, 600))

            self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

    def OnCloseWindow(self, event):
        if self.panel and hasattr(self.panel, "OnCloseWindow"):
            self.panel.OnCloseWindow(event)

            if event.GetVeto():
                return
        self.Destroy()
