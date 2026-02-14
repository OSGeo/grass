"""
@package jupyter_notebook.server

@brief Simple interface for launching and managing
a local Jupyter server.

Classes:
 - server::JupyterServerInstance
 - server:: JupyterServerRegistry

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import socket
import time
import subprocess
import threading
import os
import shutil
import pathlib


class JupyterServerInstance:
    """Manage the lifecycle of a Jupyter server instance."""

    def __init__(self, workdir, integrated=True):
        """Initialize Jupyter server instance.

        :param workdir: Working directory for the Jupyter server (str).
        """
        self.workdir = workdir

        self.proc = None
        self._reset_state()

    def _reset_state(self):
        """Reset internal state related to the server."""
        self.pid = None
        self.port = None
        self.server_url = ""
        self.proc = None

    @staticmethod
    def find_free_port():
        """Find a free port on the local machine.

        :return: A free port number (int).
        """
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            return sock.getsockname()[1]

    def is_alive(self):
        """Check if the server process is still running.

        :return: True if process is running, False otherwise (bool).
        """
        if not self.proc:
            return False
        return self.proc.poll() is None

    def is_server_running(self, retries=50, delay=0.2):
        """Check if the server is responding on the given port.

        :param retries: Number of retries before giving up (int).
        :param delay: Delay between retries in seconds (float).
        :return: True if the server is up, False otherwise (bool).
        """
        if not self.port:
            return False

        for _ in range(retries):
            try:
                with socket.create_connection(("127.0.0.1", self.port), timeout=0.5):
                    return True
            except OSError:
                time.sleep(delay)

        return False

    def start_server(self, integrated=True):
        """
        Start a Jupyter server in the working directory on a free port.

        :param integrated:
            - If False, the notebook is launched in the default web browser (external, no GUI integration).
            - If True (default), the server runs headless and is intended for integration into the GRASS GUI (suitable for embedded WebView).
        :raises RuntimeError: If Jupyter is not installed, the working directory is invalid,
                            or the server fails to start.
        """
        # Validation checks
        if not pathlib.Path(self.workdir).is_dir():
            raise RuntimeError(
                _("Working directory does not exist: {}").format(self.workdir)
            )

        if not os.access(self.workdir, os.W_OK):
            raise RuntimeError(
                _("Working directory is not writable: {}").format(self.workdir)
            )

        if self.is_alive():
            raise RuntimeError(
                _("Server is already running on port {}").format(self.port)
            )

        # Find free port and build server url
        self.port = JupyterServerInstance.find_free_port()
        self.server_url = "http://127.0.0.1:{}".format(self.port)

        # Check if Jupyter is available in PATH
        jupyter = shutil.which("jupyter")
        if not jupyter:
            raise RuntimeError(
                _(
                    "Jupyter executable not found in PATH. "
                    "Please install Jupyter Notebook and ensure it is available in your system PATH."
                )
            )

        # Build command to start Jupyter Notebook server
        cmd = [
            jupyter,
            "notebook",
            "--NotebookApp.token=''",
            "--NotebookApp.password=''",
            "--port",
            str(self.port),
            "--notebook-dir",
            self.workdir,
        ]

        if integrated:
            cmd.insert(2, "--no-browser")

        # Start server
        try:
            self.proc = subprocess.Popen(
                cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                start_new_session=True,
            )
            self.pid = self.proc.pid

        except (OSError, ValueError, subprocess.SubprocessError) as e:
            raise RuntimeError(
                _(
                    "Failed to start Jupyter server. Ensure all dependencies are installed and accessible: {}"
                ).format(e)
            ) from e

        # Check if the server is up
        if not self.is_server_running():
            # Server failed to start
            try:
                self.proc.kill()
                self.proc.wait()
                self.proc.wait(timeout=3)
            except (OSError, subprocess.SubprocessError):
                pass

            self._reset_state()
            raise RuntimeError(
                _(
                    "Failed to start Jupyter server. "
                    "Check for port conflicts, missing dependencies, or insufficient permissions."
                )
            )

    def stop_server(self):
        """Stop the Jupyter server, ensuring no zombie processes.

        :raises RuntimeError: If the server cannot be stopped.
        """
        if not self.proc or not self.pid:
            return

        try:
            if self.proc.poll() is None:  # Still running
                try:
                    self.proc.terminate()  # Send SIGTERM
                    self.proc.wait(timeout=5)  # Wait up to 5 seconds, reap zombie
                except subprocess.TimeoutExpired:
                    # Force kill if terminate doesn't work
                    self.proc.kill()  # Send SIGKILL
                    self.proc.wait()  # Still need to reap after kill
            else:
                # already finished, just reap
                self.proc.wait()
        except (OSError, subprocess.SubprocessError) as e:
            raise RuntimeError(
                _("Error stopping Jupyter server (PID {}): {}").format(self.pid, e)
            ) from e

        finally:
            # Clean up internal state
            self._reset_state()

    def get_url(self, file_name):
        """Return full URL to a file served by this server.

        :param file_name: Name of the file (e.g. 'example.ipynb') (str).
        :return: Full URL to access the file (str).
        :raises RuntimeError: If server is not running or URL not set.
        """
        if not self.server_url:
            raise RuntimeError(_("Server URL is not set. Start the server first."))

        if not self.is_alive():
            raise RuntimeError(_("Jupyter server has stopped unexpectedly."))

        return "{}/notebooks/{}".format(self.server_url.rstrip("/"), file_name)


class JupyterServerRegistry:
    """Thread-safe registry of running JupyterServerInstance objects. Track integrated servers only."""

    _instance = None
    _lock = threading.Lock()

    @classmethod
    def get(cls):
        """Get the singleton registry instance (thread-safe).

        :return: The JupyterServerRegistry singleton instance.
        """
        if cls._instance is None:
            with cls._lock:
                # Double-check after acquiring lock
                if cls._instance is None:
                    cls._instance = cls()
        return cls._instance

    def __init__(self):
        """Initialize the registry."""
        self.servers = []
        self._servers_lock = threading.Lock()

    def register(self, server):
        """Register a server instance.

        :param server: JupyterServerInstance to register.
        """
        with self._servers_lock:
            if server not in self.servers:
                self.servers.append(server)

    def unregister(self, server):
        """Unregister a server instance."""
        with self._servers_lock:
            self.servers = [s for s in self.servers if s != server]

    def stop_all_servers(self):
        """Stop all registered servers."""
        errors = []

        with self._servers_lock:
            servers = list(self.servers)
            self.servers.clear()

        for server in servers:
            try:
                server.stop_server()
            except Exception as e:
                errors.append(str(e))

        if errors:
            raise RuntimeError(
                _("Some Jupyter servers failed to stop:\n{}").format("\n".join(errors))
            )
