"""
GRASS Python testing framework utilities (general and test-specific)

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
"""

import errno
import os
from pathlib import Path
import shutil
import sys
from unittest import expectedFailure
import warnings


def ensure_dir(directory):
    """Create all directories in the given path if needed."""
    if not os.path.exists(directory):
        os.makedirs(directory)


def add_gitignore_to_dir(directory):
    gitignore_path = Path(directory) / ".gitignore"
    if not Path(gitignore_path).exists():
        Path(gitignore_path).write_text("*")


def silent_rmtree(filename):
    """Remove the file but do nothing if file does not exist."""
    try:
        shutil.rmtree(filename)
    except OSError as e:
        # errno.ENOENT is "No such file or directory"
        # re-raise if a different error occurred
        if e.errno != errno.ENOENT:
            raise


def do_doctest_gettext_workaround():
    """Setups environment for doing a doctest with gettext usage.

    When using gettext with dynamically defined underscore function
    (``_("For translation")``), doctest does not work properly. One option is
    to use `import as` instead of dynamically defined underscore function but
    this would require change all modules which are used by tested module.
    This should be considered for the future. The second option is to define
    dummy underscore function and one other function which creates the right
    environment to satisfy all. This is done by this function.
    """

    def new_displayhook(string):
        """A replacement for default `sys.displayhook`"""
        if string is not None:
            sys.stdout.write("%r\n" % (string,))

    def new_translator(string):
        """A fake gettext underscore function."""
        return string

    sys.displayhook = new_displayhook

    import builtins

    builtins.__dict__["_"] = new_translator


_MAX_LENGTH = 80


# taken from unittest.util (Python 2.7) since it is not part of API
# but we need it for the same reason as it is used un unittest's TestCase
def safe_repr(obj, short=False):
    try:
        result = repr(obj)
    except Exception:
        result = object.__repr__(obj)  # noqa: PLC2801
    if not short or len(result) < _MAX_LENGTH:
        return result
    return result[:_MAX_LENGTH] + " [truncated]..."


def xfail_windows(test_item):
    """Marks a test as an expected failure or error only on Windows
    Equivalent to applying @unittest.expectedFailure only when running
    on Windows.
    """
    if not sys.platform.startswith("win"):
        return test_item
    warnings.warn(
        "Once the test is fixed and passing, remove the @xfail_windows decorator",
        stacklevel=2,
    )
    return expectedFailure(test_item)
