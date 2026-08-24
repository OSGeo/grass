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

from .storage import JupyterStorageManager, WELCOME_NOTEBOOK_NAME
from .server import JupyterServerInstance, JupyterServerRegistry


class JupyterEnvironment:
    """Orchestrates storage manager and Jupyter server lifecycle."""

    def __init__(self, storage: Path | None, create_template: bool) -> None:
        """Initialize Jupyter environment.

        :param storage: Storage path for notebooks
        :param create_template: Whether to create template notebooks
        """
        self.storage_manager = JupyterStorageManager(storage, create_template)
        self.server = JupyterServerInstance(storage)

    def setup(self) -> None:
        """Prepare files and start server."""
        # Prepare files
        self.storage_manager.prepare_files()

        # Start server
        self.server.start_server()

    def stop(self) -> None:
        """Stop server and unregister it."""
        self.server.stop_server()
        JupyterServerRegistry.get().unregister(self.server)

    @classmethod
    def stop_all(cls) -> None:
        """Stop all running Jupyter servers and unregister them."""
        JupyterServerRegistry.get().stop_all_servers()

    @property
    def server_url(self) -> str | None:
        """Get server URL.

        :return: Server URL or None if server is not available
        """
        return self.server.server_url if self.server else None

    @property
    def pid(self) -> int | None:
        """Get server process ID.

        :return: Process ID or None if server is not running
        """
        return self.server.pid if self.server else None

    @property
    def storage(self) -> Path | None:
        """Get Jupyter notebook storage path.

        :return: Storage path or None if storage manager is not available
        """
        return self.storage_manager.storage if self.storage_manager else None

    @property
    def files(self) -> list[Path]:
        """Return list of notebook files.

        :return: List of notebook file paths
        """
        return self.storage_manager.files if self.storage_manager else []

    @property
    def template_url(self) -> str | None:
        """Get URL to welcome notebook if it exists.

        :return: URL to welcome.ipynb if it exists, None otherwise
        """
        if not self.server_url:
            return None

        # Check if welcome notebook exists
        template_path = Path(self.storage_manager.storage) / WELCOME_NOTEBOOK_NAME
        if template_path.exists():
            return "{}/notebooks/{}".format(self.server_url, WELCOME_NOTEBOOK_NAME)

        return None
