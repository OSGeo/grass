"""
@package utils.py

@brief Misc utilities for GRASS wxPython GUI

(C) 2007-2008 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa, Jachym Cepicky

@date 2007-2008
"""

import os
import sys

import grass

import globalvar
import gcmd
import grassenv
try:
    import subprocess
except:
    compatPath = os.path.join(globalvar.ETCWXDIR, "compat")
    sys.path.append(compatPath)
    import subprocess

def GetTempfile(pref=None):
    """
    Creates GRASS temporary file using defined prefix.

    @todo Fix path on MS Windows/MSYS

    @param pref prefer the given path

    @return Path to file name (string) or None
    """
    import gcmd

    tempfileCmd = gcmd.Command(["g.tempfile",
                                "pid=%d" % os.getpid()])

    tempfile = tempfileCmd.ReadStdOutput()[0].strip()

    # FIXME
    # ugly hack for MSYS (MS Windows)
    if subprocess.mswindows:
	tempfile = tempfile.replace("/", "\\")
    try:
        path, file = os.path.split(tempfile)
        if pref:
            return os.path.join(pref, file)
	else:
	    return tempfile
    except:
        return None

def GetLayerNameFromCmd(dcmd, fullyQualified=False, param=None,
                        layerType=None):
    """Get map name from GRASS command

    @param dcmd GRASS command (given as list)
    @param fullyQualified change map name to be fully qualified
    @param force parameter otherwise 'input'/'map'
    @param update change map name in command
    @param layerType check also layer type ('raster', 'vector', '3d-raster', ...)
    
    @return map name
    @return '' if no map name found in command
    """
    mapname = ''

    if len(dcmd) < 1:
        return mapname
    
    if 'd.grid' == dcmd[0]:
        mapname = 'grid'
    elif 'd.geodesic' in dcmd[0]:
        mapname = 'geodesic'
    elif 'd.rhumbline' in dcmd[0]:
        mapname = 'rhumb'
    elif 'labels=' in dcmd[0]:
        mapname = dcmd[idx].split('=')[1]+' labels'
    else:
        for idx in range(len(dcmd)):
            if param and param in dcmd[idx]:
                break
            elif not param:
                if 'map=' in dcmd[idx] or \
                        'input=' in dcmd[idx] or \
                        'red=' in dcmd[idx] or \
                        'h_map=' in dcmd[idx] or \
                        'reliefmap' in dcmd[idx]:
                    break
            
        if idx < len(dcmd):
            mapname = dcmd[idx].split('=')[1]
            if fullyQualified and '@' not in mapname:
                if layerType in ('raster', 'vector', '3d-raster'):
                    try:
                        if layerType == 'raster':
                            findType = 'cell'
                        else:
                            findType = layerType
                        result = grass.find_file(mapname, element=findType)
                    except AttributeError, e: # not found
                        return ''
                    if result:
                        mapname = result['fullname']
                    else:
                        mapname += '@' + grassenv.GetGRASSVariable('MAPSET')
                else:
                    mapname += '@' + grassenv.GetGRASSVariable('MAPSET')
                dcmd[idx] = dcmd[idx].split('=')[0] + '=' + mapname
                
    return mapname

def GetValidLayerName(name):
    """Make layer name SQL compliant, based on G_str_to_sql()
    
    @todo: Better use directly GRASS Python SWIG...
    """
    retName = str(name).strip()
    
    # check if name is fully qualified
    if '@' in retName:
        retName, mapset = retName.split('@')
    else:
        mapset = None
        
    for c in retName:
        # c = toascii(c)
        
        if not (c >= 'A' and c <= 'Z') and \
               not (c >= 'a' and c <= 'z') and \
               not (c >= '0' and c <= '9'):
            c = '_'
        
    if not (retName[0] >= 'A' and retName[0] <= 'Z') and \
           not (retName[0] >= 'a' and retName[0] <= 'z'):
        retName = 'x' + retName[1:]

    if mapset:
        retName = retName + '@' + mapset
        
    return retName

def ListOfCatsToRange(cats):
    """Convert list of category number to range(s)

    Used for example for d.vect cats=[range]

    @param cats category list

    @return category range string
    @return '' on error
    """

    catstr = ''

    try:
        cats = map(int, cats)
    except:
        return catstr

    i = 0
    while i < len(cats):
        next = 0
        j = i + 1
        while j < len(cats):
            if cats[i + next] == cats[j] - 1:
                next += 1
            else:
                break
            j += 1

        if next > 1:
            catstr += '%d-%d,' % (cats[i], cats[i + next])
            i += next + 1
        else:
            catstr += '%d,' % (cats[i])
            i += 1
        
    return catstr.strip(',')

def ListOfMapsets(all=False):
    """Get list of available/accessible mapsets

    @param all if True get list of all mapsets

    @return list of mapsets
    """
    mapsets = []

    ### FIXME
    # problem using Command here (see preferences.py)
    # cmd = gcmd.Command(['g.mapsets', '-l'])
    if all:
        cmd = subprocess.Popen(['g.mapsets' + globalvar.EXT_BIN, '-l'],
                               stdout=subprocess.PIPE)
    
        try:
            # for mset in cmd.ReadStdOutput()[0].split(' '):
            for line in cmd.stdout.readlines():
                for mset in line.strip('%s' % os.linesep).split(' '):
                    if len(mset) == 0:
                        continue
                    mapsets.append(mset)
        except:
            raise gcmd.CmdError(_('Unable to get list of available mapsets.'))
    
    else:
        # cmd = gcmd.Command(['g.mapsets', '-p'])
        cmd = subprocess.Popen(['g.mapsets' + globalvar.EXT_BIN, '-p'],
                               stdout=subprocess.PIPE)
        try:
            # for mset in cmd.ReadStdOutput()[0].split(' '):
            for line in cmd.stdout.readlines():
                for mset in line.strip('%s' % os.linesep).split(' '):
                    if len(mset) == 0:
                        continue
                    mapsets.append(mset)
        except:
            raise gcmd.CmdError(_('Unable to get list of accessible mapsets.'))

        ListSortLower(mapsets)
    
    return mapsets

def ListSortLower(list):
    """Sort list items (not case-sensitive)"""
    list.sort(cmp=lambda x, y: cmp(x.lower(), y.lower()))

def reexec_with_pythonw():
    """Re-execute Python on Mac OS"""
    if sys.platform == 'darwin' and \
            not sys.executable.endswith('MacOS/Python'):
        print >> sys.stderr, 're-executing using pythonw'
        os.execvp('pythonw', ['pythonw', __file__] + sys.argv[1:])
