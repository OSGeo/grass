# -*- coding: utf-8 -*-
"""!@package grass.gunittest.invoker

@brief GRASS Python testing framework test files invoker (runner)

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

@author Vaclav Petras
"""

import os
import sys
import shutil
import string
import subprocess

from unittest.main import TestProgram, USAGE_AS_MAIN
TestProgram.USAGE = USAGE_AS_MAIN

from .loader import GrassTestLoader, discover_modules
from .reporters import (GrassTestFilesMultiReporter,
                        GrassTestFilesTextReporter,
                        GrassTestFilesHtmlReporter)
from .utils import silent_rmtree, ensure_dir

import grass.script.setup as gsetup


class GrassTestFilesInvoker(object):
    """A class used to invoke test files and create the main report"""

    # TODO: it is not clear what clean_outputs mean, if should be split
    # std stream, random outputs, saved results, profiling
    # not stdout and stderr if they contain test results
    # we can also save only failed tests, or generate only if assert fails
    def __init__(self, start_dir,
                 clean_mapsets=True, clean_outputs=True, clean_before=True,
                 testsuite_dir='testsuite'):
        """

        :param bool clean_mapsets: if the mapsets should be removed
        :param bool clean_outputs: meaning is unclear: random tests outputs,
            saved images from maps, profiling?
        :param bool clean_before: if mapsets, outputs, and results
            should be removed before the tests start
            (advantageous when the previous run left everything behind)
        """
        self.start_dir = start_dir
        self.clean_mapsets = clean_mapsets
        self.clean_outputs = clean_outputs
        self.clean_before = clean_before
        self.testsuite_dir = testsuite_dir
        # reporter is created for each call of run_in_location()
        self.reporter = None

    def _create_mapset(self, gisdbase, location, module):
        """Create mapset according to informations in module.

        :param loader.GrassTestPythonModule module:
        """
        # using path.sep but also / and \ for cases when it is confused
        # (namely the case of Unix path on MS Windows)
        # replace . to get rid of unclean path
        # TODO: clean paths
        # note that backslash cannot be at the end of raw string
        dir_as_name = module.tested_dir.translate(string.maketrans(r'/\.', '___'))
        mapset = dir_as_name + '_' + module.name
        # TODO: use grass module to do this? but we are not in the right gisdbase
        mapset_dir = os.path.join(gisdbase, location, mapset)
        if self.clean_before:
            silent_rmtree(mapset_dir)
        os.mkdir(mapset_dir)
        # TODO: default region in mapset will be what?
        # copy WIND file from PERMANENT
        # TODO: this should be a function in grass.script (used also in gis_set.py, PyGRASS also has its way with Mapset)
        # TODO: are premisions an issue here?
        shutil.copy(os.path.join(gisdbase, location, 'PERMANENT', 'WIND'),
                    os.path.join(mapset_dir))
        return mapset, mapset_dir

    def _run_test_module(self, module, results_dir, gisdbase, location):
        """Run one test file."""
        cwd = os.path.join(results_dir, module.tested_dir, module.name)
        data_dir = os.path.join(module.file_dir, 'data')
        if os.path.exists(data_dir):
            # TODO: link dir intead of copy tree
            shutil.copytree(data_dir, os.path.join(cwd, 'data'),
                            ignore=shutil.ignore_patterns('*.svn*'))
        ensure_dir(os.path.abspath(cwd))
        # TODO: put this to constructor and copy here again
        env = os.environ.copy()
        mapset, mapset_dir = self._create_mapset(gisdbase, location, module)
        gisrc = gsetup.write_gisrc(gisdbase, location, mapset)
        env['GISRC'] = gisrc

        stdout_path = os.path.join(cwd, 'stdout.txt')
        stderr_path = os.path.join(cwd, 'stderr.txt')
        stdout = open(stdout_path, 'w')
        stderr = open(stderr_path, 'w')

        self.reporter.start_file_test(module)
        # TODO: we might clean the directory here before test if non-empty
        # TODO: use some grass function to run?
        # add also '-Qwarn'?
        p = subprocess.Popen([sys.executable, '-tt', '-3',
                              module.abs_file_path],
                             cwd=cwd, env=env,
                             stdout=stdout, stderr=stderr)
        returncode = p.wait()
        stdout.close()
        stderr.close()
        self.reporter.end_file_test(module=module, cwd=cwd,
                                    returncode=returncode,
                                    stdout=stdout_path, stderr=stderr_path)
        # TODO: add some try-except or with for better error handling
        os.remove(gisrc)
        # TODO: only if clean up
        if self.clean_mapsets:
            shutil.rmtree(mapset_dir)

    def run_in_location(self, gisdbase, location, location_shortcut,
                        results_dir):
        """Run tests in a given location"""
        if os.path.abspath(results_dir) == os.path.abspath(self.start_dir):
            raise RuntimeError("Results root directory should not be the same"
                               " as discovery start directory")
        self.reporter = GrassTestFilesMultiReporter(
            reporters=[
                GrassTestFilesTextReporter(stream=sys.stderr),
                GrassTestFilesHtmlReporter(),
            ])

        # TODO: move constants out of loader class or even module
        modules = discover_modules(start_dir=self.start_dir,
                                   grass_location=location_shortcut,
                                   file_pattern=GrassTestLoader.files_in_testsuite,
                                   skip_dirs=GrassTestLoader.skip_dirs,
                                   testsuite_dir=GrassTestLoader.testsuite_dir,
                                   all_locations_value=GrassTestLoader.all_tests_value,
                                   universal_location_value=GrassTestLoader.universal_tests_value,
                                   import_modules=False)

        self.reporter.start(results_dir)
        for module in modules:
            self._run_test_module(module=module, results_dir=results_dir,
                                  gisdbase=gisdbase, location=location)

        self.reporter.finish()
