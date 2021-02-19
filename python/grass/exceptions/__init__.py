"""GRASS GIS interface to Python exceptions
"""

import subprocess


class DBError(Exception):
    pass


class FatalError(Exception):
    pass


class FlagError(Exception):
    pass


class GrassError(Exception):
    pass


class ImplementationError(Exception):
    pass


class OpenError(Exception):
    pass


class ParameterError(Exception):
    pass


class ScriptError(Exception):
    """Raised during script execution. ::

        >>> error = ScriptError('My error message!')
        >>> error.value
        'My error message!'
        >>> print(error)
        My error message!
    """

    def __init__(self, value):
        self.value = value

    def __str__(self):
        return self.value


class Usage(Exception):
    pass


# TODO: we inherit from subprocess to be aligned with check_call but it is needed?
# perhaps it would be better to inherit from Exception or from ScriptError
class CalledModuleError(subprocess.CalledProcessError):
    """Raised when a called module ends with error (non-zero return code)

    :param module: module name
    :param code: some code snipped which contains parameters
    :param rc: process returncode
    :param error: errors provided by the module (stderr)
    """

    def __init__(self, module, code, returncode, errors=None):
        # CalledProcessError has undocumented constructor
        super(CalledModuleError, self).__init__(returncode, module)
        msg = _("Module run %s %s ended with error") % (module, code)
        msg += _("\nProcess ended with non-zero return code %s") % returncode
        if errors:
            msg += _(". See the following errors:\n%s") % errors
        else:
            # here could be written "above" but it wouldn't work in some cases
            # e.g., for testing framework
            msg += _(". See errors in the (error) output.")
        self.msg = msg
        # TODO: handle other parameters

    def __str__(self):
        return self.msg
