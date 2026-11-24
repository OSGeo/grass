"""
Fast and exit-safe interface to PyGRASS Raster and Vector layer
using multiprocessing

SPDX-FileCopyrightText: 2015-2024 GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later

:authors: Soeren Gebbert
"""

from __future__ import annotations

import logging
import multiprocessing.util
import sys
import threading
from typing import TYPE_CHECKING, NoReturn

from grass.exceptions import FatalError
from grass.script.utils import _get_multiprocessing_context

if TYPE_CHECKING:
    from multiprocessing.connection import Connection
    from multiprocessing.synchronize import _LockLike


logger: logging.Logger = logging.getLogger(__name__)

# The function is not documented (although multiprocessing.queues uses it),
# so do not fail if it is missing.
_mp_is_exiting = getattr(multiprocessing.util, "is_exiting", lambda: False)


###############################################################################


def dummy_server(lock: _LockLike, conn: Connection) -> NoReturn:
    """Dummy server process

    :param lock: A multiprocessing.Lock
    :param conn: A multiprocessing.connection.Connection object obtained from
                 multiprocessing.Pipe
    """

    while True:
        # Avoid busy waiting
        conn.poll(None)
        data = conn.recv()
        with lock:
            if data[0] == 0:
                conn.close()
                sys.exit()
            if data[0] == 1:
                msg = "Server process intentionally killed by exception"
                raise Exception(msg)


class RPCServerBase:
    """This is the base class for send and receive RPC server
    It uses a Pipe for IPC.

    .. code-block:: pycon

        >>> import grass.script as gscript
        >>> from grass.pygrass.rpc.base import RPCServerBase
        >>> import time
        >>> provider = RPCServerBase()

        >>> provider.is_server_alive()
        True

        >>> provider.is_check_thread_alive()
        True

        >>> provider.stop()
        >>> time.sleep(1)
        >>> provider.is_server_alive()
        False

        >>> provider.is_check_thread_alive()
        False

        >>> provider = RPCServerBase()
        >>> provider.is_server_alive()
        True
        >>> provider.is_check_thread_alive()
        True

        Kill the server process with an exception, it should restart

        >>> provider.client_conn.send([1])
        >>> provider.is_server_alive()
        True

        >>> provider.is_check_thread_alive()
        True

    """

    #: Seconds between checks of the server process by the checker thread. It
    #: bounds how long a crashed server goes unnoticed. It does not delay stop().
    check_interval: float = 0.2

    def __init__(self) -> None:
        self.client_conn: Connection | None = None
        self.server_conn: Connection | None = None
        self.queue = None
        self.server = None
        self.checkThread: threading.Thread | None = None
        # Set to ask the current checker thread to return.
        self._checker_stop = threading.Event()
        self.threadLock = threading.Lock()
        self.start_server()
        self.start_checker_thread()
        self.stopped = True
        # logging.basicConfig(level=logging.DEBUG)

    def is_server_alive(self):
        return self.server.is_alive() if self.server is not None else False

    def is_check_thread_alive(self):
        return self.checkThread.is_alive() if self.checkThread is not None else False

    def start_checker_thread(self):
        if self.checkThread is not None and self.checkThread.is_alive():
            self.stop_checker_thread()

        # Each thread gets its own event. A replaced thread keeps the event
        # that was set for it and does not see the new one.
        self._checker_stop = threading.Event()
        self.checkThread = threading.Thread(
            target=self.thread_checker, args=(self._checker_stop,)
        )
        self.checkThread.daemon = True
        self.checkThread.start()

    def stop_checker_thread(self):
        self._checker_stop.set()
        if self.checkThread is not None:
            self.checkThread.join(None)

    def thread_checker(self, stop: threading.Event):
        """Restart the server whenever it is found dead, until stop is set

        The server is checked every check_interval seconds. Waiting on the
        event, and not sleeping, lets stop_checker_thread() end the thread at
        once instead of after the remainder of the interval.

        :param stop: Event which is set to ask this thread to return
        """
        # wait() returns True as soon as the event is set, so no restart is
        # attempted once stop was requested.
        while not stop.wait(self.check_interval):
            try:
                self._check_restart_server(caller="Server check thread")
            except FatalError:
                # Raised only when the server was terminated at exit.
                return

    def start_server(self):
        """This function must be re-implemented in the subclasses"""
        logger.debug("Start the libgis server")

        ctx = _get_multiprocessing_context()
        self.client_conn, self.server_conn = ctx.Pipe(True)
        self.lock = ctx.Lock()
        self.server = ctx.Process(
            target=dummy_server, args=(self.lock, self.server_conn)
        )
        self.server.daemon = True
        self.server.start()

    def check_server(self):
        self._check_restart_server()

    def _check_restart_server(self, caller="main thread") -> None:
        """Restart the server if it was terminated"""
        logger.debug("Check libgis server restart")

        with self.threadLock:
            if self.server is not None and self.server.is_alive() is True:
                return
            # Do not restart a server that multiprocessing terminated at exit.
            if _mp_is_exiting():
                msg = f"The libgis server was terminated at exit, caller: {caller}"
                raise FatalError(msg)
            if self.client_conn is not None:
                self.client_conn.close()
            if self.server_conn is not None:
                self.server_conn.close()
            self.start_server()

            if self.stopped is not True:
                logger.warning(
                    "Needed to restart the libgis server, caller: %(caller)s",
                    {"caller": caller},
                )

        self.stopped = False

    def safe_receive(self, message):
        """Receive the data and throw a FatalError exception in case the server
        process was killed and the pipe was closed by the checker thread"""
        if logger.isEnabledFor(logging.DEBUG):
            logger.debug("Receive message: %s", message)

        try:
            ret = self.client_conn.recv()
            if isinstance(ret, FatalError):
                raise ret
            return ret
        except (EOFError, OSError, FatalError) as e:
            # The pipe was closed by the checker thread because
            # the server process was killed
            raise FatalError(
                "Exception raised: " + str(e) + " Message: " + message
            ) from e

    def stop(self):
        """Stop the check thread, the libgis server and close the pipe

        This method should be called at exit using the package atexit
        """
        logger.debug("Stop libgis server")

        self.stop_checker_thread()
        if self.server is not None and self.server.is_alive():
            if self.client_conn is not None:
                self.client_conn.send(
                    [
                        0,
                    ]
                )
            # Let the server exit on its own before falling back to terminate.
            self.server.join(timeout=5)
            self.server.terminate()
            # A process still starting up reopens the lock semaphore by name,
            # so it must be gone before the reference is dropped below.
            self.server.join(timeout=5)
        if self.client_conn is not None:
            self.client_conn.close()
        # Dropping the reference unlinks the semaphore now, not at exit.
        self.lock = None
        self.stopped = True


if __name__ == "__main__":
    import doctest

    doctest.testmod()
