# -*- coding: utf-8 -*-
"""!@package grass.gunittest.main

@brief GRASS Python testing framework module for running from command line

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

@author Vaclav Petras
"""

import os
import sys

from unittest.main import TestProgram, USAGE_AS_MAIN
TestProgram.USAGE = USAGE_AS_MAIN

from .loader import GrassTestLoader
from .runner import GrassTestRunner
from .invoker import GrassTestFilesInvoker
from .utils import silent_rmtree

import grass.script.core as gcore


class GrassTestProgram(TestProgram):
    """A class to be used by individual test files (wrapped in the function)"""

    def __init__(self, exit_at_end, grass_location, clean_outputs=True,
                 unittest_argv=None, module=None,
                 verbosity=1,
                 failfast=None, catchbreak=None):
        """Prepares the tests in GRASS way and then runs the tests.

        :param bool clean_outputs: if outputs in mapset and in ?
        """
        self.test = None
        self.grass_location = grass_location
        # it is unclear what the exact behavior is in unittest
        # buffer stdout and stderr during tests
        buffer_stdout_stderr = False

        grass_loader = GrassTestLoader(grass_location=self.grass_location)
        grass_runner = GrassTestRunner(verbosity=verbosity,
                                       failfast=failfast,
                                       buffer=buffer_stdout_stderr)

        super(GrassTestProgram, self).__init__(module=module,
                                               argv=unittest_argv,
                                               testLoader=grass_loader,
                                               testRunner=grass_runner,
                                               exit=exit_at_end,
                                               verbosity=verbosity,
                                               failfast=failfast,
                                               catchbreak=catchbreak,
                                               buffer=buffer_stdout_stderr)


def test():
    """Run a test of a module.
    """
    # TODO: put the link to to the report only if available
    # TODO: how to disable Python code coverage for module and C tests?
    doing_coverage = False
    try:
        import coverage
        doing_coverage = True
        cov = coverage.coverage(omit="*testsuite*")
        cov.start()
    except ImportError:
        pass
        # TODO: add some message somewhere

    # TODO: enable passing omit to exclude also gunittest or nothing
    program = GrassTestProgram(module='__main__', exit_at_end=False, grass_location='all')
    # TODO: check if we are in the directory where the test file is
    # this will ensure that data directory is available when it is requested

    if doing_coverage:
        cov.stop()
        cov.html_report(directory='testcodecoverage')

    # TODO: is sys.exit the right thing here
    sys.exit(not program.result.wasSuccessful())


# TODO: test or main? test looks more general
# unittest has main() but doctest has testmod()
main = test


def discovery():
    """Recursively find all tests in testsuite directories and run them

    Everything is imported and runs in this process.

    Runs using::
        python main.py discovery [start_directory]
    """
    doing_coverage = False
    try:
        import coverage
        doing_coverage = True
        cov = coverage.coverage(omit="*testsuite*")
        cov.start()
    except ImportError:
        pass
        # TODO: add some message somewhere

    program = GrassTestProgram(grass_location='nc', exit_at_end=False)

    if doing_coverage:
        cov.stop()
        cov.html_report(directory='testcodecoverage')

    sys.exit(not program.result.wasSuccessful())


# TODO: makefile rule should depend on the whole build
# TODO: create a full interface (using grass parser or argparse)
if __name__ == '__main__':
    if len(sys.argv) == 4:
        gisdbase = sys.argv[1]
        location = sys.argv[2]
        location_shortcut = sys.argv[3]
    elif len(sys.argv) == 3:
        location = sys.argv[1]
        location_shortcut = sys.argv[2]
        gisdbase = gcore.gisenv()['GISDBASE']
    else:
        sys.stderr.write("Usage: %s [gisdbase] location location_shortcut\n" % sys.argv[0])
        sys.exit(1)
    assert gisdbase
    if not os.path.exists(gisdbase):
        sys.stderr.write("GISDBASE <%s> does not exist\n" % gisdbase)
        sys.exit(1)
    results_dir = 'testreport'
    silent_rmtree(results_dir)  # TODO: too brute force?

    invoker = GrassTestFilesInvoker(start_dir='.')
    # we can just iterate over all locations available in database
    # but the we don't know the right location label/shortcut
    invoker.run_in_location(gisdbase=gisdbase,
                            location=location,
                            location_shortcut=location_shortcut,
                            results_dir=results_dir)
