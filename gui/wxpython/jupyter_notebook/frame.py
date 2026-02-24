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
        parent: wx.Window,
        giface,
        action: str = "integrated",
        storage: Path | None = None,
        create_template: bool = False,
        id: int = wx.ID_ANY,
        **kwargs,
    ) -> None:
        """Initialize Jupyter frame.

        :param parent: Parent window
        :param giface: GRASS interface
        :param action: Mode - "integrated" or "browser"
        :param storage: Storage path for notebooks
        :param create_template: Whether to create template notebooks
        :param id: Window ID
        :param kwargs: Additional arguments passed to wx.Frame
        """
        # Set title based on mode and storage
        title = self._format_title(action, storage)

        super().__init__(parent=parent, id=id, title=title, **kwargs)

        self.SetName("JupyterFrame")

        icon_path = Path(globalvar.ICONDIR) / "grass.ico"
        self.SetIcon(wx.Icon(str(icon_path), wx.BITMAP_TYPE_ICO))

        self.statusbar = self.CreateStatusBar(number=1)
        self.panel = None
        self.storage = storage
        self.giface = giface

        # Try integrated mode first if requested
        if action == "integrated":
            success = self._setup_integrated_mode(storage, create_template)
            if success:
                self._layout()
                return

            # Integrated mode failed, offer browser fallback
            response = wx.MessageBox(
                _(
                    "Integrated mode failed: wx.html2.WebView is not functional on this system.\n\n"
                    "Would you like to open Jupyter Notebook in your external browser instead?"
                ),
                _("WebView Not Supported"),
                wx.ICON_ERROR | wx.YES_NO,
            )

            if response == wx.YES:
                action = "browser"
                # Update title for browser mode
                self.SetTitle(self._format_title("browser", storage))
            else:
                self.Close()
                return

        # Set up browser mode
        if action == "browser":
            success = self._setup_browser_mode(storage, create_template)
            if success:
                self._layout()
            else:
                self.Close()

    def _format_title(self, mode: str, storage: Path | None) -> str:
        """Format frame title with mode and storage path.

        :param mode: Mode name - "integrated" or "browser"
        :param storage: Storage path for notebooks
        :return: Formatted title string
        """
        if storage:
            mode_name = _("Integrated") if mode == "integrated" else _("Browser")
            # Show full absolute path
            return _("Jupyter Notebook ({}) - {}").format(mode_name, storage.resolve())
        return _("Jupyter Notebook")

    def _setup_integrated_mode(
        self, storage: Path | None, create_template: bool
    ) -> bool:
        """Setup integrated Jupyter panel. Returns True on success.

        :param storage: Storage path for notebooks
        :param create_template: Whether to create template notebooks
        :return: True if setup succeeded, False otherwise
        """
        try:
            self.panel = JupyterPanel(
                parent=self,
                giface=self.giface,
                storage=storage,
                create_template=create_template,
                statusbar=self.statusbar,
                dockable=False,
            )

            # Setup environment and load notebooks
            if not self.panel.SetUpEnvironment():
                self.panel.Destroy()
                self.panel = None
                return False

            return True

        except NotImplementedError:
            # WebView.New() raised NotImplementedError - not functional
            if self.panel:
                self.panel.Destroy()
                self.panel = None
            return False

    def _setup_browser_mode(self, storage: Path | None, create_template: bool) -> bool:
        """Setup browser-based Jupyter panel. Returns True on success.

        :param storage: Storage path for notebooks
        :param create_template: Whether to create template notebooks
        :return: True if setup succeeded, False otherwise
        """
        self.panel = JupyterBrowserPanel(
            parent=self,
            giface=self.giface,
            storage=storage,
            create_template=create_template,
            statusbar=self.statusbar,
            dockable=False,
        )

        # Setup environment and open in browser
        if not self.panel.SetUpEnvironment():
            self.panel.Destroy()
            self.panel = None
            return False

        return True

    def _layout(self) -> None:
        """Setup frame layout and size."""
        if self.panel:
            sizer = wx.BoxSizer(wx.VERTICAL)
            sizer.Add(self.panel, 1, wx.EXPAND)
            self.SetSizer(sizer)

            self.SetSize((800, 600))

            self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

    def OnCloseWindow(self, event: wx.CloseEvent) -> None:
        """Handle window close event.

        :param event: Close event
        """
        if self.panel and hasattr(self.panel, "OnCloseWindow"):
            self.panel.OnCloseWindow(event)

            if event.GetVeto():
                return
        self.Destroy()
