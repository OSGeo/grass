"""
@package jupyter_notebook::panel

@brief Integration of Jupyter notebook to GUI.

Classes:
 - panel::JupyterPanel

(C) 2025 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import wx

try:
    import wx.html2 as html  # wx.html2 is available in wxPython 4.0 and later
except ImportError as e:
    raise RuntimeError(_("wx.html2 is required for Jupyter integration.")) from e

from main_window.page import MainPageBase

from grass.notebooks.launcher import NotebookServerManager
from grass.notebooks.directory import NotebookDirectoryManager


class JupyterPanel(wx.Panel, MainPageBase):
    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        title=_("Jupyter Notebook"),
        statusbar=None,
        dockable=False,
        **kwargs,
    ):
        """Jupyter Notebook main panel
        :param parent: parent window
        :param giface: GRASS interface
        :param id: window id
        :param title: window title

        :param kwargs: wx.Panel' arguments
        """
        self.parent = parent
        self._giface = giface
        self.statusbar = statusbar

        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)
        MainPageBase.__init__(self, dockable)

        self.SetName("Jupyter")

    def _hide_file_menu(self, event):
        """Inject JavaScript to hide Jupyter's File menu
        and header after load.
        :param event: wx.EVT_WEBVIEW_LOADED event
        """
        # Hide File menu and header
        webview = event.GetEventObject()
        js = """
        var interval = setInterval(function() {
            // Hide File menu
            var fileMenu = document.querySelector('li#file_menu, a#filelink, a[aria-controls="file_menu"]');
            if (fileMenu) {
                if (fileMenu.tagName === "LI") {
                    fileMenu.style.display = 'none';
                } else if (fileMenu.parentElement && fileMenu.parentElement.tagName === "LI") {
                    fileMenu.parentElement.style.display = 'none';
                }
            }
            // Hide top header
            var header = document.getElementById('header-container');
            if (header) {
                header.style.display = 'none';
            }
            // Stop checking once both are hidden
            if (fileMenu && header) {
                clearInterval(interval);
            }
        }, 500);
        """
        webview.RunScript(js)

    def _add_notebook_tab(self, url, title):
        """Add a new tab with a Jupyter notebook loaded in a WebView.

        This method creates a new browser tab inside the notebook panel
        and loads the given URL. After the page is loaded, it injects
        JavaScript to hide certain UI elements from the Jupyter interface.

        :param url: URL to the Jupyter notebook
        :param title: Title of the new tab
        """
        webview = html.WebView.New(self.notebook)
        wx.CallAfter(webview.LoadURL, url)
        wx.CallAfter(webview.Bind, html.EVT_WEBVIEW_LOADED, self._hide_file_menu)
        self.notebook.AddPage(webview, title)

    def SetUpPage(self):
        """Set up the Jupyter Notebook interface."""
        # Create a directory manager to handle notebook files
        # and a server manager to run the Jupyter server
        dir_manager = NotebookDirectoryManager()
        dir_manager.prepare_notebook_files()

        server_manager = NotebookServerManager(dir_manager.notebook_workdir)
        server_manager.start_server()

        self.notebook = wx.Notebook(self)

        # Create a new tab for each notebook file
        for fname in dir_manager.notebook_files:
            print(fname)
            url = server_manager.get_notebook_url(fname)
            self._add_notebook_tab(url, fname)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.notebook, 1, wx.EXPAND)
        self.SetSizer(sizer)
