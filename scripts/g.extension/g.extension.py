#!/usr/bin/env python

############################################################################
#
# MODULE:       g.extension
# AUTHOR(S):    Markus Neteler
#               Pythonized & upgraded for GRASS 7 by Martin Landa <landa.martin gmail.com>
# PURPOSE:      Tool to download and install extensions from GRASS Addons SVN into
#               local GRASS installation
# COPYRIGHT:    (C) 2009-2014 by Markus Neteler, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
# TODO: add sudo support where needed (i.e. check first permission to write into
#       $GISBASE directory)
#############################################################################

#%module
#% label: Maintains GRASS Addons extensions in local GRASS installation.
#% description: Downloads, installs extensions from GRASS Addons SVN repository into local GRASS installation or removes installed extensions.
#% keyword: general
#% keyword: installation
#% keyword: extensions
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
#% key: svnurl
#% type: string
#% key_desc: url
#% description: SVN Addons repository URL
#% answer: http://svn.osgeo.org/grass/grass-addons/grass7
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

#%flag
#% key: l
#% description: List available extensions in the GRASS Addons SVN repository
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: c
#% description: List available extensions in the GRASS Addons SVN repository including module description
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: g
#% description: List available extensions in the GRASS Addons SVN repository (shell script style)
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

#%rules
#% required: extension, -l, -c, -g, -a
#% exclusive: extension, -l, -c, -g, -a
#%end


from __future__ import print_function
import os
import sys
import re
import atexit
import shutil
import zipfile
import tempfile

try:
    from urllib2 import HTTPError
    from urllib import urlopen, urlretrieve
except ImportError:
    from urllib.request import HTTPError, urlopen, urlretrieve

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree  # Python <= 2.4

from grass.script.utils import try_rmdir
from grass.script import core as grass

# temp dir
REMOVE_TMPDIR = True
PROXIES = {}

# check requirements


def check_progs():
    """Check if the necessary programs are available"""
    for prog in ('svn', 'make', 'gcc'):
        if not grass.find_program(prog, '--help'):
            grass.fatal(_("'%s' required. Please install '%s' first.")
                        % (prog, prog))

# expand prefix to class name


def expand_module_class_name(class_letters):
    """Convert module class (family) letter or letters to class (family) name

    The letter or letters are used in module names, e.g. r.slope.aspect.
    The names are used in directories in Addons but also in the source code.

    >>> expand_module_class_name('r')
    raster
    >>> expand_module_class_name('v')
    vector
    """
    name = {'d': 'display',
            'db': 'database',
            'g': 'general',
            'i': 'imagery',
            'm': 'misc',
            'ps': 'postscript',
            'p': 'paint',
            'r': 'raster',
            'r3': 'raster3d',
            's': 'sites',
            'v': 'vector',
            'wx': 'gui/wxpython'
            }

    return name.get(class_letters, class_letters)


def get_installed_extensions(force=False):
    """Get list of installed extensions or toolboxes (if -t is set)"""
    if flags['t']:
        return get_installed_toolboxes(force)

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
    fo = open(xml_file, 'r')
    try:
        tree = etree.fromstring(fo.read())
    except:
        os.remove(xml_file)
        write_xml_toolboxes(xml_file)
        return []
    fo.close()

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
            grass.debug(1, "No addons metadata file available")
        return []

    # read XML file
    fo = open(xml_file, 'r')
    try:
        tree = etree.fromstring(fo.read())
    except:
        os.remove(xml_file)
        write_xml_modules(xml_file)
        return []
    fo.close()

    ret = list()
    for tnode in tree.findall('task'):
        ret.append(tnode.get('name').strip())

    return ret

# list extensions (read XML file from grass.osgeo.org/addons)


def list_available_extensions(url):
    """List available extensions/modules or toolboxes (if -t is given)

    For toolboxes it lists also all modules.
    """
    if flags['t']:
        grass.message(_("List of available extensions (toolboxes):"))
        tlist = get_available_toolboxes(url)
        for toolbox_code, toolbox_data in tlist.iteritems():
            if flags['g']:
                print('toolbox_name=' + toolbox_data['name'])
                print('toolbox_code=' + toolbox_code)
            else:
                print('%s (%s)' % (toolbox_data['name'], toolbox_code))
            if flags['c'] or flags['g']:
                list_available_modules(url, toolbox_data['modules'])
            else:
                if toolbox_data['modules']:
                    print(os.linesep.join(map(lambda x: '* ' + x, toolbox_data['modules'])))
    else:
        grass.message(_("List of available extensions (modules):"))
        list_available_modules(url)


def get_available_toolboxes(url):
    """Return toolboxes available in the repository"""
    tdict = dict()
    url = url + "toolboxes.xml"
    try:
        f = urlopen(url, proxies=PROXIES)
        tree = etree.fromstring(f.read())
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
    except HTTPError:
        grass.fatal(_("Unable to fetch addons metadata file"))

    return tdict


def get_toolbox_modules(url, name):
    """Get modules inside a toolbox in toolbox file at given URL

    :param url: URL of the directory (file name will be attached)
    :param name: toolbox name
    """
    tlist = list()

    url = url + "toolboxes.xml"

    try:
        f = urlopen(url, proxies=PROXIES)
        tree = etree.fromstring(f.read())
        for tnode in tree.findall('toolbox'):
            if name == tnode.get('code'):
                for mnode in tnode.findall('task'):
                    tlist.append(mnode.get('name'))
                break
    except HTTPError:
        grass.fatal(_("Unable to fetch addons metadata file"))

    return tlist


def get_optional_params(mnode):
    """Return description and keywords as a tuple

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
    """
    url = url + "modules.xml"
    grass.debug("url=%s" % url, 1)
    try:
        f = urlopen(url, proxies=PROXIES)
        try:
            tree = etree.fromstring(f.read())
        except:
            grass.warning(_("Unable to parse '%s'. Trying to scan"
                            " SVN repository (may take some time)...") % url)
            list_available_extensions_svn()
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
    except HTTPError:
        list_available_extensions_svn()

# list extensions (scan SVN repo)


def list_available_extensions_svn():
    """List available extensions from HTML given by URL

    ``<li><a href=...`` is parsed to find module names.
    """
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
                'p', 'r', 'r3', 's', 'v']
    for prefix in prefixes:
        modclass = expand_module_class_name(prefix)
        grass.verbose(_("Checking for '%s' modules...") % modclass)

        url = '%s/%s' % (options['svnurl'], modclass)
        grass.debug("url = %s" % url, debug=2)
        try:
            f = urlopen(url, proxies=PROXIES)
        except HTTPError:
            grass.debug(_("Unable to fetch '%s'") % url, debug=1)
            continue

        for line in f.readlines():
            # list extensions
            sline = pattern.search(line)
            if not sline:
                continue
            name = sline.group(2).rstrip('/')
            if name.split('.', 1)[0] == prefix:
                print(name)

    # get_wxgui_extensions()


def get_wxgui_extensions():
    """Return list of extensions/addons in wxGUI directory at given URL"""
    mlist = list()
    grass.debug('Fetching list of wxGUI extensions from '
                'GRASS-Addons SVN repository (be patient)...')
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)
    grass.verbose(_("Checking for '%s' modules...") % 'gui/wxpython')

    url = '%s/%s' % (options['svnurl'], 'gui/wxpython')
    grass.debug("url = %s" % url, debug=2)
    f = urlopen(url, proxies=PROXIES)
    if not f:
        grass.warning(_("Unable to fetch '%s'") % url)
        return

    for line in f.readlines():
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
    fo = open(name, 'w')
    fo.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    fo.write('<!DOCTYPE task SYSTEM "grass-addons.dtd">\n')
    fo.write('<addons version="%s">\n' % version[0])

    libgis_revison = grass.version()['libgis_revision']
    if tree is not None:
        for tnode in tree.findall('task'):
            indent = 4
            fo.write('%s<task name="%s">\n' %
                     (' ' * indent, tnode.get('name')))
            indent += 4
            fo.write('%s<description>%s</description>\n' %
                     (' ' * indent, tnode.find('description').text))
            fo.write('%s<keywords>%s</keywords>\n' %
                     (' ' * indent, tnode.find('keywords').text))
            bnode = tnode.find('binary')
            if bnode is not None:
                fo.write('%s<binary>\n' % (' ' * indent))
                indent += 4
                for fnode in bnode.findall('file'):
                    fo.write('%s<file>%s</file>\n' %
                             (' ' * indent, os.path.join(options['prefix'],
                                                         fnode.text)))
                indent -= 4
                fo.write('%s</binary>\n' % (' ' * indent))
            fo.write('%s<libgis revision="%s" />\n' %
                     (' ' * indent, libgis_revison))
            indent -= 4
            fo.write('%s</task>\n' % (' ' * indent))

    fo.write('</addons>\n')
    fo.close()


def write_xml_toolboxes(name, tree=None):
    """Write element tree as a toolboxes matadata file

    If the *tree* is not given, an empty file is created.

    :param name: file name
    :param tree: XML element tree
    """
    fo = open(name, 'w')
    fo.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    fo.write('<!DOCTYPE toolbox SYSTEM "grass-addons.dtd">\n')
    fo.write('<addons version="%s">\n' % version[0])
    if tree is not None:
        for tnode in tree.findall('toolbox'):
            indent = 4
            fo.write('%s<toolbox name="%s" code="%s">\n' %
                     (' ' * indent, tnode.get('name'), tnode.get('code')))
            indent += 4
            for cnode in tnode.findall('correlate'):
                fo.write('%s<correlate code="%s" />\n' %
                         (' ' * indent, tnode.get('code')))
            for mnode in tnode.findall('task'):
                fo.write('%s<task name="%s" />\n' %
                         (' ' * indent, mnode.get('name')))
            indent -= 4
            fo.write('%s</toolbox>\n' % (' ' * indent))

    fo.write('</addons>\n')
    fo.close()


def install_extension(source, url, xmlurl):
    """Install extension (e.g. one module) or a toolbox (list of modules)"""
    gisbase = os.getenv('GISBASE')
    if not gisbase:
        grass.fatal(_('$GISBASE not defined'))

    if options['extension'] in get_installed_extensions(force=True):
        grass.warning(_("Extension <%s> already installed. Re-installing...") %
                      options['extension'])

    if flags['t']:
        grass.message(_("Installing toolbox <%s>...") % options['extension'])
        mlist = get_toolbox_modules(xmlurl, options['extension'])
    else:
        mlist = [options['extension']]
    if not mlist:
        grass.warning(_("Nothing to install"))
        return

    ret = 0
    for module in mlist:
        if sys.platform == "win32":
            ret += install_extension_win(module)
        else:
            ret += install_extension_std_platforms(module,
                                                   source=source, url=url)
        if len(mlist) > 1:
            print('-' * 60)

    if flags['d']:
        return

    if ret != 0:
        grass.warning(_('Installation failed, sorry.'
                        ' Please check above error messages.'))
    else:
        grass.message(_("Updating addons metadata file..."))
        blist = install_extension_xml(xmlurl, mlist)
        for module in blist:
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
        f = urlopen(url, proxies=PROXIES)
        tree = etree.fromstring(f.read())
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
    except HTTPError:
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
    data = {}
    bin_list = []
    try:
        f = urlopen(url, proxies=PROXIES)
        try:
            tree = etree.fromstring(f.read())
        except:
            grass.warning(_("Unable to parse '%s'.") % url)
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
    except:
        grass.error(
            _("Unable to read addons metadata file from the remote server"))
    return data, bin_list


def install_extension_xml(url, mlist):
    """Update XML files with metadata about installed modules and toolbox

    Uses the remote/repository XML files for modules to obtain the metadata.

    :returns: list of executables (useable for ``update_manual_page()``)
    """
    if len(mlist) > 1:
        # read metadata from remote server (toolboxes)
        install_toolbox_xml(url, options['extension'])

    # read metadata from remote server (modules)
    url = url + "modules.xml"
    data, bin_list = get_addons_metadata(url, mlist)
    if not data:
        grass.warning(_("No addons metadata available."
                        " Addons metadata file not updated."))
        return []

    xml_file = os.path.join(options['prefix'], 'modules.xml')
    # create an empty file if not exists
    if not os.path.exists(xml_file):
        write_xml_modules(xml_file)

    # read XML file
    fo = open(xml_file, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()

    # update tree
    for name in mlist:
        tnode = None
        for node in tree.findall('task'):
            if node.get('name') == name:
                tnode = node
                break

        if name not in data:
            grass.warning(_("No addons metadata found for <%s>") % name)
            continue

        ndata = data[name]
        if tnode is not None:
            # update existing node
            dnode = tnode.find('description')
            if dnode is not None:
                dnode.text = ndata['desc']
            knode = tnode.find('keywords')
            if knode is not None:
                knode.text = ndata['keyw']
            bnode = tnode.find('binary')
            if bnode is not None:
                tnode.remove(bnode)
            bnode = etree.Element('binary')
            for file_name in ndata['files']:
                fnode = etree.Element('file')
                fnode.text = file_name
                bnode.append(fnode)
            tnode.append(bnode)
        else:
            # create new node for task
            tnode = etree.Element('task', attrib={'name': name})
            dnode = etree.Element('description')
            dnode.text = ndata['desc']
            tnode.append(dnode)
            knode = etree.Element('keywords')
            knode.text = ndata['keyw']
            tnode.append(knode)
            bnode = etree.Element('binary')
            for file_name in ndata['files']:
                fnode = etree.Element('file')
                fnode.text = file_name
                bnode.append(fnode)
            tnode.append(bnode)
            tree.append(tnode)

    write_xml_modules(xml_file, tree)

    return bin_list


def install_extension_win(name):
    """Install extension on MS Windows"""
    # do not use hardcoded url -
    # http://wingrass.fsv.cvut.cz/grassXX/addonsX.X.X
    grass.message(_("Downloading precompiled GRASS Addons <%s>...") %
                  options['extension'])
    url = "http://wingrass.fsv.cvut.cz/" \
          "grass%(major)s%(minor)s/addons/" \
          "grass-%(major)s.%(minor)s.%(patch)s/" % \
        {'major': version[0], 'minor': version[1], 'patch': version[2]}

    grass.debug("url=%s" % url, 1)

    try:
        zfile = url + name + '.zip'
        f = urlopen(zfile, proxies=PROXIES)

        # create addons dir if not exists
        if not os.path.exists(options['prefix']):
            try:
                os.mkdir(options['prefix'])
            except OSError as error:
                grass.fatal(_("Unable to create <{}>. {}")
                            .format(options['prefix'], error))

        # download data
        fo = tempfile.TemporaryFile()
        fo.write(f.read())
        try:
            zfobj = zipfile.ZipFile(fo)
        except zipfile.BadZipfile as error:
            grass.fatal('%s: %s' % (error, zfile))

        for name in zfobj.namelist():
            if name.endswith('/'):
                directory = os.path.join(options['prefix'], name)
                if not os.path.exists(directory):
                    os.mkdir(directory)
            else:
                outfile = open(os.path.join(options['prefix'], name), 'wb')
                outfile.write(zfobj.read(name))
                outfile.close()

        fo.close()
    except HTTPError:
        grass.fatal(_("GRASS Addons <%s> not found") % name)

    return 0


def download_source_code_svn(url, name, outdev, directory=None):
    """Download source code from a Subversion reporsitory

    .. note:
        Stdout is passed to to *outdev* while stderr is will be just printed.

    :param url: URL of the repository
        (module class/family and name are attached)
    :param name: module name
    :param outdev: output devide for the standard output of the svn command
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


def move_extracted_files(extract_dir, target_dir, files):
    """Fix state of extracted file by moving them to different diretcory

    When extracting, it is not clear what will be the root directory
    or if there will be one at all. So this function moves the files to
    a different directory in the way that if there was one direcory extracted,
    the contained files are moved.
    """
    if len(files) == 1:
        shutil.copytree(os.path.join(extract_dir, files[0]), target_dir)
        print("copytree:", os.path.join(extract_dir, files[0]), target_dir)
    else:
        for file_name in files:
            actual_file = os.path.join(extract_dir, file_name)
            if os.path.isdir(actual_file):
                shutil.copytree(actual_file, os.path.join(target_dir, file_name))
            else:
                shutil.copy(actual_file, target_dir)


# Original copyright and license of the original version of the CRLF function
# Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
# Python Software Foundation; All Rights Reserved
# Python Software Foundation License Version 2
# http://svn.python.org/projects/python/trunk/Tools/scripts/crlf.py
def fix_newlines(directory):
    """Replace CRLF with LF in all files in the directory

    Binary files are ignored. Recurses into subdirectories.
    """
    for root, unused, files in os.walk(directory):
        for name in files:
            filename = os.path.join(root, name)
            data = open(filename, 'rb').read()
            if '\0' in data:
                continue  # ignore binary files
            # we don't expect there would be CRLF file by purpose
            # if we want to allow CRLF files we would have to whitelite .py etc
            newdata = data.replace('\r\n', '\n')
            if newdata != data:
                f = open(filename, 'wb')
                f.write(newdata)
                f.close()


def extract_zip(name, directory, tmpdir):
    """Extract a ZIP file into a directory"""
    zip_file = zipfile.ZipFile(name, mode='r')
    file_list = zip_file.namelist()
    # we suppose we can write to parent of the given dir (supposing a tmp dir)
    extract_dir = os.path.join(tmpdir, 'extract_dir')
    os.mkdir(extract_dir)
    for subfile in file_list:
        # this should be safe in Python 2.7.4
        zip_file.extract(subfile, extract_dir)
    files = os.listdir(extract_dir)
    move_extracted_files(extract_dir=extract_dir,
                         target_dir=directory, files=files)


# TODO: solve the other related formats
def extract_tar(name, directory, tmpdir):
    """Extract a TAR or a similar file into a directory"""
    import tarfile
    tar = tarfile.open(name, "r:gz")
    tar.extractall()
    files = os.listdir(tmpdir)
    move_extracted_files(extract_dir=tmpdir,
                         target_dir=directory, files=files)


def download_source_code(source, url, name, outdev,
                         directory=None, tmpdir=None):
    """Get source code to a local directory for compilation"""
    if source == 'svn':
        download_source_code_svn(url, name, outdev, directory)
    elif source == 'remote_zip':
        # we expect that the module.zip file is not by chance in the archive
        zip_name = os.path.join(tmpdir, 'module.zip')
        urlretrieve(url, zip_name)
        extract_zip(name=zip_name, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source == 'remote_tar.gz':
        # we expect that the module.tar.gz file is not by chance in the archive
        archive_name = os.path.join(tmpdir, 'module.tar.gz')
        urlretrieve(url, archive_name)
        extract_tar(name=archive_name, directory=directory, tmpdir=tmpdir)
        fix_newlines(directory)
    elif source == 'zip':
        os.mkdir(directory)
        extract_zip(name=url, directory=directory)
        fix_newlines(directory)
    elif source == 'tar':
        os.mkdir(directory)
        extract_tar(name=url, directory=directory)
        fix_newlines(directory)
    elif source == 'dir':
        shutil.copytree(url, directory)
        fix_newlines(directory)
    else:
        # probably programmer error
        grass.fatal(_("Unknown extension (addon) source '{}'."
                      " Please report this to grass-user mailing list.")
                    .format(source))


def install_extension_std_platforms(name, source, url):
    """Install extension on standard plaforms"""
    gisbase = os.getenv('GISBASE')
    grass.message(_("Fetching <%s> from"
                    " GRASS-Addons SVN repository (be patient)...") % name)

    # to hide non-error messages from subprocesses
    if grass.verbosity() <= 2:
        outdev = open(os.devnull, 'w')
    else:
        outdev = sys.stdout

    os.chdir(TMPDIR)  # this is just to not leave something behind
    srcdir = os.path.join(TMPDIR, name)
    download_source_code(source=source, url=url, name=name,
                         outdev=outdev, directory=srcdir, tmpdir=TMPDIR)
    os.chdir(srcdir)

    dirs = {'bin': os.path.join(TMPDIR, name, 'bin'),
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

    make_cmd = ['make',
                'MODULE_TOPDIR=%s' % gisbase.replace(' ', r'\ '),
                'RUN_GISRC=%s' % os.environ['GISRC'],
                'BIN=%s' % dirs['bin'],
                'HTMLDIR=%s' % dirs['html'],
                'RESTDIR=%s' % dirs['rest'],
                'MANBASEDIR=%s' % dirs['man'],
                'SCRIPTDIR=%s' % dirs['script'],
                'STRINGDIR=%s' % dirs['string'],
                'ETC=%s' % os.path.join(dirs['etc'])
                ]

    install_cmd = ['make',
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
        return 0

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
        return 0

    grass.message(_("Installing..."))

    return grass.call(install_cmd,
                      stdout=outdev)


def remove_extension(force=False):
    """Remove existing extension (module or toolbox if -t is given)"""
    if flags['t']:
        mlist = get_toolbox_modules(options['prefix'], options['extension'])
    else:
        mlist = [options['extension']]

    if force:
        grass.verbose(_("List of removed files:"))
    else:
        grass.info(_("Files to be removed:"))

    remove_modules(mlist, force)

    if force:
        grass.message(_("Updating addons metadata file..."))
        remove_extension_xml(mlist)
        grass.message(_("Extension <%s> successfully uninstalled.") %
                      options['extension'])
    else:
        grass.warning(_("Extension <%s> not removed. "
                        "Re-run '%s' with '-f' flag to force removal")
                      % (options['extension'], 'g.extension'))

# remove existing extension(s) (reading XML file)


def remove_modules(mlist, force=False):
    """Remove extensions/modules specified in a list

    Collects the file names from the file with metadata and fallbacks
    to standard layout of files on prefix path on error.
    """
    # try to read XML metadata file first
    xml_file = os.path.join(options['prefix'], 'modules.xml')
    installed = get_installed_modules()

    if os.path.exists(xml_file):
        f = open(xml_file, 'r')
        tree = etree.fromstring(f.read())
        f.close()
    else:
        tree = None

    for name in mlist:
        if name not in installed:
            # try even if module does not seem to be available,
            # as the user may be trying to get rid of left over cruft
            grass.warning(_("Extension <%s> not found") % name)

        if tree is not None:
            flist = []
            for task in tree.findall('task'):
                if name == task.get('name') and \
                        task.find('binary') is not None:
                    for file_node in task.find('binary').findall('file'):
                        flist.append(file_node.text)
                    break

            if flist:
                removed = False
                err = list()
                for fpath in flist:
                    try:
                        if force:
                            grass.verbose(fpath)
                            removed = True
                            os.remove(fpath)
                        else:
                            print(fpath)
                    except OSError:
                        err.append((_("Unable to remove file '%s'") % fpath))
                if force and not removed:
                    grass.fatal(_("Extension <%s> not found") % name)

                if err:
                    for error_line in err:
                        grass.error(error_line)
            else:
                remove_extension_std(name, force)
        else:
            remove_extension_std(name, force)


def remove_extension_std(name, force=False):
    """Remove extension/module expecting the standard layout"""
    for fpath in [os.path.join(options['prefix'], 'bin', name),
                  os.path.join(options['prefix'], 'scripts', name),
                  os.path.join(
                      options['prefix'], 'docs', 'html', name + '.html'),
                  os.path.join(
                      options['prefix'], 'docs', 'rest', name + '.txt'),
                  os.path.join(options['prefix'], 'docs', 'man', 'man1',
                               name + '.1')]:
        if os.path.isfile(fpath):
            if force:
                grass.verbose(fpath)
                os.remove(fpath)
            else:
                print(fpath)


def remove_from_toolbox_xml(name):
    """Update local meta-file when removing existing toolbox"""
    xml_file = os.path.join(options['prefix'], 'toolboxes.xml')
    if not os.path.exists(xml_file):
        return

    # read XML file
    fo = open(xml_file, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()

    for node in tree.findall('toolbox'):
        if node.get('code') != name:
            continue
        tree.remove(node)

    write_xml_toolboxes(xml_file, tree)


def remove_extension_xml(modules):
    """Update local meta-file when removing existing extension"""
    if len(modules) > 1:
        # update also toolboxes metadata
        remove_from_toolbox_xml(options['extension'])

    xml_file = os.path.join(options['prefix'], 'modules.xml')
    if not os.path.exists(xml_file):
        return

    # read XML file
    fo = open(xml_file, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()

    for name in modules:
        for node in tree.findall('task'):
            if node.get('name') != name:
                continue
            tree.remove(node)

    write_xml_modules(xml_file, tree)

# check links in CSS


def check_style_files(fil):
    """Ensures that a specified HTML documentation support file exists

    If the file, e.g. a CSS file does not exist, the file is copied from
    the distribution.
    """
    dist_file = os.path.join(os.getenv('GISBASE'), 'docs', 'html', fil)
    addons_file = os.path.join(options['prefix'], 'docs', 'html', fil)

    if os.path.isfile(addons_file):
        return

    try:
        shutil.copyfile(dist_file, addons_file)
    except OSError as error:
        grass.fatal(_("Unable to create '%s': %s") % (addons_file, error))


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
    check_style_files('grass_logo.png')
    check_style_files('grassdocs.css')
    create_dir(os.path.join(options['prefix'], 'etc'))
    create_dir(os.path.join(options['prefix'], 'docs', 'man', 'man1'))
    create_dir(os.path.join(options['prefix'], 'scripts'))

# fix file URI in manual page


def update_manual_page(module):
    """Fix manual page for addons which are at different directory then rest"""
    if module.split('.', 1)[0] == 'wx':
        return  # skip for GUI modules

    grass.verbose(_("Manual page for <%s> updated") % module)
    # read original html file
    htmlfile = os.path.join(
        options['prefix'], 'docs', 'html', module + '.html')
    try:
        f = open(htmlfile)
        shtml = f.read()
    except IOError as error:
        grass.fatal(_("Unable to read manual page: %s") % error)
    else:
        f.close()

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
        f = open(htmlfile, 'w')
        f.write(ohtml)
    except IOError as error:
        grass.fatal(_("Unable for write manual page: %s") % error)
    else:
        f.close()


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
        grass.fatal(_("You don't have permission to install extension to <{}>."
                      " Try to run {} with administrator rights"
                      " (su or sudo).")
                    .format(path, 'g.extension'))
    # ensure dir sep at the end for cases where path is used as URL and pasted
    # together with file names
    if not path.endswith(os.path.sep):
        path = path + os.path.sep
    return path


def resolve_xmlurl_prefix(url):
    """Determine and check the URL where the XML metadata files are stored"""
    if 'svn.osgeo.org/grass/grass-addons/grass7' in url:
        # use pregenerated modules XML file
        url = "http://grass.osgeo.org/addons/grass%s" % version[0]
    # else try to get modules XMl from SVN repository (provided URL)
    # the exact action depends on subsequent code (somewhere)

    if not url.endswith('/'):
        url = url + "/"
    return url


def resolve_source_code(url):
    if os.path.isdir(url):
        return 'dir', url
    elif os.path.exists(url):
        for suffix in ['.zip', '.tar.gz']:
            if url.endswith(suffix):
                return suffix.lstrip('.'), url
    # https://trac.osgeo.org/grass/browser/grass-addons/grass7/raster/r.modis?format=zip
    # return 'remote_zip', url
    # https://github.com/user/module
    # https://github.com/user/module/archive/master.zip
    elif url.startswith('github.com') and \
            not (url.endswith('.zip') or url.endswith('.tar.gz')):
        url = 'https://{}/archive/master.zip'.format(url)
        return 'remote_zip', url
    else:
        for suffix in ['zip', 'tar.gz']:
            if url.endswith(suffix):
                print(suffix)
                return 'remote_' + suffix, url
        # fallback to classic behavior
        return 'svn', url


def main():
    # check dependecies
    if sys.platform != "win32":
        check_progs()

    # manage proxies
    global PROXIES
    if options['proxy']:
        PROXIES = {}
        for ptype, purl in (p.split('=') for p in options['proxy'].split(',')):
            PROXIES[ptype] = purl

    # define path
    options['prefix'] = resolve_install_prefix(path=options['prefix'],
                                               to_system=flags['s'])

    # list available extensions
    if flags['l'] or flags['c'] or flags['g']:
        xmlurl = resolve_xmlurl_prefix(options['svnurl'])
        list_available_extensions(xmlurl)
        return 0
    elif flags['a']:
        list_installed_extensions(toolboxes=flags['t'])
        return 0

    if flags['d']:
        if options['operation'] != 'add':
            grass.warning(_("Flag 'd' is relevant only to"
                            " 'operation=add'. Ignoring this flag."))
        else:
            global REMOVE_TMPDIR
            REMOVE_TMPDIR = False

    if options['operation'] == 'add':
        check_dirs()
        source, url = resolve_source_code(options['svnurl'])
        xmlurl = resolve_xmlurl_prefix(options['svnurl'])
        install_extension(source=source, url=url, xmlurl=xmlurl)
    else:  # remove
        remove_extension(force=flags['f'])

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    global TMPDIR
    TMPDIR = tempfile.mkdtemp()
    atexit.register(cleanup)
    version = grass.version()['version'].split('.')
    sys.exit(main())
