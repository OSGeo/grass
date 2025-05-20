"""
GRASS Python testing framework module for report generation

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
"""

from __future__ import annotations

import os
import datetime
from pathlib import Path
from xml.sax import saxutils
import xml.etree.ElementTree as ET
import subprocess
import collections
import re
from collections.abc import Iterable

from .utils import add_gitignore_to_dir, ensure_dir
from .checkers import text_to_keyvalue


from io import StringIO


# TODO: change text_to_keyvalue to same sep as here
# TODO: create keyvalue file and move it there together with things from checkers
def keyvalue_to_text(keyvalue, sep="=", vsep="\n", isep=",", last_vertical=None):
    if not last_vertical:
        last_vertical = vsep == "\n"
    items = []
    for key, value in keyvalue.items():
        # TODO: use isep for iterables other than strings
        if not isinstance(value, str) and isinstance(value, Iterable):
            # TODO: this does not work for list of non-strings
            value = isep.join(value)
        items.append("{key}{sep}{value}".format(key=key, sep=sep, value=value))
    text = vsep.join(items)
    if last_vertical:
        text += vsep
    return text


def replace_in_file(file_path, pattern, repl):
    """

    :param repl: a repl parameter of ``re.sub()`` function
    """
    # using tmp file to store the replaced content
    tmp_file_path = file_path + ".tmp"
    with open(file_path) as old_file, open(tmp_file_path, "w") as new_file:
        new_file.writelines(
            re.sub(pattern=pattern, string=line, repl=repl) for line in old_file
        )
    # remove old file since it must not exist for rename/move
    os.remove(file_path)
    # replace old file by new file
    # TODO: this can fail in some (random) cases on MS Windows
    os.rename(tmp_file_path, file_path)


class NoopFileAnonymizer:
    def anonymize(self, filenames):
        pass


# TODO: why not remove GISDBASE by default?
class FileAnonymizer:
    def __init__(self, paths_to_remove, remove_gisbase=True, remove_gisdbase=False):
        self._paths_to_remove = []
        if remove_gisbase:
            gisbase = os.environ["GISBASE"]
            self._paths_to_remove.append(gisbase)
        if remove_gisdbase:
            # import only when really needed to avoid problems with
            # translations when environment is not set properly
            import grass.script as gs

            gisdbase = gs.gisenv()["GISDBASE"]
            self._paths_to_remove.append(gisdbase)
        if paths_to_remove:
            self._paths_to_remove.extend(paths_to_remove)

    def anonymize(self, filenames):
        # besides GISBASE and test recursion start directory (which is
        # supposed to be source root directory or similar) we can also try
        # to remove user home directory and GISDBASE
        # we suppose that we run in standard grass session
        # TODO: provide more effective implementation

        # regex for a trailing separator
        path_end = r"[\\/]?"
        for path in self._paths_to_remove:
            for filename in filenames:
                # literal special characters (e.g., ^\()[]{}.*+-$) in path need
                # to be escaped in a regex (2nd argument); otherwise, they will
                # be interpreted as special characters
                replace_in_file(filename, re.escape(path) + path_end, "")


def get_source_url(path, revision, line=None):
    """

    :param path: directory or file path relative to remote repository root
    :param revision: SVN revision (should be a number)
    :param line: line in the file (should be None for directories)
    """
    tracurl = "https://trac.osgeo.org/grass/browser/"
    if line:
        return "{tracurl}{path}?rev={revision}#L{line}".format(**locals())
    return "{tracurl}{path}?rev={revision}".format(**locals())


def html_escape(text):
    """Escape ``'&'``, ``'<'``, and ``'>'`` in a string of data."""
    return saxutils.escape(text)


def html_unescape(text):
    """Unescape ``'&amp;'``, ``'&lt;'``, and ``'&gt;'`` in a string of data."""
    return saxutils.unescape(text)


def color_error_line(line):
    if line.startswith("ERROR: "):
        # TODO: use CSS class
        # ignoring the issue with \n at the end, HTML don't mind
        line = '<span style="color: red">' + line + "</span>"
    if line.startswith("FAIL: "):
        # TODO: use CSS class
        # ignoring the issue with \n at the end, HTML don't mind
        line = '<span style="color: red">' + line + "</span>"
    if line.startswith("WARNING: "):
        # TODO: use CSS class
        # ignoring the issue with \n at the end, HTML don't mind
        line = '<span style="color: blue">' + line + "</span>"
    # if line.startswith('Traceback ('):
    #    line = '<span style="color: red">' + line + "</span>"
    return line


def to_web_path(path):
    """Replace OS dependent path separator with slash.

    Path on MS Windows are not usable in links on web. For MS Windows,
    this replaces backslash with (forward) slash.
    """
    if os.path.sep != "/":
        return path.replace(os.path.sep, "/")
    return path


def get_svn_revision():
    """Get SVN revision number

    :returns: SVN revision number as string or None if it is
        not possible to get
    """
    # TODO: here should be starting directory
    # but now we are using current as starting
    with subprocess.Popen(
        ["svnversion", "."], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    ) as p:
        stdout, stderr = p.communicate()
        rc = p.poll()
        if rc:
            return None
        stdout = stdout.strip()
        stdout = stdout.removesuffix("M")
        if ":" in stdout:
            # the first one is the one of source code
            stdout = stdout.split(":")[0]
        return stdout


def get_svn_info():
    """Get important information from ``svn info``

    :returns: SVN info as dictionary or None
        if it is not possible to obtain it
    """
    try:
        # TODO: introduce directory, not only current
        p = subprocess.Popen(
            ["svn", "info", ".", "--xml"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        stdout, stderr = p.communicate()
        rc = p.poll()
        info = {}
        if not rc:
            root = ET.fromstring(stdout)
            # TODO: get also date if this make sense
            # expecting only one <entry> element
            entry = root.find("entry")
            info["revision"] = entry.get("revision")
            info["url"] = entry.find("url").text
            relurl = entry.find("relative-url")
            # element which is not found is None
            # empty element would be bool(el) == False
            if relurl is not None:
                relurl = relurl.text
                # relative path has ^ at the beginning in SVN version 1.8.8
                relurl = relurl.removeprefix("^")
            else:
                # SVN version 1.8.8 supports relative-url but older do not
                # so, get relative part from absolute URL
                const_url_part = "https://svn.osgeo.org/grass/"
                relurl = info["url"][len(const_url_part) :]
            info["relative-url"] = relurl
            return info
    # TODO: add this to svnversion function
    except OSError as e:
        import errno

        # ignore No such file or directory
        if e.errno != errno.ENOENT:
            raise
    return None


def years_ago(date, years):
    # dateutil relative date would be better but this is more portable
    return date - datetime.timedelta(weeks=years * 52)


# TODO: these functions should be called only if we know that svn is installed
# this will simplify the functions, caller must handle it anyway
def get_svn_path_authors(path, from_date=None):
    """

    :returns: a set of authors
    """
    # "BASE:1" is the SVN default for local copies
    revision_range = "BASE:1" if from_date is None else "BASE:{%s}" % from_date
    try:
        # TODO: allow also usage of --limit
        p = subprocess.Popen(
            ["svn", "log", "--xml", "--revision", revision_range, path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        stdout, stderr = p.communicate()
        rc = p.poll()
        if not rc:
            root = ET.fromstring(stdout)
            # TODO: get also date if this make sense
            # expecting only one <entry> element
            author_nodes = root.iterfind("*/author")
            authors = [n.text for n in author_nodes]
            return set(authors)
    except OSError as e:
        import errno

        # ignore No such file or directory
        if e.errno != errno.ENOENT:
            raise
    return None


def get_html_test_authors_table(directory, tests_authors):
    # SVN gives us authors of code together with authors of tests
    # so test code authors list also contains authors of tests only
    # TODO: don't do this for the top level directories?
    tests_authors = set(tests_authors)
    no_svn_text = (
        '<span style="font-size: 60%">Test file authors were not obtained.</span>'
    )
    if not tests_authors or (len(tests_authors) == 1 and list(tests_authors)[0] == ""):
        return "<h3>Code and test authors</h3>" + no_svn_text
    from_date = years_ago(datetime.date.today(), years=1)
    tested_dir_authors = get_svn_path_authors(directory, from_date)
    if tested_dir_authors is not None:
        not_testing_authors = tested_dir_authors - tests_authors
    else:
        no_svn_text = (
            '<span style="font-size: 60%">Authors cannot be obtained using SVN.</span>'
        )
        not_testing_authors = tested_dir_authors = [no_svn_text]
    if not not_testing_authors:
        not_testing_authors = ["all recent authors contributed tests"]

    return (
        "<h3>Code and test authors</h3>"
        '<p style="font-size: 60%"><em>'
        "Note that determination of authors is approximate and only"
        " recent code authors are considered."
        "</em></p>"
        "<table><tbody>"
        "<tr><td>Test authors:</td><td>{file_authors}</td></tr>"
        "<tr><td>Authors of tested code:</td><td>{code_authors}</td></tr>"
        "<tr><td>Authors owing tests:</td><td>{not_testing}</td></tr>"
        "</tbody></table>".format(
            file_authors=", ".join(sorted(tests_authors)),
            code_authors=", ".join(sorted(tested_dir_authors)),
            not_testing=", ".join(sorted(not_testing_authors)),
        )
    )


class GrassTestFilesMultiReporter:
    """Interface to multiple repoter objects

    For start and finish of the tests and of a test of one file,
    it calls corresponding methods of all contained reporters.
    For all other attributes, it returns attribute of a first reporter
    which has this attribute using the order in which the reporters were
    provided.
    """

    def __init__(self, reporters, forgiving=False):
        self.reporters = reporters
        self.forgiving = forgiving

    def start(self, results_dir):
        # TODO: no directory cleaning (self.clean_before)? now cleaned by caller
        # TODO: perhaps only those who need it should do it (even multiple times)
        # and there is also the delete problem
        ensure_dir(os.path.abspath(results_dir))
        add_gitignore_to_dir(os.path.abspath(results_dir))
        for reporter in self.reporters:
            try:
                reporter.start(results_dir)
            except AttributeError:
                if self.forgiving:
                    pass
                else:
                    raise

    def finish(self):
        for reporter in self.reporters:
            try:
                reporter.finish()
            except AttributeError:
                if self.forgiving:
                    pass
                else:
                    raise

    def start_file_test(self, module):
        for reporter in self.reporters:
            try:
                reporter.start_file_test(module)
            except AttributeError:
                if self.forgiving:
                    pass
                else:
                    raise

    def end_file_test(self, **kwargs):
        for reporter in self.reporters:
            try:
                reporter.end_file_test(**kwargs)
            except AttributeError:
                if self.forgiving:
                    pass
                else:
                    raise

    def __getattr__(self, name):
        for reporter in self.reporters:
            try:
                return getattr(reporter, name)
            except AttributeError:
                continue
        raise AttributeError


class GrassTestFilesCountingReporter:
    def __init__(self):
        self.test_files = None
        self.files_fail = None
        self.files_pass = None

        self.file_pass_per = None
        self.file_fail_per = None

        self.main_start_time = None
        self.main_end_time = None
        self.main_time = None

        self.file_start_time = None
        self.file_end_time = None
        self.file_time = None
        self._start_file_test_called = False

    def start(self, results_dir):
        self.test_files = 0
        self.files_fail = 0
        self.files_pass = 0

        # this might be moved to some report start method
        self.main_start_time = datetime.datetime.now()

    def finish(self):
        self.main_end_time = datetime.datetime.now()
        self.main_time = self.main_end_time - self.main_start_time

        assert self.test_files == self.files_fail + self.files_pass
        if self.test_files:
            self.file_pass_per = 100 * float(self.files_pass) / self.test_files
            self.file_fail_per = 100 * float(self.files_fail) / self.test_files
        else:
            # if no tests were executed, probably something bad happened
            # try to report at least something
            self.file_pass_per = None
            self.file_fail_per = None

    def start_file_test(self, module):
        self.file_start_time = datetime.datetime.now()
        self._start_file_test_called = True
        self.test_files += 1

    def end_file_test(self, returncode, **kwargs):
        assert self._start_file_test_called
        self.file_end_time = datetime.datetime.now()
        self.file_time = self.file_end_time - self.file_start_time
        if returncode:
            self.files_fail += 1
        else:
            self.files_pass += 1
        self._start_file_test_called = False


def percent_to_html(percent):
    if percent is None:
        return '<span style="color: {color}">unknown percentage</span>'
    if percent > 100 or percent < 0:
        return "? {:.2f}% ?".format(percent)
    if percent < 40:
        color = "red"
    elif percent < 70:
        color = "orange"
    else:
        color = "green"
    return '<span style="color: {color}">{percent:.1f}%</span>'.format(
        percent=percent, color=color
    )


def wrap_stdstream_to_html(infile, outfile, module, stream):
    before = "<html><body><h1>%s</h1><pre>" % (module.name + " " + stream)
    after = "</pre></body></html>"
    with open(outfile, "w") as html, open(infile) as text:
        html.write(before)
        html.writelines(color_error_line(html_escape(line)) for line in text)
        html.write(after)


def html_file_preview(filename):
    before = "<pre>"
    after = "</pre>"
    if not os.path.isfile(filename):
        return '<p style="color: red>File %s does not exist</p>' % filename
    size = os.path.getsize(filename)
    if not size:
        return '<p style="color: red>File %s is empty</p>' % filename
    max_size = 10000
    html = StringIO()
    html.write(before)
    if size < max_size:
        with open(filename) as text:
            for line in text:
                html.write(color_error_line(html_escape(line)))
    elif size < 10 * max_size:

        def tail(filename, n):
            return collections.deque(open(filename), n)  # noqa: SIM115

        html.write("... (lines omitted)\n")
        for line in tail(filename, 50):
            html.write(color_error_line(html_escape(line)))
    else:
        return '<p style="color: red>File %s is too large to show</p>' % filename
    html.write(after)
    return html.getvalue()


def returncode_to_html_text(returncode, timed_out=None):
    if returncode:
        extra = f" (timeout >{timed_out}s)" if timed_out is not None else ""
        return f'<span style="color: red">FAILED{extra}</span>'
    # alternatives: SUCCEEDED, passed, OK
    return '<span style="color: green">succeeded</span>'


# not used
def returncode_to_html_sentence(returncode):
    if returncode:
        return (
            '<span style="color: red">&#x274c;</span>'
            " Test failed (return code %d)" % (returncode)
        )
    return (
        '<span style="color: green">&#x2713;</span>'
        " Test succeeded (return code %d)" % (returncode)
    )


def returncode_to_success_html_par(returncode):
    if returncode:
        return '<p> <span style="color: red">&#x274c;</span> Test failed</p>'
    return '<p> <span style="color: green">&#x2713;</span> Test succeeded</p>'


def success_to_html_text(total, successes):
    if successes < total:
        return '<span style="color: red">FAILED</span>'
    if successes == total:
        # alternatives: SUCCEEDED, passed, OK
        return '<span style="color: green">succeeded</span>'
    return (
        '<span style="color: red; font-size: 60%">? more successes than total ?</span>'
    )


UNKNOWN_NUMBER_HTML = '<span style="font-size: 60%">unknown</span>'


def success_to_html_percent(total, successes):
    if total:
        pass_per = 100 * (float(successes) / total)
        return percent_to_html(pass_per)
    return UNKNOWN_NUMBER_HTML


def format_percentage(percentage: float | None) -> str:
    if percentage is not None:
        return "{nsper:.1f}%".format(nsper=percentage)
    return "unknown percentage"


class GrassTestFilesHtmlReporter(GrassTestFilesCountingReporter):
    unknown_number = UNKNOWN_NUMBER_HTML

    def __init__(self, file_anonymizer, main_page_name="index.html"):
        super().__init__()
        self.main_index = None
        self._file_anonymizer = file_anonymizer
        self._main_page_name = main_page_name

    def start(self, results_dir):
        super().start(results_dir)
        # having all variables public although not really part of API
        main_page_name = os.path.join(results_dir, self._main_page_name)
        # TODO: Ensure file is closed in all situations
        self.main_index = open(main_page_name, "w")  # noqa: SIM115

        # TODO: this can be moved to the counter class
        self.failures = 0
        self.errors = 0
        self.skipped = 0
        self.successes = 0
        self.expected_failures = 0
        self.unexpected_success = 0
        self.total = 0

        svn_info = get_svn_info()
        if not svn_info:
            svn_text = (
                '<span style="font-size: 60%">SVN revision cannot be obtained</span>'
            )
        else:
            url = get_source_url(
                path=svn_info["relative-url"], revision=svn_info["revision"]
            )
            svn_text = ('SVN revision <a href="{url}">{rev}</a>').format(
                url=url, rev=svn_info["revision"]
            )
        self.main_index.write(
            "<html><body>"
            "<h1>Test results</h1>"
            "{time:%Y-%m-%d %H:%M:%S}"
            " ({svn})"
            "<table>"
            "<thead><tr>"
            "<th>Tested directory</th>"
            "<th>Test file</th>"
            "<th>Status</th>"
            "<th>Tests</th><th>Successful</td>"
            "<th>Failed</th><th>Percent successful</th>"
            "</tr></thead><tbody>".format(time=self.main_start_time, svn=svn_text)
        )

    def finish(self):
        super().finish()

        pass_per = success_to_html_percent(total=self.total, successes=self.successes)
        tfoot = (
            "<tfoot>"
            "<tr>"
            "<td>Summary</td>"
            "<td>{nfiles} test files</td>"
            "<td>{nsper}</td>"
            "<td>{total}</td><td>{st}</td><td>{ft}</td><td>{pt}</td>"
            "</tr>"
            "</tfoot>".format(
                nfiles=self.test_files,
                nsper=percent_to_html(self.file_pass_per),
                st=self.successes,
                ft=self.failures + self.errors,
                total=self.total,
                pt=pass_per,
            )
        )

        summary_sentence = (
            "\nExecuted {nfiles} test files in {time:}."
            "\nFrom them, {nsfiles} files ({nsper}) were successful"
            " and {nffiles} files ({nfper}) failed.\n".format(
                nfiles=self.test_files,
                time=self.main_time,
                nsfiles=self.files_pass,
                nffiles=self.files_fail,
                nsper=format_percentage(self.file_pass_per),
                nfper=format_percentage(self.file_fail_per),
            )
        )

        self.main_index.write(
            "<tbody>{tfoot}</table><p>{summary}</p></body></html>".format(
                tfoot=tfoot, summary=summary_sentence
            )
        )
        self.main_index.close()

    def start_file_test(self, module):
        super().start_file_test(module)
        self.main_index.flush()  # to get previous lines to the report

    def end_file_test(
        self, module, cwd, returncode, stdout, stderr, test_summary, timed_out=None
    ):
        super().end_file_test(
            module=module,
            cwd=cwd,
            returncode=returncode,
            stdout=stdout,
            stderr=stderr,
            timed_out=timed_out,
        )
        # considering others according to total is OK when we more or less
        # know that input data make sense (total >= errors + failures)
        total = test_summary.get("total", None)
        failures = test_summary.get("failures", 0)
        errors = test_summary.get("errors", 0)
        # Python unittest TestResult class is reporting success for no
        # errors or failures, so skipped, expected failures and unexpected
        # success are ignored
        # but successful tests are only total - the others
        # TODO: add success counter to GrassTestResult base class
        skipped = test_summary.get("skipped", 0)
        expected_failures = test_summary.get("expected_failures", 0)
        unexpected_successes = test_summary.get("unexpected_successes", 0)
        successes = test_summary.get("successes", 0)

        self.failures += failures
        self.errors += errors
        self.skipped += skipped
        self.expected_failures += expected_failures
        self.unexpected_success += unexpected_successes

        # zero would be valid here
        if total is not None:
            # success are only the clear ones
            # percentage is influenced by all
            # but putting only failures to table
            self.successes += successes
            self.total += total
            # this will handle zero
            pass_per = success_to_html_percent(total=total, successes=successes)
        else:
            total = successes = pass_per = self.unknown_number
        bad_ones = failures + errors
        self.main_index.write(
            "<tr><td>{d}</td>"
            '<td><a href="{d}/{m}/index.html">{m}</a></td>'
            "<td>{status}</td>"
            "<td>{ntests}</td><td>{stests}</td>"
            "<td>{ftests}</td><td>{ptests}</td>"
            "<tr>".format(
                d=to_web_path(module.tested_dir),
                m=module.name,
                status=returncode_to_html_text(returncode, timed_out),
                stests=successes,
                ftests=bad_ones,
                ntests=total,
                ptests=pass_per,
            )
        )
        wrap_stdstream_to_html(
            infile=stdout,
            outfile=os.path.join(cwd, "stdout.html"),
            module=module,
            stream="stdout",
        )
        wrap_stdstream_to_html(
            infile=stderr,
            outfile=os.path.join(cwd, "stderr.html"),
            module=module,
            stream="stderr",
        )

        file_index_path = os.path.join(cwd, "index.html")
        header = (
            '<!DOCTYPE html><html><head><meta charset="utf-8"></head><body>'
            "<h1>{m.name}</h1>"
            "<h2>{m.tested_dir} &ndash; {m.name}</h2>"
            "{status}".format(
                m=module,
                status=returncode_to_success_html_par(returncode),
            )
        )

        # TODO: include optionally hyper link to test suite
        # TODO: file_path is reconstructed in a naive way
        # file_path should be stored in the module/test file object and just used here
        summary_section = (
            "<table><tbody>"
            "<tr><td>Test</td><td>{m}</td></tr>"
            "<tr><td>Testsuite</td><td>{d}</td></tr>"
            "<tr><td>Test file</td><td>{file_path}</td></tr>"
            "<tr><td>Status</td><td>{status}</td></tr>"
            "<tr><td>Return code</td><td>{rc}</td></tr>"
            "<tr><td>Number of tests</td><td>{ntests}</td></tr>"
            "<tr><td>Successful tests</td><td>{stests}</td></tr>"
            "<tr><td>Failed tests</td><td>{ftests}</td></tr>"
            "<tr><td>Percent successful</td><td>{ptests}</td></tr>"
            "<tr><td>Test duration</td><td>{dur}</td></tr>".format(
                d=module.tested_dir,
                m=module.name,
                file_path=os.path.join(
                    module.tested_dir, "testsuite", module.name + "." + module.file_type
                ),
                status=returncode_to_html_text(returncode, timed_out),
                stests=successes,
                ftests=bad_ones,
                ntests=total,
                ptests=pass_per,
                rc=returncode,
                dur=self.file_time,
            )
        )

        modules = test_summary.get("tested_modules", None)
        if modules:
            # TODO: replace by better handling of potential lists when parsing
            # TODO: create link to module if running in grass or in addons
            # alternatively a link to module test summary
            if not isinstance(modules, list):
                modules = [modules]

        # here we would have also links to coverage, profiling, ...
        # '<li><a href="testcodecoverage/index.html">code coverage</a></li>'
        files_section = (
            "<h3>Supplementary files</h3>"
            "<ul>"
            '<li><a href="stdout.html">standard output (stdout)</a></li>'
            '<li><a href="stderr.html">standard error output (stderr)</a></li>'
        )

        supplementary_files = test_summary.get("supplementary_files", None)
        if supplementary_files:
            # this is something we might want to do once for all and not
            # risk that it will be done twice or rely that somebody else
            # will do it for use
            # the solution is perhaps do the multi reporter more grass-specific
            # and do all common things, so that other can rely on it and
            # moreover something can be shared with other explicitly
            # using constructors as seems advantageous for counting
            self._file_anonymizer.anonymize(supplementary_files)

        with open(file_index_path, "w") as file_index:
            file_index.write(header)
            file_index.write(summary_section)
            if modules:
                file_index.write(
                    "<tr><td>Tested modules</td><td>{0}</td></tr>".format(
                        ", ".join(sorted(set(modules)))
                    )
                )
            file_index.write("</tbody></table>")

            file_index.write(files_section)

            if supplementary_files:
                file_index.writelines(
                    '<li><a href="{f}">{f}</a></li>'.format(f=f)
                    for f in supplementary_files
                )

            file_index.write("</ul>")

            if returncode:
                file_index.write("<h3>Standard error output (stderr)</h3>")
                file_index.write(html_file_preview(stderr))

            file_index.write("</body></html>")

        if returncode:
            pass
            # TODO: here we don't have opportunity to write error file
            # to stream (stdout/stderr)
            # a stream can be added and if not none, we could write


# TODO: document info: additional information to be stored type: dict
# allows overwriting what was collected
class GrassTestFilesKeyValueReporter(GrassTestFilesCountingReporter):
    def __init__(self, info=None):
        super().__init__()
        self.result_dir = None
        self._info = info

    def start(self, results_dir):
        super().start(results_dir)
        # having all variables public although not really part of API
        self.result_dir = results_dir

        # TODO: this can be moved to the counter class
        self.failures = 0
        self.errors = 0
        self.skipped = 0
        self.successes = 0
        self.expected_failures = 0
        self.unexpected_success = 0
        self.total = 0

        # TODO: document: tested_dirs is a list and it should fit with names
        self.names = []
        self.tested_dirs = []
        self.files_returncodes = []

        # sets (no size specified)
        self.modules = set()
        self.test_files_authors = set()

    def finish(self):
        super().finish()

        # this shoul be moved to some additional meta passed in constructor
        svn_info = get_svn_info()
        svn_revision = "" if not svn_info else svn_info["revision"]

        summary = {
            "files_total": self.test_files,
            "files_successes": self.files_pass,
            "files_failures": self.files_fail,
            "names": self.names,
            "tested_dirs": self.tested_dirs,
            # TODO: we don't have a general mechanism for storing any type in text
            "files_returncodes": [str(item) for item in self.files_returncodes],
            # let's use seconds as a universal time delta format
            # (there is no standard way how to store time delta as string)
            "time": self.main_time.total_seconds(),
            "status": "failed" if self.files_fail else "succeeded",
            "total": self.total,
            "successes": self.successes,
            "failures": self.failures,
            "errors": self.errors,
            "skipped": self.skipped,
            "expected_failures": self.expected_failures,
            "unexpected_successes": self.unexpected_success,
            "test_files_authors": self.test_files_authors,
            "tested_modules": self.modules,
            "svn_revision": svn_revision,
            # ignoring issues with time zones
            "timestamp": self.main_start_time.strftime("%Y-%m-%d %H:%M:%S"),
            # TODO: add some general metadata here (passed in constructor)
            # add additional information
            **dict(self._info.items()),
        }

        summary_filename = os.path.join(self.result_dir, "test_keyvalue_result.txt")
        text = keyvalue_to_text(summary, sep="=", vsep="\n", isep=",")
        Path(summary_filename).write_text(text)

    def end_file_test(
        self, module, cwd, returncode, stdout, stderr, test_summary, timed_out=None
    ):
        super().end_file_test(
            module=module,
            cwd=cwd,
            returncode=returncode,
            stdout=stdout,
            stderr=stderr,
            timed_out=timed_out,
        )
        # TODO: considering others according to total, OK?
        # here we are using 0 for total but HTML reporter is using None
        total = test_summary.get("total", 0)
        failures = test_summary.get("failures", 0)
        errors = test_summary.get("errors", 0)
        # Python unittest TestResult class is reporting success for no
        # errors or failures, so skipped, expected failures and unexpected
        # success are ignored
        # but successful tests are only total - the others
        skipped = test_summary.get("skipped", 0)
        expected_failures = test_summary.get("expected_failures", 0)
        unexpected_successes = test_summary.get("unexpected_successes", 0)
        successes = test_summary.get("successes", 0)

        # TODO: move this to counter class and perhaps use aggregation
        # rather then inheritance
        self.failures += failures
        self.errors += errors
        self.skipped += skipped
        self.expected_failures += expected_failures
        self.unexpected_success += unexpected_successes

        # TODO: should we test for zero?
        if total is not None:
            # success are only the clear ones
            # percentage is influenced by all
            # but putting only failures to table
            self.successes += successes
            self.total += total

        self.files_returncodes.append(returncode)

        self.tested_dirs.append(module.tested_dir)
        self.names.append(module.name)

        modules = test_summary.get("tested_modules", None)
        if modules:
            # TODO: replace by better handling of potential lists when parsing
            # TODO: create link to module if running in grass or in addons
            # alternatively a link to module test summary
            if type(modules) not in [list, set]:
                modules = [modules]
            self.modules.update(modules)

        test_file_authors = test_summary["test_file_authors"]
        if type(test_file_authors) not in [list, set]:
            test_file_authors = [test_file_authors]
        self.test_files_authors.update(test_file_authors)


class GrassTestFilesTextReporter(GrassTestFilesCountingReporter):
    def __init__(self, stream):
        super().__init__()
        self._stream = stream

    def finish(self):
        super().finish()

        summary_sentence = (
            "\nExecuted {nfiles} test files in {time:}."
            "\nFrom them"
            " {nsfiles} files ({nsper}) were successful"
            " and {nffiles} files ({nfper}) failed.\n".format(
                nfiles=self.test_files,
                time=self.main_time,
                nsfiles=self.files_pass,
                nffiles=self.files_fail,
                nsper=format_percentage(self.file_pass_per),
                nfper=format_percentage(self.file_fail_per),
            )
        )
        self._stream.write(summary_sentence)

    def start_file_test(self, module):
        super().start_file_test(module)
        self._stream.write("Running {file}...\n".format(file=module.file_path))
        # get the above line and all previous ones to the report
        self._stream.flush()

    def end_file_test(
        self, module, cwd, returncode, stdout, stderr, test_summary, timed_out=None
    ):
        super().end_file_test(
            module=module,
            cwd=cwd,
            returncode=returncode,
            stdout=stdout,
            stderr=stderr,
            timed_out=timed_out,
        )

        if returncode:
            width = 72
            self._stream.write(width * "=")
            self._stream.write("\n")
            self._stream.write(Path(stderr).read_text())
            self._stream.write(width * "=")
            self._stream.write("\n")
            self._stream.write(f"FAILED {module.file_path}")
            if timed_out:
                self._stream.write(f" - Timeout >{timed_out}s")
            num_failed = test_summary.get("failures", 0)
            num_failed += test_summary.get("errors", 0)
            if num_failed:
                text = " ({f} tests failed)" if num_failed > 1 else " ({f} test failed)"
                self._stream.write(text.format(f=num_failed))
            self._stream.write("\n")
            # TODO: here we lost the possibility to include also file name
            # of the appropriate report


# TODO: there is a quite a lot duplication between this class and html reporter
# TODO: document: do not use it for two reports, it accumulates the results
# TODO: add also keyvalue summary generation?
# wouldn't this conflict with collecting data from report afterwards?
class TestsuiteDirReporter:
    def __init__(
        self,
        main_page_name,
        testsuite_page_name="index.html",
        top_level_testsuite_page_name=None,
    ):
        self.main_page_name = main_page_name
        self.testsuite_page_name = testsuite_page_name
        self.top_level_testsuite_page_name = top_level_testsuite_page_name

        # TODO: this might be even a object which could add and validate
        self.failures = 0
        self.errors = 0
        self.skipped = 0
        self.successes = 0
        self.expected_failures = 0
        self.unexpected_successes = 0
        self.total = 0

        self.testsuites = 0
        self.testsuites_successes = 0
        self.files = 0
        self.files_successes = 0

    def report_for_dir(self, root, directory, test_files):
        # TODO: create object from this, so that it can be passed from
        # one function to another
        # TODO: put the inside of for loop to another function
        dir_failures = 0
        dir_errors = 0
        dir_skipped = 0
        dir_successes = 0
        dir_expected_failures = 0
        dir_unexpected_success = 0
        dir_total = 0
        test_files_authors = []

        file_total = 0
        file_successes = 0

        page_name = os.path.join(root, directory, self.testsuite_page_name)
        if self.top_level_testsuite_page_name and os.path.abspath(
            os.path.join(root, directory)
        ) == os.path.abspath(root):
            page_name = os.path.join(root, self.top_level_testsuite_page_name)

        # TODO: should we use forward slashes also for the HTML because
        # it is simpler are more consistent with the rest on MS Windows?
        head = "<html><body><h1>{name} testsuite results</h1>".format(name=directory)
        tests_table_head = (
            "<h3>Test files results</h3>"
            "<table>"
            "<thead><tr>"
            "<th>Test file</th><th>Status</th>"
            "<th>Tests</th><th>Successful</td>"
            "<th>Failed</th><th>Percent successful</th>"
            "</tr></thead><tbody>"
        )
        with open(page_name, "w") as page:
            page.write(head)
            page.write(tests_table_head)
            for test_file_name in test_files:
                # TODO: put keyvalue fine name to constant
                summary_filename = os.path.join(
                    root, directory, test_file_name, "test_keyvalue_result.txt"
                )
                # if os.path.exists(summary_filename):
                summary = text_to_keyvalue(Path(summary_filename).read_text(), sep="=")
                # else:
                # TODO: write else here
                #    summary = None

                if "total" not in summary:
                    bad_ones = successes = UNKNOWN_NUMBER_HTML
                    total = None
                else:
                    bad_ones = summary["failures"] + summary["errors"]
                    successes = summary["successes"]
                    total = summary["total"]

                    self.failures += summary["failures"]
                    self.errors += summary["errors"]
                    self.skipped += summary["skipped"]
                    self.successes += summary["successes"]
                    self.expected_failures += summary["expected_failures"]
                    self.unexpected_successes += summary["unexpected_successes"]
                    self.total += summary["total"]

                    dir_failures += summary["failures"]
                    dir_errors += summary["failures"]
                    dir_skipped += summary["skipped"]
                    dir_successes += summary["successes"]
                    dir_expected_failures += summary["expected_failures"]
                    dir_unexpected_success += summary["unexpected_successes"]
                    dir_total += summary["total"]

                # TODO: keyvalue method should have types for keys function
                # perhaps just the current post processing function is enough
                test_file_authors = summary.get("test_file_authors")
                if not test_file_authors:
                    test_file_authors = []
                if not isinstance(test_file_authors, list):
                    test_file_authors = [test_file_authors]
                test_files_authors.extend(test_file_authors)

                file_total += 1
                # Use non-zero return code in case it is missing.
                # (This can happen when the test has timed out.)
                return_code = summary.get("returncode", 1)
                file_successes += 0 if return_code else 1

                pass_per = success_to_html_percent(total=total, successes=successes)
                row = (
                    "<tr>"
                    '<td><a href="{f}/index.html">{f}</a></td>'
                    "<td>{status}</td>"
                    "<td>{ntests}</td><td>{stests}</td>"
                    "<td>{ftests}</td><td>{ptests}</td>"
                    "<tr>".format(
                        f=test_file_name,
                        status=returncode_to_html_text(return_code),
                        stests=successes,
                        ftests=bad_ones,
                        ntests=total,
                        ptests=pass_per,
                    )
                )
                page.write(row)

            self.testsuites += 1
            self.testsuites_successes += 1 if file_successes == file_total else 0
            self.files += file_total
            self.files_successes += file_successes

            dir_pass_per = success_to_html_percent(
                total=dir_total, successes=dir_successes
            )
            file_pass_per = success_to_html_percent(
                total=file_total, successes=file_successes
            )
            tests_table_foot = (
                "</tbody><tfoot><tr>"
                "<td>Summary</td>"
                "<td>{status}</td>"
                "<td>{ntests}</td><td>{stests}</td>"
                "<td>{ftests}</td><td>{ptests}</td>"
                "</tr></tfoot></table>".format(
                    status=file_pass_per,
                    stests=dir_successes,
                    ftests=dir_failures + dir_errors,
                    ntests=dir_total,
                    ptests=dir_pass_per,
                )
            )
            page.write(tests_table_foot)
            test_authors = get_html_test_authors_table(
                directory=directory, tests_authors=test_files_authors
            )
            page.write(test_authors)
            page.write("</body></html>")

        status = success_to_html_text(total=file_total, successes=file_successes)
        return (
            "<tr>"
            '<td><a href="{d}/{page}">{d}</a></td><td>{status}</td>'
            "<td>{nfiles}</td><td>{sfiles}</td><td>{pfiles}</td>"
            "<td>{ntests}</td><td>{stests}</td>"
            "<td>{ftests}</td><td>{ptests}</td>"
            "<tr>".format(
                d=to_web_path(directory),
                page=self.testsuite_page_name,
                status=status,
                nfiles=file_total,
                sfiles=file_successes,
                pfiles=file_pass_per,
                stests=dir_successes,
                ftests=dir_failures + dir_errors,
                ntests=dir_total,
                ptests=dir_pass_per,
            )
        )

    def report_for_dirs(self, root, directories):
        # TODO: this will need changes according to potential changes in
        # absolute/relative paths

        page_name = os.path.join(root, self.main_page_name)
        head = "<html><body><h1>Testsuites results</h1>"
        tests_table_head = (
            "<table>"
            "<thead><tr>"
            "<th>Testsuite</th>"
            "<th>Status</th>"
            "<th>Test files</th><th>Successful</td>"
            "<th>Percent successful</th>"
            "<th>Tests</th><th>Successful</td>"
            "<th>Failed</th><th>Percent successful</th>"
            "</tr></thead><tbody>"
        )

        pass_per = success_to_html_percent(total=self.total, successes=self.successes)
        file_pass_per = success_to_html_percent(
            total=self.files, successes=self.files_successes
        )
        testsuites_pass_per = success_to_html_percent(
            total=self.testsuites, successes=self.testsuites_successes
        )
        tests_table_foot = (
            "<tfoot>"
            "<tr>"
            "<td>Summary</td><td>{status}</td>"
            "<td>{nfiles}</td><td>{sfiles}</td><td>{pfiles}</td>"
            "<td>{ntests}</td><td>{stests}</td>"
            "<td>{ftests}</td><td>{ptests}</td>"
            "</tr>"
            "</tfoot>".format(
                status=testsuites_pass_per,
                nfiles=self.files,
                sfiles=self.files_successes,
                pfiles=file_pass_per,
                stests=self.successes,
                ftests=self.failures + self.errors,
                ntests=self.total,
                ptests=pass_per,
            )
        )

        with open(page_name, "w") as page:
            page.write(head)
            page.write(tests_table_head)

            for directory, test_files in directories.items():
                row = self.report_for_dir(
                    root=root, directory=directory, test_files=test_files
                )
                page.write(row)

            page.write(tests_table_foot)
            page.write("</body></html>")
