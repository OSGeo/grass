"""
GRASS Python testing framework module for running from command line

SPDX-FileCopyrightText: 2014-2021 GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later

:authors: Vaclav Petras
"""

import os
import random
import sys
import argparse
import configparser
from pathlib import Path

from unittest.main import TestProgram

import grass.script.core as gs

from .checkers import text_to_keyvalue
from .loader import GrassTestLoader
from .runner import (
    GrassTestRunner,
    KeyValueTestResult,
    MultiTestResult,
    TextTestResult,
    _WritelnDecorator,
)
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
        *,
        tb_locals=False,
        **kwargs,
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
            stream=_WritelnDecorator(sys.stderr), descriptions=True, verbosity=verbosity
        )
        with open("test_keyvalue_result.txt", "w", encoding="utf-8") as keyval_file:
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
                testRunner=grass_runner,
                testLoader=grass_loader,
                exit=exit_at_end,
                verbosity=verbosity,
                failfast=failfast,
                catchbreak=catchbreak,
                buffer=buffer_stdout_stderr,
                tb_locals=tb_locals,
                **kwargs,
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

    :raises OSError: if file is not accessible, e.g., if it exists,
        but there is an issue with permissions.
    :raises ValueError: If neither start_directory nor config_file are set.
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


def seed_type(value: str) -> int | str:
    """Convert a seed option value to int unless it is the special value last"""
    if value == "last":
        return value
    try:
        return int(value)
    except ValueError:
        msg = f"invalid seed value: {value!r} (use an integer or 'last')"
        raise argparse.ArgumentTypeError(msg) from None


def get_last_seed(results_dir) -> int | None:
    """Return the seed of the previous run stored in its report, if any"""
    summary_file = Path(results_dir) / "test_keyvalue_result.txt"
    try:
        text = summary_file.read_text(encoding="utf-8")
    except OSError:
        return None
    seed = text_to_keyvalue(text, sep="=").get("random_seed")
    return seed if isinstance(seed, int) else None


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
    parser.add_argument(
        "--random-order",
        dest="random_order",
        action="store_true",
        help=(
            "Run test files in random order using a newly generated seed"
            " (reports still list test files in the usual order)"
        ),
    )
    parser.add_argument(
        "--randomly-seed",
        dest="randomly_seed",
        action="store",
        type=seed_type,
        default=None,
        help=(
            "Run test files in random order generated from this seed,"
            " or pass the special value 'last' to reuse the seed from the"
            " previous run with the same output directory"
            " (implies --random-order)"
        ),
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
    if not Path(gisdbase).exists():
        return f"GISDBASE (grassdata directory) <{gisdbase}> does not exist\n"
    if not Path(gisdbase, location).exists():
        return (
            f"GRASS Location <{location}>"
            f" does not exist in GRASS Database <{gisdbase}>\n"
        )
    results_dir = args.output
    random_seed = args.randomly_seed
    if random_seed == "last":
        # The previous report is removed below, so get the seed first.
        random_seed = get_last_seed(results_dir)
        if random_seed is None:
            return (
                f"No seed of a previous run in random order found in <{results_dir}>\n"
            )
    elif args.random_order and random_seed is None:
        random_seed = random.randrange(2**32)
    silent_rmtree(results_dir)  # TODO: too brute force?

    start_dir = "."
    abs_start_dir = os.path.abspath(start_dir)

    try:
        config = get_config(start_directory=start_dir, config_file=args.config)
    except OSError as error:
        return f"Error reading configuration: {error}"

    if random_seed is not None:
        print(f"Using --randomly-seed={random_seed}", file=sys.stderr)

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
        random_seed=random_seed,
    )
    if not reporter.test_files:
        return "No tests found or executed"
    if reporter.file_pass_per >= args.min_success:
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
