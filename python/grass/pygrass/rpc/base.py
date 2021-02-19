# -*- coding: utf-8 -*-
"""
Fast and exit-safe interface to PyGRASS Raster and Vector layer
using multiprocessing

(C) 2015 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from grass.exceptions import FatalError
import time
import threading
import sys
from multiprocessing import Process, Lock, Pipe
import logging

###############################################################################

def dummy_server(lock, conn):
    """Dummy server process

       :param lock: A multiprocessing.Lock
       :param conn: A multiprocessing.Pipe
    """

    while True:
        # Avoid busy waiting
        conn.poll(None)
        data = conn.recv()
        lock.acquire()
        if data[0] == 0:
            conn.close()
            lock.release()
            sys.exit()
        if data[0] == 1:
            raise Exception("Server process intentionally killed by exception")
        lock.release()

class RPCServerBase(object):
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

    def __init__(self):
        self.client_conn = None
        self.server_conn = None
        self.queue = None
        self.server = None
        self.checkThread = None
        self.threadLock = threading.Lock()
        self.start_server()
        self.start_checker_thread()
        self.stopThread = False
        self.stopped = True
        # logging.basicConfig(level=logging.DEBUG)

    def is_server_alive(self):
        return self.server.is_alive()

    def is_check_thread_alive(self):
        return self.checkThread.is_alive()

    def start_checker_thread(self):
        if self.checkThread is not None and self.checkThread.is_alive():
            self.stop_checker_thread()

        self.checkThread = threading.Thread(target=self.thread_checker)
        self.checkThread.daemon = True
        self.stopThread = False
        self.checkThread.start()

    def stop_checker_thread(self):
        self.threadLock.acquire()
        self.stopThread = True
        self.threadLock.release()
        self.checkThread.join(None)

    def thread_checker(self):
        """Check every 200 micro seconds if the server process is alive"""
        while True:
            time.sleep(0.2)
            self._check_restart_server(caller="Server check thread")
            self.threadLock.acquire()
            if self.stopThread is True:
                self.threadLock.release()
                return
            self.threadLock.release()

    def start_server(self):
        """This function must be re-implemented in the subclasses
        """
        logging.debug("Start the libgis server")

        self.client_conn, self.server_conn = Pipe(True)
        self.lock = Lock()
        self.server = Process(target=dummy_server, args=(self.lock,
                                                         self.server_conn))
        self.server.daemon = True
        self.server.start()

    def check_server(self):
        self._check_restart_server()

    def _check_restart_server(self, caller="main thread"):
        """Restart the server if it was terminated
        """
        logging.debug("Check libgis server restart")

        self.threadLock.acquire()
        if self.server.is_alive() is True:
            self.threadLock.release()
            return
        self.client_conn.close()
        self.server_conn.close()
        self.start_server()

        if self.stopped is not True:
            logging.warning("Needed to restart the libgis server, caller: %s" % (caller))

        self.threadLock.release()
        self.stopped = False

    def safe_receive(self, message):
        """Receive the data and throw a FatalError exception in case the server
           process was killed and the pipe was closed by the checker thread"""
        logging.debug("Receive message: {message}")

        try:
            ret = self.client_conn.recv()
            if isinstance(ret, FatalError):
                raise ret
            return ret
        except (EOFError, IOError, FatalError) as e:
            # The pipe was closed by the checker thread because
            # the server process was killed
            raise FatalError("Exception raised: " + str(e) + " Message: " + message)

    def stop(self):
        """Stop the check thread, the libgis server and close the pipe

           This method should be called at exit using the package atexit
        """
        logging.debug("Stop libgis server")

        self.stop_checker_thread()
        if self.server is not None and self.server.is_alive():
            self.client_conn.send([0, ])
            self.server.terminate()
        if self.client_conn is not None:
            self.client_conn.close()
        self.stopped = True


if __name__ == "__main__":
    import doctest
    doctest.testmod()
