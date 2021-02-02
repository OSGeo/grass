#!/usr/bin/env python3

############################################################################
#
# MODULE:       g.extension
# AUTHOR(S):    Markus Neteler (original shell script)
#               Martin Landa <landa.martin gmail com> (Pythonized & upgraded for GRASS 7)
#               Vaclav Petras <wenzeslaus gmail com> (support for general sources)
# PURPOSE:      Tool to download and install extensions into local installation
#
# COPYRIGHT:    (C) 2009-2019 by Markus Neteler, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
# TODO:         - add sudo support where needed (i.e. check first permission to write into
#                 $GISBASE directory)
#               - fix toolbox support in install_private_extension_xml()
#############################################################################

#%module
#% label: Maintains GRASS Addons extensions in local GRASS installation.
#% description: Downloads and installs extensions from GRASS Addons repository or other source into the local GRASS installation or removes installed extensions.
#% keyword: general
#% keyword: installation
#% keyword: extensions
#% keyword: addons
#% keyword: download
#%end

#%option
#% key: extension
#% type: string
#% key_desc: name
#% label: Name of extension to install or remove
#% description: Name of toolbox (set of extensions) when -t flag is given
#% required: yes
#%end
#%option
#% key: operation
#% type: string
#% description: Operation to be performed
#% required: yes
#% options: add,remove
#% answer: add
#%end
#%option
#% key: url
#% type: string
#% key_desc: url
#% label: URL or directory to get the extension from (supported only on Linux and Mac)
#% description: The official repository is used by default. User can specify a ZIP file, directory or a repository on common hosting services. If not identified, Subversion repository is assumed. See manual for all options.
#%end
#%option
#% key: prefix
#% type: string
#% key_desc: path
#% description: Prefix where to install extension (ignored when flag -s is given)
#% answer: $GRASS_ADDON_BASE
#% required: no
#%end
#%option
#% key: proxy
#% type: string
#% key_desc: proxy
#% description: Set the proxy with: "http=<value>,ftp=<value>"
#% required: no
#% multiple: yes
#%end
#%option
#% key: branch
#% type: string
#% key_desc: branch
#% description: Specific branch to fetch addon from (only used when fetching from git)
#% required: no
#% multiple: no
#% answer: main
#%end

#%flag
#% key: l
#% description: List available extensions in the official GRASS GIS Addons repository
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: c
#% description: List available extensions in the official GRASS GIS Addons repository including module description
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: g
#% description: List available extensions in the official GRASS GIS Addons repository (shell script style)
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: a
#% description: List locally installed extensions
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: s
#% description: Install system-wide (may need system administrator rights)
#% guisection: Install
#%end
#%flag
#% key: d
#% description: Download source code and exit
#% guisection: Install
#%end
#%flag
#% key: i
#% description: Do not install new extension, just compile it
#% guisection: Install
#%end
#%flag
#% key: f
#% description: Force removal when uninstalling extension (operation=remove)
#% guisection: Remove
#%end
#%flag
#% key: t
#% description: Operate on toolboxes instead of single modules (experimental)
#% suppress_required: yes
#%end
#%flag
#% key: o
#% description: url refers to a fork of the official extension repository
#%end
#%flag
#% key: j
#% description: Generates JSON file containing the download URLs of the official Addons
#% guisection: Install
#% suppress_required: yes
#%end


#%rules
#% required: extension, -l, -c, -g, -a, -j
#% exclusive: extension, -l, -c, -g
#% exclusive: extension, -l, -c, -a
#% requires: -o, url
#% requires: branch, url
#%end

# TODO: solve addon-extension(-module) confusion


from __future__ import print_function
import fileinput
import http
import os
import sys
import re
import atexit
import shutil
import zipfile
import tempfile
import json
import xml.etree.ElementTree as etree
from distutils.dir_util import copy_tree

from six.moves.urllib import request as urlrequest
from six.moves.urllib.error import HTTPError, URLError

# Get the XML parsing exceptions to catch. The behavior changed with Python 2.7
# and ElementTree 1.3.
from xml.parsers import expat  # TODO: works for any Python?
if hasattr(etree, 'ParseError'):
    ETREE_EXCEPTIONS = (etree.ParseError, expat.ExpatError)
else:
    ETREE_EXCEPTIONS = (expat.ExpatError)

import grass.script as gscript
from grass.script.utils import try_rmdir
from grass.script import core as grass
from grass.script import task as gtask

# temp dir
REMOVE_TMPDIR = True
PROXIES = {}
HEADERS = {
    "User-Agent": "Mozilla/5.0",
}
HTTP_STATUS_CODES = list(http.HTTPStatus)


def urlretrieve(url, filename, *args, **kwargs):
    """Same function as 'urlretrieve', but with the ability to
    define headers.
    """
    request = urlrequest.Request(url, headers=HEADERS)
    response = urlrequest.urlopen(request, *args, **kwargs)
    with open(filename, 'wb') as f:
        f.write(response.read())


def urlopen(url, *args, **kwargs):
    """Wrapper around urlopen. Same function as 'urlopen', but with the
    ability to define headers.
    """
    request = urlrequest.Request(url, headers=HEADERS)
    return urlrequest.urlopen(request, *args, **kwargs)


def download_addons_paths_file(
        url, response_format, *args, **kwargs):
    """Generates JSON file containing the download URLs of the official
    Addons

    :param str url: url address
    :param str response_format: content type

    :return response: urllib.request.urlopen response object or None
    """
    try:
        response = urlopen(url, *args, **kwargs)

        if not response.code == 200:
            index = HTTP_STATUS_CODES.index(response.code)
            desc = HTTP_STATUS_CODES[index].description
            gscript.fatal(
                _(
                    "Download file from <{url}>, "
                    "return status code {code}, "
                    "{desc}".format(
                        url=url,
                        code=response.code,
                        desc=desc,
                    ),
                ),
            )
        if response_format not in response.getheader('Content-Type'):
            gscript.fatal(
                _(
                    "Wrong downloaded file format. "
                    "Check url <{url}>. Allowed file format is "
                    "{response_format}.".format(
                        url=url,
                        response_format=response_format,
                    ),
                ),
            )
        return response

    except HTTPError as err:
        if (
                err.code == 403 and
                err.msg == 'rate limit exceeded'
        ):
            gscript.warning(
                _(
                    "The download of the json file with add-ons paths "
                    "from the github server wasn't successful, "
                    "{}. The previous downloaded json file "
                    " will be used if exists.".format(err.msg)
                ),
            )
        else:
            return download_addons_paths_file(
                url=url.replace('main', 'master'),
                response_format=response_format,
            )
    except URLError:
        gscript.fatal(
            _(
                "Download file from <{url}>, "
                "failed. Check internet connection.".format(
                    url=url,
                ),
            ),
        )


def etree_fromfile(filename):
    """Create XML element tree from a given file name"""
    with open(filename, 'r') as file_:
        return etree.fromstring(file_.read())


def etree_fromurl(url):
    """Create XML element tree from a given URL"""
    file_ = urlopen(url)
    return etree.fromstring(file_.read())


def check_progs():
    """Check if the necessary programs are available"""
    # git to be tested once supported instead of `svn`
    for prog in ('make', 'gcc', 'svn'):
        if not grass.find_program(prog, '--help'):
            grass.fatal(_("'%s' required. Please install '%s' first.")
                        % (prog, prog))

# expand prefix to class name


def expand_module_class_name(class_letters):
    """Convert module class (family) letter or letters to class (family) name

    The letter or letters are used in module names, e.g. r.slope.aspect.
    The names are used in directories in Addons but also in the source code.

    >>> expand_module_class_name('r')
    'raster'
    >>> expand_module_class_name('v')
    'vector'
    """
    name = {
        'd': 'display',
        'db': 'db',
        'g': 'general',
        'i': 'imagery',
        'm': 'misc',
        'ps': 'postscript',
        'p': 'paint',
        'r': 'raster',
        'r3': 'raster3d',
        's': 'sites',
        't': 'temporal',
        'v': 'vector',
        'wx': 'gui/wxpython'
    }

    return name.get(class_letters, class_letters)


def get_module_class_name(module_name):
    """Return class (family) name for a module

    The names are used in directories in Addons but also in the source code.

    >>> get_module_class_name('r.slope.aspect')
    'raster'
    >>> get_module_class_name('v.to.rast')
    'vector'
    """
    classchar = module_name.split('.', 1)[0]
    return expand_module_class_name(classchar)


def get_installed_extensions(force=False):
    """Get list of installed extensions or toolboxes (if -t is set)"""
    if flags['t']:
        return get_installed_toolboxes(force)

    # TODO: extension != module
    return get_installed_modules(force)


def list_installed_extensions(toolboxes=False):
    """List installed extensions"""
    elist = get_installed_extensions()
    if elist:
        if toolboxes:
            grass.message(_("List of installed extensions (toolboxes):"))
        else:
            grass.message(_("List of installed extensions (modules):"))
        sys.stdout.write('\n'.join(elist))
        sys.stdout.write('\n')
    else:
        if toolboxes:
            grass.info(_("No extension (toolbox) installed"))
        else:
            grass.info(_("No extension (module) installed"))


def get_installed_toolboxes(force=False):
    """Get list of installed toolboxes

    Writes toolboxes file if it does not exist.
    Creates a new toolboxes file if it is not possible
    to read the current one.
    """
    xml_file = os.path.join(options['prefix'], 'toolboxes.xml')
    if not os.path.exists(xml_file):
        write_xml_toolboxes(xml_file)
    # read XML file
    try:
        tree = etree_fromfile(xml_file)
    except ETREE_EXCEPTIONS + (OSError, IOError):
        os.remove(xml_file)
        write_xml_toolboxes(xml_file)
        return []
    ret = list()
    for tnode in tree.findall('toolbox'):
        ret.append(tnode.get('code'))
    return ret


def get_installed_modules(force=False):
    """Get list of installed modules.

    Writes modules file if it does not exist and *force* is set to ``True``.
    Creates a new modules file if it is not possible
    to read the current one.
    """
    xml_file = os.path.join(options['prefix'], 'modules.xml')
    if not os.path.exists(xml_file):
        if force:
            write_xml_modules(xml_file)
        else:
            grass.debug("No addons metadata file available", 1)
        return []
    # read XML file
    try:
        tree = etree_fromfile(xml_file)
    except ETREE_EXCEPTIONS + (OSError, IOError):
        os.remove(xml_file)
        write_xml_modules(xml_file)
        return []
    ret = list()
    for tnode in tree.findall('task'):
        if flags['g']:
            desc, keyw = get_optional_params(tnode)
            ret.append('name={0}'.format(tnode.get('name').strip()))
            ret.append('description={0}'.format(desc))
            ret.append('keywords={0}'.format(keyw))
            ret.append('executables={0}'.format(','.join(
                get_module_executables(tnode))
            ))
        else:
            ret.append(tnode.get('name').strip())

    return ret

# list extensions (read XML file from grass.osgeo.org/addons)


def list_available_extensions(url):
    """List available extensions/modules or toolboxes (if -t is given)

    For toolboxes it lists also all modules.
    """
    gscript.debug("list_available_extensions(url={0})".format(url))
    if flags['t']:
        grass.message(_("List of available extensions (toolboxes):"))
        tlist = get_available_toolboxes(url)
        tkeys = sorted(tlist.keys())
        for toolbox_code in tkeys:
            toolbox_data = tlist[toolbox_code]
            if flags['g']:
                print('toolbox_name=' + toolbox_data['name'])
                print('toolbox_code=' + toolbox_code)
            else:
                print('%s (%s)' % (toolbox_data['name'], toolbox_code))
            if flags['c'] or flags['g']:
                list_available_modules(url, toolbox_data['modules'])
            else:
                if toolbox_data['modules']:
                    print(os.linesep.join(['* ' + x for x in toolbox_data['modules']]))
    else:
        grass.message(_("List of available extensions (modules):"))
        # TODO: extensions with several modules + lib
        list_available_modules(url)


def get_available_toolboxes(url):
    """Return toolboxes available in the repository"""
    tdict = dict()
    url = url + "toolboxes.xml"
    try:
        tree = etree_fromurl(url)
        for tnode in tree.findall('toolbox'):
            mlist = list()
            clist = list()
            tdict[tnode.get('code')] = {'name': tnode.get('name'),
                                        'correlate': clist,
                                        'modules': mlist}

            for cnode in tnode.findall('correlate'):
                clist.append(cnode.get('name'))

            for mnode in tnode.findall('task'):
                mlist.append(mnode.get('name'))
    except (HTTPError, IOError, OSError):
        grass.fatal(_("Unable to fetch addons metadata file"))

    return tdict


def get_toolbox_extensions(url, name):
    """Get extensions inside a toolbox in toolbox file at given URL

    :param url: URL of the directory (file name will be attached)
    :param name: toolbox name
    """
    # dictionary of extensions
    edict = dict()

    url = url + "toolboxes.xml"

    try:
        tree = etree_fromurl(url)
        for tnode in tree.findall('toolbox'):
            if name == tnode.get('code'):
                for enode in tnode.findall('task'):
                    # extension name
                    ename = enode.get('name')
                    edict[ename] = dict()
                    # list of modules installed by this extension
                    edict[ename]['mlist'] = list()
                    # list of files installed by this extension
                    edict[ename]['flist'] = list()
                break
    except (HTTPError, IOError, OSError):
        grass.fatal(_("Unable to fetch addons metadata file"))

    return edict


def get_module_files(mnode):
    """Return list of module files

    :param mnode: XML node for a module
    """
    flist = []
    if mnode.find('binary') is None:
        return flist
    for file_node in mnode.find('binary').findall('file'):
        filepath = file_node.text
        flist.append(filepath)

    return flist


def get_module_executables(mnode):
    """Return list of module executables

    :param mnode: XML node for a module
    """
    flist = []
    for filepath in get_module_files(mnode):
        if filepath.startswith(options['prefix'] + os.path.sep + 'bin') or \
           (sys.platform != 'win32' and
                filepath.startswith(options['prefix'] + os.path.sep + 'scripts')):
            filename = os.path.basename(filepath)
            if sys.platform == 'win32':
                filename = os.path.splitext(filename)[0]
            flist.append(filename)

    return flist


def get_optional_params(mnode):
    """Return description and keywords of a module as a tuple

    :param mnode: XML node for a module
    """
    try:
        desc = mnode.find('description').text
    except AttributeError:
        desc = ''
    if desc is None:
        desc = ''
    try:
        keyw = mnode.find('keywords').text
    except AttributeError:
        keyw = ''
    if keyw is None:
        keyw = ''

    return desc, keyw


def list_available_modules(url, mlist=None):
    """List modules available in the repository

    Tries to use XML metadata file first. Fallbacks to HTML page with a list.

    :param url: URL of the directory (file name will be attached)
    :param mlist: list only modules in this list
    """
    file_url = url + "modules.xml"
    grass.debug("url=%s" % file_url, 1)
    try:
        tree = etree_fromurl(file_url)
    except ETREE_EXCEPTIONS:
        grass.warning(_("Unable to parse '%s'. Trying to scan"
                        " SVN repository (may take some time)...") % file_url)
        list_available_extensions_svn(url)
        return
    except (HTTPError, URLError, IOError, OSError):
        list_available_extensions_svn(url)
        return

    for mnode in tree.findall('task'):
        name = mnode.get('name').strip()
        if mlist and name not in mlist:
            continue
        if flags['c'] or flags['g']:
            desc, keyw = get_optional_params(mnode)

        if flags['g']:
            print('name=' + name)
            print('description=' + desc)
            print('keywords=' + keyw)
        elif flags['c']:
            if mlist:
                print('*', end='')
            print(name + ' - ' + desc)
        else:
            print(name)


# TODO: this is now broken/dead code, SVN is basically not used
# fallback for Trac should parse Trac HTML page
# this might be useful for potential SVN repos or anything
# which would list the extensions/addons as list
# TODO: fail when nothing is accessible
def list_available_extensions_svn(url):
    """List available extensions from HTML given by URL

    Filename is generated based on the module class/family.
    This works well for the structure which is in grass-addons repository.

    ``<li><a href=...`` is parsed to find module names.
    This works well for HTML page generated by Subversion.

    :param url: a directory URL (filename will be attached)
    """
    gscript.debug("list_available_extensions_svn(url=%s)" % url, 2)
    grass.message(_('Fetching list of extensions from'
                    ' GRASS-Addons SVN repository (be patient)...'))
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)

    if flags['c']:
        grass.warning(
            _("Flag 'c' ignored, addons metadata file not available"))
    if flags['g']:
        grass.warning(
            _("Flag 'g' ignored, addons metadata file not available"))

    prefixes = ['d', 'db', 'g', 'i', 'm', 'ps',
                'p', 'r', 'r3', 's', 't', 'v']
    for prefix in prefixes:
        modclass = expand_module_class_name(prefix)
        grass.verbose(_("Checking for '%s' modules...") % modclass)

        # construct a full URL of a file
        file_url = '%s/%s' % (url, modclass)
        grass.debug("url = %s" % file_url, debug=2)
        try:
            file_ = urlopen(url)
        except (HTTPError, IOError, OSError):
            grass.debug(_("Unable to fetch '%s'") % file_url, debug=1)
            continue

        for line in file_.readlines():
            # list extensions
            sline = pattern.search(line)
            if not sline:
                continue
            name = sline.group(2).rstrip('/')
            if name.split('.', 1)[0] == prefix:
                print(name)

    # get_wxgui_extensions(url)


# TODO: this is a dead code, not clear why not used, but seems not needed
def get_wxgui_extensions(url):
    """Return list of extensions/addons in wxGUI directory at given URL

    :param url: a directory URL (filename will be attached)
    """
    mlist = list()
    grass.debug('Fetching list of wxGUI extensions from '
                'GRASS-Addons SVN repository (be patient)...')
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)
    grass.verbose(_("Checking for '%s' modules...") % 'gui/wxpython')

    # construct a full URL of a file
    url = '%s/%s' % (url, 'gui/wxpython')
    grass.debug("url = %s" % url, debug=2)
    file_ = urlopen(url)
    if not file_:
        grass.warning(_("Unable to fetch '%s'") % url)
        return

    for line in file.readlines():
        # list extensions
        sline = pattern.search(line)
        if not sline:
            continue
        name = sline.group(2).rstrip('/')
        if name not in ('..', 'Makefile'):
            mlist.append(name)

    return mlist


def cleanup():
    """Cleanup after the downloads and copilation"""
    if REMOVE_TMPDIR:
        try_rmdir(TMPDIR)
    else:
        grass.message("\n%s\n" % _("Path to the source code:"))
        sys.stderr.write('%s\n' % os.path.join(TMPDIR, options['extension']))


def write_xml_modules(name, tree=None):
    """Write element tree as a modules matadata file

    If the *tree* is not given, an empty file is created.

    :param name: file name
    :param tree: XML element tree
    """
    file_ = open(name, 'w')
    file_.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    file_.write('<!DOCTYPE task SYSTEM "grass-addons.dtd">\n')
    file_.write('<addons version="%s">\n' % version[0])

    libgis_revison = grass.version()['libgis_revision']
    if tree is not None:
        for tnode in tree.findall('task'):
            indent = 4
            file_.write('%s<task name="%s">\n' %
                        (' ' * indent, tnode.get('name')))
            indent += 4
            file_.write('%s<description>%s</description>\n' %
                        (' ' * indent, tnode.find('description').text))
            file_.write('%s<keywords>%s</keywords>\n' %
                        (' ' * indent, tnode.find('keywords').text))
            bnode = tnode.find('binary')
            if bnode is not None:
                file_.write('%s<binary>\n' % (' ' * indent))
                indent += 4
                for fnode in bnode.findall('file'):
                    file_.write('%s<file>%s</file>\n' %
                                (' ' * indent, os.path.join(options['prefix'],
                                                            fnode.text)))
                indent -= 4
                file_.write('%s</binary>\n' % (' ' * indent))
            file_.write('%s<libgis revision="%s" />\n' %
                        (' ' * indent, libgis_revison))
            indent -= 4
            file_.write('%s</task>\n' % (' ' * indent))

    file_.write('</addons>\n')
    file_.close()


def write_xml_extensions(name, tree=None):
    """Write element tree as a modules matadata file

    If the *tree* is not given, an empty file is created.

    :param name: file name
    :param tree: XML element tree
    """
    file_ = open(name, 'w')
    file_.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    file_.write('<!DOCTYPE task SYSTEM "grass-addons.dtd">\n')
    file_.write('<addons version="%s">\n' % version[0])

    libgis_revison = grass.version()['libgis_revision']
    if tree is not None:
        for tnode in tree.findall('task'):
            indent = 4
            # extension name
            file_.write('%s<task name="%s">\n' %
                        (' ' * indent, tnode.get('name')))
            indent += 4
            """
            file_.write('%s<description>%s</description>\n' %
                        (' ' * indent, tnode.find('description').text))
            file_.write('%s<keywords>%s</keywords>\n' %
                        (' ' * indent, tnode.find('keywords').text))
            """
            # extension files
            bnode = tnode.find('binary')
            if bnode is not None:
                file_.write('%s<binary>\n' % (' ' * indent))
                indent += 4
                for fnode in bnode.findall('file'):
                    file_.write('%s<file>%s</file>\n' %
                                (' ' * indent, os.path.join(options['prefix'],
                                                            fnode.text)))
                indent -= 4
                file_.write('%s</binary>\n' % (' ' * indent))
            # extension modules
            mnode = tnode.find('modules')
            if mnode is not None:
                file_.write('%s<modules>\n' % (' ' * indent))
                indent += 4
                for fnode in mnode.findall('module'):
                    file_.write('%s<module>%s</module>\n' %
                                (' ' * indent, fnode.text))
                indent -= 4
                file_.write('%s</modules>\n' % (' ' * indent))

            file_.write('%s<libgis revision="%s" />\n' %
                        (' ' * indent, libgis_revison))
            indent -= 4
            file_.write('%s</task>\n' % (' ' * indent))

    file_.write('</addons>\n')
    file_.close()


def write_xml_toolboxes(name, tree=None):
    """Write element tree as a toolboxes matadata file

    If the *tree* is not given, an empty file is created.

    :param name: file name
    :param tree: XML element tree
    """
    file_ = open(name, 'w')
    file_.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    file_.write('<!DOCTYPE toolbox SYSTEM "grass-addons.dtd">\n')
    file_.write('<addons version="%s">\n' % version[0])
    if tree is not None:
        for tnode in tree.findall('toolbox'):
            indent = 4
            file_.write('%s<toolbox name="%s" code="%s">\n' %
                        (' ' * indent, tnode.get('name'), tnode.get('code')))
            indent += 4
            for cnode in tnode.findall('correlate'):
                file_.write('%s<correlate code="%s" />\n' %
                            (' ' * indent, tnode.get('code')))
            for mnode in tnode.findall('task'):
                file_.write('%s<task name="%s" />\n' %
                            (' ' * indent, mnode.get('name')))
            indent -= 4
            file_.write('%s</toolbox>\n' % (' ' * indent))

    file_.write('</addons>\n')
    file_.close()


def install_extension(source, url, xmlurl, branch):
    """Install extension (e.g. one module) or a toolbox (list of modules)"""
    gisbase = os.getenv('GISBASE')
    if not gisbase:
        grass.fatal(_('$GISBASE not defined'))

    if options['extension'] in get_installed_extensions(force=True):
        grass.warning(_("Extension <%s> already installed. Re-installing...") %
                      options['extension'])

    # create a dictionary of extensions
    # for each extension
    #   - a list of modules installed by this extension
    #   - a list of files installed by this extension

    edict = None
    if flags['t']:
        grass.message(_("Installing toolbox <%s>...") % options['extension'])
        edict = get_toolbox_extensions(xmlurl, options['extension'])
    else:
        edict = dict()
        edict[options['extension']] = dict()
        # list of modules installed by this extension
        edict[options['extension']]['mlist'] = list()
        # list of files installed by this extension
        edict[options['extension']]['flist'] = list()
    if not edict:
        grass.warning(_("Nothing to install"))
        return

    ret = 0
    tmp_dir = None

    new_modules = list()
    for extension in edict:
        ret1 = 0
        new_modules_ext = None
        if sys.platform == "win32":
            ret1, new_modules_ext, new_files_ext = install_extension_win(extension)
        else:
            ret1, new_modules_ext, new_files_ext, tmp_dir = install_extension_std_platforms(extension,
                                                            source=source, url=url, branch=branch)
        if not flags['d'] and not flags['i']:
            edict[extension]['mlist'].extend(new_modules_ext)
            edict[extension]['flist'].extend(new_files_ext)
            new_modules.extend(new_modules_ext)
            ret += ret1
            if len(edict) > 1:
                print('-' * 60)

    if flags['d'] or flags['i']:
        return

    if ret != 0:
        grass.warning(_('Installation failed, sorry.'
                        ' Please check above error messages.'))
    else:
        # update extensions metadata file
        grass.message(_("Updating extensions metadata file..."))
        install_extension_xml(edict)

        # update modules metadata file
        grass.message(_("Updating extension modules metadata file..."))
        install_module_xml(new_modules)

        for module in new_modules:
            update_manual_page(module)

        grass.message(_("Installation of <%s> successfully finished") %
                      options['extension'])

    if not os.getenv('GRASS_ADDON_BASE'):
        grass.warning(_('This add-on module will not function until'
                        ' you set the GRASS_ADDON_BASE environment'
                        ' variable (see "g.manual variables")'))


def get_toolboxes_metadata(url):
    """Return metadata for all toolboxes from given URL

    :param url: URL of a modules matadata file
    :param mlist: list of modules to get metadata for
    :returns: tuple where first item is dictionary with module names as keys
        and dictionary with dest, keyw, files keys as value, the second item
        is list of 'binary' files (installation files)
    """
    data = dict()
    try:
        tree = etree_fromurl(url)
        for tnode in tree.findall('toolbox'):
            clist = list()
            for cnode in tnode.findall('correlate'):
                clist.append(cnode.get('code'))

            mlist = list()
            for mnode in tnode.findall('task'):
                mlist.append(mnode.get('name'))

            code = tnode.get('code')
            data[code] = {
                'name': tnode.get('name'),
                'correlate': clist,
                'modules': mlist,
            }
    except (HTTPError, IOError, OSError):
        grass.error(_("Unable to read addons metadata file "
                      "from the remote server"))
    return data


def install_toolbox_xml(url, name):
    """Update local toolboxes metadata file"""
    # read metadata from remote server (toolboxes)
    url = url + "toolboxes.xml"
    data = get_toolboxes_metadata(url)
    if not data:
        grass.warning(_("No addons metadata available"))
        return
    if name not in data:
        grass.warning(_("No addons metadata available for <%s>") % name)
        return

    xml_file = os.path.join(options['prefix'], 'toolboxes.xml')
    # create an empty file if not exists
    if not os.path.exists(xml_file):
        write_xml_modules(xml_file)

    # read XML file
    with open(xml_file, 'r') as xml:
        tree = etree.fromstring(xml.read())

    # update tree
    tnode = None
    for node in tree.findall('toolbox'):
        if node.get('code') == name:
            tnode = node
            break

    tdata = data[name]
    if tnode is not None:
        # update existing node
        for cnode in tnode.findall('correlate'):
            tnode.remove(cnode)
        for mnode in tnode.findall('task'):
            tnode.remove(mnode)
    else:
        # create new node for task
        tnode = etree.Element(
            'toolbox', attrib={'name': tdata['name'], 'code': name})
        tree.append(tnode)

    for cname in tdata['correlate']:
        cnode = etree.Element('correlate', attrib={'code': cname})
        tnode.append(cnode)
    for tname in tdata['modules']:
        mnode = etree.Element('task', attrib={'name': tname})
        tnode.append(mnode)

    write_xml_toolboxes(xml_file, tree)


def get_addons_metadata(url, mlist):
    """Return metadata for list of modules from given URL

    :param url: URL of a modules matadata file
    :param mlist: list of modules to get metadata for
    :returns: tuple where first item is dictionary with module names as keys
        and dictionary with dest, keyw, files keys as value, the second item
        is list of 'binary' files (installation files)
    """

    # TODO: extensions with multiple modules
    data = {}
    bin_list = []
    try:
        tree = etree_fromurl(url)
    except (HTTPError, URLError, IOError, OSError) as error:
        grass.error(_("Unable to read addons metadata file"
                      " from the remote server: {0}").format(error))
        return data, bin_list
    except ETREE_EXCEPTIONS as error:
        grass.warning(_("Unable to parse '%s': {0}").format(error) % url)
        return data, bin_list
    for mnode in tree.findall('task'):
        name = mnode.get('name')
        if name not in mlist:
            continue
        file_list = list()
        bnode = mnode.find('binary')
        windows = sys.platform == 'win32'
        if bnode is not None:
            for fnode in bnode.findall('file'):
                path = fnode.text.split('/')
                if path[0] == 'bin':
                    bin_list.append(path[-1])
                    if windows:
                        path[-1] += '.exe'
                elif path[0] == 'scripts':
                    bin_list.append(path[-1])
                    if windows:
                        path[-1] += '.py'
                file_list.append(os.path.sep.join(path))
        desc, keyw = get_optional_params(mnode)
        data[name] = {
            'desc': desc,
            'keyw': keyw,
            'files': file_list,
        }
    return data, bin_list


def install_extension_xml(edict):
    """Update XML files with metadata about installed modules and toolbox
    of an private addon

    """
    # TODO toolbox
    # if len(mlist) > 1:
    #     # read metadata from remote server (toolboxes)
    #     install_toolbox_xml(url, options['extension'])

    xml_file = os.path.join(options['prefix'], 'extensions.xml')
    # create an empty file if not exists
    if not os.path.exists(xml_file):
        write_xml_extensions(xml_file)

    # read XML file
    tree = etree_fromfile(xml_file)

    # update tree
    for name in edict:

        # so far extensions do not have description or keywords
        # only modules have
        """
        try:
            desc = gtask.parse_interface(name).description
            # mname = gtask.parse_interface(name).name
            keywords = gtask.parse_interface(name).keywords
        except Exception as e:
            grass.warning(_("No addons metadata available."
                            " Addons metadata file not updated."))
            return []
        """

        tnode = None
        for node in tree.findall('task'):
            if node.get('name') == name:
                tnode = node
                break

        if tnode is None:
            # create new node for task
            tnode = etree.Element('task', attrib={'name': name})
            """
            dnode = etree.Element('description')
            dnode.text = desc
            tnode.append(dnode)
            knode = etree.Element('keywords')
            knode.text = (',').join(keywords)
            tnode.append(knode)
            """

            # create binary
            bnode = etree.Element('binary')
            # list of all installed files for this extension
            for file_name in edict[name]['flist']:
                fnode = etree.Element('file')
                fnode.text = file_name
                bnode.append(fnode)
            tnode.append(bnode)

            # create modules
            msnode = etree.Element('modules')
            # list of all installed modules for this extension
            for module_name in edict[name]['mlist']:
                mnode = etree.Element('module')
                mnode.text = module_name
                msnode.append(mnode)
            tnode.append(msnode)
            tree.append(tnode)
        else:
            grass.verbose("Extension already listed in metadata file; metadata not updated!")
    write_xml_extensions(xml_file, tree)

    return None


def install_module_xml(mlist):
    """Update XML files with metadata about installed modules and toolbox
    of an private addon

    """

    xml_file = os.path.join(options['prefix'], 'modules.xml')
    # create an empty file if not exists
    if not os.path.exists(xml_file):
        write_xml_modules(xml_file)

    # read XML file
    tree = etree_fromfile(xml_file)

    # update tree
    for name in mlist:

        try:
            desc = gtask.parse_interface(name).description
            # mname = gtask.parse_interface(name).name
            keywords = gtask.parse_interface(name).keywords
        except Exception as e:
            grass.warning(_("No metadata available for module '%s'.")
                          % name)
            continue

        tnode = None
        for node in tree.findall('task'):
            if node.get('name') == name:
                tnode = node
                break

        if tnode is None:
            # create new node for task
            tnode = etree.Element('task', attrib={'name': name})
            dnode = etree.Element('description')
            dnode.text = desc
            tnode.append(dnode)
            knode = etree.Element('keywords')
            knode.text = (',').join(keywords)
            tnode.append(knode)

            # binary files installed with an extension are now
            # listed in extensions.xml

            """
            # create binary
            bnode = etree.Element('binary')
            list_of_binary_files = []
            for file_name in os.listdir(url):
                file_type = os.path.splitext(file_name)[-1]
                file_n = os.path.splitext(file_name)[0]
                html_path = os.path.join(options['prefix'], 'docs', 'html')
                c_path = os.path.join(options['prefix'], 'bin')
                py_path = os.path.join(options['prefix'], 'scripts')
                # html or image file
                if file_type in ['.html', '.jpg', '.png'] \
                        and file_n in os.listdir(html_path):
                    list_of_binary_files.append(os.path.join(html_path, file_name))
                # c file
                elif file_type in ['.c'] and file_name in os.listdir(c_path):
                    list_of_binary_files.append(os.path.join(c_path, file_n))
                # python file
                elif file_type in ['.py'] and file_name in os.listdir(py_path):
                    list_of_binary_files.append(os.path.join(py_path, file_n))
            # man file
            man_path = os.path.join(options['prefix'], 'docs', 'man', 'man1')
            if name + '.1' in os.listdir(man_path):
                list_of_binary_files.append(os.path.join(man_path, name + '.1'))
            # add binaries to xml file
            for binary_file_name in list_of_binary_files:
                fnode = etree.Element('file')
                fnode.text = binary_file_name
                bnode.append(fnode)
            tnode.append(bnode)
            """
            tree.append(tnode)
        else:
            grass.verbose("Extension module already listed in metadata file; metadata not updated!")
    write_xml_modules(xml_file, tree)

    return mlist


def install_extension_win(name):
    """Install extension on MS Windows"""
    grass.message(_("Downloading precompiled GRASS Addons <%s>...") %
                  options['extension'])

    # build base URL
    if build_platform == 'x86_64':
        platform = build_platform
    else:
        platform = 'x86'
    base_url = "http://wingrass.fsv.cvut.cz/" \
               "grass%(major)s%(minor)s/%(platform)s/addons/" \
               "grass-%(major)s.%(minor)s.%(patch)s" % \
               {'platform': platform,
                'major': version[0], 'minor': version[1],
                'patch': version[2]}

    # resolve ZIP URL
    source, url = resolve_source_code(url='{0}/{1}.zip'.format(base_url, name))

    # to hide non-error messages from subprocesses
    if grass.verbosity() <= 2:
        outdev = open(os.devnull, 'w')
    else:
        outdev = sys.stdout

    # download Addons ZIP file
    os.chdir(TMPDIR)  # this is just to not leave something behind
    srcdir = os.path.join(TMPDIR, name)
    download_source_code(source=source, url=url, name=name,
                         outdev=outdev, directory=srcdir, tmpdir=TMPDIR)

    # collect module names and file names
    module_list = list()
    for r, d, f in os.walk(srcdir):
        for file in f:
            if file.endswith('.py'):
                modulename = file.rsplit('.py')[0]
                module_list.append(modulename)
            if file.endswith('.exe'):
                modulename = file.rsplit('.exe')[0]
                module_list.append(modulename)
    # remove duplicates in case there are .exe wrappers for python scripts
    module_list = set(module_list)

    # change shebang from python to python3
    pyfiles = []
    for r, d, f in os.walk(srcdir):
        for file in f:
            if file.endswith('.py'):
                pyfiles.append(os.path.join(r, file))

    for filename in pyfiles:
        with fileinput.FileInput(filename, inplace=True) as file:
            for line in file:
                print(line.replace(
                    "#!/usr/bin/env python\n",
                    "#!/usr/bin/env python3\n"
                ), end='')

    # collect old files
    old_file_list = list()
    for r, d, f in os.walk(options['prefix']):
        for filename in f:
            fullname = os.path.join(r, filename)
            old_file_list.append(fullname)

    # copy Addons copy tree to destination directory
    move_extracted_files(extract_dir=srcdir, target_dir=options['prefix'],
                         files=os.listdir(srcdir))

    # collect new files
    file_list = list()
    for r, d, f in os.walk(options['prefix']):
        for filename in f:
            fullname = os.path.join(r, filename)
            if fullname not in old_file_list:
                file_list.append(fullname)

    return 0, module_list, file_list


def download_source_code_svn(url, name, outdev, directory=None):
    """Download source code from a Subversion repository

    .. note:
        Stdout is passed to to *outdev* while stderr is will be just printed.

    :param url: URL of the repository
        (module class/family and name are attached)
    :param name: module name
    :param outdev: output divide for the standard output of the svn command
    :param directory: directory where the source code will be downloaded
        (default is the current directory with name attached)

    :returns: full path to the directory with the source code
        (useful when you not specify directory, if *directory* is specified
        the return value is equal to it)
    """
    if not directory:
        directory = os.path.join(os.getcwd, name)
    classchar = name.split('.', 1)[0]
    moduleclass = expand_module_class_name(classchar)
    url = url + '/' + moduleclass + '/' + name
    if grass.call(['svn', 'checkout',
                   url, directory], stdout=outdev) != 0:
        grass.fatal(_("GRASS Addons <%s> not found") % name)
    return directory


def download_source_code_official_github(url, name, outdev, directory=None):
    """Download source code from a official GitHub repository

    .. note:
        Stdout is passed to to *outdev* while stderr is will be just printed.

    :param url: URL of the repository
        (module class/family and name are attached)
    :param name: module name
    :param outdev: output divide for the standard output of the svn command
    :param directory: directory where the source code will be downloaded
        (default is the current directory with name attached)

    :returns: full path to the directory with the source code
        (useful when you not specify directory, if *directory* is specified
        the return value is equal to it)
    """
    if not directory:
        directory = os.path.join(os.getcwd, name)
    classchar = name.split('.', 1)[0]
    moduleclass = expand_module_class_name(classchar)
    if grass.call(['svn', 'export',
                   url, directory], stdout=outdev) != 0:
        grass.fatal(_("GRASS Addons <%s> not found") % name)
    return directory


def move_extracted_files(extract_dir, target_dir, files):
    """Fix state of extracted files by moving them to different directory

    When extracting, it is not clear what will be the root directory
    or if there will be one at all. So this function moves the files to
    a different directory in the way that if there was one directory extracted,
    the contained files are moved.
    """
    gscript.debug("move_extracted_files({0})".format(locals()))
    if len(files) == 1:
        shutil.copytree(os.path.join(extract_dir, files[0]), target_dir)
    else:
        if not os.path.exists(target_dir):
            os.mkdir(target_dir)
        for file_name in files:
            actual_file = os.path.join(extract_dir, file_name)
            if os.path.isdir(actual_file):
                # shutil.copytree() replaced by copy_tree() because
                # shutil's copytree() fails when subdirectory exists
                copy_tree(actual_file,
                          os.path.join(target_dir, file_name))
            else:
                shutil.copy(actual_file, os.path.join(target_dir, file_name))


# Original copyright and license of the original version of the CRLF function
# Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
# Python Software Foundation; All Rights Reserved
# Python Software Foundation License Version 2
# http://svn.python.org/projects/python/trunk/Tools/scripts/crlf.py
def fix_newlines(directory):
    """Replace CRLF with LF in all files in the directory

    Binary files are ignored. Recurses into subdirectories.
    """
    # skip binary files
    # see https://stackoverflow.com/a/7392391
    textchars = bytearray({7, 8, 9, 10, 12, 13, 27} | set(range(0x20, 0x100)) - {0x7f})

    def is_binary_string(bytes):
        return bool(bytes.translate(None, textchars))

    for root, unused, files in os.walk(directory):
        for name in files:
            filename = os.path.join(root, name)
            if is_binary_string(open(filename, 'rb').read(1024)):
                continue  # ignore binary files

            # read content of text file
            with open(filename, 'rb') as fd:
                data = fd.read()

            # we don't expect there would be CRLF file by
            # purpose if we want to allow CRLF files we would
            # have to whitelite .py etc
            newdata = data.replace(b'\r\n', b'\n')
            if newdata != data:
                with open(filename, 'wb') as newfile:
                    newfile.write(newdata)


def extract_zip(name, directory, tmpdir):
    """Extract a ZIP file into a directory"""
    gscript.debug("extract_zip(name={name}, directory={directory},"
                  " tmpdir={tmpdir})".format(name=name, directory=directory,
                                             tmpdir=tmpdir), 3)
    try:
        zip_file = zipfile.ZipFile(name, mode='r')
        file_list = zip_file.namelist()
        # we suppose we can write to parent of the given dir
        # (supposing a tmp dir)
        extract_dir = os.path.join(tmpdir, 'extract_dir')
        os.mkdir(extract_dir)
        for subfile in file_list:
            # this should be safe in Python 2.7.4
            zip_file.extract(subfile, extract_dir)
        files = os.listdir(extract_dir)
        move_extracted_files(extract_dir=extract_dir,
                             target_dir=directory, files=files)
    except zipfile.BadZipfile as error:
        gscript.fatal(_("ZIP file is unreadable: {0}").format(error))


# TODO: solve the other related formats
def extract_tar(name, directory, tmpdir):
    """Extract a TAR or a similar file into a directory"""
    gscript.debug("extract_tar(name={name}, directory={directory},"
                  " tmpdir={tmpdir})".format(name=name, directory=directory,
                                             tmpdir=tmpdir), 3)
    try:
        import tarfile  # we don't need it anywhere else
        tar = tarfile.open(name)
        extract_dir = os.path.join(tmpdir, 'extract_dir')
        os.mkdir(extract_dir)
        tar.extractall(path=extract_dir)
        files = os.listdir(extract_dir)
        move_extracted_files(extract_dir=extract_dir,
                             target_dir=directory, files=files)
    except tarfile.TarError as error:
        gscript.fatal(_("Archive file is unreadable: {0}").format(error))

extract_tar.supported_formats = ['tar.gz', 'gz', 'bz2', 'tar', 'gzip', 'targz']


def download_source_code(source, url, name, outdev,
                         directory=None, tmpdir=None, branch=None):
    """Get source code to a local directory for compilation"""
    gscript.verbose(_("Type of source identified as '{source}'.")
                    .format(source=source))
    if source == 'official':
        gscript.message(_("Fetching <%s> from "
                          "GRASS GIS Addons repository (be patient)...") % name)
        download_source_code_official_github(url, name, outdev, directory)
    elif source == 'official_fork':
        gscript.message(_("Fetching <{name}> from "
                          "<{url}> (be patient)...").format(name=name, url=url))
        download_source_code_official_github(url, name, outdev, directory)
    elif source == 'svn':
        gscript.message(_("Fetching <{name}> from "
                          "<{url}> (be patient)...").format(name=name, url=url))
        download_source_code_svn(url, name, outdev, directory)
    elif source in ['remote_zip']:  # , 'official'
        gscript.message(_("Fetching <{name}> from "
                          "<{url}> (be patient)...").format(name=name, url=url))
        # we expect that the module.zip file is not by chance in the archive
        zip_name = os.path.join(tmpdir, 'extension.zip')
        try:
            response = urlopen(url)
        except URLError:
            # Try download add-on from 'master' branch if default "main" fails
            if branch == "main":
                try:
                    url = url.replace('main', 'master')
                    gscript.message(_("Expected default branch not found. "
                                    "Trying again from <{url}>...")
                                    .format(url=url))
                    response = urlopen(url)
                except URLError:
                    grass.fatal(_("Extension <{name}> not found. Please check "
                                  "'url' and 'branch' options".format(name=name)))
            else:
                grass.fatal(_("Extension <%s> not found") % name)

        with open(zip_name, 'wb') as out_file:
            shutil.copyfileobj(response, out_file)
        extract_zip(name=zip_name, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source.startswith('remote_') and \
            source.split('_')[1] in extract_tar.supported_formats:
        # we expect that the module.tar.gz file is not by chance in the archive
        archive_name = os.path.join(tmpdir,
                                    'extension.' + source.split('_')[1])
        urlretrieve(url, archive_name)
        extract_tar(name=archive_name, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source == 'zip':
        extract_zip(name=url, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source in extract_tar.supported_formats:
        extract_tar(name=url, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source == 'dir':
        shutil.copytree(url, directory)
        fix_newlines(directory)
    else:
        # probably programmer error
        grass.fatal(_("Unknown extension (addon) source type '{0}'."
                      " Please report this to the grass-user mailing list.")
                    .format(source))
    assert os.path.isdir(directory)


def install_extension_std_platforms(name, source, url, branch):
    """Install extension on standard platforms"""
    gisbase = os.getenv('GISBASE')
    source_url = 'https://github.com/OSGeo/grass-addons/tree/master/grass7/'

    # to hide non-error messages from subprocesses
    if grass.verbosity() <= 2:
        outdev = open(os.devnull, 'w')
    else:
        outdev = sys.stdout

    os.chdir(TMPDIR)  # this is just to not leave something behind
    srcdir = os.path.join(TMPDIR, name)
    download_source_code(source=source, url=url, name=name,
                         outdev=outdev, directory=srcdir, tmpdir=TMPDIR,
                         branch=branch)
    os.chdir(srcdir)

    pgm_not_found_message = _('Module name not found.'
                              ' Check module Makefile syntax (PGM variable).')
    # collect module names
    module_list = list()
    for r, d, f in os.walk(srcdir):
        for filename in f:
            if filename == "Makefile":
                # get the module name: PGM = <module name>
                with open(os.path.join(r, 'Makefile')) as fp:
                    for line in fp.readlines():
                        if re.match(r'PGM.*.=|PGM=', line):
                            try:
                                modulename = line.split('=')[1].strip()
                                if modulename:
                                    module_list.append(modulename)
                                else:
                                    grass.fatal(pgm_not_found_message)
                            except IndexError:
                                grass.fatal(pgm_not_found_message)

    # change shebang from python to python3
    pyfiles = []
    # r=root, d=directories, f = files
    for r, d, f in os.walk(srcdir):
        for file in f:
            if file.endswith('.py'):
                pyfiles.append(os.path.join(r, file))

    for filename in pyfiles:
        with fileinput.FileInput(filename, inplace=True) as file:
            for line in file:
                print(line.replace(
                    "#!/usr/bin/env python\n",
                    "#!/usr/bin/env python3\n"
                ), end='')

    dirs = {
        'bin': os.path.join(TMPDIR, name, 'bin'),
        'docs': os.path.join(TMPDIR, name, 'docs'),
        'html': os.path.join(TMPDIR, name, 'docs', 'html'),
        'rest': os.path.join(TMPDIR, name, 'docs', 'rest'),
        'man': os.path.join(TMPDIR, name, 'docs', 'man'),
        'script': os.path.join(TMPDIR, name, 'scripts'),
        # TODO: handle locales also for addons
        #             'string'  : os.path.join(TMPDIR, name, 'locale'),
        'string': os.path.join(TMPDIR, name),
        'etc': os.path.join(TMPDIR, name, 'etc'),
    }

    make_cmd = [
        'make',
        'MODULE_TOPDIR=%s' % gisbase.replace(' ', r'\ '),
        'RUN_GISRC=%s' % os.environ['GISRC'],
        'BIN=%s' % dirs['bin'],
        'HTMLDIR=%s' % dirs['html'],
        'RESTDIR=%s' % dirs['rest'],
        'MANBASEDIR=%s' % dirs['man'],
        'SCRIPTDIR=%s' % dirs['script'],
        'STRINGDIR=%s' % dirs['string'],
        'ETC=%s' % os.path.join(dirs['etc']),
        'SOURCE_URL=%s' % source_url
    ]

    install_cmd = [
        'make',
        'MODULE_TOPDIR=%s' % gisbase,
        'ARCH_DISTDIR=%s' % os.path.join(TMPDIR, name),
        'INST_DIR=%s' % options['prefix'],
        'install'
    ]

    if flags['d']:
        grass.message("\n%s\n" % _("To compile run:"))
        sys.stderr.write(' '.join(make_cmd) + '\n')
        grass.message("\n%s\n" % _("To install run:"))
        sys.stderr.write(' '.join(install_cmd) + '\n')
        return 0, None, None, None

    os.chdir(os.path.join(TMPDIR, name))

    grass.message(_("Compiling..."))
    if not os.path.exists(os.path.join(gisbase, 'include',
                                       'Make', 'Module.make')):
        grass.fatal(_("Please install GRASS development package"))

    if 0 != grass.call(make_cmd,
                       stdout=outdev):
        grass.fatal(_('Compilation failed, sorry.'
                      ' Please check above error messages.'))

    if flags['i']:
        return 0, None, None, None

    # collect old files
    old_file_list = list()
    for r, d, f in os.walk(options['prefix']):
        for filename in f:
            fullname = os.path.join(r, filename)
            old_file_list.append(fullname)

    grass.message(_("Installing..."))
    ret = grass.call(install_cmd, stdout=outdev)

    # collect new files
    file_list = list()
    for r, d, f in os.walk(options['prefix']):
        for filename in f:
            fullname = os.path.join(r, filename)
            if fullname not in old_file_list:
                file_list.append(fullname)

    return ret, module_list, file_list, os.path.join(TMPDIR, name)


def remove_extension(force=False):
    """Remove existing extension
       extension or toolbox with extensions if -t is given)"""
    if flags['t']:
        edict = get_toolbox_extensions(options['prefix'], options['extension'])
    else:
        edict = dict()
        edict[options['extension']] = dict()
        # list of modules installed by this extension
        edict[options['extension']]['mlist'] = list()
        # list of files installed by this extension
        edict[options['extension']]['flist'] = list()

    # collect modules and files installed by these extensions
    mlist = list()
    xml_file = os.path.join(options['prefix'], 'extensions.xml')
    if os.path.exists(xml_file):
        # read XML file
        tree = None
        try:
            tree = etree_fromfile(xml_file)
        except ETREE_EXCEPTIONS + (OSError, IOError):
            os.remove(xml_file)
            write_xml_extensions(xml_file)

        if tree is not None:
            for tnode in tree.findall('task'):
                ename = tnode.get('name').strip()
                if ename in edict:
                    # modules installed by this extension
                    mnode = tnode.find('modules')
                    if mnode:
                        for fnode in mnode.findall('module'):
                            mname = fnode.text.strip()
                            edict[ename]['mlist'].append(mname)
                            mlist.append(mname)
                    # files installed by this extension
                    bnode = tnode.find('binary')
                    if bnode:
                        for fnode in bnode.findall('file'):
                            bname = fnode.text.strip()
                            edict[ename]['flist'].append(bname)
    else:
        if force:
            write_xml_extensions(xml_file)

        xml_file = os.path.join(options['prefix'], 'modules.xml')
        if not os.path.exists(xml_file):
            if force:
                write_xml_modules(xml_file)
            else:
                grass.debug("No addons metadata file available", 1)

        # read XML file
        tree = None
        try:
            tree = etree_fromfile(xml_file)
        except ETREE_EXCEPTIONS + (OSError, IOError):
            os.remove(xml_file)
            write_xml_modules(xml_file)
            return []

        if tree is not None:
            for tnode in tree.findall('task'):
                ename = tnode.get('name').strip()
                if ename in edict:
                    # assume extension name == module name
                    edict[ename]['mlist'].append(ename)
                    mlist.append(ename)
                    # files installed by this extension
                    bnode = tnode.find('binary')
                    if bnode:
                        for fnode in bnode.findall('file'):
                            bname = fnode.text.strip()
                            edict[ename]['flist'].append(bname)

    if force:
        grass.verbose(_("List of removed files:"))
    else:
        grass.info(_("Files to be removed:"))

    eremoved = remove_extension_files(edict, force)

    if force:
        if len(eremoved) > 0:
            grass.message(_("Updating addons metadata file..."))
            remove_extension_xml(mlist, edict)
            for ename in edict:
                if ename in eremoved:
                    grass.message(_("Extension <%s> successfully uninstalled.") %
                                  ename)
    else:
        if flags['t']:
            grass.warning(_("Toolbox <%s> not removed. "
                            "Re-run '%s' with '-f' flag to force removal")
                          % (options['extension'], 'g.extension'))
        else:
            grass.warning(_("Extension <%s> not removed. "
                            "Re-run '%s' with '-f' flag to force removal")
                          % (options['extension'], 'g.extension'))

# remove existing extension(s) (reading XML file)


def remove_extension_files(edict, force=False):
    """Remove extensions specified in a dictionary

    Uses the file names from the file list of the dictionary
    Fallbacks to standard layout of files on prefix path on error.
    """
    # try to read XML metadata file first
    xml_file = os.path.join(options['prefix'], 'extensions.xml')

    einstalled = list()
    eremoved = list()

    if os.path.exists(xml_file):
        tree = etree_fromfile(xml_file)
        if tree is not None:
            for task in tree.findall('task'):
                ename = task.get('name').strip()
                einstalled.append(ename)
    else:
        tree = None

    for name in edict:
        removed = True
        if len(edict[name]['flist']) > 0:
            err = list()
            for fpath in edict[name]['flist']:
                grass.verbose(fpath)
                if force:
                    try:
                        os.remove(fpath)
                    except OSError:
                        msg = "Unable to remove file '%s'"
                        err.append((_(msg) % fpath))
                        removed = False
            if len(err) > 0:
                for error_line in err:
                    grass.error(error_line)
        else:
            if name not in einstalled:
                # try even if module does not seem to be available,
                # as the user may be trying to get rid of left over cruft
                grass.warning(_("Extension <%s> not found") % name)

            remove_extension_std(name, force)
            removed = False

        if removed is True:
            eremoved.append(name)

    return eremoved


def remove_extension_std(name, force=False):
    """Remove extension/module expecting the standard layout

    Any images for manuals or files installed in etc will not be
    removed
    """
    for fpath in [os.path.join(options['prefix'], 'bin', name),
                  os.path.join(options['prefix'], 'scripts', name),
                  os.path.join(
                      options['prefix'], 'docs', 'html', name + '.html'),
                  os.path.join(
                      options['prefix'], 'docs', 'rest', name + '.txt'),
                  os.path.join(options['prefix'], 'docs', 'man', 'man1',
                               name + '.1')]:
        if os.path.isfile(fpath):
            grass.verbose(fpath)
            if force:
                os.remove(fpath)

    # remove module libraries under GRASS_ADDONS/etc/{name}/*
    libpath = os.path.join(options['prefix'], 'etc', name)
    if os.path.isdir(libpath):
        grass.verbose(libpath)
        if force:
            shutil.rmtree(libpath)


def remove_from_toolbox_xml(name):
    """Update local meta-file when removing existing toolbox"""
    xml_file = os.path.join(options['prefix'], 'toolboxes.xml')
    if not os.path.exists(xml_file):
        return
    # read XML file
    tree = etree_fromfile(xml_file)
    for node in tree.findall('toolbox'):
        if node.get('code') != name:
            continue
        tree.remove(node)

    write_xml_toolboxes(xml_file, tree)


def remove_extension_xml(mlist, edict):
    """Update local meta-file when removing existing extension"""
    if len(edict) > 1:
        # update also toolboxes metadata
        remove_from_toolbox_xml(options['extension'])

    # modules
    xml_file = os.path.join(options['prefix'], 'modules.xml')
    if os.path.exists(xml_file):
        # read XML file
        tree = etree_fromfile(xml_file)
        for name in mlist:
            for node in tree.findall('task'):
                if node.get('name') != name:
                    continue
                tree.remove(node)
        write_xml_modules(xml_file, tree)

    # extensions
    xml_file = os.path.join(options['prefix'], 'extensions.xml')
    if os.path.exists(xml_file):
        # read XML file
        tree = etree_fromfile(xml_file)
        for name in edict:
            for node in tree.findall('task'):
                if node.get('name') != name:
                    continue
                tree.remove(node)
        write_xml_extensions(xml_file, tree)

# check links in CSS


def check_style_file(name):
    """Ensures that a specified HTML documentation support file exists

    If the file, e.g. a CSS file does not exist, the file is copied from
    the distribution.

    If the files are missing, a warning is issued.
    """
    dist_file = os.path.join(os.getenv('GISBASE'), 'docs', 'html', name)
    addons_file = os.path.join(options['prefix'], 'docs', 'html', name)

    if os.path.isfile(addons_file):
        return

    try:
        shutil.copyfile(dist_file, addons_file)
    except OSError as error:
        grass.warning(
            _("Unable to create '{filename}': {error}."
              " Is the GRASS GIS documentation package installed?"
              " Installation continues,"
              " but documentation may not look right.").format(
              filename=addons_file, error=error))


def create_dir(path):
    """Creates the specified directory (with all dirs in between)

    NOOP for existing directory.
    """
    if os.path.isdir(path):
        return

    try:
        os.makedirs(path)
    except OSError as error:
        grass.fatal(_("Unable to create '%s': %s") % (path, error))

    grass.debug("'%s' created" % path)


def check_dirs():
    """Ensure that the necessary directories in prefix path exist"""
    create_dir(os.path.join(options['prefix'], 'bin'))
    create_dir(os.path.join(options['prefix'], 'docs', 'html'))
    create_dir(os.path.join(options['prefix'], 'docs', 'rest'))
    check_style_file('grass_logo.png')
    check_style_file('grassdocs.css')
    create_dir(os.path.join(options['prefix'], 'etc'))
    create_dir(os.path.join(options['prefix'], 'docs', 'man', 'man1'))
    create_dir(os.path.join(options['prefix'], 'scripts'))

# fix file URI in manual page


def update_manual_page(module):
    """Fix manual page for addons which are at different directory
       than core modules"""
    if module.split('.', 1)[0] == 'wx':
        return  # skip for GUI modules

    grass.verbose(_("Manual page for <%s> updated") % module)
    # read original html file
    htmlfile = os.path.join(
        options['prefix'], 'docs', 'html', module + '.html')
    try:
        oldfile = open(htmlfile)
        shtml = oldfile.read()
    except IOError as error:
        gscript.fatal(_("Unable to read manual page: %s") % error)
    else:
        oldfile.close()

    pos = []

    # fix logo URL
    pattern = r'''<a href="([^"]+)"><img src="grass_logo.png"'''
    for match in re.finditer(pattern, shtml):
        pos.append(match.start(1))

    # find URIs
    pattern = r'''<a href="([^"]+)">([^>]+)</a>'''
    addons = get_installed_extensions(force=True)
    for match in re.finditer(pattern, shtml):
        if match.group(1)[:4] == 'http':
            continue
        if match.group(1).replace('.html', '') in addons:
            continue
        pos.append(match.start(1))

    if not pos:
        return  # no match

    # replace file URIs
    prefix = 'file://' + '/'.join([os.getenv('GISBASE'), 'docs', 'html'])
    ohtml = shtml[:pos[0]]
    for i in range(1, len(pos)):
        ohtml += prefix + '/' + shtml[pos[i - 1]:pos[i]]
    ohtml += prefix + '/' + shtml[pos[-1]:]

    # write updated html file
    try:
        newfile = open(htmlfile, 'w')
        newfile.write(ohtml)
    except IOError as error:
        gscript.fatal(_("Unable for write manual page: %s") % error)
    else:
        newfile.close()


def resolve_install_prefix(path, to_system):
    """Determine and check the path for installation"""
    if to_system:
        path = os.environ['GISBASE']
    if path == '$GRASS_ADDON_BASE':
        if not os.getenv('GRASS_ADDON_BASE'):
            grass.warning(_("GRASS_ADDON_BASE is not defined, "
                            "installing to ~/.grass%s/addons") % version[0])
            path = os.path.join(
                os.environ['HOME'], '.grass%s' % version[0], 'addons')
        else:
            path = os.environ['GRASS_ADDON_BASE']
    if os.path.exists(path) and \
       not os.access(path, os.W_OK):
        grass.fatal(_("You don't have permission to install extension to <{0}>."
                      " Try to run {1} with administrator rights"
                      " (su or sudo).")
                    .format(path, 'g.extension'))
    # ensure dir sep at the end for cases where path is used as URL and pasted
    # together with file names
    if not path.endswith(os.path.sep):
        path = path + os.path.sep
    return os.path.abspath(path)  # make likes absolute paths


def resolve_xmlurl_prefix(url, source=None):
    """Determine and check the URL where the XML metadata files are stored

    It ensures that there is a single slash at the end of URL, so we can attach
     file name easily:

    >>> resolve_xmlurl_prefix('https://grass.osgeo.org/addons')
    'https://grass.osgeo.org/addons/'
    >>> resolve_xmlurl_prefix('https://grass.osgeo.org/addons/')
    'https://grass.osgeo.org/addons/'
    """
    gscript.debug("resolve_xmlurl_prefix(url={0}, source={1})".format(url, source))
    if source == 'official':
        # use pregenerated modules XML file
        url = 'https://grass.osgeo.org/addons/grass%s/' % version[0]
    # else try to get extensions XMl from SVN repository (provided URL)
    # the exact action depends on subsequent code (somewhere)

    if not url.endswith('/'):
        url = url + '/'
    return url


KNOWN_HOST_SERVICES_INFO = {
    'OSGeo Trac': {
        'domain': 'trac.osgeo.org',
        'ignored_suffixes': ['format=zip'],
        'possible_starts': ['', 'https://', 'http://'],
        'url_start': 'https://',
        'url_end': '?format=zip',
    },
    'GitHub': {
        'domain': 'github.com',
        'ignored_suffixes': ['.zip', '.tar.gz'],
        'possible_starts': ['', 'https://', 'http://'],
        'url_start': 'https://',
        'url_end': '/archive/{branch}.zip',
    },
    'GitLab': {
        'domain': 'gitlab.com',
        'ignored_suffixes': ['.zip', '.tar.gz', '.tar.bz2', '.tar'],
        'possible_starts': ['', 'https://', 'http://'],
        'url_start': 'https://',
        'url_end': '/-/archive/{branch}/{name}-{branch}.zip',
    },
    'Bitbucket': {
        'domain': 'bitbucket.org',
        'ignored_suffixes': ['.zip', '.tar.gz', '.gz', '.bz2'],
        'possible_starts': ['', 'https://', 'http://'],
        'url_start': 'https://',
        'url_end': '/get/{branch}.zip',
    },
}

# TODO: support ZIP URLs which don't end with zip
# https://gitlab.com/user/reponame/repository/archive.zip?ref=b%C3%A9po


def resolve_known_host_service(url, name, branch):
    """Determine source type and full URL for known hosting service

    If the service is not determined from the provided URL, tuple with
    is two ``None`` values is returned.

    :param url: URL
    :param name: module name
    """
    match = None
    actual_start = None
    for key, value in KNOWN_HOST_SERVICES_INFO.items():
        for start in value['possible_starts']:
            if url.startswith(start + value['domain']):
                match = value
                actual_start = start
                gscript.verbose(_("Identified {0} as known hosting service")
                                .format(key))
                for suffix in value['ignored_suffixes']:
                    if url.endswith(suffix):
                        gscript.verbose(
                            _("Not using {service} as known hosting service"
                              " because the URL ends with '{suffix}'")
                            .format(service=key, suffix=suffix))
                        return None, None
    if match:
        if not actual_start:
            actual_start = match['url_start']
        else:
            actual_start = ''
        if 'branch' in  match['url_end']:
            suffix = match['url_end'].format(name=name, branch=branch)
        else:
            suffix = match['url_end'].format(name=name)
        url = '{prefix}{base}{suffix}'.format(prefix=actual_start,
                                              base=url.rstrip('/'),
                                              suffix=suffix)
        gscript.verbose(_("Will use the following URL for download: {0}")
                        .format(url))
        return 'remote_zip', url
    else:
        return None, None


# TODO: add also option to enforce the source type
def resolve_source_code(url=None, name=None, branch=None):
    """Return type and URL or path of the source code

    Local paths are not presented as URLs to be usable in standard functions.
    Path is identified as local path if the directory of file exists which
    has the unfortunate consequence that the not existing files are evaluated
    as remote URLs. When path is not evaluated, Subversion is assumed for
    backwards compatibility. When GitHub repository is specified, ZIP file
    link is returned. The ZIP is for {branch} branch, not the default one because
    GitHub does not provide the default branch in the URL (July 2015).

    :returns: tuple with type of source and full URL or path

    Official repository:

    >>> resolve_source_code(name='g.example') # doctest: +SKIP
    ('official', 'https://trac.osgeo.org/.../general/g.example')

    Subversion:

    >>> resolve_source_code('https://svn.osgeo.org/grass/grass-addons/grass7')
    ('svn', 'https://svn.osgeo.org/grass/grass-addons/grass7')

    ZIP files online:

    >>> resolve_source_code('https://trac.osgeo.org/.../r.modis?format=zip') # doctest: +SKIP
    ('remote_zip', 'https://trac.osgeo.org/.../r.modis?format=zip')

    Local directories and ZIP files:

    >>> resolve_source_code(os.path.expanduser("~")) # doctest: +ELLIPSIS
    ('dir', '...')
    >>> resolve_source_code('/local/directory/downloaded.zip') # doctest: +SKIP
    ('zip', '/local/directory/downloaded.zip')

    OSGeo Trac:

    >>> resolve_source_code('trac.osgeo.org/.../r.agent.aco') # doctest: +SKIP
    ('remote_zip', 'https://trac.osgeo.org/.../r.agent.aco?format=zip')
    >>> resolve_source_code('https://trac.osgeo.org/.../r.agent.aco') # doctest: +SKIP
    ('remote_zip', 'https://trac.osgeo.org/.../r.agent.aco?format=zip')

    GitHub:

    >>> resolve_source_code('github.com/user/g.example') # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')
    >>> resolve_source_code('github.com/user/g.example/') # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')
    >>> resolve_source_code('https://github.com/user/g.example') # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')
    >>> resolve_source_code('https://github.com/user/g.example/') # doctest: +SKIP
    ('remote_zip', 'https://github.com/user/g.example/archive/master.zip')

    GitLab:

    >>> resolve_source_code('gitlab.com/JoeUser/GrassModule') # doctest: +SKIP
    ('remote_zip', 'https://gitlab.com/JoeUser/GrassModule/-/archive/master/GrassModule-master.zip')
    >>> resolve_source_code('https://gitlab.com/JoeUser/GrassModule') # doctest: +SKIP
    ('remote_zip', 'https://gitlab.com/JoeUser/GrassModule/-/archive/master/GrassModule-master.zip')

    Bitbucket:

    >>> resolve_source_code('bitbucket.org/joe-user/grass-module') # doctest: +SKIP
    ('remote_zip', 'https://bitbucket.org/joe-user/grass-module/get/default.zip')
    >>> resolve_source_code('https://bitbucket.org/joe-user/grass-module') # doctest: +SKIP
    ('remote_zip', 'https://bitbucket.org/joe-user/grass-module/get/default.zip')
    """
    if not url and name:
        module_class = get_module_class_name(name)
        # note: 'trunk' is required to make URL usable for 'svn export' call
        git_url = 'https://github.com/OSGeo/grass-addons/trunk/' \
                   'grass{version}/{module_class}/{module_name}' \
                   .format(version=version[0],
                           module_class=module_class, module_name=name)
        # trac_url = 'https://trac.osgeo.org/grass/browser/grass-addons/' \
        #            'grass{version}/{module_class}/{module_name}?format=zip' \
        #            .format(version=version[0],
        #                    module_class=module_class, module_name=name)
        # return 'official', trac_url
        return 'official', git_url

    if url and flags['o']:
        module_class = get_module_class_name(name)

        # note: 'trunk' is required to make URL usable for 'svn export' call
        if branch in ['master', 'main']:
            svn_reference = 'trunk'
        else:
            svn_reference = 'branches/{}'.format(branch)

        git_url = '{url}/{branch}/' \
                   'grass{version}/{module_class}/{module_name}' \
                   .format(url=url, version=version[0],
                           module_class=module_class, module_name=name,
                           branch=svn_reference)
        # trac_url = 'https://trac.osgeo.org/grass/browser/grass-addons/' \
        #            'grass{version}/{module_class}/{module_name}?format=zip' \
        #            .format(version=version[0],
        #                    module_class=module_class, module_name=name)
        # return 'official', trac_url
        return 'official_fork', git_url

    # Check if URL can be found
    # Catch corner case if local URL is given starting with file://
    url = url[6:] if url.startswith('file://') else url
    if not os.path.exists(url):
        url_validated = False
        if url.startswith('http'):
            try:
                open_url = urlopen(url)
                open_url.close()
                url_validated = True
            except:
                pass
        else:
            try:
                open_url = urlopen('http://' + url)
                open_url.close()
                url_validated = True
            except:
                pass
            try:
                open_url = urlopen('https://' + url)
                open_url.close()
                url_validated = True
            except:
                pass

        if not url_validated:
            grass.fatal(_('Cannot open URL: {}'.format(url)))

    # Handle local URLs
    if os.path.isdir(url):
        return 'dir', os.path.abspath(url)
    elif os.path.exists(url):
        if url.endswith('.zip'):
            return 'zip', os.path.abspath(url)
        for suffix in extract_tar.supported_formats:
            if url.endswith('.' + suffix):
                return suffix, os.path.abspath(url)
    # Handle remote URLs
    else:
        source, resolved_url = resolve_known_host_service(url, name, branch)
        if source:
            return source, resolved_url
        # we allow URL to end with =zip or ?zip and not only .zip
        # unfortunately format=zip&version=89612 would require something else
        # special option to force the source type would solve it
        if url.endswith('zip'):
            return 'remote_zip', url
        for suffix in extract_tar.supported_formats:
            if url.endswith(suffix):
                return 'remote_' + suffix, url
        # fallback to the classic behavior
        return 'svn', url


def get_addons_paths(gg_addons_base_dir):
    """Get and save extensions paths as 'extensions_paths.json' json file
    in the $GRASS_ADDON_BASE dir. The file serves as a list of all addons,
    and their paths (mkhmtl.py tool)
    """
    get_addons_paths.json_file = 'addons_paths.json'

    url = 'https://api.github.com/repos/OSGeo/grass-addons/git/trees/'\
        'main?recursive=1'

    response = download_addons_paths_file(
        url=url, response_format='application/json',
    )
    if response:
        addons_paths = json.loads(gscript.decode(response.read()))
        with open(os.path.join(gg_addons_base_dir, get_addons_paths.json_file),
                  'w') as f:
            json.dump(addons_paths, f)


def main():
    # check dependencies
    if not flags['a'] and sys.platform != "win32":
        check_progs()

    original_url = options['url']
    branch = options['branch']

    # manage proxies
    global PROXIES
    if options['proxy']:
        PROXIES = {}
        for ptype, purl in (p.split('=') for p in options['proxy'].split(',')):
            PROXIES[ptype] = purl
        proxy = urlrequest.ProxyHandler(PROXIES)
        opener = urlrequest.build_opener(proxy)
        urlrequest.install_opener(opener)

    # define path
    options['prefix'] = resolve_install_prefix(path=options['prefix'],
                                               to_system=flags['s'])

    if flags['j']:
        get_addons_paths(gg_addons_base_dir=options['prefix'])
        return 0

    # list available extensions
    if flags['l'] or flags['c'] or (flags['g'] and not flags['a']):
        # using dummy extension, we don't need any extension URL now,
        # but will work only as long as the function does not check
        # if the URL is actually valid or something
        source, url = resolve_source_code(name='dummy',
                                          url=original_url,
                                          branch=branch)
        xmlurl = resolve_xmlurl_prefix(original_url, source=source)
        list_available_extensions(xmlurl)
        return 0
    elif flags['a']:
        list_installed_extensions(toolboxes=flags['t'])
        return 0

    if flags['d'] or flags['i']:
        flag = 'd' if flags['d'] else 'i'
        if options['operation'] != 'add':
            grass.warning(_("Flag '{}' is relevant only to"
                            " 'operation=add'. Ignoring this flag.").format(
                                flag))
        else:
            global REMOVE_TMPDIR
            REMOVE_TMPDIR = False

    if options['operation'] == 'add':
        check_dirs()
        if original_url == '' or flags['o']:
            """
            Query GitHub API only if extension will be downloaded
            from official GRASS GIS addon repository
            """
            get_addons_paths(gg_addons_base_dir=options['prefix'])
        source, url = resolve_source_code(name=options['extension'],
                                          url=original_url,
                                          branch=branch)
        xmlurl = resolve_xmlurl_prefix(original_url, source=source)
        install_extension(source=source, url=url, xmlurl=xmlurl, branch=branch)
    else:  # remove
        remove_extension(force=flags['f'])

    return 0


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == '--doctest':
        import doctest
        sys.exit(doctest.testmod().failed)
    options, flags = grass.parser()
    global TMPDIR
    TMPDIR = tempfile.mkdtemp()
    atexit.register(cleanup)

    grass_version = grass.version()
    version = grass_version['version'].split('.')
    build_platform = grass_version['build_platform'].split('-', 1)[0]

    sys.exit(main())
