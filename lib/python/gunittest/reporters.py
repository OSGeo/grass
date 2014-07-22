# -*- coding: utf-8 -*-
"""!@package grass.gunittest.reporters

@brief GRASS Python testing framework module for report generation

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

@author Vaclav Petras
"""


import os
import datetime
import xml.sax.saxutils as saxutils
import xml.etree.ElementTree as et
import subprocess

from .utils import ensure_dir


def get_source_url(path, revision, line=None):
    """

    :param path: directory or file path relative to remote repository root
    :param revision: SVN revision (should be a number)
    :param line: line in the file (should be None for directories)
    """
    tracurl = 'http://trac.osgeo.org/grass/browser/'
    if line:
        return '{tracurl}{path}?rev={revision}#L{line}'.format(**locals())
    else:
        return '{tracurl}{path}?rev={revision}'.format(**locals())


def html_escape(text):
    """Escape ``'&'``, ``'<'``, and ``'>'`` in a string of data."""
    return saxutils.escape(text)


def html_unescape(text):
    """Unescape ``'&amp;'``, ``'&lt;'``, and ``'&gt;'`` in a string of data."""
    return saxutils.unescape(text)


def color_error_line(line):
    if line.startswith('ERROR: '):
        # TODO: use CSS class
        # ignoring the issue with \n at the end, HTML don't mind
        line = '<span style="color: red">' + line + "</span>"
    if line.startswith('FAIL: '):
        # TODO: use CSS class
        # ignoring the issue with \n at the end, HTML don't mind
        line = '<span style="color: red">' + line + "</span>"
    if line.startswith('WARNING: '):
        # TODO: use CSS class
        # ignoring the issue with \n at the end, HTML don't mind
        line = '<span style="color: blue">' + line + "</span>"
    #if line.startswith('Traceback ('):
    #    line = '<span style="color: red">' + line + "</span>"
    return line


def get_svn_revision():
    """Get SVN revision number

    :returns: SVN revision number as string or None if it is
        not possible to get
    """
    # TODO: here should be starting directory
    # but now we are using current as starting
    p = subprocess.Popen(['svnversion', '.'],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    rc = p.poll()
    if not rc:
        stdout = stdout.strip()
        if stdout.endswith('M'):
            stdout = stdout[:-1]
        if ':' in stdout:
            # the first one is the one of source code
            stdout = stdout.split(':')[0]
        return stdout
    else:
        return None


def get_svn_info():
    """Get important information from ``svn info``

    :returns: SVN info as dictionary or None
        if it is not possible to obtain it
    """
    try:
        # TODO: introduce directory, not only current
        p = subprocess.Popen(['svn', 'info', '.', '--xml'],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        rc = p.poll()
        info = {}
        if not rc:
            root = et.fromstring(stdout)
            # TODO: get also date if this make sense
            # expecting only one <entry> element
            entry = root.find('entry')
            info['revision'] = entry.get('revision')
            info['url'] = entry.find('url').text
            relurl = entry.find('relative-url')
            # element which is not found is None
            # empty element would be bool(el) == False
            if relurl is not None:
                relurl = relurl.text
                # relative path has ^ at the beginning in SVN version 1.8.8
                if relurl.startswith('^'):
                    relurl = relurl[1:]
            else:
                # SVN version 1.8.8 supports relative-url but older do not
                # so, get relative part from absolute URL
                const_url_part = 'https://svn.osgeo.org/grass/'
                relurl = info['url'][len(const_url_part):]
            info['relative-url'] = relurl
            return info
    # TODO: add this to svnversion function
    except OSError as e:
        import errno
        # ignore No such file or directory
        if e.errno != errno.ENOENT:
            raise
    return None


class GrassTestFilesMultiReporter(object):

    def __init__(self, reporters, forgiving=False):
        self.reporters = reporters
        self.forgiving = forgiving

    def start(self, results_dir):
        # TODO: no directory cleaning (self.clean_before)? now cleaned by caller
        # TODO: perhaps only those whoe need it should do it (even multiple times)
        # and there is also the delet problem
        ensure_dir(os.path.abspath(results_dir))
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

    def end_file_test(self, module, cwd, returncode, stdout, stderr):
        for reporter in self.reporters:
            try:
                reporter.end_file_test(module, cwd, returncode, stdout, stderr)
            except AttributeError:
                if self.forgiving:
                    pass
                else:
                    raise


class GrassTestFilesCountingReporter(object):
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
        self.file_pass_per = 100 * float(self.files_pass) / self.test_files
        self.file_fail_per = 100 * float(self.files_fail) / self.test_files

    def start_file_test(self, module):
        self.file_start_time = datetime.datetime.now()
        self._start_file_test_called = True
        self.test_files += 1

    def end_file_test(self, module, cwd, returncode, stdout, stderr):
        assert self._start_file_test_called
        self.file_end_time = datetime.datetime.now()
        self.file_time = self.file_end_time - self.file_start_time
        if returncode:
            self.files_fail += 1
        else:
            self.files_pass += 1
        self._start_file_test_called = False


class GrassTestFilesHtmlReporter(GrassTestFilesCountingReporter):

    def __init__(self):
        super(GrassTestFilesHtmlReporter, self).__init__()
        self.main_index = None

    def start(self, results_dir):
        super(GrassTestFilesHtmlReporter, self).start(results_dir)
        # having all variables public although not really part of API
        self.main_index = open(os.path.join(results_dir, 'index.html'), 'w')

        svn_info = get_svn_info()
        if not svn_info:
            svn_text = ('<span style="font-size: 60%">'
                        'SVN revision cannot be be obtained'
                        '</span>')
        else:
            url = get_source_url(path=svn_info['relative-url'],
                                 revision=svn_info['revision'])
            svn_text = ('SVN revision'
                        ' <a href="{url}">'
                        '{rev}</a>'
                        ).format(url=url, rev=svn_info['revision'])
        self.main_index.write('<html><body>'
                              '<h1>Test results</h1>'
                              '{time:%Y-%m-%d %H:%M:%S}'
                              ' ({svn})'
                              '<table>'
                              '<thead><tr>'
                              '<th>Tested directory</th>'
                              '<th>Test file</th>'
                              '<th>Status</th>'
                              '</tr></thead><tbody>'.format(
                                  time=self.main_start_time,
                                  svn=svn_text))

    def finish(self):
        super(GrassTestFilesHtmlReporter, self).finish()

        tfoot = ('<tfoot>'
                 '<tr>'
                 '<td>Summary</td>'
                 '<td>{nfiles} test files</td>'
                 '<td>{nsper:.2f}% successful</td>'
                 '</tr>'
                 '</tfoot>'.format(nfiles=self.test_files,
                                   nsper=self.file_pass_per))

        summary_sentence = ('Executed {nfiles} test files in {time:}.'
                            ' From them'
                            ' {nsfiles} files ({nsper:.2f}%) were successful'
                            ' and {nffiles} files ({nfper:.2f}%) failed.'
                            .format(
                                nfiles=self.test_files,
                                time=self.main_time,
                                nsfiles=self.files_pass,
                                nffiles=self.files_fail,
                                nsper=self.file_pass_per,
                                nfper=self.file_fail_per))

        self.main_index.write('<tbody>{tfoot}</table>'
                              '<p>{summary}</p>'
                              '</body></html>'
                              .format(
                                  tfoot=tfoot,
                                  summary=summary_sentence))
        self.main_index.close()

    def start_file_test(self, module):
        super(GrassTestFilesHtmlReporter, self).start_file_test(module)
        self.main_index.flush()  # to get previous lines to the report

    def wrap_stdstream_to_html(self, infile, outfile, module, stream):
        before = '<html><body><h1>%s</h1><pre>' % (module.name + ' ' + stream)
        after = '</pre></body></html>'
        html = open(outfile, 'w')
        html.write(before)
        with open(infile) as text:
            for line in text:
                html.write(color_error_line(html_escape(line)))
        html.write(after)
        html.close()

    def returncode_to_html_text(self, returncode):
        if returncode:
            return '<span style="color: red">FAILED</span>'
        else:
            return '<span style="color: green">succeeded</span>'  # SUCCEEDED

    def returncode_to_html_sentence(self, returncode):
        if returncode:
            return ('<span style="color: red">&#x274c;</span>'
                    ' Test failed (return code %d)' % (returncode))
        else:
            return ('<span style="color: green">&#x2713;</span>'
                    ' Test succeeded (return code %d)' % (returncode))

    def end_file_test(self, module, cwd, returncode, stdout, stderr):
        super(GrassTestFilesHtmlReporter, self).end_file_test(
            module=module, cwd=cwd, returncode=returncode,
            stdout=stdout, stderr=stderr)
        self.main_index.write(
            '<tr><td>{d}</td>'
            '<td><a href="{d}/{m}/index.html">{m}</a></td><td>{sf}</td>'
            '<tr>'.format(
                d=module.tested_dir, m=module.name,
                sf=self.returncode_to_html_text(returncode)))
        self.wrap_stdstream_to_html(infile=stdout,
                                    outfile=os.path.join(cwd, 'stdout.html'),
                                    module=module, stream='stdout')
        self.wrap_stdstream_to_html(infile=stderr,
                                    outfile=os.path.join(cwd, 'stderr.html'),
                                    module=module, stream='stderr')
        file_index_path = os.path.join(cwd, 'index.html')
        file_index = open(file_index_path, 'w')
        file_index.write(
            '<html><body>'
            '<h1>{m.name}</h1>'
            '<h2>{m.tested_dir} &ndash; {m.name}</h2>'
            '<p>{status}'
            '<p>Test duration: {dur}'
            '<ul>'
            '<li><a href="stdout.html">standard output (stdout)</a>'
            '<li><a href="stderr.html">standard error output (stderr)</a>'
            '<li><a href="testcodecoverage/index.html">code coverage</a>'
            '</ul>'
            '</body></html>'
            .format(
                dur=self.file_time, m=module,
                status=self.returncode_to_html_sentence(returncode)))
        file_index.close()

        if returncode:
            pass
            # TODO: here we don't have oportunity to write error file
            # to stream (stdout/stderr)
            # a stream can be added and if not none, we could write


class GrassTestFilesTextReporter(GrassTestFilesCountingReporter):

    def __init__(self, stream):
        super(GrassTestFilesTextReporter, self).__init__()
        self._stream = stream

    def start(self, results_dir):
        super(GrassTestFilesTextReporter, self).start(results_dir)

    def finish(self):
        super(GrassTestFilesTextReporter, self).finish()

        summary_sentence = ('\nExecuted {nfiles} test files in {time:}.'
                            '\nFrom them'
                            ' {nsfiles} files ({nsper:.2f}%) were successful'
                            ' and {nffiles} files ({nfper:.2f}%) failed.\n'
                            .format(
                                nfiles=self.test_files,
                                time=self.main_time,
                                nsfiles=self.files_pass,
                                nffiles=self.files_fail,
                                nsper=self.file_pass_per,
                                nfper=self.file_fail_per))
        self._stream.write(summary_sentence)

    def start_file_test(self, module):
        super(GrassTestFilesTextReporter, self).start_file_test(module)
        self._stream.flush()  # to get previous lines to the report

    def end_file_test(self, module, cwd, returncode, stdout, stderr):
        super(GrassTestFilesTextReporter, self).end_file_test(
            module=module, cwd=cwd, returncode=returncode,
            stdout=stdout, stderr=stderr)

        if returncode:
            self._stream.write(
                '{m} from {d} failed\n'
                .format(
                    d=module.tested_dir,
                    m=module.name))
            # TODO: here we lost the possibility to include also file name
            # of the appropriate report
