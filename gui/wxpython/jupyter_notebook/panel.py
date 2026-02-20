"""
@package jupyter_notebook.panel

@brief Panel for integration of Jupyter Notebook to GUI.

Classes:
 - panel::JupyterPanel
 - panel::JupyterBrowserPanel

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

from pathlib import Path

import wx
import webbrowser

from main_window.page import MainPageBase

from .environment import JupyterEnvironment
from .notebook import JupyterAuiNotebook
from .toolbars import JupyterToolbar


class JupyterPanel(wx.Panel, MainPageBase):
    """Integrated Jupyter Notebook panel with embedded browser (requires wx.html2).

    This panel provides a full-featured Jupyter Notebook interface embedded
    directly in the GRASS GUI using wx.html2.WebView. Notebooks are displayed
    in tabs within the GUI itself.

    For a lightweight alternative that opens notebooks in an external browser
    without requiring wx.html2, see JupyterBrowserPanel.

    The Jupyter server is automatically stopped when this panel is closed.
    """

    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        title=_("Jupyter Notebook"),
        statusbar=None,
        dockable=False,
        storage=None,
        create_template=False,
        **kwargs,
    ):
        super().__init__(parent=parent, id=id, **kwargs)
        MainPageBase.__init__(self, dockable)

        self.parent = parent
        self._giface = giface
        self.statusbar = statusbar
        self.SetName("JupyterIntegrated")

        # Create environment in integrated mode (requires wx.html2)
        self.env = JupyterEnvironment(
            storage=storage,
            create_template=create_template,
        )

        self.toolbar = JupyterToolbar(parent=self)
        self.aui_notebook = JupyterAuiNotebook(parent=self)

        self._layout()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.toolbar, proportion=0, flag=wx.EXPAND)
        sizer.Add(self.aui_notebook, proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()

    def SetUpEnvironment(self):
        """Setup integrated Jupyter notebook environment and load initial notebooks.

        - Prepares notebook files in the notebook storage
        - Starts the Jupyter server
        - Loads all existing notebooks as tabs in the embedded browser

        :return: bool: True if setup was successful, False otherwise
        """
        try:
            # Prepare files and start server
            self.env.setup()
        except RuntimeError as e:
            wx.MessageBox(
                _("Failed to start Jupyter environment:\n{}").format(e),
                _("Startup Error"),
                wx.ICON_ERROR,
            )
            return False

        # Load notebook tabs in embedded AUI notebook
        for fname in self.env.files:
            try:
                url = self.env.server.get_url(fname.name)
            except RuntimeError as e:
                wx.MessageBox(
                    _("Failed to get Jupyter server URL:\n{}").format(e),
                    _("Startup Error"),
                    wx.ICON_ERROR,
                )
                return False

            self.aui_notebook.AddPage(url=url, title=fname.name)

        self.SetStatusText(
            _(
                "Jupyter server running at {url} (PID: {pid}) | Storage: {storage}"
            ).format(
                url=self.env.server_url,
                pid=self.env.pid,
                storage=self.env.storage,
            )
        )
        return True

    def Switch(self, file_name):
        """Switch to existing notebook tab.

        :param file_name: Name of the .ipynb file (e.g., 'example.ipynb')
        :return: True if the notebook was found and switched to, False otherwise
        """
        for i in range(self.aui_notebook.GetPageCount()):
            if self.aui_notebook.GetPageText(i) == file_name:
                self.aui_notebook.SetSelection(i)
                return True
        return False

    def Open(self, file_name):
        """Open a Jupyter notebook in a new tab and switch to it.

        :param file_name: Name of the .ipynb file (e.g., 'example.ipynb')
        """
        try:
            url = self.env.server.get_url(file_name)
            self.aui_notebook.AddPage(url=url, title=file_name)
            self.aui_notebook.SetSelection(self.aui_notebook.GetPageCount() - 1)
        except RuntimeError as e:
            wx.MessageBox(
                _("Failed to get Jupyter server URL:\n{}").format(e),
                _("URL Error"),
                wx.ICON_ERROR,
            )

    def OpenOrSwitch(self, file_name):
        """Switch to .ipynb file if open, otherwise open it.

        :param file_name: Name of the .ipynb file (e.g., 'example.ipynb')
        """
        if self.Switch(file_name):
            self.SetStatusText(_("File '{}' is already opened.").format(file_name), 0)
        else:
            self.Open(file_name)
            self.SetStatusText(_("File '{}' opened.").format(file_name), 0)

    def Import(self, source_path, new_name=None):
        """Import a .ipynb file into notebook storage and open it in a new tab.

        :param source_path: Path to the source .ipynb file to be imported
        :param new_name: Optional new name for the imported file
        """
        try:
            path = self.env.storage_manager.import_file(source_path, new_name=new_name)
            self.Open(path.name)
            self.SetStatusText(_("File '{}' imported and opened.").format(path.name), 0)
        except (FileNotFoundError, ValueError, FileExistsError) as e:
            wx.MessageBox(
                _("Failed to import file:\n{}").format(e),
                _("Notebook Import Error"),
                wx.ICON_ERROR | wx.OK,
            )

    def OnImport(self, event=None):
        """Import an existing Jupyter notebook file into notebook storage.

        Prompts user to select a .ipynb file:
        - If the file is already in the notebook storage: switch to it or open it
        - If the file is from elsewhere: import and open it (prompt for new name if needed)
        """
        # Open file dialog to select an existing Jupyter notebook file
        with wx.FileDialog(
            parent=wx.GetActiveWindow() or self.GetTopLevelParent(),
            message=_("Import file"),
            defaultDir=str(Path.cwd()),
            wildcard="Jupyter notebooks (*.ipynb)|*.ipynb",
            style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST,
        ) as dlg:
            if dlg.ShowModal() == wx.ID_CANCEL:
                return

            source_path = Path(dlg.GetPath())
            file_name = source_path.name
            target_path = self.env.storage / file_name

            # File is already in the storage
            if source_path.resolve() == target_path.resolve():
                self.OpenOrSwitch(file_name)
                return

            # File is from outside the storage
            new_name = None
            if target_path.exists():
                # Prompt user for a new name if the notebook already exists
                with wx.TextEntryDialog(
                    self,
                    message=_(
                        "File '{}' already exists in notebook storage.\nPlease enter a new name:"
                    ).format(file_name),
                    caption=_("Rename File"),
                    value="{}_copy".format(file_name.removesuffix(".ipynb")),
                ) as name_dlg:
                    if name_dlg.ShowModal() == wx.ID_CANCEL:
                        return
                    new_name = name_dlg.GetValue().strip()
                    if not new_name.endswith(".ipynb"):
                        new_name += ".ipynb"

            # Perform the import and open the notebook
            self.Import(source_path, new_name=new_name)

    def OnExport(self, event=None):
        """Export the currently opened Jupyter notebook to a user-selected location."""
        current_page = self.aui_notebook.GetSelection()
        if current_page == wx.NOT_FOUND:
            wx.MessageBox(
                _("No page for export is currently selected."),
                caption=_("Notebook Export Error"),
                style=wx.ICON_WARNING | wx.OK,
            )
            return
        file_name = self.aui_notebook.GetPageText(current_page)

        with wx.FileDialog(
            parent=wx.GetActiveWindow() or self.GetTopLevelParent(),
            message=_("Export file as..."),
            defaultFile=file_name,
            wildcard="Jupyter notebooks (*.ipynb)|*.ipynb",
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT,
        ) as dlg:
            if dlg.ShowModal() == wx.ID_CANCEL:
                return

            destination_path = Path(dlg.GetPath())

            try:
                self.env.storage_manager.export_file(
                    file_name, destination_path, overwrite=True
                )
                self.SetStatusText(
                    _("File {} exported to {}.").format(file_name, destination_path), 0
                )
            except (FileNotFoundError, FileExistsError) as e:
                wx.MessageBox(
                    _("Failed to export file:\n{}").format(e),
                    caption=_("Notebook Export Error"),
                    style=wx.ICON_ERROR | wx.OK,
                )

    def OnCreate(self, event=None):
        """Create a new empty Jupyter notebook in the notebook storage and open it."""
        with wx.TextEntryDialog(
            self,
            message=_("Enter a name for the new notebook:"),
            caption=_("New Notebook"),
            value="untitled",
        ) as dlg:
            if dlg.ShowModal() == wx.ID_CANCEL:
                return

            name = dlg.GetValue().strip()
            if not name:
                return

            try:
                path = self.env.storage_manager.create_new_notebook(new_name=name)
            except (FileExistsError, ValueError) as e:
                wx.MessageBox(
                    _("Failed to create notebook:\n{}").format(e),
                    caption=_("Notebook Creation Error"),
                    style=wx.ICON_ERROR | wx.OK,
                )
                return

            # Open the newly created notebook in the GUI
            self.Open(path.name)
            self.SetStatusText(_("New file '{}' created.").format(path.name), 0)

    def OnCloseWindow(self, event=None):
        """Cleanup when panel is being closed (called by parent notebook).

        This method is called by mainnotebook when the tab is being closed.
        """
        confirm = wx.MessageBox(
            _("Do you really want to close this tab and stop the Jupyter server?"),
            _("Confirm Close"),
            wx.ICON_QUESTION | wx.YES_NO | wx.NO_DEFAULT,
        )

        if confirm != wx.YES:
            if event and hasattr(event, "Veto"):
                event.Veto()
            return

        if self.env:
            # Get server info before stopping
            url = self.env.server_url
            pid = self.env.pid

            try:
                self.env.stop()
                if self.statusbar:
                    self.statusbar.SetStatusText(
                        _(
                            "Jupyter server at {url} (PID: {pid}) has been stopped"
                        ).format(url=url, pid=pid)
                    )
            except Exception as e:
                wx.MessageBox(
                    _("Failed to stop Jupyter server:\n{}").format(str(e)),
                    _("Error"),
                    wx.ICON_ERROR,
                )
                if event and hasattr(event, "Veto"):
                    event.Veto()
                return

        self._onCloseWindow(event)

    def SetStatusText(self, *args):
        """Set text in the status bar."""
        self.statusbar.SetStatusText(*args)

    def GetStatusBar(self):
        """Get statusbar."""
        return self.statusbar


class JupyterBrowserPanel(wx.Panel, MainPageBase):
    """Lightweight panel for Jupyter running in external browser.

    This panel doesn't require wx.html2 and just shows info about
    the running Jupyter server with a button to stop it.
    """

    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        statusbar=None,
        dockable=False,
        storage=None,
        create_template=False,
        **kwargs,
    ):
        super().__init__(parent=parent, id=id, **kwargs)
        MainPageBase.__init__(self, dockable)

        self.parent = parent
        self._giface = giface
        self.statusbar = statusbar
        self.SetName("JupyterBrowser")

        self.env = JupyterEnvironment(storage=storage, create_template=create_template)

        self._layout()

    def _layout(self):
        """Create simple layout with message and controls."""
        main_sizer = wx.BoxSizer(wx.VERTICAL)
        main_sizer.AddStretchSpacer(1)

        # Info icon and message
        info_sizer = wx.BoxSizer(wx.HORIZONTAL)
        info_icon = wx.StaticBitmap(
            self,
            bitmap=wx.ArtProvider.GetBitmap(wx.ART_INFORMATION, wx.ART_MESSAGE_BOX),
        )
        info_sizer.Add(info_icon, flag=wx.ALL, border=5)

        info_text = wx.StaticText(
            self,
            label=_("Jupyter Notebook has been opened in your default browser."),
        )
        info_text.SetFont(info_text.GetFont().Bold())
        info_sizer.Add(info_text, flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=5)

        main_sizer.Add(info_sizer, flag=wx.ALL | wx.ALIGN_CENTER, border=10)

        # Server details (will be populated after setup)
        details_sizer = wx.BoxSizer(wx.VERTICAL)

        self.url_text = wx.StaticText(self, label="")
        details_sizer.Add(self.url_text, flag=wx.ALL | wx.ALIGN_CENTER, border=3)

        self.pid_text = wx.StaticText(self, label="")
        details_sizer.Add(self.pid_text, flag=wx.ALL | wx.ALIGN_CENTER, border=3)

        self.dir_text = wx.StaticText(self, label="")
        details_sizer.Add(self.dir_text, flag=wx.ALL | wx.ALIGN_CENTER, border=3)

        main_sizer.Add(details_sizer, flag=wx.ALL | wx.ALIGN_CENTER, border=10)

        # Buttons
        button_sizer = wx.BoxSizer(wx.HORIZONTAL)

        open_browser_btn = wx.Button(self, label=_("Open in Browser Again"))
        open_browser_btn.Bind(wx.EVT_BUTTON, self.OnOpenBrowser)
        button_sizer.Add(open_browser_btn, flag=wx.ALL, border=5)

        stop_btn = wx.Button(self, label=_("Stop Jupyter Server"))
        stop_btn.Bind(wx.EVT_BUTTON, self.OnStop)
        button_sizer.Add(stop_btn, flag=wx.ALL, border=5)

        main_sizer.Add(button_sizer, flag=wx.ALL | wx.ALIGN_CENTER, border=10)
        main_sizer.AddStretchSpacer(2)

        self.SetSizer(main_sizer)
        self.Layout()

    def SetUpEnvironment(self):
        """Setup Jupyter environment and open in browser.

        :return: bool: True if setup was successful, False otherwis
        """

        try:
            # Prepare files and start server
            self.env.setup()
        except RuntimeError as e:
            wx.MessageBox(
                _("Failed to start Jupyter environment:\n{}").format(e),
                _("Startup Error"),
                wx.ICON_ERROR,
            )
            return False

        # Update UI with server info
        self.url_text.SetLabel(_("Server URL: {}").format(self.env.server_url))
        self.pid_text.SetLabel(_("Process ID: {}").format(self.env.pid))
        self.dir_text.SetLabel(_("Notebook Storage: {}").format(self.env.storage))

        self.Layout()

        self.SetStatusText(
            _(
                "Jupyter server running at {url} (PID: {pid}) | Storage: {storage}"
            ).format(
                url=self.env.server_url,
                pid=self.env.pid,
                storage=self.env.storage,
            )
        )

        # Open in default browser
        webbrowser.open(self.env.server_url)

        return True

    def OnOpenBrowser(self, event):
        """Re-open the Jupyter server URL in browser."""
        if self.env and self.env.server and self.env.server_url:
            webbrowser.open(self.env.server_url)

    def OnStop(self, event):
        """Stop the Jupyter server and close the tab."""
        if self.env:
            # Get server info before stopping
            url = self.env.server_url
            pid = self.env.pid

            try:
                # Stop server and unregister it
                self.env.stop()
                self.statusbar.SetStatusText(
                    _("Jupyter server at {url} (PID: {pid}) has been stopped").format(
                        url=url, pid=pid
                    )
                )
                # Close this tab/panel
                parent_notebook = self.GetParent()
                if hasattr(parent_notebook, "DeletePage"):
                    # Find our page index and delete it
                    for i in range(parent_notebook.GetPageCount()):
                        if parent_notebook.GetPage(i) == self:
                            parent_notebook.DeletePage(i)
                            break
            except Exception as e:
                wx.MessageBox(
                    _("Failed to stop server:\n{}").format(str(e)),
                    _("Error"),
                    wx.ICON_ERROR,
                )

    def OnCloseWindow(self, event=None):
        """Cleanup when panel is being closed (called by parent notebook).

        This method is called by mainnotebook when the tab is being closed.
        """
        confirm = wx.MessageBox(
            _("Do you really want to close this tab and stop the Jupyter server?"),
            _("Confirm Close"),
            wx.ICON_QUESTION | wx.YES_NO | wx.NO_DEFAULT,
        )

        if confirm != wx.YES:
            if event and hasattr(event, "Veto"):
                event.Veto()
            return

        if self.env:
            # Get server info before stopping
            url = self.env.server_url
            pid = self.env.pid

            try:
                self.env.stop()
                if self.statusbar:
                    self.statusbar.SetStatusText(
                        _(
                            "Jupyter server at {url} (PID: {pid}) has been stopped"
                        ).format(url=url, pid=pid)
                    )
            except Exception as e:
                wx.MessageBox(
                    _("Failed to stop Jupyter server:\n{}").format(str(e)),
                    _("Error"),
                    wx.ICON_ERROR,
                )
                if event and hasattr(event, "Veto"):
                    event.Veto()
                return

        self._onCloseWindow(event)

    def SetStatusText(self, *args):
        """Set text in the status bar."""
        if self.statusbar:
            self.statusbar.SetStatusText(*args)

    def GetStatusBar(self):
        """Get status bar."""
        return self.statusbar
