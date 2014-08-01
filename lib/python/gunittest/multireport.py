# -*- coding: utf-8 -*-

import sys
import os
import argparse
import itertools
import datetime
import operator
from collections import defaultdict


# TODO: we should be able to work without matplotlib
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.dates import date2num

from grass.gunittest.checkers import text_to_keyvalue
from grass.gunittest.utils import ensure_dir
from grass.gunittest.reporters import success_to_html_percent


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
        self.time = []
        self.names = []

        self.report = None


def tests_plot(x, xlabels, results, filename):

    total = [result.total for result in results]
    successes = [result.successes for result in results]
    # TODO: document: counting errors and failures together
    failures = [result.failures + result.errors for result in results]

    fig = plt.figure()

    graph = fig.add_subplot(111)

    # Plot the data as a red line with round markers
    graph.plot(x, total, 'b-o')
    graph.plot(x, successes, 'g-o')
    graph.plot(x, failures, 'r-o')
    fig.autofmt_xdate()

    # Set the xtick locations to correspond to just the dates you entered.
    graph.set_xticks(x)

    # Set the xtick labels to correspond to just the dates you entered.
    graph.set_xticklabels(xlabels)

    fig.savefig(filename)


def files_plot(x, xlabels, results, filename):
    total = [result.files_total for result in results]
    successes = [result.files_successes for result in results]
    failures = [result.files_failures for result in results]

    fig = plt.figure()

    graph = fig.add_subplot(111)

    # Plot the data as a red line with round markers
    graph.plot(x, total, 'b-o')
    graph.plot(x, successes, 'g-o')
    graph.plot(x, failures, 'r-o')
    #fig.autofmt_xdate(bottom=0.2, rotation=30, ha='left')
    fig.autofmt_xdate()

    # Set the xtick locations to correspond to just the dates you entered.
    graph.set_xticks(x)

    # Set the xtick labels to correspond to just the dates you entered.
    graph.set_xticklabels(xlabels)

    fig.savefig(filename)


def info_plot(x, xlabels, results, filename):

    modules = [len(result.tested_modules) for result in results]
    names = [len(result.names) for result in results]
    authors = [len(result.test_files_authors) for result in results]

    fig = plt.figure()

    graph = fig.add_subplot(111)

    # Plot the data as a red line with round markers
    graph.plot(x, names, 'b-o', label="Test files")
    graph.plot(x, modules, 'g-o', label="Tested modules")
    graph.plot(x, authors, 'r-o', label="Test authors")
    fig.autofmt_xdate()

    # Now add the legend with some customizations.
    graph.legend(loc='upper center', shadow=True)

    # Set the xtick locations to correspond to just the dates you entered.
    graph.set_xticks(x)

    # Set the xtick labels to correspond to just the dates you entered.
    graph.set_xticklabels(xlabels)

    fig.savefig(filename)


# TODO: solve the directory inconsitencies, implemement None
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
        for result in results:
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
                                 ' report <%s>, skipping.\n')
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
            print 'File %s does not have right values (%s)' % (report, e.message) 

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

    for location_type, results in results_in_locations.iteritems():
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
        xlabels = [result.timestamp.strftime("%Y-%m-%d") + ' (r' + result.svn_revision + ')' for result in results]
        tests_plot(x=x, xlabels=xlabels, results=results,
                   filename=os.path.join(directory, 'tests_plot.png'))
        files_plot(x=x, xlabels=xlabels, results=results,
                   filename=os.path.join(directory, 'files_plot.png'))
        info_plot(x=x, xlabels=xlabels, results=results,
                  filename=os.path.join(directory, 'info_plot.png'))

        main_page(results=results, filename='index.html',
                  images=['tests_plot.png', 'files_plot.png', 'info_plot.png'],
                  captions=['Success of individual tests', 'Success of test files',
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
