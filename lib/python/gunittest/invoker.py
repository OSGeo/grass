# -*- coding: utf-8 -*-
"""GRASS Python testing framework test files invoker (runner)

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
"""

import os
import sys
import shutil
import subprocess

from .checkers import text_to_keyvalue

from .loader import GrassTestLoader, discover_modules
from .reporters import (GrassTestFilesMultiReporter,
                        GrassTestFilesTextReporter, GrassTestFilesHtmlReporter,
                        TestsuiteDirReporter, GrassTestFilesKeyValueReporter,
                        get_svn_path_authors,
                        NoopFileAnonymizer, keyvalue_to_text)
from .utils import silent_rmtree, ensure_dir

from grass.script.utils import decode, encode, _get_encoding

try:
    from string import maketrans
except ImportError:
    maketrans = str.maketrans

# needed for write_gisrc
# TODO: it would be good to find some way of writing rc without the need to
# have GRASS proprly set (anything from grass.script requires translations to
# be set, i.e. the GRASS environment properly set)
import grass.script.setup as gsetup

import collections


# TODO: this might be more extend then update
def update_keyval_file(filename, module, returncode):
    if os.path.exists(filename):
        with open(filename, 'r') as keyval_file:
            keyval = text_to_keyvalue(keyval_file.read(), sep='=')
    else:
        keyval = {}

    # this is for one file
    test_file_authors = get_svn_path_authors(module.abs_file_path)
    # in case that SVN is not available use empty authors
    if test_file_authors is None:
        test_file_authors = ''

    # always owerwrite name and status
    keyval['name'] = module.name
    keyval['tested_dir'] = module.tested_dir
    if 'status' not in keyval.keys():
        keyval['status'] = 'failed' if returncode else 'passed'
    keyval['returncode'] = returncode
    keyval['test_file_authors'] = test_file_authors

    with open(filename, 'w') as keyval_file:
        keyval_file.write(keyvalue_to_text(keyval))
    return keyval


class GrassTestFilesInvoker(object):
    """A class used to invoke test files and create the main report"""

    # TODO: it is not clear what clean_outputs mean, if should be split
    # std stream, random outputs, saved results, profiling
    # not stdout and stderr if they contain test results
    # we can also save only failed tests, or generate only if assert fails
    def __init__(self, start_dir,
                 clean_mapsets=True, clean_outputs=True, clean_before=True,
                 testsuite_dir='testsuite', file_anonymizer=None):
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
        self.testsuite_dir = testsuite_dir  # TODO: solve distribution of this constant
        # reporter is created for each call of run_in_location()
        self.reporter = None

        self.testsuite_dirs = None
        if file_anonymizer is None:
            self._file_anonymizer = NoopFileAnonymizer()
        else:
            self._file_anonymizer = file_anonymizer

    def _create_mapset(self, gisdbase, location, module):
        """Create mapset according to information in module.

        :param loader.GrassTestPythonModule module:
        """
        # TODO: use g.mapset -c, no need to duplicate functionality
        # using path.sep but also / and \ for cases when it is confused
        # (namely the case of Unix path on MS Windows)
        # replace . to get rid of unclean path
        # TODO: clean paths
        # note that backslash cannot be at the end of raw string
        dir_as_name = module.tested_dir.translate(maketrans(r'/\.', '___'))
        mapset = dir_as_name + '_' + module.name
        # TODO: use grass module to do this? but we are not in the right gisdbase
        mapset_dir = os.path.join(gisdbase, location, mapset)
        if self.clean_before:
            silent_rmtree(mapset_dir)
        os.mkdir(mapset_dir)
        # TODO: default region in mapset will be what?
        # copy DEFAULT_WIND file from PERMANENT to WIND
        # TODO: this should be a function in grass.script (used also in gis_set.py, PyGRASS also has its way with Mapset)
        # TODO: are premisions an issue here?
        shutil.copy(os.path.join(gisdbase, location, 'PERMANENT', 'DEFAULT_WIND'),
                    os.path.join(mapset_dir, 'WIND'))
        return mapset, mapset_dir

    def _run_test_module(self, module, results_dir, gisdbase, location):
        """Run one test file."""
        self.testsuite_dirs[module.tested_dir].append(module.name)
        cwd = os.path.join(results_dir, module.tested_dir, module.name)
        data_dir = os.path.join(module.file_dir, 'data')
        if os.path.exists(data_dir):
            # TODO: link dir instead of copy tree and remove link afterwads
            # (removing is good because of testsuite dir in samplecode)
            # TODO: use different dir name in samplecode and test if it works
            shutil.copytree(data_dir, os.path.join(cwd, 'data'),
                            ignore=shutil.ignore_patterns('*.svn*'))
        ensure_dir(os.path.abspath(cwd))
        # TODO: put this to constructor and copy here again
        env = os.environ.copy()
        mapset, mapset_dir = self._create_mapset(gisdbase, location, module)
        gisrc = gsetup.write_gisrc(gisdbase, location, mapset)

        # here is special setting of environmental variables for running tests
        # some of them might be set from outside in the future and if the list
        # will be long they should be stored somewhere separately

        # use custom gisrc, not current session gisrc
        env['GISRC'] = gisrc
        # percentage in plain format is 0...10...20... ...100
        env['GRASS_MESSAGE_FORMAT'] = 'plain'

        stdout_path = os.path.join(cwd, 'stdout.txt')
        stderr_path = os.path.join(cwd, 'stderr.txt')

        self.reporter.start_file_test(module)
        # TODO: we might clean the directory here before test if non-empty

        if module.file_type == 'py':
            # ignoring shebang line to use current Python
            # and also pass parameters to it
            # add also '-Qwarn'?
            if sys.version_info.major >= 3:
                args = [sys.executable, '-tt', module.abs_file_path]
            else:
                args = [sys.executable, '-tt', '-3', module.abs_file_path]
            p = subprocess.Popen(args, cwd=cwd, env=env,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        elif module.file_type == 'sh':
            # ignoring shebang line to pass parameters to shell
            # expecting system to have sh or something compatible
            # TODO: add some special checks for MS Windows
            # using -x to see commands in stderr
            # using -e to terminate fast
            # from dash manual:
            # -e errexit     If not interactive, exit immediately if any
            #                untested command fails.  The exit status of a com‐
            #                mand is considered to be explicitly tested if the
            #                command is used to control an if, elif, while, or
            #                until; or if the command is the left hand operand
            #                of an '&&' or '||' operator.
            p = subprocess.Popen(['sh', '-e', '-x', module.abs_file_path],
                                 cwd=cwd, env=env,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        else:
            p = subprocess.Popen([module.abs_file_path],
                                 cwd=cwd, env=env,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        returncode = p.returncode
        encodings = [_get_encoding(), 'utf8', 'latin-1', 'ascii']
        detected = False
        idx = 0
        while not detected:
            try:
                stdout = decode(stdout, encoding=encodings[idx])
                detected = True
            except:
                idx += 1
                pass

        detected = False
        idx = 0
        while not detected:
            try:
                stderr = decode(stderr, encoding=encodings[idx])
                detected = True
            except:
                idx += 1
                pass

        with open(stdout_path, 'w') as stdout_file:
            stdout_file.write(stdout)
        with open(stderr_path, 'w') as stderr_file:
            if type(stderr) == 'bytes':
                stderr_file.write(decode(stderr))
            else:
                if isinstance(stderr, str):
                    stderr_file.write(stderr)
                else:
                    stderr_file.write(stderr.encode('utf8'))
        self._file_anonymizer.anonymize([stdout_path, stderr_path])

        test_summary = update_keyval_file(
            os.path.join(os.path.abspath(cwd), 'test_keyvalue_result.txt'),
            module=module, returncode=returncode)
        self.reporter.end_file_test(module=module, cwd=cwd,
                                    returncode=returncode,
                                    stdout=stdout_path, stderr=stderr_path,
                                    test_summary=test_summary)
        # TODO: add some try-except or with for better error handling
        os.remove(gisrc)
        # TODO: only if clean up
        if self.clean_mapsets:
            shutil.rmtree(mapset_dir)

    def run_in_location(self, gisdbase, location, location_type,
                        results_dir):
        """Run tests in a given location"""
        if os.path.abspath(results_dir) == os.path.abspath(self.start_dir):
            raise RuntimeError("Results root directory should not be the same"
                               " as discovery start directory")
        self.reporter = GrassTestFilesMultiReporter(
            reporters=[
                GrassTestFilesTextReporter(stream=sys.stderr),
                GrassTestFilesHtmlReporter(
                    file_anonymizer=self._file_anonymizer,
                    main_page_name='testfiles.html'),
                GrassTestFilesKeyValueReporter(
                    info=dict(location=location, location_type=location_type))
            ])
        self.testsuite_dirs = collections.defaultdict(list)  # reset list of dirs each time
        # TODO: move constants out of loader class or even module
        modules = discover_modules(start_dir=self.start_dir,
                                   grass_location=location_type,
                                   file_regexp=r'.*\.(py|sh)$',
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

        # TODO: move this to some (new?) reporter
        # TODO: add basic summary of linked files so that the page is not empty
        with open(os.path.join(results_dir, 'index.html'), 'w') as main_index:
            main_index.write(
                '<html><body>'
                '<h1>Tests for &lt;{location}&gt;'
                ' using &lt;{type}&gt; type tests</h1>'
                '<ul>'
                '<li><a href="testsuites.html">Results by testsuites</a>'
                ' (testsuite directories)</li>'
                '<li><a href="testfiles.html">Results by test files</a></li>'
                '<ul>'
                '</body></html>'
                .format(location=location, type=location_type))

        testsuite_dir_reporter = TestsuiteDirReporter(
            main_page_name='testsuites.html', testsuite_page_name='index.html',
            top_level_testsuite_page_name='testsuite_index.html')
        testsuite_dir_reporter.report_for_dirs(root=results_dir,
                                               directories=self.testsuite_dirs)
