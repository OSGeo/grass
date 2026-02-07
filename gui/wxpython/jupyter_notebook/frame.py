"""
@package jupyter_notebook.frame

@brief Manages the Jupyter frame widget for multi-window GUI

Classes:
 - frame::JupyterFrame

(C) 2025-2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import os
import wx

from core import globalvar
from jupyter_notebook.panel import JupyterPanel


class JupyterFrame(wx.Frame):
    """Main window for the Jupyter Notebook interface."""

    def __init__(
        self,
        parent,
        giface,
        workdir=None,
        create_template=False,
        id=wx.ID_ANY,
        title=_("Jupyter Notebook"),
        **kwargs,
    ):
        super().__init__(parent=parent, id=id, title=title, **kwargs)

        self.SetName("JupyterFrame")

        icon_path = os.path.join(globalvar.ICONDIR, "grass.ico")
        self.SetIcon(wx.Icon(icon_path, wx.BITMAP_TYPE_ICO))

        self.statusbar = self.CreateStatusBar(number=1)

        self.panel = JupyterPanel(
            parent=self,
            giface=giface,
            workdir=workdir,
            create_template=create_template,
            statusbar=self.statusbar,
        )
        self.panel.SetUpNotebookInterface()

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.panel, 1, wx.EXPAND)
        self.SetSizer(sizer)

        self.SetSize((800, 600))

        self.Bind(wx.EVT_CLOSE, self.panel.OnCloseWindow)
