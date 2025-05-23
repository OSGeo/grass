#
# AUTHOR(S): Linda Karlovska <linda.karlovska@seznam.cz>
#
# PURPOSE:   Provides a simple interface for launching and managing a local Jupyter Notebook
#            server within the current GRASS mapset. Includes utility methods for
#            detecting Jupyter installation, managing server lifecycle, and retrieving
#            process details.
#
# COPYRIGHT: (C) 2025 by Linda Karlovska and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""
This module provides a class `NotebookServerManager` for starting and stopping a
Jupyter Notebook server inside the current GRASS session. It also handles:

- Checking if Jupyter Notebook is installed
- Finding an available port
- Verifying server startup
- Returning the server URL

Intended for internal use within GRASS tools or scripts.
"""

import socket
import time
import subprocess
import threading
import http.client

from grass.jupyter import init


class NotebookServerManager:
    """Manage the lifecycle of a Jupyter Notebook server.

    Handles launching, stopping, and tracking a local Jupyter server
    within a specified working directory.
    """

    def __init__(self, notebook_workdir):
        self.notebook_workdir = notebook_workdir
        self.port = None
        self.server_url = None
        self.pid = None

    def _find_free_port(self):
        """Find a free port on the local machine.
        :return: A free port number.
        """
        sock = socket.socket()
        sock.bind(("", 0))
        port = sock.getsockname()[1]
        sock.close()
        return port

    @staticmethod
    def is_jupyter_notebook_installed():
        """Check if Jupyter notebook is installed.
        :return: True if Jupyter notebook is installed, False otherwise.
        """
        try:
            subprocess.check_output(["jupyter", "notebook", "--version"])
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False

    def is_server_running(self, port, retries=10, delay=0.2):
        """Wait until an HTTP server responds on the given port.
        :param port: Port number to check.
        :param retries: Number of retries before giving up.
        :param delay: Delay between retries in seconds.
        :return: True if the server is up, False otherwise.
        """
        for _ in range(retries):
            try:
                conn = http.client.HTTPConnection("localhost", port, timeout=0.5)
                conn.request("GET", "/")
                resp = conn.getresponse()
                if resp.status in (200, 302, 403):
                    conn.close()
                    return True
                conn.close()
            except Exception:
                time.sleep(delay)
        return False

    def start_server(self):
        """Run Jupyter notebook server in the given directory on a free port.
        :param notebooks_dir: Directory where the Jupyter notebook server will be started
        :return server_url str: URL of the Jupyter notebook server.
        """
        # Check if Jupyter notebook is installed
        if not NotebookServerManager.is_jupyter_notebook_installed():
            raise RuntimeError(_("Jupyter notebook is not installed"))

        # Find free port and build server url
        self.port = self._find_free_port()
        self.server_url = "http://localhost:{}".format(self.port)

        # Create container for PIDs
        pid_container = []

        # Run Jupyter notebook server
        def run_server(pid_container):
            proc = subprocess.Popen(
                [
                    "jupyter",
                    "notebook",
                    "--no-browser",
                    "--NotebookApp.token=''",
                    "--NotebookApp.password=''",
                    "--port",
                    str(self.port),
                    "--notebook-dir",
                    self.notebook_workdir,
                ],
            )
            pid_container.append(proc.pid)

        # Save the PID of the Jupyter notebook server
        self.pid = pid_container[0] if pid_container else None

        # Start the server in a separate thread
        thread = threading.Thread(target=run_server, args=(pid_container,), daemon=True)
        thread.start()

        # Initialize the grass.jupyter session for the current mapset
        self.initialize_session()

        # Check if the server is up
        if not self.is_server_running(self.port):
            raise RuntimeError(_("Jupyter server is not running"))

    def initialize_session(self):
        """Initialize the Jupyter notebook session.

        This method is called to set up the Jupyter notebook .
        """
        # Derive mapset path and initialize GRASS backend
        mapset_path = self.notebook_workdir.parent
        self.session = init(mapset_path)

    def get_notebook_url(self, notebook_name):
        """Return full URL to a notebook served by this server.

        :param notebook_name: Name of the notebook file (e.g. 'example.ipynb')
        :return: Full URL to access the notebook
        """
        if not self.server_url:
            raise RuntimeError(_("Server URL is not set. Start the server first."))

        return "{base}/notebooks/{file}".format(
            base=self.server_url.rstrip("/"), file=notebook_name
        )

    def stop_server(self):
        """Stop the Jupyter notebook server.
        :return: None
        """
        # Find the PID of the Jupyter notebook server
        try:
            subprocess.check_call(["kill", str(self.pid)])
        except subprocess.CalledProcessError:
            pass  # No Jupyter server running
