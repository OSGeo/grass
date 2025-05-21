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

import json
import subprocess
import threading
from pathlib import Path

import wx

try:
    import wx.html2 as html  # wx.html2 is available in wxPython 4.0 and later
except ImportError as e:
    raise RuntimeError(_("wx.html2 is required for Jupyter integration.")) from e

import grass.script as gs
import grass.jupyter as gj

from main_window.page import MainPageBase


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

    def start_jupyter_server(self, notebooks_dir):
        """Spustí Jupyter notebook server v daném adresáři na volném portu."""
        import socket
        import time

        # Najdi volný port
        sock = socket.socket()
        sock.bind(("", 0))
        port = sock.getsockname()[1]
        sock.close()

        # Spusť server v samostatném vlákně
        def run_server():
            subprocess.Popen(
                [
                    "jupyter",
                    "notebook",
                    "--no-browser",
                    "--NotebookApp.token=''",
                    "--NotebookApp.password=''",
                    "--port",
                    str(port),
                    "--notebook-dir",
                    notebooks_dir,
                ]
            )

        threading.Thread(target=run_server, daemon=True).start()

        output = subprocess.check_output(["jupyter", "notebook", "list"]).decode()
        print(output)

        # Počkej, až server naběhne (lepší by bylo kontrolovat výstup, zde jen krátké čekání)
        time.sleep(3)
        print(port)
        return f"http://localhost:{port}"

    def SetUpPage(self):
        """Set up the Jupyter Notebook interface."""
        gisenv = gs.gisenv()
        gisdbase = gisenv["GISDBASE"]
        location = gisenv["LOCATION_NAME"]
        mapset = gisenv["MAPSET"]
        mapset_path = f"{gisdbase}/{location}/{mapset}"
        notebooks_dir = Path(mapset_path) / "notebooks"
        notebooks_dir.mkdir(parents=True, exist_ok=True)
        self.session = gj.init(mapset_path)

        # Spusť Jupyter server v adresáři notebooks
        url_base = self.start_jupyter_server(notebooks_dir)

        # Najdi všechny .ipynb soubory v notebooks/
        ipynb_files = [f for f in Path.iterdir(notebooks_dir) if f.endswith(".ipynb")]
        print(ipynb_files)

        if not ipynb_files:
            print("No notebooks found in the directory.")
            # Pokud nejsou žádné soubory, vytvoř template
            new_notebook_name = "template.ipynb"
            print(new_notebook_name)
            new_notebook_path = Path(notebooks_dir) / (new_notebook_name)
            template = {
                "cells": [
                    {
                        "cell_type": "markdown",
                        "metadata": {},
                        "source": [
                            "# Template file\n",
                        ],
                    },
                    {
                        "cell_type": "markdown",
                        "metadata": {},
                        "source": [
                            "You can add your own code here\n",
                            "or create new notebooks in the GRASS GUI\n",
                            "and they will be automatically saved in the directory: `{}`\n".format(
                                notebooks_dir.replace("\\", "/")
                            ),
                            "and opened in the Jupyter Notebook interface.\n",
                            "\n",
                        ],
                    },
                    {
                        "cell_type": "code",
                        "execution_count": None,
                        "metadata": {},
                        "outputs": [],
                        "source": [
                            "import grass.script as gs",
                        ],
                    },
                    {
                        "cell_type": "code",
                        "execution_count": None,
                        "metadata": {},
                        "outputs": [],
                        "source": [
                            "print('Raster maps in the current mapset:')\n",
                            "for rast in gs.list_strings(type='raster'):\n",
                            "    print('  ', rast)\n",
                        ],
                    },
                    {
                        "cell_type": "code",
                        "execution_count": None,
                        "metadata": {},
                        "outputs": [],
                        "source": [
                            "print('\\nVector maps in the current mapset:')\n",
                            "for vect in gs.list_strings(type='vector'):\n",
                            "    print('  ', vect)\n",
                        ],
                    },
                ],
                "metadata": {
                    "kernelspec": {
                        "display_name": "Python 3",
                        "language": "python",
                        "name": "python3",
                    },
                    "language_info": {"name": "python", "version": "3.x"},
                },
                "nbformat": 4,
                "nbformat_minor": 2,
            }
            print(new_notebook_path)
            print("template")
            with open(new_notebook_path, "w", encoding="utf-8") as f:
                json.dump(template, f, ensure_ascii=False, indent=2)
            ipynb_files.append(new_notebook_name)

        notebook = wx.Notebook(self)

        # Po načtení stránky injektujte JS pro skrytí File menu
        def hide_file_menu(event):
            browser = event.GetEventObject()
            print(browser)
            js = """
            var interval = setInterval(function() {
                // Skrytí File menu
                var fileMenu = document.querySelector('li#file_menu, a#filelink, a[aria-controls="file_menu"]');
                if (fileMenu) {
                    if (fileMenu.tagName === "LI") {
                        fileMenu.style.display = 'none';
                    } else if (fileMenu.parentElement && fileMenu.parentElement.tagName === "LI") {
                        fileMenu.parentElement.style.display = 'none';
                    }
                }
                // Skrytí horního panelu
                var header = document.getElementById('header-container');
                if (header) {
                    header.style.display = 'none';
                }
                // Ukonči interval, pokud jsou oba prvky nalezeny
                if (fileMenu && header) {
                    clearInterval(interval);
                }
            }, 500);
            """

            browser.RunScript(js)

        for fname in ipynb_files:
            url_base = url_base.rstrip("/")
            url = f"{url_base}/notebooks/{fname}"
            browser = html.WebView.New(notebook)
            wx.CallAfter(browser.LoadURL, url)
            wx.CallAfter(browser.Bind, html.EVT_WEBVIEW_LOADED, hide_file_menu)
            notebook.AddPage(browser, fname)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(notebook, 1, wx.EXPAND)
        self.SetSizer(sizer)
