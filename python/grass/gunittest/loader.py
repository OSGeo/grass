"""
GRASS Python testing framework test loading functionality

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras, Edouard Choinière
"""

from __future__ import annotations

import collections
import fnmatch
import os
import re
import unittest
from pathlib import PurePath
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterable


def fnmatch_exclude_with_base(
    files: Iterable[str],
    base: str | os.PathLike,
    exclude: Iterable[str | os.PathLike | PurePath],
) -> list[str]:
    """Return list of files not matching any exclusion pattern

    :param files: list of file names
    :param base: directory (path) where the files are
    :param exclude: list of fnmatch glob patterns for exclusion
    """
    not_excluded = []
    patterns = {str(PurePath(item)) for item in exclude}
    base_path = PurePath(base)
    for filename in files:
        test_filename: PurePath = base_path / filename
        matches = False
        for pattern in patterns:
            if fnmatch.fnmatch(str(test_filename), pattern):
                matches = True
                break
        if not matches:
            not_excluded.append(filename)
    return not_excluded


# TODO: resolve test file versus test module
GrassTestPythonModule = collections.namedtuple(
    "GrassTestPythonModule",
    [
        "name",
        "module",
        "file_type",
        "tested_dir",
        "file_dir",
        "file_path",
        "abs_file_path",
    ],
)


# TODO: implement loading without the import
def discover_modules(
    start_dir,
    skip_dirs,
    testsuite_dir,
    grass_location,
    all_locations_value,
    universal_location_value,
    import_modules,
    add_failed_imports=True,
    file_pattern=None,
    file_regexp=None,
    exclude=None,
):
    """Find all test files (modules) in a directory tree.

    The function is designed specifically for GRASS testing framework
    test layout. It expects some directories to have a "testsuite"
    directory where test files (test modules) are present.
    Additionally, it also handles loading of test files which specify
    in which location they can run.

    :param start_dir: directory to start the search
    :param file_pattern: pattern of files in a test suite directory
        (using Unix shell-style wildcards)
    :param skip_dirs: directories not to recurse to (e.g. ``.svn``)
    :param testsuite_dir: name of directory where the test files are found,
        the function will not recurse to this directory
    :param grass_location: string with an accepted location type (category, shortcut)
    :param all_locations_value: string used to say that all locations
        should be loaded (grass_location can be set to this value)
    :param universal_location_value: string marking a test as
        location-independent (same as not providing any)
    :param import_modules: True if files should be imported as modules,
        False if the files should be just searched for the needed values

    :returns: a list of GrassTestPythonModule objects

    .. todo::
        Implement import_modules.
    """
    modules = []
    for root, dirs, unused_files in os.walk(start_dir, topdown=True):
        dirs.sort()
        for dir_pattern in skip_dirs:
            to_skip = fnmatch.filter(dirs, dir_pattern)
            for skip in to_skip:
                dirs.remove(skip)

        if testsuite_dir not in dirs:
            continue
        dirs.remove(testsuite_dir)  # do not recurse to testsuite
        full = os.path.join(root, testsuite_dir)

        files = os.listdir(full)
        if file_pattern:
            files = fnmatch.filter(files, file_pattern)
        if file_regexp:
            files = [f for f in files if re.match(file_regexp, f)]
        if exclude:
            files = fnmatch_exclude_with_base(files, full, exclude)
        files = sorted(files)
        # get test/module name without .py
        # expecting all files to end with .py
        # this will not work for invoking bat files but it works fine
        # as long as we handle only Python files (and using Python
        # interpreter for invoking)

        # TODO: warning about no tests in a testsuite
        # (in what way?)
        for file_name in files:
            # TODO: add also import if requested
            # (see older versions of this file)
            # TODO: check if there is some main in .py
            # otherwise we can have successful test just because
            # everything was loaded into Python
            # TODO: check if there is set -e or exit !0 or ?
            # otherwise we can have successful because nothing was reported
            abspath = os.path.abspath(full)
            abs_file_path = os.path.join(abspath, file_name)
            if file_name.endswith(".py"):
                if file_name == "__init__.py":
                    # we always ignore __init__.py
                    continue
                file_type = "py"
                name = file_name[:-3]
            elif file_name.endswith(".sh"):
                file_type = "sh"
                name = file_name[:-3]
            else:
                file_type = None  # alternative would be '', now equivalent
                name = file_name

            add = False
            try:
                if grass_location == all_locations_value:
                    add = True
                else:
                    try:
                        locations = ["nc", "stdmaps", "all"]
                    except AttributeError:
                        add = True  # test is universal
                    else:
                        if universal_location_value in locations:
                            add = True  # cases when it is explicit
                        if grass_location in locations:
                            add = True  # standard case with given location
                        if not locations:
                            add = True  # count not specified as universal
            except ImportError as e:
                if add_failed_imports:
                    add = True
                else:
                    raise ImportError(
                        "Cannot import module named %s in %s (%s)"
                        % (name, full, e.message)
                    )
                    # alternative is to create TestClass which will raise
                    # see unittest.loader
            if add:
                modules.append(
                    GrassTestPythonModule(
                        name=name,
                        module=None,
                        tested_dir=root,
                        file_dir=full,
                        abs_file_path=abs_file_path,
                        file_path=os.path.join(full, file_name),
                        file_type=file_type,
                    )
                )
                # in else with some verbose we could tell about skipped test
    return modules


# TODO: find out if this is useful for us in some way
# we are now using only discover_modules directly
class GrassTestLoader(unittest.TestLoader):
    """Class handles GRASS-specific loading of test modules."""

    skip_dirs = [".git", ".svn", "dist.*", "bin.*", "OBJ.*"]
    testsuite_dir = "testsuite"
    files_in_testsuite = "*.py"
    all_tests_value = "all"
    universal_tests_value = "universal"

    def __init__(self, grass_location):
        super().__init__()
        self.grass_location = grass_location

    # TODO: what is the purpose of top_level_dir, can it be useful?
    # probably yes, we need to know grass src or dist root
    # TODO: not using pattern here
    def discover(self, start_dir, pattern="test*.py", top_level_dir=None):
        """Load test modules from in GRASS testing framework way."""
        modules = discover_modules(
            start_dir=start_dir,
            file_pattern=self.files_in_testsuite,
            skip_dirs=self.skip_dirs,
            testsuite_dir=self.testsuite_dir,
            grass_location=self.grass_location,
            all_locations_value=self.all_tests_value,
            universal_location_value=self.universal_tests_value,
            import_modules=True,
        )
        tests = [self.loadTestsFromModule(module.module) for module in modules]
        return self.suiteClass(tests)


if __name__ == "__main__":
    for expression in [r".*\.py$", r".*\.sh$"]:
        modules = discover_modules(
            start_dir=".",
            grass_location="all",
            file_regexp=expression,
            skip_dirs=GrassTestLoader.skip_dirs,
            testsuite_dir=GrassTestLoader.testsuite_dir,
            all_locations_value=GrassTestLoader.all_tests_value,
            universal_location_value=GrassTestLoader.universal_tests_value,
            import_modules=False,
            exclude=None,
        )
        print("Expression:", expression)
        print(len(modules))
        print([module.file_path for module in modules])
