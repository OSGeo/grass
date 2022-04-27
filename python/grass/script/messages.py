from .core import (
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
    def __init__(self, raise_on_error=False):
        self.set_raise_on_error(raise_on_error)

    def message(self, message):
        info(message)

    def verbose(self, message):
        verbose(message)

    def important(self, message):
        self.message(message)

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
