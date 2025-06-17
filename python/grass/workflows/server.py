#
# AUTHOR(S): Linda Karlovska <linda.karlovska@seznam.cz>
#
# PURPOSE:   Provides a simple interface for launching and managing
#            a local Jupyter server.
#
# COPYRIGHT: (C) 2025 by Linda Karlovska and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""
This module provides two classes for managing Jupyter Notebook servers
programmatically within GRASS GIS tools or scripting environments:

Classes:
- `JupyterServerInstance`: Manages a single Jupyter Notebook server instance.
- `JupyterServerRegistry`: Manages multiple `JupyterServerInstance` objects
  and provides methods to start, track, and stop all active servers.

Features of `JupyterServerInstance`:
- Checks if Jupyter Notebook is installed.
- Finds an available local port.
- Starts the server in a background thread.
- Verifies that the server is running and accessible.
- Provides the URL to access served files.
- Tracks and manages the server PID.
- Stops the server cleanly on request.
- Registers cleanup routines to stop the server on:
  - Normal interpreter exit
  - SIGINT (e.g., Ctrl+C)
  - SIGTERM (e.g., kill from shell)

Features of `JupyterServerRegistry`:
- Register and unregister server instances
- Keeps track of all active server instances.
- Stops all servers on global cleanup (e.g., GRASS shutdown).

Designed for use within GRASS GUI tools or scripting environments.
"""

import socket
import time
import subprocess
import threading
import http.client
import atexit
import signal
import sys
import shutil


class JupyterServerInstance:
    """Manage the lifecycle of a Jupyter server instance."""

    def __init__(self, workdir):
        self.workdir = workdir
        self._reset_state()
        self._setup_cleanup_handlers()

    def _reset_state(self):
        """Reset internal state related to the server."""
        self.pid = None
        self.port = None
        self.server_url = ""

    def _setup_cleanup_handlers(self):
        """Set up handlers to ensure the server is stopped on process exit or signals."""
        # Stop the server when the program exits normally (e.g., via sys.exit() or interpreter exit)
        atexit.register(self._safe_stop_server)

        # Stop the server when SIGINT is received (e.g., user presses Ctrl+C)
        signal.signal(signal.SIGINT, self._handle_exit_signal)

        # Stop the server when SIGTERM is received (e.g., 'kill PID')
        signal.signal(signal.SIGTERM, self._handle_exit_signal)

    def _safe_stop_server(self):
        """
        Quietly stop the server without raising exceptions.

        Used for cleanup via atexit or signal handlers.
        """
        try:
            self.stop_server()
        except Exception:
            pass

    def _handle_exit_signal(self, signum, frame):
        """Handle termination signals and ensure the server is stopped."""
        try:
            threading.Thread(target=self._safe_stop_server, daemon=True).start()
        except Exception:
            pass
        finally:
            sys.exit(0)

    @staticmethod
    def is_jupyter_notebook_installed():
        """Check if Jupyter Notebook is installed.
        :return: True if Jupyter Notebook is installed, False otherwise (bool).
        """
        try:
            subprocess.check_output(["jupyter", "notebook", "--version"])
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False

    @staticmethod
    def find_free_port():
        """Find a free port on the local machine.
        :return: A free port number (int).
        """
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            return sock.getsockname()[1]

    def is_server_running(self, retries=10, delay=0.2):
        """Check if the server in responding on the given port.
        :param retries: Number of retries before giving up (int).
        :param delay: Delay between retries in seconds (float).
        :return: True if the server is up, False otherwise (bool).
        """
        for _ in range(retries):
            try:
                conn = http.client.HTTPConnection("localhost", self.port, timeout=0.5)
                conn.request("GET", "/")
                if conn.getresponse().status in (200, 302, 403):
                    conn.close()
                    return True
                conn.close()
            except Exception:
                time.sleep(delay)
        return False

    def start_server(self):
        """Run Jupyter server in the given directory on a free port."""
        # Check if Jupyter Notebook is installed
        if not JupyterServerInstance.is_jupyter_notebook_installed():
            raise RuntimeError(_("Jupyter Notebook is not installed"))

        # Find free port and build server url
        self.port = JupyterServerInstance.find_free_port()
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
                    self.workdir,
                ],
            )
            pid_container.append(proc.pid)

        # Start the server in a separate thread
        thread = threading.Thread(target=run_server, args=(pid_container,), daemon=True)
        thread.start()

        # Check if the server is up
        if not self.is_server_running(self.port):
            raise RuntimeError(_("Jupyter server is not running"))

        # Save the PID of the Jupyter server
        self.pid = pid_container[0] if pid_container else None

    def stop_server(self):
        """Stop the Jupyter server.
        :raises RuntimeError: If the server is not running or cannot be stopped.
        """
        if not self.pid or self.pid <= 0:
            raise RuntimeError(_("Jupyter server is not running or PID is invalid."))

        # Check if the process with the given PID is a Jupyter server
        try:
            ps_cmd = shutil.which("ps")
            if not ps_cmd:
                raise RuntimeError(_("Unable to find 'ps' command in PATH."))
            proc_name = (
                subprocess.check_output(["ps", "-p", str(self.pid), "-o", "args="])
                .decode()
                .strip()
            )
            if "jupyter-notebook" not in proc_name:
                raise RuntimeError(
                    _(
                        "Process with PID {} is not a Jupyter server: found '{}'."
                    ).format(self.pid, proc_name)
                )
        except subprocess.CalledProcessError as e:
            raise RuntimeError(
                _("No process found with PID {}.").format(self.pid)
            ) from e

        # Attempt to terminate the server process
        if self.is_server_running(self.port):
            try:
                kill_cmd = shutil.which("kill")
                if not kill_cmd:
                    raise RuntimeError(_("Unable to find 'kill' command in PATH."))
                subprocess.check_call(["kill", str(self.pid)])
            except subprocess.CalledProcessError as e:
                raise RuntimeError(
                    _("Could not terminate Jupyter server with PID {}.").format(
                        self.pid
                    )
                ) from e

        # Clean up internal state
        self._reset_state()

    def get_url(self, file_name):
        """Return full URL to a file served by this server.

        :param file_name: Name of the file (e.g. 'example.ipynb') (str).
        :return: Full URL to access the file (str).
        """
        if not self.server_url:
            raise RuntimeError(_("Server URL is not set. Start the server first."))

        return "{base}/notebooks/{file}".format(
            base=self.server_url.rstrip("/"), file=file_name
        )


class JupyterServerRegistry:
    """Registry of running JupyterServerInstance objects."""

    _instance = None

    @classmethod
    def get(cls):
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self):
        self.servers = []

    def register(self, server):
        if server not in self.servers:
            self.servers.append(server)

    def unregister(self, server):
        if server in self.servers:
            self.servers.remove(server)

    def stop_all_servers(self):
        for server in self.servers[:]:
            try:
                server.stop_server()
            except Exception as e:
                print(f"Failed to stop Jupyter server: {e}")
            finally:
                self.unregister(server)

    def get_servers(self):
        return list(self.servers)
