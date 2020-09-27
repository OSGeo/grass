# -*- coding: utf-8 -*-
"""Testing framework module for multi report

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
"""


import sys
import os
import argparse
import itertools
import datetime
import operator
from collections import defaultdict, namedtuple

from grass.gunittest.checkers import text_to_keyvalue
from grass.gunittest.utils import ensure_dir
from grass.gunittest.reporters import success_to_html_percent

# TODO: we should be able to work without matplotlib
import matplotlib
matplotlib.use('Agg')
# This counts as code already, so silence "import not at top of file".
# Perhaps in the future, switch_backend() could be used.
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.dates import date2num  # noqa: E402

class TestResultSummary(object):
    def __init__(self):
        self.timestamp = None
        self.svn_revision = None
        self.location = None
        self.location_type = None

        self.total = None
        self.successes = None
        self.failures = None
        self.errors = None

        self.skipped = []
        self.expected_failures = []
        self.unexpected_successes = []

        self.files_total = None
        self.files_successes = None
        self.files_failures = None

        self.tested_modules = []
        self.tested_dirs = []
        self.test_files_authors = []
        self.tested_dirs = []
        self.time = []
        self.names = []

        self.report = None


def plot_percents(x, xticks, xlabels, successes, failures, filename, style):
    fig = plt.figure()
    graph = fig.add_subplot(111)

    # Plot the data as a red line with round markers
    graph.plot(x, successes, color=style.success_color,
               linestyle=style.linestyle, linewidth=style.linewidth)
    graph.plot(x, failures, color=style.fail_color,
               linestyle=style.linestyle, linewidth=style.linewidth)

    fig.autofmt_xdate()
    graph.set_xticks(xticks)
    graph.set_xticklabels(xlabels)

    percents = range(0, 110, 10)
    graph.set_yticks(percents)
    graph.set_yticklabels(['%d%%' % p for p in percents])

    fig.savefig(filename)


def plot_percent_successful(x, xticks, xlabels, successes, filename, style):
    fig = plt.figure()
    graph = fig.add_subplot(111)
   
    def median(values):
        n = len(values)
        if n == 1:
            return values[0]
        sorted_values = sorted(values)
        if n % 2 == 0:
            return (sorted_values[n / 2 - 1] + sorted_values[n / 2]) / 2
        else:
            return sorted_values[n / 2]
    
    # this is useful for debugging or some other stat
    # cmeans = []
    # cmedians = []
    # csum = 0
    # count = 0
    # for i, s in enumerate(successes):
    #     csum += s
    #     count += 1
    #     cmeans.append(csum/count)
    #     cmedians.append(median(successes[:i + 1]))

    smedian = median(successes)
    smax = max(successes)
    if successes[-1] < smedian:
        color = 'r'
    else:
        color = 'g'
    # another possibility is to color according to the gradient, ideally
    # on the whole curve but that's much more complicated

    graph.plot(x, successes, color=color,
               linestyle=style.linestyle, linewidth=style.linewidth)

    # rotates the xlabels
    fig.autofmt_xdate()
    graph.set_xticks(xticks)
    graph.set_xticklabels(xlabels)

    step = 5
    ymin = int(min(successes) / step) * step
    ymax = int(smax / step) * step
    percents = range(ymin, ymax + step + 1, step)
    graph.set_yticks(percents)
    graph.set_yticklabels(['%d%%' % p for p in percents])

    fig.savefig(filename)


def tests_successful_plot(x, xticks, xlabels, results, filename, style):
    successes = []
    for result in results:
        if result.total:
            successes.append(float(result.successes) / result.total * 100)
        else:
            # this is not expected to happen
            # but we don't want any exceptions if it happens
            successes.append(0)

    plot_percent_successful(x=x, xticks=xticks, xlabels=xlabels,
                            successes=successes,
                            filename=filename, style=style)


def tests_plot(x, xticks, xlabels, results, filename, style):

    total = [result.total for result in results]
    successes = [result.successes for result in results]
    # TODO: document: counting errors and failures together
    failures = [result.failures + result.errors for result in results]

    fig = plt.figure()

    graph = fig.add_subplot(111)

    graph.plot(x, total, color=style.total_color,
               linestyle=style.linestyle, linewidth=style.linewidth)
    graph.plot(x, successes, color=style.success_color,
               linestyle=style.linestyle, linewidth=style.linewidth)
    graph.plot(x, failures, color=style.fail_color,
               linestyle=style.linestyle, linewidth=style.linewidth)

    fig.autofmt_xdate()
    graph.set_xticks(xticks)
    graph.set_xticklabels(xlabels)

    fig.savefig(filename)

def tests_percent_plot(x, xticks, xlabels, results, filename, style):
    successes = []
    failures = []
    for result in results:
        if result.total:
            successes.append(float(result.successes) / result.total * 100)
            # TODO: again undocumented, counting errors and failures together
            failures.append(float(result.failures + result.errors) / result.total * 100)
        else:
            # this is not expected to happen
            # but we don't want any exceptions if it happens
            successes.append(0)
            failures.append(0)

    plot_percents(x=x, xticks=xticks, xlabels=xlabels,
                  successes=successes, failures=failures,
                  filename=filename, style=style)


def files_successful_plot(x, xticks, xlabels, results, filename, style):
    successes = []
    for result in results:
        if result.total:
            successes.append(float(result.files_successes) / result.files_total * 100)
        else:
            # this is not expected to happen
            # but we don't want any exceptions if it happens
            successes.append(0)

    plot_percent_successful(x=x, xticks=xticks, xlabels=xlabels,
                            successes=successes,
                            filename=filename, style=style)


def files_plot(x, xticks, xlabels, results, filename, style):
    total = [result.files_total for result in results]
    successes = [result.files_successes for result in results]
    failures = [result.files_failures for result in results]

    fig = plt.figure()

    graph = fig.add_subplot(111)

    graph.plot(x, total, color=style.total_color,
               linestyle=style.linestyle, linewidth=style.linewidth)
    graph.plot(x, successes, color=style.success_color,
               linestyle=style.linestyle, linewidth=style.linewidth)
    graph.plot(x, failures, color=style.fail_color,
               linestyle=style.linestyle, linewidth=style.linewidth)

    fig.autofmt_xdate()
    graph.set_xticks(xticks)
    graph.set_xticklabels(xlabels)

    fig.savefig(filename)


def files_percent_plot(x, xticks, xlabels, results, filename, style):
    successes = []
    failures = []
    for result in results:
        if result.files_total:
            successes.append(float(result.files_successes) / result.files_total * 100)
            failures.append(float(result.files_failures) / result.files_total * 100)
        else:
            # this is not expected to happen
            # but we don't want any exceptions if it happens
            successes.append(0)
            failures.append(0)

    plot_percents(x=x, xticks=xticks, xlabels=xlabels,
                  successes=successes, failures=failures,
                  filename=filename, style=style)


def info_plot(x, xticks, xlabels, results, filename, style):

    modules = [len(result.tested_modules) for result in results]
    names = [len(result.names) for result in results]
    authors = [len(result.test_files_authors) for result in results]
    # we want just unique directories
    dirs = [len(set(result.tested_dirs)) for result in results]

    fig = plt.figure()

    graph = fig.add_subplot(111)

    graph.plot(x, names, color='b', label="Test files",
               linestyle=style.linestyle, linewidth=style.linewidth)
    graph.plot(x, modules, color='g', label="Tested modules",
               linestyle=style.linestyle, linewidth=style.linewidth)
    # dirs == testsuites
    graph.plot(x, dirs, color='orange', label="Tested directories",
               linestyle=style.linestyle, linewidth=style.linewidth)
    graph.plot(x, authors, color='r', label="Test authors",
               linestyle=style.linestyle, linewidth=style.linewidth)

    graph.legend(loc='best', shadow=False)

    fig.autofmt_xdate()
    graph.set_xticks(xticks)
    graph.set_xticklabels(xlabels)

    fig.savefig(filename)


# TODO: solve the directory inconsitencies, implement None
def main_page(results, filename, images, captions, title='Test reports',
              directory=None):
    filename = os.path.join(directory, filename)
    with open(filename, 'w') as page:
        page.write(
            '<html><body>'
            '<h1>{title}</h1>'
            '<table>'
            '<thead><tr>'
            '<th>Date (timestamp)</th><th>SVN revision</th><th>Name</th>'
            '<th>Successful files</th><th>Successful tests</th>'
            '</tr></thead>'
            '<tbody>'
            .format(title=title)
        )
        for result in reversed(results):
            # TODO: include name to summary file
            # now using location or test report directory as name
            if result.location != 'unknown':
                name = result.location
            else:
                name = os.path.basename(result.report)
                if not name:
                    # Python basename returns '' for 'abc/'
                    for d in reversed(os.path.split(result.report)):
                        if d:
                            name = d
                            break
            per_test = success_to_html_percent(
                total=result.total, successes=result.successes)
            per_file = success_to_html_percent(
                total=result.files_total, successes=result.files_successes)
            report_path = os.path.relpath(path=result.report, start=directory)
            page.write(
                '<tr>'
                '<td><a href={report_path}/index.html>{result.timestamp}</a></td>'
                '<td>{result.svn_revision}</td>'
                '<td><a href={report_path}/index.html>{name}</a></td>'
                '<td>{pfiles}</td><td>{ptests}</td>'
                '</tr>'
                .format(result=result, name=name, report_path=report_path,
                        pfiles=per_file, ptests=per_test))
        page.write('</tbody></table>')
        for image, caption in itertools.izip(images, captions):
            page.write(
                '<h3>{caption}<h3>'
                '<img src="{image}" alt="{caption}" title="{caption}">'
                .format(image=image, caption=caption))
        page.write('</body></html>')


def main():

    parser = argparse.ArgumentParser(
        description='Create overall report from several individual test reports')
    parser.add_argument('reports', metavar='report_directory',
                        type=str, nargs='+',
                        help='Directories with reports')
    parser.add_argument('--output', dest='output', action='store',
                        default='testreports_summary',
                        help='Output directory')
    parser.add_argument('--timestamps', dest='timestamps', action='store_true',
                        help='Use file timestamp instead of date in test summary')

    args = parser.parse_args()
    output = args.output
    reports = args.reports
    use_timestamps = args.timestamps

    ensure_dir(output)

    all_results = []
    results_in_locations = defaultdict(list)

    for report in reports:
        try:
            summary_file = os.path.join(report, 'test_keyvalue_result.txt')
            if not os.path.exists(summary_file):
                sys.stderr.write('WARNING: Key-value summary not available in'
                                 ' report <%s>, skipping.\n' % summary_file)
                # skipping incomplete reports
                # use only results list for further processing
                continue
            summary = text_to_keyvalue(open(summary_file).read(), sep='=')
            if use_timestamps:
                test_timestamp = datetime.datetime.fromtimestamp(os.path.getmtime(summary_file))
            else:
                test_timestamp = datetime.datetime.strptime(summary['timestamp'], "%Y-%m-%d %H:%M:%S")

            result = TestResultSummary()
            result.timestamp = test_timestamp
            result.total = summary['total']
            result.successes = summary['successes']
            result.failures = summary['failures']
            result.errors = summary['errors']

            result.files_total = summary['files_total']
            result.files_successes = summary['files_successes']
            result.files_failures = summary['files_failures']

            result.svn_revision = str(summary['svn_revision'])
            result.tested_modules = summary['tested_modules']
            result.names = summary['names']
            result.test_files_authors = summary['test_files_authors']
            result.tested_dirs = summary['tested_dirs']
            result.report = report

            # let's consider no location as valid state and use 'unknown'
            result.location = summary.get('location', 'unknown')
            result.location_type = summary.get('location_type', 'unknown')
            # grouping according to location types
            # this can cause that two actual locations tested at the same time
            # will end up together, this is not ideal but testing with
            # one location type and different actual locations is not standard
            # and although it will not break anything it will not give a nice
            # report
            results_in_locations[result.location_type].append(result)

            all_results.append(result)
            del result
        except KeyError as e:
            print('File %s does not have right values (%s)' % (report, e.message))

    locations_main_page = open(os.path.join(output, 'index.html'), 'w')
    locations_main_page.write(
        '<html><body>'
        '<h1>Test reports grouped by location type</h1>'
        '<table>'
        '<thead><tr>'
        '<th>Location</th>'
        '<th>Successful files</th><th>Successful tests</th>'
        '</tr></thead>'
        '<tbody>'
    )

    PlotStyle = namedtuple('PlotStyle',
                           ['linestyle', 'linewidth',
                           'success_color', 'fail_color', 'total_color'])
    plot_style = PlotStyle(linestyle='-', linewidth=4.0,
                           success_color='g', fail_color='r', total_color='b')

    for location_type, results in results_in_locations.items():
        results = sorted(results, key=operator.attrgetter('timestamp'))
        # TODO: document: location type must be a valid dir name
        directory = os.path.join(output, location_type)
        ensure_dir(directory)

        if location_type == 'unknown':
            title = 'Test reports'
        else:
            title = ('Test reports for &lt;{type}&gt; location type'
                     .format(type=location_type))
        
        x = [date2num(result.timestamp) for result in results]
        # the following would be an alternative but it does not work with
        # labels and automatic axis limits even after removing another date fun
        # x = [result.svn_revision for result in results]
        xlabels = [result.timestamp.strftime("%Y-%m-%d") + ' (r' + result.svn_revision + ')' for result in results]
        step = len(x) / 10
        xticks = x[step::step]
        xlabels = xlabels[step::step]
        tests_successful_plot(x=x, xticks=xticks, xlabels=xlabels, results=results,
                              filename=os.path.join(directory, 'tests_successful_plot.png'),
                              style=plot_style)
        files_successful_plot(x=x, xticks=xticks, xlabels=xlabels, results=results,
                              filename=os.path.join(directory, 'files_successful_plot.png'),
                              style=plot_style)
        tests_plot(x=x, xticks=xticks, xlabels=xlabels, results=results,
                   filename=os.path.join(directory, 'tests_plot.png'),
                   style=plot_style)
        tests_percent_plot(x=x, xticks=xticks, xlabels=xlabels, results=results,
                           filename=os.path.join(directory, 'tests_percent_plot.png'),
                           style=plot_style)
        files_plot(x=x, xticks=xticks, xlabels=xlabels, results=results,
                   filename=os.path.join(directory, 'files_plot.png'),
                   style=plot_style)
        files_percent_plot(x=x, xticks=xticks, xlabels=xlabels, results=results,
                           filename=os.path.join(directory, 'files_percent_plot.png'),
                           style=plot_style)
        info_plot(x=x, xticks=xticks, xlabels=xlabels, results=results,
                  filename=os.path.join(directory, 'info_plot.png'),
                   style=plot_style)

        main_page(results=results, filename='index.html',
                  images=['tests_successful_plot.png',
                          'files_successful_plot.png',
                          'tests_plot.png',
                          'files_plot.png',
                          'tests_percent_plot.png',
                          'files_percent_plot.png',
                          'info_plot.png'],
                  captions=['Success of individual tests in percents',
                            'Success of test files in percents',
                            'Successes, failures and number of individual tests',
                            'Successes, failures and number of test files',
                            'Successes and failures of individual tests in percent',
                            'Successes and failures of test files in percents',
                            'Additional information'],
                  directory=directory,
                  title=title)

        files_successes = sum(result.files_successes for result in results)
        files_total = sum(result.files_total for result in results)
        successes = sum(result.successes for result in results)
        total = sum(result.total for result in results)
        per_test = success_to_html_percent(
            total=total, successes=successes)
        per_file = success_to_html_percent(
            total=files_total, successes=files_successes)
        locations_main_page.write(
            '<tr>'
            '<td><a href={location}/index.html>{location}</a></td>'
            '<td>{pfiles}</td><td>{ptests}</td>'
            '</tr>'
            .format(location=location_type,
                    pfiles=per_file, ptests=per_test))
    locations_main_page.write('</tbody></table>')
    locations_main_page.write('</body></html>')
    locations_main_page.close()
    return 0

if __name__ == '__main__':
    sys.exit(main())
