"""
@package jupyter_notebook.panel

@brief Panel for integration of Jupyter Notebook to GUI.

Classes:
 - panel::JupyterPanel

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

from pathlib import Path

import wx

from main_window.page import MainPageBase

from .environment import JupyterEnvironment
from .notebook import JupyterAuiNotebook
from .toolbars import JupyterToolbar


class JupyterPanel(wx.Panel, MainPageBase):
    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        title=_("Jupyter Notebook"),
        statusbar=None,
        dockable=False,
        workdir=None,
        create_template=False,
        **kwargs,
    ):
        """Jupyter main panel."""
        super().__init__(parent=parent, id=id, **kwargs)
        MainPageBase.__init__(self, dockable)

        self.parent = parent
        self._giface = giface
        self.statusbar = statusbar
        self.workdir = workdir
        self.SetName("Jupyter")

        self.env = JupyterEnvironment(
            workdir=self.workdir, create_template=create_template, integrated=True
        )

        self.toolbar = JupyterToolbar(parent=self)
        self.aui_notebook = JupyterAuiNotebook(parent=self)

        self._layout()

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.toolbar, proportion=0, flag=wx.EXPAND)
        sizer.Add(self.aui_notebook, proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()

    def SetUpNotebookInterface(self):
        """Setup Jupyter notebook environment and load initial notebooks."""
        try:
            self.env.setup()
        except Exception as e:
            wx.MessageBox(
                _("Failed to start Jupyter environment:\n{}").format(str(e)),
                _("Startup Error"),
                wx.ICON_ERROR,
            )
            return

        # Load notebook tabs in embedded AUI notebook
        for fname in self.env.directory.files:
            try:
                url = self.env.server.get_url(fname.name)
            except RuntimeError as e:
                wx.MessageBox(
                    _("Failed to get Jupyter server URLt:\n{}").format(str(e)),
                    _("Startup Error"),
                    wx.ICON_ERROR,
                )
                return
            self.aui_notebook.AddPage(url=url, title=fname.name)

        self.SetStatusText(
            _("Jupyter server started at {url} (PID: {pid}), directory: {dir}").format(
                url=self.env.server.server_url,
                pid=self.env.server.pid,
                dir=str(self.workdir),
            )
        )

    def Switch(self, file_name):
        """
        Switch to existing notebook tab.
        :param file_name: Name of the .ipynb file (e.g., 'example.ipynb') (str).
        :return: True if the notebook was found and switched to, False otherwise.
        """
        for i in range(self.aui_notebook.GetPageCount()):
            if self.aui_notebook.GetPageText(i) == file_name:
                self.aui_notebook.SetSelection(i)
                return True
        return False

    def Open(self, file_name):
        """
        Open a Jupyter notebook to a new tab and switch to it.
        :param file_name: Name of the .ipynb file (e.g., 'example.ipynb') (str).
        """
        try:
            url = self.env.server.get_url(file_name)
            self.aui_notebook.AddPage(url=url, title=file_name)
            self.aui_notebook.SetSelection(self.aui_notebook.GetPageCount() - 1)
        except RuntimeError as e:
            wx.MessageBox(
                _("Failed to get Jupyter server URL:\n{}").format(str(e)),
                _("URL Error"),
                wx.ICON_ERROR,
            )

    def OpenOrSwitch(self, file_name):
        """
        Switch to .ipynb file if open, otherwise open it.
        :param file_name: Name of the .ipynb file (e.g., 'example.ipynb') (str).
        """
        if self.Switch(file_name):
            self.SetStatusText(_("File '{}' is already opened.").format(file_name), 0)
        else:
            self.Open(file_name)
            self.SetStatusText(_("File '{}' opened.").format(file_name), 0)

    def Import(self, source_path, new_name=None):
        """
        Import a .ipynb file into a working directory and open it to a new tab.
        :param source_path: Path to the source .ipynb file to be imported (Path).
        :param new_name: Optional new name for the imported file (str).
        """
        try:
            path = self.env.directory.import_file(source_path, new_name=new_name)
            self.Open(path.name)
            self.SetStatusText(_("File '{}' imported and opened.").format(path.name), 0)
        except Exception as e:
            wx.MessageBox(
                _("Failed to import file:\n{}").format(str(e)),
                _("Notebook Import Error"),
                wx.ICON_ERROR | wx.OK,
            )

    def OnImport(self, event=None):
        """
        Import an existing Jupyter notebook file into the working directory
        and open it in the GUI.
        - Prompts user to select a .ipynb file.
        - If the selected file is already in the notebook directory:
            - Switch to it or open it.
        - If the file is from elsewhere:
            - Import the notebook and open it (if needed, prompt for a new name).
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
            target_path = self.workdir / file_name

            # File is already in the working directory
            if source_path.resolve() == target_path.resolve():
                self.OpenOrSwitch(file_name)
                return

            # File is from outside the working directory
            new_name = None
            if target_path.exists():
                # Prompt user for a new name if the notebook already exists
                with wx.TextEntryDialog(
                    self,
                    message=_(
                        "File '{}' already exists in working directory.\nPlease enter a new name:"
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
                self.env.directory.export_file(
                    file_name, destination_path, overwrite=True
                )
                self.SetStatusText(
                    _("File {} exported to {}.").format(file_name, destination_path), 0
                )
            except Exception as e:
                wx.MessageBox(
                    _("Failed to export file:\n{}").format(str(e)),
                    caption=_("Notebook Export Error"),
                    style=wx.ICON_ERROR | wx.OK,
                )

    def OnCreate(self, event=None):
        """
        Prompt the user to create a new empty Jupyter notebook in the working directory,
        and open it in the GUI.
        """
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
                path = self.env.directory.create_new_notebook(new_name=name)
            except Exception as e:
                wx.MessageBox(
                    _("Failed to create notebook:\n{}").format(str(e)),
                    caption=_("Notebook Creation Error"),
                    style=wx.ICON_ERROR | wx.OK,
                )
                return

            # Open the newly created notebook in the GUI
            self.Open(path.name)

    def SetStatusText(self, *args):
        """Set text in the status bar."""
        self.statusbar.SetStatusText(*args)

    def GetStatusBar(self):
        """Get statusbar"""
        return self.statusbar

    def OnCloseWindow(self, event):
        """Prompt user, then stop server and close panel."""
        confirm = wx.MessageBox(
            _("Do you really want to close this window and stop the Jupyter server?"),
            _("Confirm Close"),
            wx.ICON_QUESTION | wx.YES_NO | wx.NO_DEFAULT,
        )

        if confirm != wx.YES:
            if event and hasattr(event, "Veto"):
                event.Veto()
            return

        # Get server info
        url = self.env.server.server_url
        pid = self.env.server.pid

        # Stop server and close panel
        try:
            self.env.stop()
        except RuntimeError as e:
            wx.MessageBox(
                _("Failed to stop Jupyter server at {url} (PID: {pid}):\n{err}").format(
                    url=url, pid=pid, err=str(e)
                ),
                caption=_("Error"),
                style=wx.ICON_ERROR | wx.OK,
            )
            return
        self.SetStatusText(
            _("Jupyter server at {url} (PID: {pid}) has been stopped").format(
                url=url, pid=pid
            )
        )
        self._onCloseWindow(event)
