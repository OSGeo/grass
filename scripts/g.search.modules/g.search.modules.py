#!/usr/bin/env python3
############################################################################
#
# MODULE:	g.search.modules
# AUTHOR(S):	Jachym Cepicky <jachym.cepicky gmail.com>
# PURPOSE:	g.search.modules in grass modules using keywords
# COPYRIGHT:	(C) 2015-2019 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Search in GRASS modules using keywords
#% keyword: general
#% keyword: modules
#% keyword: search
#%end
#%option
#% key: keyword
#% multiple: yes
#% type: string
#% label: Keyword to be searched
#% description: List all modules if not given
#% required : no
#%end
#%flag
#% key: a
#% description: Display only modules where all keywords are available (AND), default: OR
#% guisection: Output
#%end
#%flag
#% key: n
#% description: Invert selection (logical NOT)
#% guisection: Output
#%end
#%flag
#% key: m
#% description: Search in manual pages too (can be slow)
#% guisection: Output
#%end
#%flag
#% key: k
#% label: Search only for the exact keyword in module keyword list
#% description: Instead of full text search, search only in actual keywords
#% guisection: Output
#%end
#%flag
#% key: c
#% description: Use colorized (more readable) output to terminal
#% guisection: Output
#%end
#%flag
#% key: g
#% description: Shell script format
#% guisection: Output
#%end
#%flag
#% key: j
#% description: JSON format
#% guisection: Output
#%end

from __future__ import print_function
import os
import sys

from grass.script.utils import diff_files, try_rmdir
from grass.script import core as grass
from grass.exceptions import CalledModuleError

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree  # Python <= 2.4

COLORIZE = False


def main():
    global COLORIZE
    AND = flags['a']
    NOT = flags['n']
    manpages = flags['m']
    exact_keywords = flags['k']
    out_format = None
    if flags['g']:
        out_format = 'shell'
    elif flags['j']:
        out_format = 'json'
    else:
        COLORIZE = flags['c']

    keywords = None
    if options['keyword']:
        if exact_keywords:
            keywords = options['keyword'].split(',')
        else:
            keywords = options['keyword'].lower().split(',')
    else:
        NOT = None

    modules = _search_module(keywords, AND, NOT, manpages, exact_keywords)

    print_results(modules, out_format)


def print_results(data, out_format=None):
    """
    Print result of the searching method

    each data item should have

    {
        'name': name of the item,
        'attributes': {
            # list of attributes to be shown too
        }
    }

    :param list.<dict> data: input list of found data items
    :param str out_format: output format 'shell', 'json', None
    """

    if not out_format:
        _print_results(data)

    elif out_format == 'shell':
        _print_results_shell(data)

    elif out_format == 'json':
        _print_results_json(data)


def _print_results_shell(data):
    """Print just the name attribute"""

    for item in data:
        print(item['name'])


def _print_results_json(data):
    """Print JSON output"""

    import json
    print(json.dumps(data, sort_keys=True, indent=4, separators=(',', ': ')))


def _print_results(data):

    import textwrap

    for item in data:
        print('\n{0}'.format(colorize(item['name'], attrs=['bold'])))
        for attr in item['attributes']:
            out = '{0}: {1}'.format(attr, item['attributes'][attr])
            out = textwrap.wrap(out, width=79, initial_indent=4 * ' ',
                                subsequent_indent=4 * ' ' + len(attr) * ' ' + '  ')
            for line in out:
                print(line)


def colorize(text, attrs=None, pattern=None):
    """Colorize given text input

    :param string text: input text to be colored
    :param list.<string> attrs: list of attributes as defined in termcolor package
    :param string pattern: text to be highlighted in input text
    :return: colored string
    """

    if COLORIZE:
        try:
            from termcolor import colored
        except ImportError:
            grass.fatal(_("Cannot colorize, python-termcolor is not installed"))
    else:
        def colored(pattern, attrs):
            return pattern

    if pattern:
        return text.replace(pattern, colored(pattern, attrs=attrs))
    else:
        return colored(text, attrs=attrs)


def _search_module(keywords=None, logical_and=False, invert=False, manpages=False,
                   exact_keywords=False):
    """Search modules by given keywords

    :param list.<str> keywords: list of keywords
    :param boolean logical_and: use AND (default OR)
    :param boolean manpages: search in manpages too
    :return dict: modules
    """

    WXGUIDIR = os.path.join(os.getenv("GISBASE"), "gui", "wxpython")
    filename = os.path.join(WXGUIDIR, 'xml', 'module_items.xml')
    menudata_file = open(filename, 'r')

    menudata = etree.parse(menudata_file)
    menudata_file.close()

    items = menudata.findall('module-item')

    # add installed addons to modules list
    if os.getenv("GRASS_ADDON_BASE"):
        filename_addons = os.path.join(os.getenv("GRASS_ADDON_BASE"), 'modules.xml')
        if os.path.isfile(filename_addons):
            addon_menudata_file = open(filename_addons, 'r')
            addon_menudata = etree.parse(addon_menudata_file)
            addon_menudata_file.close()
            addon_items = addon_menudata.findall('task')
            items.extend(addon_items)

    # add system-wide installed addons to modules list
    filename_addons_s = os.path.join(os.getenv("GISBASE"), 'modules.xml')
    if os.path.isfile(filename_addons_s):
        addon_menudata_file_s = open(filename_addons_s, 'r')
        addon_menudata_s = etree.parse(addon_menudata_file_s)
        addon_menudata_file_s.close()
        addon_items_s = addon_menudata_s.findall('task')
        items.extend(addon_items_s)

    found_modules = []
    for item in items:
        name = item.attrib['name']
        description = item.find('description').text
        module_keywords = item.find('keywords').text

        if not keywords:
            # list all modules
            found = [True]
        else:
            found = [False]
            if logical_and:
                found = [False] * len(keywords)

            for idx in range(len(keywords)):
                keyword = keywords[idx]
                keyword_found = False

                if exact_keywords:
                    keyword_found = _exact_search(keyword, module_keywords)
                else:
                    keyword_found = _basic_search(keyword, name, description,
                                                  module_keywords)

                # meta-modules (i.sentinel, r.modis, ...) do not have descriptions
                # and keywords, but they have a manpage
                # TODO change the handling of meta-modules
                if (description and module_keywords) and not keyword_found and manpages:
                    keyword_found = _manpage_search(keyword, name)

                if keyword_found:
                    if logical_and:
                        found[idx] = True
                    else:
                        found = [True]

                    description = colorize(description,
                                           attrs=['underline'],
                                           pattern=keyword)
                    module_keywords = colorize(module_keywords,
                                               attrs=['underline'],
                                               pattern=keyword)

        add = False not in found
        if invert:
            add = not add
        if add:
            found_modules.append({
                'name': name,
                'attributes': {
                    'keywords': module_keywords,
                    'description': description
                }
            })

    return sorted(found_modules, key=lambda k: k['name'])


def _basic_search(pattern, name, description, module_keywords):
    """Search for a string in all the provided strings.

    This lowercases the strings before searching in them, so the pattern
    string should be lowercased too.
    """
    if (name and description and module_keywords):
        if name.lower().find(pattern) > -1 or\
           description.lower().find(pattern) > -1 or\
           module_keywords.lower().find(pattern) > -1:
            return True
        else:
            return False
    else:
        return False


def _exact_search(keyword, module_keywords):
    """Compare exact keyword with module keywords

    :param keyword: exact keyword to find in the list (not lowercased)
    :param module_keywords: comma separated list of keywords
    """
    module_keywords = module_keywords.split(',')
    for current in module_keywords:
        if keyword == current:
            return True
    return False


def _manpage_search(pattern, name):
    try:
        manpage = grass.read_command('g.manual', flags='m', entry=name)
    except CalledModuleError:
        # in case man page is missing
        return False

    return manpage.lower().find(pattern) > -1

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
