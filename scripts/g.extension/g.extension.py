#!/usr/bin/env python

############################################################################
#
# MODULE:       g.extension.add
# AUTHOR(S):   	Markus Neteler
#               Pythonized by Martin Landa
# PURPOSE:      tool to download and install extensions from GRASS Addons SVN into 
#               local GRASS installation
# COPYRIGHT:    (C) 2009 by the Markus Neteler, GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
# TODO: add sudo support where needed (i.e. check first permission to write into
#       $GISBASE directory)
#############################################################################

#%Module
#%  description: Tool to download and install extensions from GRASS Addons SVN repository into local GRASS installation.
#%  keywords: installation, extensions
#%End

#%option
#% key: extension
#% type: string
#% key_desc : name
#% description: Name of extension to install from GRASS Addons SVN repository
#% required : no
#%end
#%flag
#%  key: l
#%  description: List available modules in the GRASS Addons SVN repository
#%end

import os
import sys
import re
import atexit

import urllib

from grass.script import core as grass

# definitions
svnurl_addons = "https://svn.osgeo.org/grass/grass-addons/"

# temp dir
tmpdir = grass.tempfile()
grass.try_remove(tmpdir)
os.mkdir(tmpdir)

def check():
    # check if we have the svn client
    if not grass.find_program('svn'):
        grass.fatal('svn client required. Please install subversion first.')

def expand_module_class_name(c):
    name = { 'd'  : 'display',
             'db' : 'database',
             'g'  : 'general',
             'i'  : 'imagery',
             'm'  : 'misc',
             'ps' : 'postscript',
             'p'  : 'paint',
             'r'  : 'raster',
             'r3' : 'raster3D',
             's'  : 'sites',
             'v'  : 'vector' }
    
    if name.has_key(c):
        return name[c]
    
    return c

def list_available_modules():
    global svnurl_addons
    
    grass.message('Fetching list of modules from GRASS-Addons SVN (be patient)...')
    pattern = re.compile(r'(<li><a href=".+">)(.+)(</a></li>)', re.IGNORECASE)
    for d in ['d', 'db', 'g', 'i', 'ps',
              'p', 'r', 'r3', 'v']:
        modclass = expand_module_class_name(d)
        url = svnurl_addons + modclass
        f = urllib.urlopen(url)
        if not f:
            grass.warning("Unable to fetch '%s'" % url)
            continue
        for line in f.readlines():
            sline = pattern.search(line)
            if sline and sline.group(2).split('.', 1)[0] == d:
                print sline.group(2).rstrip('/')

def cleanup():
    global tmpdir
    grass.try_rmdir(tmpdir)

def main():
    # check dependecies
    check()

    # list available modules
    if flags['l']:
        list_available_modules()
        return 0
    else:
        if not options['extension']:
            grass.fatal('You need to define an extension name or use -l')
    
    module = options['extension']
    classchar = module.split('.', 1)[0]
    moduleclass = expand_module_class_name(classchar)
    global svnurl_addons
    url = svnurl_addons + moduleclass + '/' + module
    global tmpdir
    print tmpdir
    grass.message("Fetching '%s' from GRASS-Addons SVN (be patient)..." % module)
    os.chdir(tmpdir)
    if grass.call(['svn', 'checkout',
                   url]) != 0:
        grass.fatal("GRASS Addons '%s' not found in repository" % module)

    os.chdir(os.path.join(tmpdir, module))
    grass.message("Compiling '%s'..." % module)
    gisbase = os.getenv('GISBASE')
    if grass.call(['make',
                   'MODULE_TOPDIR=%s' % gisbase]) != 0:
        cleanup()
        grass.fatal('Compilation failed, sorry. Please check above error messages')
    
    grass.message("Installing '%s'..." % module)
    # can we write ?
    try:
        # replace with something better
        file = os.path.join(gisbase, 'test')
        f = open(file, "w")
        f.close()
        os.remove(file)

        ret = grass.call(['make'
                          'MODULE_TOPDIR=%s' % gisbase,
                          'install'])
    except IOError:
        ret = grass.call(['sudo', 'make'
                          'MODULE_TOPDIR=%s' % gisbase,
                          'install'])
    
    if ret != 0:
        cleanup()
        grass.fatal('Installation failed, sorry. Please check above error messages.')
    
    grass.message("Installation of '%s' successfully finished." % module)
    cleanup()
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
