"""
GRASS Python testing framework utilities (general and test-specific)

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Vaclav Petras
"""

from __future__ import annotations

import errno
import shutil
import sys
import warnings
from pathlib import Path
from typing import TYPE_CHECKING
from unittest import expectedFailure

from grass.app.runtime import RuntimePaths

if TYPE_CHECKING:
    from _typeshed import StrOrBytesPath, StrPath


def ensure_dir(directory: StrOrBytesPath) -> None:
    """Create all directories in the given path if needed."""
    Path(directory).mkdir(parents=True, exist_ok=True)


def add_gitignore_to_dir(directory: StrPath) -> None:
    gitignore_path = Path(directory) / ".gitignore"
    if not Path(gitignore_path).exists():
        Path(gitignore_path).write_text("*", encoding="utf-8")


def silent_rmtree(filename: StrOrBytesPath) -> None:
    """Remove the file but do nothing if file does not exist."""
    try:
        shutil.rmtree(filename)
    except OSError as e:
        # errno.ENOENT is "No such file or directory"
        # re-raise if a different error occurred
        if e.errno != errno.ENOENT:
            raise


def do_doctest_gettext_workaround() -> None:
    """Setups environment for doing a doctest with gettext usage.

    When using gettext with dynamically defined underscore function
    (``_("For translation")``), doctest does not work properly. One option is
    to use `import as` instead of dynamically defined underscore function but
    this would require change all modules which are used by tested module.
    This should be considered for the future. The second option is to define
    dummy underscore function and one other function which creates the right
    environment to satisfy all. This is done by this function.
    """

    def new_displayhook(string: str | None) -> None:
        """A replacement for default `sys.displayhook`"""
        if string is not None:
            sys.stdout.write("%r\n" % (string,))

    def new_translator(string: str) -> str:
        """A fake gettext underscore function."""
        return string

    sys.displayhook = new_displayhook

    import builtins

    builtins.__dict__["_"] = new_translator


_MAX_LENGTH = 80


def xfail_cmake(test_item):
    """Marks a test as an expected failure or error only on CMake build
    Equivalent to applying @unittest.expectedFailure only when running
    on a CMake built GRASS.
    """
    runtime_paths = RuntimePaths()

    if not runtime_paths.is_cmake_build:
        return test_item
    warnings.warn(
        "Once the test is fixed and passing, remove the @xfail_cmake decorator",
        stacklevel=2,
    )
    return expectedFailure(test_item)


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
