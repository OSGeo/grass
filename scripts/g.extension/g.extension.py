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
#% description: List available modules in the GRASS Addons SVN repository
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: c
#% description: List available modules in the GRASS Addons SVN repository including module description
#% guisection: Print
#% suppress_required: yes
#%end
#%flag
#% key: g
#% description: List available modules in the GRASS Addons SVN repository (shell script style)
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
def check():
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

# list modules (read XML file from grass.osgeo.org/addons)
def list_available_modules():
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
        return list_available_modules_svn()
    
    return mlist

# list modules (scan SVN repo)
def list_available_modules_svn():
    mlist = list()
    grass.message(_('Fetching list of modules from GRASS-Addons SVN (be patient)...'))
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
            # list modules
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
        # list modules
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
    
# install extension
def install_extension():
    gisbase = os.getenv('GISBASE')
    if not gisbase:
        grass.fatal(_('$GISBASE not defined'))
    
    if grass.find_program(options['extension'], ['--help']):
        grass.warning(_("Extension <%s> already installed. Will be updated...") % options['extension'])

    if sys.platform == "win32":
        install_extension_win()
    else:
        install_extension_other()
    
    # manual page: fix href
    if os.getenv('GRASS_ADDON_PATH'):
        html_man = os.path.join(os.getenv('GRASS_ADDON_PATH'), 'docs', 'html', options['extension'] + '.html')
        if os.path.exists(html_man):
            fd = open(html_man)
            html_str = '\n'.join(fd.readlines())
            fd.close()
            for rep in ('grassdocs.css', 'grass_logo.png'):
                patt = re.compile(rep, re.IGNORECASE)
                html_str = patt.sub(os.path.join(gisbase, 'docs', 'html', rep),
                                    html_str)
                
            patt = re.compile(r'(<a href=")(d|db|g|i|m|p|ps|r|r3|s|v|wxGUI)(\.)(.+)(.html">)', re.IGNORECASE)
            while True:
                m = patt.search(html_str)
                if not m:
                    break
                html_str = patt.sub(m.group(1) + os.path.join(gisbase, 'docs', 'html',
                                                              m.group(2) + m.group(3) + m.group(4)) + m.group(5),
                                    html_str, count = 1)
            fd = open(html_man, "w")
            fd.write(html_str)
            fd.close()

    # symlink for binaries needed, see http://trac.osgeo.org/grass/changeset/49124
    src = None
    if sys.platform == 'win32':
        bin_ext = '.exe'
        sct_ext  = '.py'
    else:
        bin_ext = sct_ext = ''
    
    if os.path.exists(os.path.join(options['prefix'], 'bin',
                                   options['extension'] + bin_ext)):
        src = os.path.join(options['prefix'], 'bin', options['extension']) + bin_ext
        dst = os.path.join(options['prefix'], options['extension']) + bin_ext
    elif os.path.exists(os.path.join(options['prefix'], 'scripts',
                                     options['extension'] + sct_ext)):
        src = os.path.join(options['prefix'], 'scripts', options['extension']) + sct_ext
        dst = os.path.join(options['prefix'], options['extension']) + sct_ext
    
    if src and not os.path.exists(dst):
        if sys.platform == 'win32':
            shutil.copyfile(src, dst)
        else:
            os.symlink(src, dst)
    
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
    
    ret = grass.call(installCmd,
                     stdout = outdev)
    
    if ret != 0:
        grass.warning(_('Installation failed, sorry. Please check above error messages.'))
    else:
        grass.message(_("Installation of <%s> successfully finished.") % options['extension'])
    
    # manual page: fix href
    if os.getenv('GRASS_ADDON_PATH'):
        html_man = os.path.join(os.getenv('GRASS_ADDON_PATH'), 'docs', 'html', options['extension'] + '.html')
        if os.path.exists(html_man):
            fd = open(html_man)
            html_str = '\n'.join(fd.readlines())
            fd.close()
            for rep in ('grassdocs.css', 'grass_logo.png'):
                patt = re.compile(rep, re.IGNORECASE)
                html_str = patt.sub(os.path.join(gisbase, 'docs', 'html', rep),
                                    html_str)
                
            patt = re.compile(r'(<a href=")(d|db|g|i|m|p|ps|r|r3|s|v|wxGUI)(\.)(.+)(.html">)', re.IGNORECASE)
            while True:
                m = patt.search(html_str)
                if not m:
                    break
                html_str = patt.sub(m.group(1) + os.path.join(gisbase, 'docs', 'html',
                                                              m.group(2) + m.group(3) + m.group(4)) + m.group(5),
                                    html_str, count = 1)
            fd = open(html_man, "w")
            fd.write(html_str)
            fd.close()
    
    if not os.environ.has_key('GRASS_ADDON_PATH') or \
            not os.environ['GRASS_ADDON_PATH']:
        grass.warning(_('This add-on module will not function until you set the '
                        'GRASS_ADDON_PATH environment variable (see "g.manual variables")'))

# remove existing extension (reading XML file)
def remove_extension(force = False):
    # try to download XML metadata file first
    url = "http://grass.osgeo.org/addons/grass%s.xml" % grass.version()['version'].split('.')[0]
    name = options['extension']
    if force:
        grass.verbose(_("List of removed files:"))
    else:
        grass.info(_("Files to be removed (use flag 'f' to force removal):"))
    
    try:
        f = urlopen(url)
        tree = etree.fromstring(f.read())
        flist = []
        for task in tree.findall('task'):
            if name == task.get('name', default = '') and \
                    task.find('binary') is not None:
                for f in task.find('binary').findall('file'):
                    fname = f.text
                    if fname:
                        fpath = fname.split('/')
                        if sys.platform == 'win32':
                            if fpath[0] == 'bin':
                                fpath[-1] += '.exe'
                            if fpath[0] == 'scripts':
                                fpath[-1] += '.py'
                        
                        flist.append(fpath)
        
        if flist:
            removed = False
            err = list()
            for f in flist:
                fpath = os.path.join(options['prefix'], os.path.sep.join(f))
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
    except HTTPError:
        remove_extension_std(force)

    if force:
        grass.message(_("Extension <%s> successfully uninstalled.") % options['extension'])
    else:
        grass.warning(_("Extension <%s> not removed. "
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
    # check the links to grassdocs.css/grass_logo.png to a correct manual page of addons
    dist_file = os.path.join(os.getenv('GISBASE'), 'docs', 'html', fil)
    addons_file = os.path.join(options['prefix'], 'docs', 'html', fil)
    # check if file already exists in the grass addons docs html path
    if os.path.isfile(addons_file):
	return
    # otherwise copy the file from $GISBASE/docs/html, it doesn't use link 
    # because os.symlink it work only in Linux
    else:
	try:
	    shutil.copyfile(dist_file,addons_file)
	except OSError, e:
	    grass.fatal(_("Unable to create '%s': %s") % (addons_file, e))

def check_dirs():
    check_style_files('grass_logo.png')
    check_style_files('grassdocs.css')    

def main():
    # check dependecies
    if sys.platform != "win32":
        check()
    
    # list available modules
    if flags['l'] or flags['c'] or flags['g']:
        list_available_modules()
        return 0
    else:
        if not options['extension']:
            grass.fatal(_('You need to define an extension name or use -l'))
    
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
    
    # check dirs
    if options['operation'] == 'add':
        check_dirs()
    
    if flags['d']:
        if options['operation'] != 'add':
            grass.warning(_("Flag 'd' is relevant only to 'operation=add'. Ignoring this flag."))
        else:
            global remove_tmpdir
            remove_tmpdir = False
    
    if options['operation'] == 'add':
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
