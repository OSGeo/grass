"""
GRASS Python testing framework test files invoker (runner)

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Vaclav Petras
"""

from __future__ import annotations

import collections
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import TYPE_CHECKING


from .checkers import text_to_keyvalue

from .loader import GrassTestLoader, discover_modules
from .reporters import (
    GrassTestFilesMultiReporter,
    GrassTestFilesTextReporter,
    GrassTestFilesHtmlReporter,
    TestsuiteDirReporter,
    GrassTestFilesKeyValueReporter,
    get_svn_path_authors,
    NoopFileAnonymizer,
    keyvalue_to_text,
)
from .utils import silent_rmtree, ensure_dir

import grass.script as gs
from grass.script.utils import decode, _get_encoding

if TYPE_CHECKING:
    from _typeshed import StrPath

maketrans = str.maketrans


# TODO: this might be more extend then update
def update_keyval_file(filename, module, returncode):
    if Path(filename).exists():
        keyval = text_to_keyvalue(Path(filename).read_text(encoding="utf-8"), sep="=")
    else:
        keyval = {}

    # this is for one file
    test_file_authors = get_svn_path_authors(module.abs_file_path)
    # in case that SVN is not available use empty authors
    if test_file_authors is None:
        test_file_authors = ""

    # always overwrite name and status
    keyval["name"] = module.name
    keyval["tested_dir"] = module.tested_dir
    if "status" not in keyval.keys():
        status = "failed" if returncode is None or returncode else "passed"
        keyval["status"] = status
    keyval["returncode"] = returncode
    keyval["test_file_authors"] = test_file_authors

    Path(filename).write_text(keyvalue_to_text(keyval), encoding="utf-8")
    return keyval


class GrassTestFilesInvoker:
    """A class used to invoke test files and create the main report"""

    # TODO: it is not clear what clean_outputs mean, if should be split
    # std stream, random outputs, saved results, profiling
    # not stdout and stderr if they contain test results
    # we can also save only failed tests, or generate only if assert fails
    def __init__(
        self,
        start_dir,
        clean_mapsets: bool = True,
        clean_outputs: bool = True,
        clean_before: bool = True,
        testsuite_dir: str = "testsuite",
        file_anonymizer=None,
        timeout: float | None = None,
    ):
        """

        :param bool clean_mapsets: if the mapsets should be removed
        :param bool clean_outputs: meaning is unclear: random tests outputs,
            saved images from maps, profiling?
        :param bool clean_before: if mapsets, outputs, and results
            should be removed before the tests start
            (advantageous when the previous run left everything behind)
        :param float timeout: maximum duration of one test in seconds
        """
        self.start_dir = start_dir
        self.clean_mapsets: bool = clean_mapsets
        self.clean_outputs: bool = clean_outputs
        self.clean_before: bool = clean_before
        self.testsuite_dir = testsuite_dir  # TODO: solve distribution of this constant
        # reporter is created for each call of run_in_location()
        self.reporter = None

        self.testsuite_dirs = None
        if file_anonymizer is None:
            self._file_anonymizer = NoopFileAnonymizer()
        else:
            self._file_anonymizer = file_anonymizer

        self.timeout: float | None = timeout

    def _create_mapset(self, gisdbase, location, module) -> tuple[str, str]:
        """Create mapset according to information in module.

        :param loader.GrassTestPythonModule module:
        """
        # TODO: use g.mapset -c, no need to duplicate functionality
        # All path characters such as slash, backslash and dot are replaced.
        dir_as_name = gs.legalize_vector_name(module.tested_dir, fallback_prefix=None)
        # Multiple processes can run the same test in the same location.
        mapset = gs.append_node_pid(f"{dir_as_name}_{module.name}")
        # TODO: use grass module to do this? but we are not in the right gisdbase
        mapset_dir = os.path.join(gisdbase, location, mapset)
        if self.clean_before:
            silent_rmtree(mapset_dir)
        Path(mapset_dir).mkdir()
        # TODO: default region in mapset will be what?
        # copy DEFAULT_WIND file from PERMANENT to WIND
        # TODO: this should be a function in grass.script (used also in gis_set.py,
        # PyGRASS also has its way with Mapset)
        shutil.copy(
            os.path.join(gisdbase, location, "PERMANENT", "DEFAULT_WIND"),
            os.path.join(mapset_dir, "WIND"),
        )
        return mapset, mapset_dir

    def _run_test_module(
        self, module, results_dir: StrPath, gisdbase, location, timeout: float | None
    ) -> None:
        """Run one test file."""
        self.testsuite_dirs[module.tested_dir].append(module.name)
        cwd: str = os.path.join(results_dir, module.tested_dir, module.name)
        data_dir: str = os.path.join(module.file_dir, "data")
        if Path(data_dir).exists():
            # TODO: link dir instead of copy tree and remove link afterwads
            # (removing is good because of testsuite dir in samplecode)
            # TODO: use different dir name in samplecode and test if it works
            shutil.copytree(
                data_dir,
                os.path.join(cwd, "data"),
                ignore=shutil.ignore_patterns("*.svn*"),
            )
        ensure_dir(os.path.abspath(cwd))
        # TODO: put this to constructor and copy here again
        env = os.environ.copy()
        mapset, mapset_dir = self._create_mapset(gisdbase, location, module)
        gisrc = gs.setup.write_gisrc(gisdbase, location, mapset)

        # here is special setting of environmental variables for running tests
        # some of them might be set from outside in the future and if the list
        # will be long they should be stored somewhere separately

        # use custom gisrc, not current session gisrc
        env["GISRC"] = gisrc
        # percentage in plain format is 0...10...20... ...100
        env["GRASS_MESSAGE_FORMAT"] = "plain"

        stdout_path = os.path.join(cwd, "stdout.txt")
        stderr_path = os.path.join(cwd, "stderr.txt")

        self.reporter.start_file_test(module)
        # TODO: we might clean the directory here before test if non-empty

        if module.file_type == "py":
            # ignoring shebang line to use current Python
            # and also pass parameters to it
            args = [sys.executable, module.abs_file_path]
        elif module.file_type == "sh":
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
            args = ["sh", "-e", "-x", module.abs_file_path]
        else:
            args = [module.abs_file_path]
        try:
            p = subprocess.run(
                args,
                cwd=cwd,
                env=env,
                capture_output=True,
                timeout=timeout,
                check=False,
            )
            stdout = p.stdout
            stderr = p.stderr
            returncode: int = p.returncode
            # No timeout to report. Non-none time out values are used to indicate
            # the timeout case.
            timed_out = None
        except subprocess.TimeoutExpired as error:
            stdout = error.stdout
            stderr = error.stderr
            if stdout is None:
                stdout = ""
            if stderr is None:
                stderr = (
                    f"Process has timed out in {timeout}s and produced no error "
                    "output.\n"
                )
            # Return code is None if the process times out.
            # Rest of the code expects success to evaluate as False.
            # So, we assign a failing return code.
            # In any case, we treat the timeout case as a failure.
            returncode = 1
            timed_out = timeout

        encodings = [_get_encoding(), "utf8", "latin-1", "ascii"]

        def try_decode(data, encodings):
            """Try to decode data (bytes) using one of encodings

            Falls back to decoding as UTF-8 with replacement for bytes.
            Strings are returned unmodified.
            """
            for encoding in encodings:
                try:
                    return decode(data, encoding=encoding)
                except UnicodeError:
                    pass
            if isinstance(data, bytes):
                return data.decode(encoding="utf-8", errors="replace")
            return data

        stdout = try_decode(stdout, encodings=encodings)
        stderr = try_decode(stderr, encodings=encodings)

        Path(stdout_path).write_text(stdout)
        with open(stderr_path, "w") as stderr_file:
            if isinstance(stderr, bytes):
                stderr_file.write(decode(stderr))
            elif isinstance(stderr, str):
                stderr_file.write(stderr)
            else:
                stderr_file.write(stderr.encode("utf8"))
        self._file_anonymizer.anonymize([stdout_path, stderr_path])

        test_summary = update_keyval_file(
            os.path.join(os.path.abspath(cwd), "test_keyvalue_result.txt"),
            module=module,
            returncode=returncode,
        )
        self.reporter.end_file_test(
            module=module,
            cwd=cwd,
            returncode=returncode,
            stdout=stdout_path,
            stderr=stderr_path,
            test_summary=test_summary,
            timed_out=timed_out,
        )
        # TODO: add some try-except or with for better error handling
        os.remove(gisrc)
        # TODO: only if clean up
        if self.clean_mapsets:
            try:
                shutil.rmtree(mapset_dir)
            except OSError:
                # If there are still running processes (e.g., in timeout case),
                # the cleaning may fail on non-empty directory. Although this does
                # not guarantee removal of the directory, try it again, but this
                # time ignore errors if something happens. (More file can appear
                # later on if the processes are still running.)
                shutil.rmtree(mapset_dir, ignore_errors=True)

    def run_in_location(
        self, gisdbase, location, location_type, results_dir: StrPath, exclude
    ) -> GrassTestFilesMultiReporter:
        """Run tests in a given location

        Returns an object with counting attributes of GrassTestFilesCountingReporter,
        i.e., a file-oriented reporter as opposed to testsuite-oriented one.
        Use only the attributes related to the summary, such as file_pass_per,
        not to one file as these will simply contain the last executed file.
        """
        if os.path.abspath(results_dir) == os.path.abspath(self.start_dir):
            msg = (
                "Results root directory should not be the same"
                " as discovery start directory"
            )
            raise RuntimeError(msg)
        self.reporter = GrassTestFilesMultiReporter(
            reporters=[
                GrassTestFilesTextReporter(stream=sys.stderr),
                GrassTestFilesHtmlReporter(
                    file_anonymizer=self._file_anonymizer,
                    main_page_name="testfiles.html",
                ),
                GrassTestFilesKeyValueReporter(
                    info={"location": location, "location_type": location_type}
                ),
            ]
        )
        self.testsuite_dirs = collections.defaultdict(
            list
        )  # reset list of dirs each time
        # TODO: move constants out of loader class or even module
        modules = discover_modules(
            start_dir=self.start_dir,
            grass_location=location_type,
            file_regexp=r".*\.(py|sh)$",
            skip_dirs=GrassTestLoader.skip_dirs,
            testsuite_dir=GrassTestLoader.testsuite_dir,
            all_locations_value=GrassTestLoader.all_tests_value,
            universal_location_value=GrassTestLoader.universal_tests_value,
            import_modules=False,
            exclude=exclude,
        )

        self.reporter.start(results_dir)
        for module in modules:
            self._run_test_module(
                module=module,
                results_dir=results_dir,
                gisdbase=gisdbase,
                location=location,
                timeout=self.timeout,
            )
        self.reporter.finish()

        # TODO: move this to some (new?) reporter
        # TODO: add basic summary of linked files so that the page is not empty
        Path(os.path.join(results_dir, "index.html")).write_text(
            "<html><body>"
            "<h1>Tests for &lt;{location}&gt;"
            " using &lt;{type}&gt; type tests</h1>"
            "<ul>"
            '<li><a href="testsuites.html">Results by testsuites</a>'
            " (testsuite directories)</li>"
            '<li><a href="testfiles.html">Results by test files</a></li>'
            "<ul>"
            "</body></html>".format(location=location, type=location_type),
            encoding="utf-8",
        )

        testsuite_dir_reporter = TestsuiteDirReporter(
            main_page_name="testsuites.html",
            testsuite_page_name="index.html",
            top_level_testsuite_page_name="testsuite_index.html",
        )
        testsuite_dir_reporter.report_for_dirs(
            root=results_dir, directories=self.testsuite_dirs
        )
        return self.reporter
