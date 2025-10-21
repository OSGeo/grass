#
# AUTHOR(S): Linda Karlovska <linda.karlovska@seznam.cz>
#
# PURPOSE:   Provides an orchestration layer for Jupyter Notebook environment.
#
# COPYRIGHT: (C) 2025 by Linda Karlovska and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""
This module defines the `JupyterEnvironment` class, which coordinates
the setup and teardown of a Jupyter Notebook environment.

It acts as a high-level orchestrator that integrates:
- a working directory manager (template creation and file discovery)
- a Jupyter server instance (start, stop, URL management)
- registration of running servers in a global server registry

Designed for use within GRASS GUI tools or scripting environments.
"""

from grass.workflows.directory import JupyterDirectoryManager
from grass.workflows.server import JupyterServerInstance, JupyterServerRegistry


class JupyterEnvironment:
    """Orchestrates directory manager and server startup/shutdown."""

    def __init__(self, workdir, create_template):
        self.directory = JupyterDirectoryManager(workdir, create_template)
        self.server = JupyterServerInstance(workdir)

    def setup(self):
        """Prepare files and start server."""
        # Prepare files
        self.directory.prepare_files()

        # Start server
        self.server.start_server()

        # Register server in global registry
        JupyterServerRegistry.get().register(self.server)

    def stop(self):
        """Stop server and unregister it."""
        try:
            self.server.stop_server()
        finally:
            JupyterServerRegistry.get().unregister(self.server)

    @classmethod
    def stop_all(cls):
        """Stop all running Jupyter servers and unregister them."""
        JupyterServerRegistry.get().stop_all_servers()
