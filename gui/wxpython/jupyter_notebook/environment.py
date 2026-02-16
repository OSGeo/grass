"""
@package jupyter_notebook.environment

@brief High-level orchestrator which coordinates
the setup and teardown of a Jupyter Notebook environment.

Classes:
 - environment::JupyterEnvironment

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

from .directory import JupyterDirectoryManager
from .server import JupyterServerInstance, JupyterServerRegistry
from .utils import is_jupyter_installed, is_wx_html2_available


class JupyterEnvironment:
    """Orchestrates directory manager and Jupyter server lifecycle.

    :param workdir: Directory for notebooks
    :param create_template: Whether to create template notebooks
    :param integrated: If False, server is intended to be opened in external browser.
                       If True, server is integrated into GRASS GUI and will be
                       automatically stopped on GUI exit.
    """

    def __init__(self, workdir, create_template, integrated):
        self.directory = JupyterDirectoryManager(workdir, create_template)
        self.server = JupyterServerInstance(workdir)
        self.integrated = integrated

    def setup(self):
        """Prepare files and start server."""
        if not is_jupyter_installed():
            raise RuntimeError(_("Jupyter Notebook is not installed"))

        if self.integrated and not is_wx_html2_available():
            raise RuntimeError(_("wx.html2 (WebView) support is not available"))

        # Prepare files
        self.directory.prepare_files()

        # Start server
        self.server.start_server(self.integrated)

    def stop(self):
        """Stop server only if integrated."""
        self.server.stop_server()
        JupyterServerRegistry.get().unregister(self.server)

    @classmethod
    def stop_all(cls):
        """Stop all running Jupyter servers and unregister them."""
        JupyterServerRegistry.get().stop_all_servers()
