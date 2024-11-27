"""
Fast and exit-safe interface to PyGRASS Raster and Vector layer
using multiprocessing

(C) 2015-2024 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from __future__ import annotations

import logging
import sys
import threading
import time
from multiprocessing import Lock, Pipe, Process
from typing import TYPE_CHECKING, NoReturn

from grass.exceptions import FatalError

if TYPE_CHECKING:
    from multiprocessing.connection import Connection
    from multiprocessing.synchronize import _LockLike


logger: logging.Logger = logging.getLogger(__name__)


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
                raise Exception("Server process intentionally killed by exception")


class RPCServerBase:
    """This is the base class for send and receive RPC server
    It uses a Pipe for IPC.


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

    def __init__(self) -> None:
        self.client_conn: Connection | None = None
        self.server_conn: Connection | None = None
        self.queue = None
        self.server = None
        self.checkThread: threading.Thread | None = None
        self.threadLock = threading.Lock()
        self.start_server()
        self.start_checker_thread()
        self.stopThread = False
        self.stopped = True
        # logging.basicConfig(level=logging.DEBUG)

    def is_server_alive(self):
        return self.server.is_alive() if self.server is not None else False

    def is_check_thread_alive(self):
        return self.checkThread.is_alive() if self.checkThread is not None else False

    def start_checker_thread(self):
        if self.checkThread is not None and self.checkThread.is_alive():
            self.stop_checker_thread()

        self.checkThread = threading.Thread(target=self.thread_checker)
        self.checkThread.daemon = True
        self.stopThread = False
        self.checkThread.start()

    def stop_checker_thread(self):
        with self.threadLock:
            self.stopThread = True
        if self.checkThread is not None:
            self.checkThread.join(None)

    def thread_checker(self):
        """Check every 200 micro seconds if the server process is alive"""
        while True:
            time.sleep(0.2)
            self._check_restart_server(caller="Server check thread")
            with self.threadLock:
                if self.stopThread is True:
                    return

    def start_server(self):
        """This function must be re-implemented in the subclasses"""
        logger.debug("Start the libgis server")

        self.client_conn, self.server_conn = Pipe(True)
        self.lock = Lock()
        self.server = Process(target=dummy_server, args=(self.lock, self.server_conn))
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
            raise FatalError("Exception raised: " + str(e) + " Message: " + message)

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
            self.server.terminate()
        if self.client_conn is not None:
            self.client_conn.close()
        self.stopped = True


if __name__ == "__main__":
    import doctest

    doctest.testmod()
