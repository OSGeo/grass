#!/usr/bin/env python

############################################################################
#
# MODULE:       g.extension
# AUTHOR(S):   	Markus Neteler
#               Pythonized by Martin Landa
# PURPOSE:      Tool to download and install extensions from GRASS Addons SVN into 
#               local GRASS installation
# COPYRIGHT:    (C) 2009-2011 by Markus Neteler, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
# TODO: add sudo support where needed (i.e. check first permission to write into
#       $GISBASE directory)
#############################################################################

#%module
#% label: Tool to maintain the extensions in local GRASS installation.
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
#% required: yes
#% answer: http://svn.osgeo.org/grass/grass-addons/grass7
#%end
#%option
#% key: prefix
#% type: string
#% key_desc: path
#% description: Prefix where to install extension (ignored when flag -s is given)
#% answer: $GRASS_ADDON_PATH
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
#%end
#%flag
#% key: d
#% description: Download source code and exit
#%end
#%flag
#% key: i
#% description: Don't install new extension, just compile it
#%end
#%flag
#% key: f
#% description: Force removal when uninstalling extension (operation=remove)
#%end

import os
import sys
import re
import atexit
import shutil
import glob
import zipfile
import tempfile
import shutil

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
             'gui' : 'gui/wxpython' }
    
    if name.has_key(c):
        return name[c]
    
    return c

# list installed extensions
def get_installed_extensions(force = False):
    fXML = os.path.join(options['prefix'], 'modules.xml')
    if not os.path.exists(fXML):
        if force:
            write_xml_modules(fXML)
        else:
            grass.warning(_("No metadata file available"))
        return []
    
    # read XML file
    fo = open(fXML, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()
    
    ret = list()
    for tnode in tree.findall('task'):
        ret.append(tnode.get('name'))
    
    return ret

# list extensions (read XML file from grass.osgeo.org/addons)
def list_available_extensions():
    mlist = list()
    
    # try to download XML metadata file first
    url = "http://grass.osgeo.org/addons/grass%s.xml" % grass.version()['version'].split('.')[0]
    try:
        f = urlopen(url)
        tree = etree.fromstring(f.read())
        for mnode in tree.findall('task'):
            name = mnode.get('name')
            if flags['c'] or flags['g']:
                desc = mnode.find('description').text
                if not desc:
                    desc = ''
                keyw = mnode.find('keywords').text
                if not keyw:
                    keyw = ''
            
            if flags['g']:
                print 'name=' + name
                print 'description=' + desc
                print 'keywords=' + keyw
            elif flags['c']:
                print name + ' - ' + desc
            else:
                print name
    except HTTPError:
        return list_available_extensions_svn()
    
    return mlist

# list extensions (scan SVN repo)
def list_available_extensions_svn():
    mlist = list()
    grass.message(_('Fetching list of extensions from GRASS-Addons SVN (be patient)...'))
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)

    if flags['c']:
        grass.warning(_("Flag 'c' ignored, metadata file not available"))
    if flags['g']:
        grass.warning(_("Flag 'g' ignored, metadata file not available"))
        
    prefix = ['d', 'db', 'g', 'i', 'm', 'ps',
              'p', 'r', 'r3', 's', 'v']
    nprefix = len(prefix)
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
                mlist.append(name)
    
    mlist += list_wxgui_extensions()
        
    return mlist

# list wxGUI extensions
def list_wxgui_extensions(print_module = True):
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
            if print_module:
                print name
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
    version = grass.version()['version'].split('.')[0]
    fo.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    fo.write('<!DOCTYPE task SYSTEM "grass-addons.dtd">\n')
    fo.write('<addons version="%s">\n' % version)
    
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
            libgisRev = grass.version()['libgis_revision']
            fo.write('%s<libgis revision="%s" />\n' % \
                         (' ' * indent, libgisRev))
            indent -= 4
            fo.write('%s</task>\n' % (' ' * indent))
    
    fo.write('</addons>\n')
    fo.close()
    
# update local meta-file when installing new extension
def install_extension_xml():
    # read metadata from remote server
    url = "http://grass.osgeo.org/addons/grass%s.xml" % grass.version()['version'].split('.')[0]
    data = None
    try:
        f = urlopen(url)
        tree = etree.fromstring(f.read())
        for mnode in tree.findall('task'):
            name = mnode.get('name')
            if name != options['extension']:
                continue
            
            fList = list()
            bnode = mnode.find('binary')
            windows = sys.platform == 'win32'
            if bnode is not None:
                for fnode in bnode.findall('file'):
                    path = fnode.text.split('/')
                    if windows:
                        if path[0] == 'bin':
                            path[-1] += '.exe'
                        if path[0] == 'scripts':
                            path[-1] += '.py'
                    fList.append(os.path.sep.join(path))
            
            data = { 'name'  : name,
                     'desc'  : mnode.find('description').text,
                     'keyw'  : mnode.find('keywords').text,
                     'files' : fList,
                     }
    except HTTPError:
        grass.error(_("Unable to read metadata file from the remote server"))
    
    if not data:
        grass.warning(_("No metadata available"))
        return

    fXML = os.path.join(options['prefix'], 'modules.xml')
    # create an empty file if not exists
    if not os.path.exists(fXML):
        write_xml_modules(fXML)
    
    # read XML file
    fo = open(fXML, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()
    
    # update tree
    tnode = None
    for node in tree.findall('task'):
        if node.get('name') == options['extension']:
            tnode = node
            break
    
    if tnode is not None:
        # update existing node
        dnode = tnode.find('description')
        if dnode is not None:
            dnode.text = data['desc']
        knode = tnode.find('keywords')
        if knode is not None:
            knode.text = data['keyw']
        bnode = tnode.find('binary')
        if bnode is not None:
            tnode.remove(bnode)
        bnode = etree.Element('binary')
        for f in data['files']:
            fnode = etree.Element('file')
            fnode.text = f
            bnode.append(fnode)
        tnode.append(bnode)
    else:
        # create new node for task
        tnode = etree.Element('task', attrib = { 'name' : data['name'] })
        dnode = etree.Element('description')
        dnode.text = data['desc']
        tnode.append(dnode)
        knode = etree.Element('keywords')
        knode.text = data['keyw']
        tnode.append(knode)
        bnode = etree.Element('binary')
        for f in data['files']:
            fnode = etree.Element('file')
            fnode.text = f
            bnode.append(fnode)
        tnode.append(bnode)
        tree.append(tnode)
    
    write_xml_modules(fXML, tree)
    
# install extension on MS Windows
def install_extension_win():
    ### TODO: do not use hardcoded url
    version = grass.version()['version'].split('.')
    url = "http://wingrass.fsv.cvut.cz/grass%s%s/addons/" % (version[0], version[1])
    grass.message(_("Downloading precompiled GRASS Addons <%s>...") % options['extension'])
    try:
        f = urlopen(url + options['extension'] + '.zip')
        
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
        grass.fatal(_("GRASS Addons <%s> not found") % options['extension'])
    
    return 0

# install extension
def install_extension():
    gisbase = os.getenv('GISBASE')
    if not gisbase:
        grass.fatal(_('$GISBASE not defined'))
    
    if options['extension'] in get_installed_extensions(force = True):
        grass.warning(_("Extension <%s> already installed. Re-installing...") % options['extension'])

    if sys.platform == "win32":
        ret = install_extension_win()
    else:
        ret = install_extension_other()
    
    if ret != 0:
        grass.warning(_('Installation failed, sorry. Please check above error messages.'))
    else:
        grass.message(_("Updating metadata file..."))
        install_extension_xml()
        grass.message(_("Installation of <%s> successfully finished") % options['extension'])
    
    if not os.environ.has_key('GRASS_ADDON_PATH') or \
            not os.environ['GRASS_ADDON_PATH']:
        grass.warning(_('This add-on module will not function until you set the '
                        'GRASS_ADDON_PATH environment variable (see "g.manual variables")'))

# install extension on other plaforms
def install_extension_other():
    gisbase = os.getenv('GISBASE')
    gui_list = list_wxgui_extensions(print_module = False)

    if options['extension'] not in gui_list:
        classchar = options['extension'].split('.', 1)[0]
        moduleclass = expand_module_class_name(classchar)
        url = options['svnurl'] + '/' + moduleclass + '/' + options['extension']
    else:
        url = options['svnurl'] + '/gui/wxpython/' + options['extension']
        if not flags['s']:
            grass.fatal(_("Installation of wxGUI extension requires -%s flag.") % 's')
        
    grass.message(_("Fetching <%s> from GRASS-Addons SVN (be patient)...") % options['extension'])
    
    os.chdir(tmpdir)
    if grass.verbosity() == 0:
        outdev = open(os.devnull, 'w')
    else:
        outdev = sys.stdout
    
    if grass.call(['svn', 'checkout',
                   url], stdout = outdev) != 0:
        grass.fatal(_("GRASS Addons <%s> not found") % options['extension'])
    
    dirs = { 'bin' : os.path.join(tmpdir, options['extension'], 'bin'),
             'docs' : os.path.join(tmpdir, options['extension'], 'docs'),
             'html' : os.path.join(tmpdir, options['extension'], 'docs', 'html'),
             'man' : os.path.join(tmpdir, options['extension'], 'man'),
             'man1' : os.path.join(tmpdir, options['extension'], 'man', 'man1'),
             'scripts' : os.path.join(tmpdir, options['extension'], 'scripts'),
             'etc' : os.path.join(tmpdir, options['extension'], 'etc'),
             }
    
    makeCmd = ['make',
               'MODULE_TOPDIR=%s' % gisbase.replace(' ', '\ '),
               'BIN=%s' % dirs['bin'],
               'HTMLDIR=%s' % dirs['html'],
               'MANDIR=%s' % dirs['man1'],
               'SCRIPTDIR=%s' % dirs['scripts'],
               'ETC=%s' % os.path.join(dirs['etc'],options['extension'])
               ]
    
    installCmd = ['make',
                  'MODULE_TOPDIR=%s' % gisbase,
                  'ARCH_DISTDIR=%s' % os.path.join(tmpdir, options['extension']),
                  'INST_DIR=%s' % options['prefix'],
                  'install'
                  ]
    
    if flags['d']:
        grass.message(_("To compile run:"))
        sys.stderr.write(' '.join(makeCmd) + '\n')
        grass.message(_("To install run:\n\n"))
        sys.stderr.write(' '.join(installCmd) + '\n')
        return
    
    os.chdir(os.path.join(tmpdir, options['extension']))
    
    grass.message(_("Compiling <%s>...") % options['extension'])    
    if options['extension'] not in gui_list:
        ret = grass.call(makeCmd,
                         stdout = outdev)
    else:
        ret = grass.call(['make',
                          'MODULE_TOPDIR=%s' % gisbase.replace(' ', '\ ')],
                         stdout = outdev)
    
    if ret != 0:
        grass.fatal(_('Compilation failed, sorry. Please check above error messages.'))

    if flags['i'] or options['extension'] in gui_list:
        return
    
    grass.message(_("Installing <%s>...") % options['extension'])
    
    return grass.call(installCmd,
                      stdout = outdev)
    
# update local meta-file when removing existing extension
def remove_extension_xml():
    fXML = os.path.join(options['prefix'], 'modules.xml')
    if not os.path.exists(fXML):
        return
    
    # read XML file
    fo = open(fXML, 'r')
    tree = etree.fromstring(fo.read())
    fo.close()

    tnode = None
    for node in tree.findall('task'):
        if node.get('name') == options['extension']:
            tnode = node
            break

    if tnode is not None:
        tree.remove(tnode)
        
    write_xml_modules(fXML, tree)
    
# remove existing extension (reading XML file)
def remove_extension(force = False):
    # try to read XML metadata file first
    fXML = os.path.join(options['prefix'], 'modules.xml')
    name = options['extension']
    if name not in get_installed_extensions():
        grass.warning(_("Extension <%s> not found") % name)
    
    if force:
        grass.verbose(_("List of removed files:"))
    else:
        grass.info(_("Files to be removed (use flag 'f' to force removal):"))
    
    if os.path.exists(fXML):
        f = open(fXML, 'r')
        tree = etree.fromstring(f.read())
        flist = []
        for task in tree.findall('task'):
            if name == task.get('name', default = '') and \
                    task.find('binary') is not None:
                for f in task.find('binary').findall('file'):
                    flist.append(f.text)
        
        if flist:
            removed = False
            err = list()
            for fpath in flist:
                try:
                    if force:
                        grass.verbose(fpath)
                        os.remove(fpath)
                        removed = True
                    else:
                        print fpath
                except OSError:
                    err.append((_("Unable to remove file '%s'") % fpath))
            if force and not removed:
                grass.fatal(_("Extension <%s> not found") % options['extension'])
            
            if err:
                for e in err:
                    grass.error(e)
        else:
            remove_extension_std(force)
    else:
        remove_extension_std(force)
    
    if force:
        grass.message(_("Updating metadata file..."))
        remove_extension_xml()
        grass.message(_("Extension <%s> successfully uninstalled.") % options['extension'])
    else:
        grass.warning(_("Extension <%s> not removed.\n"
                        "Re-run '%s' with 'f' flag to force removal") % (options['extension'], 'g.extension'))
    
# remove exising extension (using standard files layout)
def remove_extension_std(force = False):
    # is module available?
    if not os.path.exists(os.path.join(options['prefix'], 'bin', options['extension'])):
        grass.fatal(_("Extension <%s> not found") % options['extension'])
    
    for fpath in [os.path.join(options['prefix'], 'bin', options['extension']),
                  os.path.join(options['prefix'], 'scripts', options['extension']),
                  os.path.join(options['prefix'], 'docs', 'html', options['extension'] + '.html'),
                  os.path.join(options['prefix'], 'man', 'man1', options['extension'] + '.1')]:
        if os.path.isfile(fpath):
            if force:
                grass.verbose(fpath)
                os.remove(fpath)
            else:
                print fpath
    
# check links in CSS
def check_style_files(fil):
    dist_file   = os.path.join(os.getenv('GISBASE'), 'docs', 'html', fil)
    addons_file = os.path.join(options['prefix'], 'docs', 'html', fil)

    if os.path.isfile(addons_file):
	return

    try:
        shutil.copyfile(dist_file,addons_file)
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
    check_style_files('grass_logo.png')
    check_style_files('grassdocs.css')
    create_dir(os.path.join(options['prefix'], 'etc'))
    create_dir(os.path.join(options['prefix'], 'man', 'man1'))
    create_dir(os.path.join(options['prefix'], 'scripts'))

def main():
    # check dependecies
    if sys.platform != "win32":
        check_progs()
    
    # define path
    if flags['s']:
        options['prefix'] = os.environ['GISBASE']
    if options['prefix'] == '$GRASS_ADDON_PATH':
        if not os.environ.has_key('GRASS_ADDON_PATH') or \
                not os.environ['GRASS_ADDON_PATH']:
            major_version = int(grass.version()['version'].split('.', 1)[0])
            grass.warning(_("GRASS_ADDON_PATH is not defined, "
                            "installing to ~/.grass%d/addons/") % major_version)
            options['prefix'] = os.path.join(os.environ['HOME'], '.grass%d' % major_version, 'addons')
        else:
            path_list = os.environ['GRASS_ADDON_PATH'].split(os.pathsep)
            if len(path_list) < 1:
                grass.fatal(_("Invalid GRASS_ADDON_PATH value - '%s'") % os.environ['GRASS_ADDON_PATH'])
            if len(path_list) > 1:
                grass.warning(_("GRASS_ADDON_PATH has more items, using first defined - '%s'") % path_list[0])
            options['prefix'] = path_list[0]
                
    # list available extensions
    if flags['l'] or flags['c'] or flags['g']:
        list_available_extensions()
        return 0
    elif flags['a']:
        grass.message(_("List of installed extensions:"))
        print os.linesep.join(get_installed_extensions())
        return 0
    else:
        if not options['extension']:
            grass.fatal(_('You need to define an extension name or use -l'))
    
    if flags['d']:
        if options['operation'] != 'add':
            grass.warning(_("Flag 'd' is relevant only to 'operation=add'. Ignoring this flag."))
        else:
            global remove_tmpdir
            remove_tmpdir = False
    
    if options['operation'] == 'add':
        check_dirs()
        install_extension()
    else: # remove
        remove_extension(flags['f'])
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    global tmpdir
    tmpdir = grass.tempdir()
    atexit.register(cleanup)
    sys.exit(main())
