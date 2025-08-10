"""GRASS interface to Python exceptions"""

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
    """Raised during script execution.

    .. code-block:: pycon

        >>> error = ScriptError("My error message!")
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

    Used for failures of modules called as subprocesses from Python code.
    """

    def __init__(self, module, code, returncode, errors=None):
        """Create an exception with a full error message based on the parameters.

        :param module: module name
        :param code: some code snipped which contains parameters
        :param returncode: process returncode (assuming non-zero)
        :param errors: errors provided by the module (e.g., stderr)
        """
        # CalledProcessError has undocumented constructor
        super().__init__(returncode, module)
        # No need to include module name if it is directly in code of if it is not set.
        # Otherwise, make sure module name is there if provided and not in code.
        executed = code if not module or module in code else f"{module} {code}"
        if errors:
            # We assume actual errors, e.g., captured stderr.
            err = _("See the following errors:\n{errors}").format(errors=errors)
        else:
            # In command line, the errors will be above, but in testing framework
            # or notebooks, the errors will be somewhere else than the traceback.
            err = _("See errors above the traceback or in the error output.")
        # The full message
        self.msg = _(
            "Module run `{executed}` ended with an error.\n"
            "The subprocess ended with a non-zero return code: {returncode}."
            " {see_errors}"
        ).format(
            executed=executed,
            returncode=returncode,
            see_errors=err,
        )

    def __str__(self):
        return self.msg
