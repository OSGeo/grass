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
import sys
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

    :returns: SVN revision number as string or None if it is not possible to get
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


class GrassTestFilesReporter(object):

    def __init__(self, results_dir):
        # TODO: no directory cleaning (self.clean_before)? now cleaned by caller
        ensure_dir(os.path.abspath(results_dir))

        # having all variables public although not really part of API
        self.main_index = open(os.path.join(results_dir, 'index.html'), 'w')
        # this might be moved to some report start method
        self.main_start_time = datetime.datetime.now()
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
        self.file_start_time = None
        self._start_file_test_called = False
        self.test_files = 0
        self.files_failed = 0
        self.files_succeeded = 0

    def finish(self):
        main_end_time = datetime.datetime.now()
        main_time = main_end_time - self.main_start_time
        assert self.test_files == self.files_failed + self.files_succeeded

        file_success_per = 100 * float(self.files_succeeded) / self.test_files
        file_fail_per = 100 * float(self.files_failed) / self.test_files
        tfoot = ('<tfoot>'
                 '<tr>'
                 '<td>Summary</td>'
                 '<td>{nfiles} test files</td>'
                 '<td>{nsper:.2f}% successful</td>'
                 '</tr>'
                 '</tfoot>'.format(nfiles=self.test_files,
                                   nsper=file_success_per))

        summary_sentence = ('Executed {nfiles} test files in {time:}.'
                            ' From them'
                            ' {nsfiles} files ({nsper:.2f}%) were successful'
                            ' and {nffiles} files ({nfper:.2f}%) failed.'
                            .format(
                                nfiles=self.test_files,
                                time=main_time,
                                nsfiles=self.files_succeeded,
                                nffiles=self.files_failed,
                                nsper=file_success_per,
                                nfper=file_fail_per))

        self.main_index.write('<tbody>{tfoot}</table>'
                              '<p>{summary}</p>'
                              '</body></html>'
                              .format(
                                  tfoot=tfoot,
                                  summary=summary_sentence))
        self.main_index.close()

    def start_file_test(self, module):
        self.file_start_time = datetime.datetime.now()
        self._start_file_test_called = True
        self.main_index.flush()  # to get previous ones to the report
        self.test_files += 1

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
            return '<span style="color: red">&#x274c;</span> Test failed (return code %d)' % (returncode)
        else:
            return '<span style="color: green">&#x2713;</span> Test succeeded (return code %d)' % (returncode)

    def end_file_test(self, module, cwd, returncode, stdout, stderr):
        assert self._start_file_test_called
        file_time = datetime.datetime.now() - self.file_start_time
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
        file_index.write('<html><body>'
                         '<h1>{m.name}</h1>'
                         '<h2>{m.tested_dir} &ndash; {m.name}</h2>'
                         '<p>{status}'
                         '<p>Test duration: {dur}'
                         '<ul>'
                         '<li><a href="stdout.html">standard output (stdout)</a>'
                         '<li><a href="stderr.html">standard error output (stderr)</a>'
                         '<li><a href="testcodecoverage/index.html">code coverage</a>'
                         '</ul>'
                         '</body></html>'.format(
                             dur=file_time, m=module,
                             status=self.returncode_to_html_sentence(returncode)))
        file_index.close()

        if returncode:
            sys.stderr.write('{d}/{m} failed (see {f})\n'.format(d=module.tested_dir,
                                                                 m=module.name,
                                                                 f=file_index_path))
            self.files_failed += 1
        else:
            self.files_succeeded += 1

        self._start_file_test_called = False
