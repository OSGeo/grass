"""
@package grass.pygrass.messages

@brief PyGRASS message interface

Fast and exit-safe interface to GRASS C-library message functions


(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
import sys
from multiprocessing import Process, Lock, Pipe

import grass.lib.gis as libgis

from grass.exceptions import FatalError


def message_server(lock, conn):
    """The GRASS message server function designed to be a target for
    multiprocessing.Process


    :param lock: A multiprocessing.Lock
    :param conn: A multiprocessing.Pipe

    This function will use the G_* message C-functions from grass.lib.gis
    to provide an interface to the GRASS C-library messaging system.

    The data that is send through the pipe must provide an
    identifier string to specify which C-function should be called.

    The following identifiers are supported:

    - "INFO"       Prints an info message, see G_message() for details
    - "IMPORTANT"  Prints an important info message,
                   see G_important_message() for details
    - "VERBOSE"    Prints a verbose message if the verbosity level is
                   set accordingly, see G_verbose_message() for details
    - "WARNING"    Prints a warning message, see G_warning() for details
    - "ERROR"      Prints a message with a leading "ERROR: " string,
                   see G_important_message() for details
    - "PERCENT"    Prints a percent value based on three integer values: n, d and s
                   see G_percent() for details
    - "STOP"       Stops the server function and closes the pipe
    - "FATAL"      Calls G_fatal_error(), this functions is only for
                   testing purpose

    The that is end through the pipe must be a list of values:

    - Messages: ["INFO|VERBOSE|WARNING|ERROR|FATAL", "MESSAGE"]
    - Debug:    ["DEBUG", level, "MESSAGE"]
    - Percent:  ["PERCENT", n, d, s]

    """
    libgis.G_debug(1, "Start messenger server")

    while True:
        # Avoid busy waiting
        conn.poll(None)
        data = conn.recv()
        message_type = data[0]

        # Only one process is allowed to write to stderr
        lock.acquire()

        # Stop the pipe and the infinite loop
        if message_type == "STOP":
            conn.close()
            lock.release()
            libgis.G_debug(1, "Stop messenger server")
            sys.exit()

        message = data[1]
        # libgis limitation
        if isinstance(message, type(" ")):
            if len(message) >= 2000:
                message = message[:1999]

        if message_type == "PERCENT":
            n = int(data[1])
            d = int(data[2])
            s = int(data[3])
            libgis.G_percent(n, d, s)
        elif message_type == "DEBUG":
            level = data[1]
            message = data[2]
            libgis.G_debug(level, message)
        elif message_type == "VERBOSE":
            libgis.G_verbose_message(message)
        elif message_type == "INFO":
            libgis.G_message(message)
        elif message_type == "IMPORTANT":
            libgis.G_important_message(message)
        elif message_type == "WARNING":
            libgis.G_warning(message)
        elif message_type == "ERROR":
            libgis.G_important_message("ERROR: %s" % message)
        # This is for testing only
        elif message_type == "FATAL":
            libgis.G_fatal_error(message)

        lock.release()


class Messenger(object):
    """Fast and exit-safe interface to GRASS C-library message functions

    This class implements a fast and exit-safe interface to the GRASS
    C-library message functions like: G_message(), G_warning(),
    G_important_message(), G_verbose_message(), G_percent() and G_debug().

    Note:

    The C-library message functions a called via ctypes in a subprocess
    using a pipe (multiprocessing.Pipe) to transfer the text messages.
    Hence, the process that uses the Messenger interface will not be
    exited, if a G_fatal_error() was invoked in the subprocess.
    In this case the Messenger object will simply start a new subprocess
    and restarts the pipeline.


    Usage:

    >>> msgr = Messenger()
    >>> msgr.debug(0, "debug 0")
    >>> msgr.verbose("verbose message")
    >>> msgr.message("message")
    >>> msgr.important("important message")
    >>> msgr.percent(1, 1, 1)
    >>> msgr.warning("Ohh")
    >>> msgr.error("Ohh no")

    >>> msgr = Messenger()
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 239, in fatal
        sys.exit(1)
    SystemExit: 1

    >>> msgr = Messenger(raise_on_error=True)
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 241, in fatal
        raise FatalError(message)
    grass.exceptions.FatalError: Ohh no no no!

    >>> msgr = Messenger(raise_on_error=True)
    >>> msgr.set_raise_on_error(False)
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 239, in fatal
        sys.exit(1)
    SystemExit: 1

    >>> msgr = Messenger(raise_on_error=False)
    >>> msgr.set_raise_on_error(True)
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 241, in fatal
        raise FatalError(message)
    grass.exceptions.FatalError: Ohh no no no!

    """

    def __init__(self, raise_on_error=False):
        self.client_conn = None
        self.server_conn = None
        self.server = None
        self.raise_on_error = raise_on_error
        self.start_server()

    def start_server(self):
        """Start the messenger server and open the pipe"""
        self.client_conn, self.server_conn = Pipe()
        self.lock = Lock()
        self.server = Process(target=message_server, args=(self.lock, self.server_conn))
        self.server.daemon = True
        self.server.start()

    def _check_restart_server(self):
        """Restart the server if it was terminated"""
        if self.server.is_alive() is True:
            return
        self.client_conn.close()
        self.server_conn.close()
        self.start_server()
        self.warning("Needed to restart the messenger server")

    def message(self, message):
        """Send a message to stderr

        :param message: the text of message
        :type message: str

           G_message() will be called in the messenger server process
        """
        self._check_restart_server()
        self.client_conn.send(["INFO", message])

    def verbose(self, message):
        """Send a verbose message to stderr

        :param message: the text of message
        :type message: str

           G_verbose_message() will be called in the messenger server process
        """
        self._check_restart_server()
        self.client_conn.send(["VERBOSE", message])

    def important(self, message):
        """Send an important message to stderr

        :param message: the text of message
        :type message: str

           G_important_message() will be called in the messenger server process
        """
        self._check_restart_server()
        self.client_conn.send(["IMPORTANT", message])

    def warning(self, message):
        """Send a warning message to stderr

        :param message: the text of message
        :type message: str

           G_warning() will be called in the messenger server process
        """
        self._check_restart_server()
        self.client_conn.send(["WARNING", message])

    def error(self, message):
        """Send an error message to stderr

        :param message: the text of message
        :type message: str

           G_important_message() with an additional "ERROR:" string at
           the start will be called in the messenger server process
        """
        self._check_restart_server()
        self.client_conn.send(["ERROR", message])

    def fatal(self, message):
        """Send an error message to stderr, call sys.exit(1) or raise FatalError

        :param message: the text of message
        :type message: str

           This function emulates the behavior of G_fatal_error(). It prints
           an error message to stderr and calls sys.exit(1). If raise_on_error
           is set True while creating the messenger object, a FatalError
           exception will be raised instead of calling sys.exit(1).
        """
        self._check_restart_server()
        self.client_conn.send(["ERROR", message])
        self.stop()

        if self.raise_on_error is True:
            raise FatalError(message)
        else:
            sys.exit(1)

    def debug(self, level, message):
        """Send a debug message to stderr

        :param message: the text of message
        :type message: str

           G_debug() will be called in the messenger server process
        """
        self._check_restart_server()
        self.client_conn.send(["DEBUG", level, message])

    def percent(self, n, d, s):
        """Send a percentage to stderr

        :param message: the text of message
        :type message: str


           G_percent() will be called in the messenger server process
        """
        self._check_restart_server()
        self.client_conn.send(["PERCENT", n, d, s])

    def stop(self):
        """Stop the messenger server and close the pipe"""
        if self.server is not None and self.server.is_alive():
            self.client_conn.send(
                [
                    "STOP",
                ]
            )
            self.server.join(5)
            self.server.terminate()
        if self.client_conn is not None:
            self.client_conn.close()

    def set_raise_on_error(self, raise_on_error=True):
        """Set the fatal error behavior

        :param raise_on_error: if True a FatalError exception will be
                               raised instead of calling sys.exit(1)
        :type raise_on_error: bool

        - If raise_on_error == True, a FatalError exception will be raised
          if fatal() is called
        - If raise_on_error == False, sys.exit(1) will be invoked if
          fatal() is called

        """
        self.raise_on_error = raise_on_error

    def get_raise_on_error(self):
        """Get the fatal error behavior

        :returns: True if a FatalError exception will be raised or False if
                  sys.exit(1) will be called in case of invoking fatal()
        """
        return self.raise_on_error

    def test_fatal_error(self, message):
        """Force the messenger server to call G_fatal_error()"""
        import time

        self._check_restart_server()
        self.client_conn.send(["FATAL", message])
        time.sleep(1)


def get_msgr(
    _instance=[
        None,
    ],
    *args,
    **kwargs,
):
    """Return a Messenger instance.

       :returns: the Messenger instance.

    >>> msgr0 = get_msgr()
    >>> msgr1 = get_msgr()
    >>> msgr2 = Messenger()
    >>> msgr0 is msgr1
    True
    >>> msgr0 is msgr2
    False
    """
    if not _instance[0]:
        _instance[0] = Messenger(*args, **kwargs)
    return _instance[0]


if __name__ == "__main__":
    import doctest

    doctest.testmod()
