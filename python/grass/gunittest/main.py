"""
GRASS Python testing framework module for running from command line

Copyright (C) 2014-2021 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
"""

import os
import sys
import argparse
import configparser
from pathlib import Path

from unittest.main import TestProgram

import grass.script.core as gs

from .loader import GrassTestLoader
from .runner import GrassTestRunner, MultiTestResult, TextTestResult, KeyValueTestResult
from .invoker import GrassTestFilesInvoker
from .utils import silent_rmtree
from .reporters import FileAnonymizer


class GrassTestProgram(TestProgram):
    """A class to be used by individual test files (wrapped in the function)"""

    def __init__(
        self,
        exit_at_end,
        grass_location,
        clean_outputs=True,
        unittest_argv=None,
        module=None,
        verbosity=1,
        failfast=None,
        catchbreak=None,
    ):
        """Prepares the tests in GRASS way and then runs the tests.

        :param bool clean_outputs: if outputs in mapset and in ?
        """
        self.test = None
        self.grass_location = grass_location
        # it is unclear what the exact behavior is in unittest
        # buffer stdout and stderr during tests
        buffer_stdout_stderr = False

        grass_loader = GrassTestLoader(grass_location=self.grass_location)

        text_result = TextTestResult(
            stream=sys.stderr, descriptions=True, verbosity=verbosity
        )
        with open("test_keyvalue_result.txt", "w") as keyval_file:
            keyval_result = KeyValueTestResult(stream=keyval_file)
            result = MultiTestResult(results=[text_result, keyval_result])

            grass_runner = GrassTestRunner(
                verbosity=verbosity,
                failfast=failfast,
                buffer=buffer_stdout_stderr,
                result=result,
            )
            super().__init__(
                module=module,
                argv=unittest_argv,
                testLoader=grass_loader,
                testRunner=grass_runner,
                exit=exit_at_end,
                verbosity=verbosity,
                failfast=failfast,
                catchbreak=catchbreak,
                buffer=buffer_stdout_stderr,
            )


def test():
    """Run a test of a module."""
    # TODO: put the link to to the report only if available
    # TODO: how to disable Python code coverage for module and C tests?
    # TODO: we probably need to have different test  functions for C, Python modules,
    # and Python code
    # TODO: combine the results using python -m coverage --help | grep combine
    # TODO: function to anonymize/beautify file names (in content and actual filenames)
    # TODO: implement coverage but only when requested by invoker and only if
    # it makes sense for tests (need to know what is tested)
    # doing_coverage = False
    # try:
    #    import coverage
    #    doing_coverage = True
    #    cov = coverage.coverage(omit="*testsuite*")
    #    cov.start()
    # except ImportError:
    #    pass
    # TODO: add some message somewhere

    # TODO: enable passing omit to exclude also gunittest or nothing
    program = GrassTestProgram(
        module="__main__", exit_at_end=False, grass_location="all"
    )
    # TODO: check if we are in the directory where the test file is
    # this will ensure that data directory is available when it is requested

    # if doing_coverage:
    #    cov.stop()
    #    cov.html_report(directory='testcodecoverage')

    # TODO: is sys.exit the right thing here
    sys.exit(not program.result.wasSuccessful())


test.__test__ = False  # prevent running this function as a test in pytest


def discovery():
    """Recursively find all tests in testsuite directories and run them

    Everything is imported and runs in this process.

    Runs using::
        python main.py discovery [start_directory]
    """

    program = GrassTestProgram(grass_location="nc", exit_at_end=False)

    sys.exit(not program.result.wasSuccessful())


CONFIG_FILENAME = ".gunittest.cfg"


def get_config(start_directory, config_file):
    """Read configuration if available, return empty section proxy if not

    If file is explicitly specified, it must exist.

    Raises OSError if file is not accessible, e.g., if it exists,
    but there is an issue with permissions.
    """
    config_parser = configparser.ConfigParser()
    if config_file:
        with open(config_file, encoding="utf-8") as file:
            config_parser.read_file(file)
    elif start_directory:
        config_file = Path(start_directory) / CONFIG_FILENAME
        # Does not check presence of the file
        config_parser.read(config_file)
    else:
        msg = "Either start_directory or config_file must be set"
        raise ValueError(msg)
    if "gunittest" not in config_parser:
        # Create an empty section if file is not available or section is not present.
        config_parser.read_dict({"gunittest": {}})
    return config_parser["gunittest"]


def main():
    parser = argparse.ArgumentParser(
        description="Run test files in all testsuite directories starting"
        " from the current one"
        " (runs on active GRASS session)"
    )
    parser.add_argument(
        "--location",
        dest="location",
        action="store",
        help="Name of location where to perform test",
        required=True,
    )
    parser.add_argument(
        "--location-type",
        dest="location_type",
        action="store",
        default="nc",
        help="Type of tests which should be run (tag corresponding to location)",
    )
    parser.add_argument(
        "--grassdata",
        dest="gisdbase",
        action="store",
        default=None,
        help="GRASS data(base) (GISDBASE) directory (current GISDBASE by default)",
    )
    parser.add_argument(
        "--output",
        dest="output",
        action="store",
        default="testreport",
        help="Output directory",
    )
    parser.add_argument(
        "--min-success",
        dest="min_success",
        action="store",
        default="100",
        type=float,
        help=(
            "Minimum success percentage (lower percentage"
            " than this will result in a non-zero return code; values 0-100)"
        ),
    )
    parser.add_argument(
        "--config",
        dest="config",
        action="store",
        type=str,
        help=f"Path to a configuration file (default: {CONFIG_FILENAME})",
    )
    args = parser.parse_args()
    gisdbase = args.gisdbase
    if gisdbase is None:
        # here we already rely on being in GRASS session
        gisdbase = gs.gisenv()["GISDBASE"]
    location = args.location
    location_type = args.location_type

    if not gisdbase:
        return "GISDBASE (grassdata directory) cannot be empty string\n"
    if not os.path.exists(gisdbase):
        return f"GISDBASE (grassdata directory) <{gisdbase}> does not exist\n"
    if not os.path.exists(os.path.join(gisdbase, location)):
        return (
            f"GRASS Location <{location}>"
            f" does not exist in GRASS Database <{gisdbase}>\n"
        )
    results_dir = args.output
    silent_rmtree(results_dir)  # TODO: too brute force?

    start_dir = "."
    abs_start_dir = os.path.abspath(start_dir)

    try:
        config = get_config(start_directory=start_dir, config_file=args.config)
    except OSError as error:
        return f"Error reading configuration: {error}"

    invoker = GrassTestFilesInvoker(
        start_dir=start_dir,
        file_anonymizer=FileAnonymizer(paths_to_remove=[abs_start_dir]),
        timeout=config.getfloat("timeout", None),
    )
    # TODO: remove also results dir from files
    # as an enhancement
    # we can just iterate over all locations available in database
    # but the we don't know the right location type (category, label, shortcut)
    reporter = invoker.run_in_location(
        gisdbase=gisdbase,
        location=location,
        location_type=location_type,
        results_dir=results_dir,
        exclude=config.get("exclude", "").split(),
    )
    if not reporter.test_files:
        return "No tests found or executed"
    if reporter.file_pass_per >= args.min_success:
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
