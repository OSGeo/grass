# -*- coding: utf-8 -*-
"""
Created on Fri Jun 26 19:10:58 2015

@author: pietro
"""
from __future__ import print_function
import argparse
import sys

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen

from build_html import *

def parse_options(lines, startswith='Opt'):
    def split_in_groups(lines):
        def count_par_diff(line):
            open_par = line.count('(')
            close_par = line.count(')')
            return open_par - close_par
        res = None
        diff = 0
        for line in lines:
            if line.startswith('case'):
                optname = line.split()[1][:-1]
                res = []
#                if optname == 'G_OPT_R_BASENAME_INPUT':
#                    import ipdb; ipdb.set_trace()
            elif line == 'break;':
                diff = 0
                yield optname, res
            elif line.startswith('G_'):
                diff = count_par_diff(line)
            elif diff > 0:
                diff -= count_par_diff(line)
            else:
                res.append(line) if res is not None else None

    def split_opt_line(line):
        index = line.index('=')
        key = line[:index].strip()
        default = line[index + 1:].strip()
        if default.startswith('_('):
            default = default[2:]
        return key, default

    def parse_glines(glines):
        res = {}
        key = None
        for line in glines:
            if line.startswith('/*'):
                continue
            if line.startswith(startswith) and line.endswith(';'):
                key, default = [w.strip() for w in split_opt_line(line[5:])]
                res[key] = default
            elif line.startswith(startswith):
                key, default = split_opt_line(line[5:])
                res[key] = [default, ]
            else:
                if key is not None:
                    if key not in res:
                        res[key] = []
                    start, end = 0, -1
                    if line.startswith('_('):
                        start = 2
                    if line.endswith(');'):
                        end = -3
                    elif line.endswith(';'):
                        end = -2
                    res[key].append(line[start:end])
        # pprint(glines)
        # pprint(res)
        return res

    def clean_value(val):
        if isinstance(val, list):
            val = ' '.join(val)
        return (val.replace('"', '').replace("\'", "'").strip().strip(';')
                ).strip().strip('_(').strip().strip(')').strip()

    # with open(optionfile, mode='r') as optfile:
    lines = [line.strip() for line in lines]
    result = []
    for optname, glines in split_in_groups(lines):
        if glines:
            res = parse_glines(glines)
            if res:
                for key, val in res.items():
                    res[key] = clean_value(val)
                result.append((optname, res))
    return result


class OptTable(object):
    """"""
    def __init__(self, list_of_dict):
        self.options = list_of_dict
        self.columns = sorted(set([key for _, d in self.options
                                   for key in d.keys()]))

    def csv(self, delimiter=';', endline='\n'):
        """Return a CSV string with the options"""
        csv = []
        csv.append(delimiter.join(self.columns))
        for optname, options in self.options:
            opts = [options.get(col, '') for col in self.columns]
            csv.append(delimiter.join([optname, ] + opts))
        return endline.join(csv)

    def html(self, endline='\n', indent='  ', toptions='border=1'):
        """Return a HTML table with the options"""
        html = ['<table{0}>'.format(' ' + toptions if toptions else '')]
        # write headers
        html.append(indent + "<thead>")
        html.append(indent + "<tr>")
        html.append(indent * 2 + "<th>{0}</th>".format('option'))
        for col in self.columns:
            html.append(indent * 2 + "<th>{0}</th>".format(col))
        html.append(indent + "</tr>")
        html.append(indent + "</thead>")
        html.append(indent + "<tbody>")
        for optname, options in self.options:
            html.append(indent + "<tr>")
            html.append(indent * 2 + "<td>{0}</td>".format(optname))
            for col in self.columns:
                html.append(indent * 2 +
                            "<td>{0}</td>".format(options.get(col, '')))
            html.append(indent + "</tr>")
        html.append(indent + "</tbody>")
        html.append("</table>")
        return endline.join(html)

    def _repr_html_(self):
        """Method used by IPython notebook"""
        return self.html()


if __name__ == "__main__":
    URL = ('https://trac.osgeo.org/grass/browser/grass/'
           'trunk/lib/gis/parser_standard_options.c?format=txt')
    parser = argparse.ArgumentParser(description='Extract GRASS default '
                                                 'options from link.')
    parser.add_argument('-f', '--format', default='html', dest='format',
                        choices=['html', 'csv', 'grass'],
                        help='Define the output format')
    parser.add_argument('-l', '--link', default=URL, dest='url', type=str,
                        help='Provide the url with the file to parse')
    parser.add_argument('-t', '--text', dest='text',
                        type=argparse.FileType('r'),
                        help='Provide the file to parse')
    parser.add_argument('-o', '--output', default=sys.stdout, dest='output',
                        type=argparse.FileType('w'),
                        help='Provide the url with the file to parse')
    parser.add_argument('-s', '--starts-with', default='Opt',
                        dest='startswith', type=str,
                        help='Extract only the options that starts with this')
    parser.add_argument('-p', '--html_params', default='border=1', type=str,
                        dest='htmlparmas', help="Options for the HTML table")
    args = parser.parse_args()

    cfile = args.text if args.text else urlopen(args.url, proxies=None)

    options = OptTable(parse_options(cfile.readlines(),
                                     startswith=args.startswith))
    outform = args.format
    if outform in ['csv', 'html']:
        print(getattr(options, outform)(), file=args.output)
        args.output.close()
    else:
        year = os.getenv("VERSION_DATE")
        name = args.output.name
        args.output.close()
        topicsfile = open(name, 'w')
        topicsfile.write(header1_tmpl.substitute(title="GRASS GIS " \
                        "%s Reference Manual: Parser standard options index" % grass_version))
        topicsfile.write(headerpso_tmpl)
        topicsfile.write(options.html(toptions=args.htmlparmas))
        write_html_footer(topicsfile, "index.html", year)
        topicsfile.close()
