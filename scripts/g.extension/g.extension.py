#!/usr/bin/env python

############################################################################
#
# MODULE:       g.extension
# AUTHOR(S):    Markus Neteler
#               Pythonized & upgraded for GRASS 7 by Martin Landa <landa.martin gmail.com>
# PURPOSE:      Tool to download and install extensions from GRASS Addons SVN into 
#               local GRASS installation
# COPYRIGHT:    (C) 2009-2013 by Markus Neteler, and the GRASS Development Team
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
#% keywords: general
#% keywords: installation
#% keywords: extensions
#%end

#%option
#% key: extension
#% type: string
#% key_desc: name
#% description: Name of extension to install/remove
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
#% description: Don't install new extension, just compile it
#% guisection: Install
#%end
#%flag
#% key: f
#% description: Force removal when uninstalling extension (operation=remove)
#% guisection: Remove
#%end
#%flag
#% key: t
#% description: Operate on toolboxes instead of single modules
#% suppress_required: yes
#%end

import os
import sys
import re
import atexit
import shutil
import zipfile
import tempfile

from urllib2 import urlopen, HTTPError

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

from grass.script import core as grass

# temp dir
remove_tmpdir = True

# check requirements
def check_progs():
    for prog in ('svn', 'make', 'gcc'):
        if not grass.find_program(prog, ['--help']):
            grass.fatal(_("'%s' required. Please install '%s' first.") % (prog, prog))

# expand prefix to class name
def expand_module_class_name(c):
    name = { 'd'   : 'display',
             'db'  : 'database',
             'g'   : 'general',
             'i'   : 'imagery',
             'm'   : 'misc',
             'ps'  : 'postscript',
             'p'   : 'paint',
             'r'   : 'raster',
             'r3'  : 'raster3d',
             's'   : 'sites',
             'v'   : 'vector',
             'wx'  : 'gui/wxpython'
             }

    return name.get(c, c)

# list installed extensions
def get_installed_extensions(force = False):
    if flags['t']:
        return get_installed_toolboxes(force)

    return get_installed_modules(force)


def get_installed_toolboxes(force = False):
    fXML = os.path.join(options['prefix'], 'toolboxes.xml')
    if not os.path.exists(fXML):
        write_xml_toolboxes(fXML)

    # read XML file
    fo = open(fXML, 'r')
    try:
        tree = etree.fromstring(fo.read())
    except:
        os.remove(fXML)
        write_xml_toolboxes(fXML)
        return []
    fo.close()

    ret = list()
    for tnode in tree.findall('toolbox'):
        ret.append(tnode.get('code'))

    return ret


def get_installed_modules(force = False):
    fXML = os.path.join(options['prefix'], 'modules.xml')
    if not os.path.exists(fXML):
        if force:
            write_xml_modules(fXML)
        else:
            grass.warning(_("No metadata file available"))
        return []

    # read XML file
    fo = open(fXML, 'r')
    try:
        tree = etree.fromstring(fo.read())
    except:
        os.remove(fXML)
        write_xml_modules(fXML)
        return []
    fo.close()

    ret = list()
    for tnode in tree.findall('task'):
        ret.append(tnode.get('name').strip())

    return ret

# list extensions (read XML file from grass.osgeo.org/addons)
def list_available_extensions(url):
    if flags['t']:
        grass.message(_("List of available extensions (toolboxes):"))
        tlist = list_available_toolboxes(url)
        for toolbox_code, toolbox_data in tlist.iteritems():
            if flags['g']:
                print 'toolbox_name=' + toolbox_data['name']
                print 'toolbox_code=' + toolbox_code
            else:
                print '%s (%s)' % (toolbox_data['name'], toolbox_code)
            if flags['c'] or flags['g']:
                list_available_modules(url, toolbox_data['modules'])
            else:
                if toolbox_data['modules']:
                    print os.linesep.join(map(lambda x: '* ' + x, toolbox_data['modules']))
    else:
        grass.message(_("List of available extensions (modules):"))
        list_available_modules(url)


def list_available_toolboxes(url):
    tdict = dict()
    url = url + "toolboxes.xml"
    try:
        f = urlopen(url)
        tree = etree.fromstring(f.read())
        for tnode in tree.findall('toolbox'):
            mlist = list()
            clist = list()
            tdict[tnode.get('code')] = { 'name' : tnode.get('name'),
                                         'correlate' : clist,
                                         'modules' : mlist }

            for cnode in tnode.findall('correlate'):
                clist.append(cnode.get('name'))

            for mnode in tnode.findall('task'):
                mlist.append(mnode.get('name'))
    except HTTPError:
        grass.fatal(_("Unable to fetch metadata file"))

    return tdict


def get_toolbox_modules(url, name):
    tlist = list()

    url = url + "toolboxes.xml"

    try:
        f = urlopen(url)
        tree = etree.fromstring(f.read())
        for tnode in tree.findall('toolbox'):
            if name == tnode.get('code'):
                for mnode in tnode.findall('task'):
                    tlist.append(mnode.get('name'))
                break
    except HTTPError:
        grass.fatal(_("Unable to fetch metadata file"))

    return tlist

def get_optional_params(mnode):
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

def list_available_modules(url, mlist = None):
    # try to download XML metadata file first
    url = url + "modules.xml"
    grass.debug("url=%s" % url, 1)
    try:
        f = urlopen(url)
        try:
            tree = etree.fromstring(f.read())
        except:
            grass.warning(_("Unable to parse '%s'. Trying to scan SVN (may take some time)...") % url)
            list_available_extensions_svn()
            return
        
        for mnode in tree.findall('task'):
            name = mnode.get('name').strip()
            if mlist and name not in mlist:
                continue
            if flags['c'] or flags['g']:
                desc, keyw = get_optional_params(mnode)
            
            if flags['g']:
                print 'name=' + name
                print 'description=' + desc
                print 'keywords=' + keyw
            elif flags['c']:
                if mlist:
                    print '*',
                print name + ' - ' + desc
            else:
                print name
    except HTTPError:
        list_available_extensions_svn()

# list extensions (scan SVN repo)
def list_available_extensions_svn():
    grass.message(_('Fetching list of extensions from GRASS-Addons SVN (be patient)...'))
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)

    if flags['c']:
        grass.warning(_("Flag 'c' ignored, metadata file not available"))
    if flags['g']:
        grass.warning(_("Flag 'g' ignored, metadata file not available"))

    prefix = ['d', 'db', 'g', 'i', 'm', 'ps',
              'p', 'r', 'r3', 's', 'v']
    for d in prefix:
        modclass = expand_module_class_name(d)
        grass.verbose(_("Checking for '%s' modules...") % modclass)

        url = '%s/%s' % (options['svnurl'], modclass)
        grass.debug("url = %s" % url, debug = 2)
        try:
            f = urlopen(url)
        except HTTPError:
            grass.debug(_("Unable to fetch '%s'") % url, debug = 1)
            continue

        for line in f.readlines():
            # list extensions
            sline = pattern.search(line)
            if not sline:
                continue
            name = sline.group(2).rstrip('/')
            if name.split('.', 1)[0] == d:
                print name

    # get_wxgui_extensions()

# list wxGUI extensions
def get_wxgui_extensions():
    mlist = list()
    grass.debug('Fetching list of wxGUI extensions from GRASS-Addons SVN (be patient)...')
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)
    grass.verbose(_("Checking for '%s' modules...") % 'gui/wxpython')

    url = '%s/%s' % (options['svnurl'], 'gui/wxpython')
    grass.debug("url = %s" % url, debug = 2)
    f = urlopen(url)
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
    if remove_tmpdir:
        grass.try_rmdir(tmpdir)
    else:
        grass.message(_("Path to the source code:"))
        sys.stderr.write('%s\n' % os.path.join(tmpdir, options['extension']))

# write out meta-file
def write_xml_modules(name, tree = None):
    fo = open(name, 'w')
    fo.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    fo.write('<!DOCTYPE task SYSTEM "grass-addons.dtd">\n')
    fo.write('<addons version="%s">\n' % version[0])

    libgisRev = grass.version()['libgis_revision']
    if tree is not None:
        for tnode in tree.findall('task'):
            indent = 4
            fo.write('%s<task name="%s">\n' % (' ' * indent, tnode.get('name')))
            indent += 4
            fo.write('%s<description>%s</description>\n' % \
                         (' ' * indent, tnode.find('description').text))
            fo.write('%s<keywords>%s</keywords>\n' % \
                         (' ' * indent, tnode.find('keywords').text))
            bnode = tnode.find('binary')
            if bnode is not None:
                fo.write('%s<binary>\n' % (' ' * indent))
                indent += 4
                for fnode in bnode.findall('file'):
                    fo.write('%s<file>%s</file>\n' % \
                                 (' ' * indent, os.path.join(options['prefix'], fnode.text)))
                indent -= 4
                fo.write('%s</binary>\n' % (' ' * indent))
            fo.write('%s<libgis revision="%s" />\n' % \
                         (' ' * indent, libgisRev))
            indent -= 4
            fo.write('%s</task>\n' % (' ' * indent))

    fo.write('</addons>\n')
    fo.close()


def write_xml_toolboxes(name, tree = None):
    fo = open(name, 'w')
    fo.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    fo.write('<!DOCTYPE toolbox SYSTEM "grass-addons.dtd">\n')
    fo.write('<addons version="%s">\n' % version[0])
    if tree is not None:
        for tnode in tree.findall('toolbox'):
            indent = 4
            fo.write('%s<toolbox name="%s" code="%s">\n' % \
                         (' ' * indent, tnode.get('name'), tnode.get('code')))
            indent += 4
            for cnode in tnode.findall('correlate'):
                fo.write('%s<correlate code="%s" />\n' % \
                         (' ' * indent, tnode.get('code')))
            for mnode in tnode.findall('task'):
                fo.write('%s<task name="%s" />\n' % \
                         (' ' * indent, mnode.get('name')))
            indent -= 4
            fo.write('%s</toolbox>\n' % (' ' * indent))

    fo.write('</addons>\n')
    fo.close()

# install extension - toolbox or module
def install_extension(url):
    gisbase = os.getenv('GISBASE')
    if not gisbase:
        grass.fatal(_('$GISBASE not defined'))

    if options['extension'] in get_installed_extensions(force = True):
        grass.warning(_("Extension <%s> already installed. Re-installing...") % options['extension'])

    if flags['t']:
        grass.message(_("Installing toolbox <%s>...") % options['extension'])
        mlist = get_toolbox_modules(url, options['extension'])
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
            ret += install_extension_other(module)
        if len(mlist) > 1:
            print '-' * 60
    
    if flags['d']:
        return

    if ret != 0:
        grass.warning(_('Installation failed, sorry. Please check above error messages.'))
    else:
        grass.message(_("Updating metadata file..."))
        blist = install_extension_xml(url, mlist)
        for module in blist:
            update_manual_page(module)
        
        grass.message(_("Installation of <%s> successfully finished") % options['extension'])
    
    if not os.getenv('GRASS_ADDON_BASE'):
        grass.warning(_('This add-on module will not function until you set the '
                        'GRASS_ADDON_BASE environment variable (see "g.manual variables")'))

# update local meta-file when installing new extension (toolbox / modules)
def install_toolbox_xml(url, name):
    # read metadata from remote server (toolboxes)
    url = url + "toolboxes.xml"

    data = dict()
    try:
        f = urlopen(url)
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
                'name'      : tnode.get('name'),
                'correlate' : clist,
                'modules'   : mlist,
                }
    except HTTPError:
        grass.error(_("Unable to read metadata file from the remote server"))

    if not data:
        grass.warning(_("No metadata available"))
        return
    if name not in data:
        grass.warning(_("No metadata available for <%s>") % name)
        return

    fXML = os.path.join(options['prefix'], 'toolboxes.xml')
    # create an empty file if not exists
    if not os.path.exists(fXML):
        write_xml_modules(fXML)

    # read XML file
    fo = open(fXML, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()

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
        tnode = etree.Element('toolbox', attrib = { 'name' : tdata['name'], 'code' : name })
        tree.append(tnode)

    for cname in tdata['correlate']:
        cnode = etree.Element('correlate', attrib = { 'code' : cname })
        tnode.append(cnode)
    for tname in tdata['modules']:
        mnode = etree.Element('task', attrib = { 'name' : tname })
        tnode.append(mnode)

    write_xml_toolboxes(fXML, tree)

# return list of executables for update_manual_page()
def install_extension_xml(url, mlist):
    if len(mlist) > 1:
        # read metadata from remote server (toolboxes)
        install_toolbox_xml(url, options['extension'])

    # read metadata from remote server (modules)
    url = url + "modules.xml"

    data = {}
    bList = []
    try:
        f = urlopen(url)
        try:
            tree = etree.fromstring(f.read())
        except:
            grass.warning(_("Unable to parse '%s'. Metadata file not updated.") % url)
            return bList

        for mnode in tree.findall('task'):
            name = mnode.get('name')
            if name not in mlist:
                continue

            fList = list()
            bnode = mnode.find('binary')
            windows = sys.platform == 'win32'
            if bnode is not None:
                for fnode in bnode.findall('file'):
                    path = fnode.text.split('/')
                    if path[0] == 'bin':
                        bList.append(path[-1])
                        if windows:
                            path[-1] += '.exe'
                    elif path[0] == 'scripts':
                        bList.append(path[-1])
                        if windows:
                            path[-1] += '.py'
                    fList.append(os.path.sep.join(path))
            
            desc, keyw = get_optional_params(mnode)
            
            data[name] = {
                'desc'  : desc,
                'keyw'  : keyw,
                'files' : fList,
                }

    except HTTPError:
        grass.error(_("Unable to read metadata file from the remote server"))

    if not data:
        grass.warning(_("No metadata available"))
        return []

    fXML = os.path.join(options['prefix'], 'modules.xml')
    # create an empty file if not exists
    if not os.path.exists(fXML):
        write_xml_modules(fXML)

    # read XML file
    fo = open(fXML, 'r')
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
            grass.warning(_("No metadata found for <%s>") % name)
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
            for f in ndata['files']:
                fnode = etree.Element('file')
                fnode.text = f
                bnode.append(fnode)
            tnode.append(bnode)
        else:
            # create new node for task
            tnode = etree.Element('task', attrib = { 'name' : name })
            dnode = etree.Element('description')
            dnode.text = ndata['desc']
            tnode.append(dnode)
            knode = etree.Element('keywords')
            knode.text = ndata['keyw']
            tnode.append(knode)
            bnode = etree.Element('binary')
            for f in ndata['files']:
                fnode = etree.Element('file')
                fnode.text = f
                bnode.append(fnode)
            tnode.append(bnode)
            tree.append(tnode)

    write_xml_modules(fXML, tree)

    return bList

# install extension on MS Windows
def install_extension_win(name):
    ### TODO: do not use hardcoded url - http://wingrass.fsv.cvut.cz/grassXX/addonsX.X.X
    grass.message(_("Downloading precompiled GRASS Addons <%s>...") % options['extension'])
    url = "http://wingrass.fsv.cvut.cz/grass%s%s/addons" % (version[0], version[1])
    grass.debug("url=%s" % url, 1)

    try:
        f = urlopen(url + '/' + name + '.zip')

        # create addons dir if not exists
        if not os.path.exists(options['prefix']):
            os.mkdir(options['prefix'])

        # download data
        fo = tempfile.TemporaryFile()
        fo.write(f.read())
        zfobj = zipfile.ZipFile(fo)
        for name in zfobj.namelist():
            if name.endswith('/'):
                d = os.path.join(options['prefix'], name)
                if not os.path.exists(d):
                    os.mkdir(d)
            else:
                outfile = open(os.path.join(options['prefix'], name), 'wb')
                outfile.write(zfobj.read(name))
                outfile.close()

        fo.close()
    except HTTPError:
        grass.fatal(_("GRASS Addons <%s> not found") % name)

    return 0

# install extension on other plaforms
def install_extension_other(name):
    gisbase = os.getenv('GISBASE')
    classchar = name.split('.', 1)[0]
    moduleclass = expand_module_class_name(classchar)
    url = options['svnurl'] + '/' + moduleclass + '/' + name
    if classchar == 'wx' and not flags['s']:
        grass.fatal(_("Installation of wxGUI extension requires -%s flag.") % 's')

    grass.message(_("Fetching <%s> from GRASS-Addons SVN (be patient)...") % name)

    os.chdir(tmpdir)
    if grass.verbosity() <= 2:
        outdev = open(os.devnull, 'w')
    else:
        outdev = sys.stdout

    if grass.call(['svn', 'checkout',
                   url], stdout = outdev) != 0:
        grass.fatal(_("GRASS Addons <%s> not found") % name)

    dirs = { 'bin'     : os.path.join(tmpdir, name, 'bin'),
             'docs'    : os.path.join(tmpdir, name, 'docs'),
             'html'    : os.path.join(tmpdir, name, 'docs', 'html'),
             'rest'    : os.path.join(tmpdir, name, 'docs', 'rest'),
             'man'     : os.path.join(tmpdir, name, 'docs', 'man', 'man1'),
             'scripts' : os.path.join(tmpdir, name, 'scripts'),
             'etc'     : os.path.join(tmpdir, name, 'etc'),
             }

    if classchar != 'wx':
        makeCmd = ['make',
                   'MODULE_TOPDIR=%s' % gisbase.replace(' ', '\ '),
                   'BIN=%s' % dirs['bin'],
                   'HTMLDIR=%s' % dirs['html'],
                   'RESTDIR=%s' % dirs['rest'],
                   'MANDIR=%s' % dirs['man'],
                   'SCRIPTDIR=%s' % dirs['scripts'],
                   'ETC=%s' % os.path.join(dirs['etc'], name)
                   ]
    else:
        makeCmd = ['make',
                   'MODULE_TOPDIR=%s' % gisbase.replace(' ', '\ ')
                   ]

    installCmd = ['make',
                  'MODULE_TOPDIR=%s' % gisbase,
                  'ARCH_DISTDIR=%s' % os.path.join(tmpdir, name),
                  'INST_DIR=%s' % options['prefix'],
                  'install'
                  ]

    if flags['d']:
        grass.message(_("To compile run:"))
        sys.stderr.write(' '.join(makeCmd) + '\n')
        grass.message(_("To install run:"))
        sys.stderr.write(' '.join(installCmd) + '\n')
        return 0

    os.chdir(os.path.join(tmpdir, name))

    grass.message(_("Compiling..."))
    if 0 != grass.call(makeCmd,
                       stdout = outdev):
        grass.fatal(_('Compilation failed, sorry. Please check above error messages.'))

    if flags['i'] or classchar == 'wx':
        return 0

    grass.message(_("Installing..."))

    return grass.call(installCmd,
                      stdout = outdev)

# remove existing extension - toolbox or module
def remove_extension(force = False):
    if flags['t']:
        mlist = get_toolbox_modules(options['extension'])
    else:
        mlist = [options['extension']]

    if force:
        grass.verbose(_("List of removed files:"))
    else:
        grass.info(_("Files to be removed (use flag 'f' to force removal):"))

    remove_modules(mlist, force)

    if force:
        grass.message(_("Updating metadata file..."))
        remove_extension_xml(mlist)
        grass.message(_("Extension <%s> successfully uninstalled.") % options['extension'])
    else:
        grass.warning(_("Extension <%s> not removed.\n"
                        "Re-run '%s' with 'f' flag to force removal") % (options['extension'], 'g.extension'))

# remove existing extension(s) (reading XML file)
def remove_modules(mlist, force = False):
    # try to read XML metadata file first
    fXML = os.path.join(options['prefix'], 'modules.xml')
    installed = get_installed_modules()

    if os.path.exists(fXML):
        f = open(fXML, 'r')
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
                    for f in task.find('binary').findall('file'):
                        flist.append(f.text)
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
                            print fpath
                    except OSError:
                        err.append((_("Unable to remove file '%s'") % fpath))
                if force and not removed:
                    grass.fatal(_("Extension <%s> not found") % name)

                if err:
                    for e in err:
                        grass.error(e)
            else:
                remove_extension_std(name, force)
        else:
            remove_extension_std(name, force)

# remove exising extension (using standard files layout)
def remove_extension_std(name, force = False):
    for fpath in [os.path.join(options['prefix'], 'bin', name),
                  os.path.join(options['prefix'], 'scripts', name),
                  os.path.join(options['prefix'], 'docs', 'html', name + '.html'),
                  os.path.join(options['prefix'], 'docs', 'rest', name + '.txt'),
                  os.path.join(options['prefix'], 'docs', 'man', 'man1', name + '.1')]:
        if os.path.isfile(fpath):
            if force:
                grass.verbose(fpath)
                os.remove(fpath)
            else:
                print fpath

# update local meta-file when removing existing extension
def remove_toolbox_xml(name):
    fXML = os.path.join(options['prefix'], 'toolboxes.xml')
    if not os.path.exists(fXML):
        return

    # read XML file
    fo = open(fXML, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()

    for node in tree.findall('toolbox'):
        if node.get('code') != name:
            continue
        tree.remove(node)

    write_xml_toolboxes(fXML, tree)

def remove_extension_xml(modules):
    if len(modules) > 1:
        # update also toolboxes metadata
        remove_toolbox_xml(options['extension'])

    fXML = os.path.join(options['prefix'], 'modules.xml')
    if not os.path.exists(fXML):
        return

    # read XML file
    fo = open(fXML, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()

    for name in modules:
        for node in tree.findall('task'):
            if node.get('name') != name:
                continue
            tree.remove(node)

    write_xml_modules(fXML, tree)

# check links in CSS
def check_style_files(fil):
    dist_file   = os.path.join(os.getenv('GISBASE'), 'docs', 'html', fil)
    addons_file = os.path.join(options['prefix'], 'docs', 'html', fil)

    if os.path.isfile(addons_file):
        return

    try:
        shutil.copyfile(dist_file, addons_file)
    except OSError, e:
        grass.fatal(_("Unable to create '%s': %s") % (addons_file, e))

def create_dir(path):   
    if os.path.isdir(path):              
        return                   

    try:                 
        os.makedirs(path)                
    except OSError, e:                   
        grass.fatal(_("Unable to create '%s': %s") % (path, e))                  

    grass.debug("'%s' created" % path)

def check_dirs():
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
    if module.split('.', 1)[0] == 'wx':
        return # skip for GUI modules

    grass.verbose(_("Manual page for <%s> updated") % module)
    # read original html file
    htmlfile = os.path.join(options['prefix'], 'docs', 'html', module + '.html')
    try:
        f = open(htmlfile)
        shtml = f.read()
    except IOError, e:
        grass.fatal(_("Unable to read manual page: %s") % e)
    else:
        f.close()
    
    # find URIs
    pattern = r'''<a href="([^"]+)">([^>]+)</a>'''
    addons = get_installed_extensions(force = True)
    pos = []
    for match in re.finditer(pattern, shtml):
        if match.group(1)[:7] == 'http://':
            continue
        if match.group(1).replace('.html', '') in addons:
            continue
        pos.append(match.start(1))

    if not pos:
        return # no match

    # replace file URIs
    prefix = 'file://' + '/'.join([os.getenv('GISBASE'), 'docs', 'html'])
    ohtml = shtml[:pos[0]]
    for i in range(1, len(pos)):
        ohtml += prefix + '/' + shtml[pos[i-1]:pos[i]]
    ohtml += prefix + '/' + shtml[pos[-1]:]

    # write updated html file
    try:
        f = open(htmlfile, 'w')
        f.write(ohtml)
    except IOError, e:
        grass.fatal(_("Unable for write manual page: %s") % e)
    else:
        f.close()

def main():
    # check dependecies
    if sys.platform != "win32":
        check_progs()

    # define path
    if flags['s']:
        options['prefix'] = os.environ['GISBASE']
    if options['prefix'] == '$GRASS_ADDON_BASE':
        if not os.getenv('GRASS_ADDON_BASE'):
            grass.warning(_("GRASS_ADDON_BASE is not defined, "
                            "installing to ~/.grass%s/addons") % version[0])
            options['prefix'] = os.path.join(os.environ['HOME'], '.grass%s' % version[0], 'addons')
        else:
            options['prefix'] = os.environ['GRASS_ADDON_BASE']
    if options['svnurl'] == 'http://svn.osgeo.org/grass/grass-addons/grass7':
        xmlurl = "http://grass.osgeo.org/addons/grass%s" % version[0]
    else:
        xmlurl = options['svnurl']

    if not xmlurl.endswith('/'):
        xmlurl = xmlurl + "/"

    # list available extensions
    if flags['l'] or flags['c'] or flags['g']:
        list_available_extensions(xmlurl)
        return 0
    elif flags['a']:
        elist = get_installed_extensions()
        if elist:
            if flags['t']:
                grass.message(_("List of installed extensions (toolboxes):"))
            else:
                grass.message(_("List of installed extensions (modules):"))
            sys.stdout.write('\n'.join(elist))
            sys.stdout.write('\n')
        else:
            if flags['t']:
                grass.info(_("No extension (toolbox) installed"))
            else:
                grass.info(_("No extension (module) installed"))
        return 0
    else:
        if not options['extension']:
            grass.fatal(_('You need to define an extension name or use -l/c/g/a'))

    if flags['d']:
        if options['operation'] != 'add':
            grass.warning(_("Flag 'd' is relevant only to 'operation=add'. Ignoring this flag."))
        else:
            global remove_tmpdir
            remove_tmpdir = False

    if options['operation'] == 'add':
        check_dirs()
        install_extension(xmlurl)
    else: # remove
        remove_extension(flags['f'])

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    global tmpdir
    tmpdir = grass.tempdir()
    atexit.register(cleanup)
    version = grass.version()['version'].split('.')
    sys.exit(main())
