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

from pathlib import Path

from .storage import JupyterStorageManager
from .server import JupyterServerInstance, JupyterServerRegistry


class JupyterEnvironment:
    """Orchestrates storage manager and Jupyter server lifecycle.

    :param storage: Storage for notebooks
    :param create_template: Whether to create template notebooks
    """

    def __init__(self, storage, create_template):
        self.storage_manager = JupyterStorageManager(storage, create_template)
        self.server = JupyterServerInstance(storage)

    def setup(self):
        """Prepare files and start server."""
        # Prepare files
        self.storage_manager.prepare_files()

        # Start server
        self.server.start_server()

    def stop(self):
        """Stop server and unregister it."""
        self.server.stop_server()
        JupyterServerRegistry.get().unregister(self.server)

    @classmethod
    def stop_all(cls):
        """Stop all running Jupyter servers and unregister them."""
        JupyterServerRegistry.get().stop_all_servers()

    @property
    def server_url(self):
        """Get server URL."""
        return self.server.server_url if self.server else None

    @property
    def pid(self):
        """Get server process ID."""
        return self.server.pid if self.server else None

    @property
    def storage(self):
        """Get Jupyter notebook storage."""
        return self.storage_manager.storage if self.storage_manager else None

    @property
    def files(self) -> list[Path]:
        """Return list of notebook files."""
        return self.storage_manager.files if self.storage_manager else []
