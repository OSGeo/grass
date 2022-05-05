"""
@package grass.script.messages

@brief Message interface built on calling g.message

Alternative to PyGRASS message interface

(C) 2022 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Petrasova
"""
from .core import (
    message as script_message,
    warning,
    error,
    fatal,
    debug,
    verbose,
    info,
    percent,
    set_raise_on_error,
    get_raise_on_error,
)


class Messenger(object):
    """Alternative implementation to PyGRASS.messages.Messanger
    to avoid invoking C library directly. Instead, it uses
    g.message wrappers from script.core."""

    def __init__(self, raise_on_error=False):
        self.set_raise_on_error(raise_on_error)

    def message(self, message):
        script_message(message)

    def verbose(self, message):
        verbose(message)

    def important(self, message):
        info(message)

    def warning(self, message):
        warning(message)

    def error(self, message):
        error(message)

    def fatal(self, message):
        fatal(message)

    def debug(self, level, message):
        debug(message, level)

    def percent(self, n, d, s):
        percent(n, d, s)

    def stop(self):
        pass

    def set_raise_on_error(self, raise_on_error=True):
        set_raise_on_error(raise_on_error)

    def get_raise_on_error(self):
        return get_raise_on_error()
