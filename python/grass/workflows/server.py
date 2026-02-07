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
This module provides a simple interface for launching and managing
a local Jupyter server.

Functions:
- `is_jupyter_installed()`: Check if Jupyter Notebook is installed on the system.
- `is_wx_html2_available()`: Check if wx.html2 module is available.

Classes:
- `JupyterServerInstance`: Manages a single Jupyter Notebook server instance.
- `JupyterServerRegistry`: Manages multiple `JupyterServerInstance` objects
  and provides methods to start, track, and stop all active servers.

Features of `JupyterServerInstance`:
- Checks if Jupyter Notebook is installed.
- Finds an available local port.
- Starts the server with proper subprocess management.
- Verifies that the server is running and accessible.
- Provides the URL to access served files.
- Tracks and manages the server PID and process object.
- Stops the server cleanly, preventing zombie processes.
- Registers cleanup routines to stop servers on:
  - Normal interpreter exit
  - SIGINT (e.g., Ctrl+C)
  - SIGTERM (e.g., kill from shell)

Features of `JupyterServerRegistry`:
- Thread-safe registration and unregistration of server instances
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
import os
import shutil
import pathlib


_cleanup_registered = False


def _register_global_cleanup():
    """Register cleanup handlers once at module level.

    This ensures that all Jupyter servers are properly stopped when:
    - The program exits normally (atexit)
    - SIGINT is received (Ctrl+C)
    - SIGTERM is received (kill command)

    Signal handlers are process-global, so we register them only once
    and have them clean up all servers via the registry.
    """
    global _cleanup_registered
    if _cleanup_registered:
        return

    def cleanup_all():
        """Stop all registered servers."""
        try:
            JupyterServerRegistry.get().stop_all_servers()
        except Exception:
            pass

    def handle_signal(signum, frame):
        """Handle termination signals."""
        cleanup_all()
        sys.exit(0)

    atexit.register(cleanup_all)
    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)
    _cleanup_registered = True


def is_jupyter_installed():
    """Check if Jupyter Notebook is installed.

    Uses shutil.which() to check if 'jupyter' command is available in PATH.
    Works on all platforms (Windows, Linux, macOS).

    :return: True if Jupyter Notebook is installed and available, False otherwise.
    """
    return shutil.which("jupyter") is not None


def is_wx_html2_available():
    """Check whether wx.html2 (WebView) support is available.

    This can be missing on some platforms or distributions (e.g. Gentoo)
    when wxPython or the underlying wxWidgets library is built without
    HTML2/WebView support.

    :return: True if wxPython/wxWidgets html2 module is available, False otherwise.
    """
    try:
        __import__("wx.html2")
        return True
    except (ImportError, ModuleNotFoundError):
        return False


class JupyterServerInstance:
    """Manage the lifecycle of a Jupyter server instance."""

    def __init__(self, workdir):
        """Initialize Jupyter server instance.

        :param workdir: Working directory for the Jupyter server (str).
        """
        self.workdir = workdir
        self.proc = None
        self._reset_state()

        # Register this instance in the global registry
        JupyterServerRegistry.get().register(self)

        # Set up global cleanup handlers (only once)
        _register_global_cleanup()

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

    def is_server_running(self, retries=10, delay=0.2):
        """Check if the server is responding on the given port.

        :param retries: Number of retries before giving up (int).
        :param delay: Delay between retries in seconds (float).
        :return: True if the server is up, False otherwise (bool).
        """
        if not self.port:
            return False

        for _ in range(retries):
            try:
                conn = http.client.HTTPConnection("localhost", self.port, timeout=0.5)
                conn.request("GET", "/")
                response = conn.getresponse()
                conn.close()
                if response.status in {200, 302, 403}:
                    return True
            except Exception:
                time.sleep(delay)
        return False

    def start_server(self):
        """Start Jupyter server in the given directory on a free port.

        :raises RuntimeError: If Jupyter is not installed, directory invalid,
                            or server fails to start.
        """
        # Validation checks
        if not is_jupyter_installed():
            raise RuntimeError(_("Jupyter Notebook is not installed"))

        if not pathlib.Path(self.workdir).is_dir():
            raise RuntimeError(
                _("Working directory does not exist: {}").format(self.workdir)
            )

        if not os.access(self.workdir, os.W_OK):
            raise RuntimeError(
                _("Working directory is not writable: {}").format(self.workdir)
            )

        if self.proc and self.is_alive():
            raise RuntimeError(
                _("Server is already running on port {}").format(self.port)
            )

        # Find free port and build server url
        self.port = JupyterServerInstance.find_free_port()
        self.server_url = "http://localhost:{}".format(self.port)

        # Check if Jupyter is available in PATH
        jupyter = shutil.which("jupyter")
        if not jupyter:
            raise RuntimeError(_("Jupyter executable not found in PATH"))

        # Start Jupyter notebook server
        try:
            self.proc = subprocess.Popen(
                [
                    jupyter,
                    "notebook",
                    "--no-browser",
                    "--NotebookApp.token=''",
                    "--NotebookApp.password=''",
                    "--port",
                    str(self.port),
                    "--notebook-dir",
                    self.workdir,
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                start_new_session=True,  # Detach from terminal
            )
            self.pid = self.proc.pid
        except Exception as e:
            raise RuntimeError(
                _("Failed to start Jupyter server: {}").format(str(e))
            ) from e

        # Check if the server is up
        if not self.is_server_running(retries=10, delay=0.5):
            # Server failed to start
            try:
                self.proc.kill()
                self.proc.wait()
            except Exception:
                pass

            self._reset_state()
            raise RuntimeError(_("Jupyter server failed to start"))

    def stop_server(self):
        """Stop the Jupyter server, ensuring no zombie processes.

        :raises RuntimeError: If the server cannot be stopped.
        """
        if not self.proc or not self.pid:
            return  # Already stopped, nothing to do

        if self.proc.poll() is None:  # Still running
            try:
                self.proc.terminate()  # Send SIGTERM
                self.proc.wait(timeout=5)  # Wait up to 5 seconds, reap zombie
            except subprocess.TimeoutExpired:
                # Force kill if terminate doesn't work
                self.proc.kill()  # Send SIGKILL
                self.proc.wait()  # Still need to reap after kill
            except Exception as e:
                # Even if there's an error, try to reap the zombie
                try:
                    self.proc.wait(timeout=1)
                except Exception:
                    pass
                raise RuntimeError(
                    _("Error stopping Jupyter server (PID {}): {}").format(
                        self.pid, str(e)
                    )
                ) from e
        else:
            # Process already terminated, just reap it
            self.proc.wait()

        # Clean up internal state
        self._reset_state()

        # Unregister from global registry
        try:
            JupyterServerRegistry.get().unregister(self)
        except Exception:
            pass

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
    """Thread-safe registry of running JupyterServerInstance objects."""

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
        """Unregister a server instance.

        :param server: JupyterServerInstance to unregister.
        """
        with self._servers_lock:
            if server in self.servers:
                self.servers.remove(server)

    def stop_all_servers(self):
        """Stop all registered servers.

        Continues attempting to stop all servers even if some fail.
        """
        with self._servers_lock:
            # Copy list to avoid modification during iteration
            servers_to_stop = self.servers[:]

        for server in servers_to_stop:
            try:
                server.stop_server()
            except Exception:
                # Continue stopping other servers even if one fails
                pass
