"""!
@package utils.py

@brief Misc utilities for wxGUI

(C) 2007-2009 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Jachym Cepicky
"""

import os
import sys
import platform
import string
import glob

import globalvar
grassPath = os.path.join(globalvar.ETCDIR, "python")
sys.path.append(grassPath)

from grass.script import core as grass

import gcmd
try:
    import subprocess
except:
    compatPath = os.path.join(globalvar.ETCWXDIR, "compat")
    sys.path.append(compatPath)
    import subprocess

def normalize_whitespace(text):
    """!Remove redundant whitespace from a string"""
    return string.join( string.split(text), ' ')

def GetTempfile(pref=None):
    """
    Creates GRASS temporary file using defined prefix.

    @todo Fix path on MS Windows/MSYS

    @param pref prefer the given path

    @return Path to file name (string) or None
    """
    import gcmd

    ret = gcmd.RunCommand('g.tempfile',
                          read = True,
                          pid = os.getpid())

    tempfile = ret.splitlines()[0].strip()

    # FIXME
    # ugly hack for MSYS (MS Windows)
    if platform.system() == 'Windows':
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
    """!Get map name from GRASS command

    @param dcmd GRASS command (given as tuple)
    @param fullyQualified change map name to be fully qualified
    @param force parameter otherwise 'input'/'map'
    @param update change map name in command
    @param layerType check also layer type ('raster', 'vector', '3d-raster', ...)
    
    @return map name
    @return '' if no map name found in command
    """
    mapname = ''

    if not dcmd:
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
            try:
                mapname = dcmd[idx].split('=')[1]
            except IndexError:
                return ''
            
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
                        mapname += '@' + grass.gisenv()['MAPSET']
                else:
                    mapname += '@' + grass.gisenv()['MAPSET']
                dcmd[idx] = dcmd[idx].split('=')[0] + '=' + mapname
                
    return mapname

def GetValidLayerName(name):
    """!Make layer name SQL compliant, based on G_str_to_sql()
    
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
    """!Convert list of category number to range(s)

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
    """!Get list of available/accessible mapsets

    @param all if True get list of all mapsets

    @return list of mapsets
    """
    mapsets = []
    
    if all:
        ret = gcmd.RunCommand('g.mapsets',
                              read = True,
                              flags = 'l',
                              fs = ';')
    
        if ret:
            mapsets = ret.rstrip('\n').split(';')
        else:
            raise gcmd.CmdError(cmd = 'g.mapsets',
                                message = _('Unable to get list of available mapsets.'))
    else:
        ret = gcmd.RunCommand('g.mapsets',
                              read = True,
                              flags = 'p',
                              fs = ';')
        if ret:
            mapsets = ret.rstrip('\n').split(';')
        else:
            raise gcmd.CmdError(cmd = 'g.mapsets',
                                message = _('Unable to get list of accessible mapsets.'))
        
        ListSortLower(mapsets)
    
    return mapsets

def ListSortLower(list):
    """!Sort list items (not case-sensitive)"""
    list.sort(cmp=lambda x, y: cmp(x.lower(), y.lower()))

def GetVectorNumberOfLayers(vector):
    """!Get list of vector layers"""
    layers = list()
    
    ret = gcmd.RunCommand('v.category',
                          read = True,
                          input = vector,
                          option = 'report')
    
    if not ret:
        return layers
    
    for line in ret.splitlines():
        if not 'Layer' in line:
            continue
        
        value = line.split(':')[1].strip()
        if '/' in value: # value/name
            layers.append(value.split('/')[0])
        else:
            layers.append(value)
    
    return layers

def Deg2DMS(lon, lat, string = True, hemisphere = True, precision = 3):
    """!Convert deg value to dms string

    @param lon longitude (x)
    @param lat latitude (y)
    @param string True to return string otherwise tuple
    @param hemisphere print hemisphere
    @param precision seconds precision
    
    @return DMS string or tuple of values
    @return empty string on error
    """
    try:
        flat = float(lat)
        flon = float(lon)
    except ValueError:
        if string:
            return ''
        else:
            return None

    # fix longitude
    while flon > 180.0:
        flon -= 360.0
    while flon < -180.0:
        flon += 360.0

    # hemisphere
    if hemisphere:
        if flat < 0.0:
            flat = abs(flat)
            hlat = 'S'
        else:
            hlat = 'N'

        if flon < 0.0:
            hlon = 'W'
            flon = abs(flon)
        else:
            hlon = 'E'
    else:
        flat = abs(flat)
        flon = abs(flon)
        hlon = ''
        hlat = ''
    
    slat = __ll_parts(flat, precision = precision)
    slon = __ll_parts(flon, precision = precision)

    if string:
        return slon + hlon + '; ' + slat + hlat
    
    return (slon + hlon, slat + hlat)

def DMS2Deg(lon, lat):
    """!Convert dms value to deg

    @param lon longitude (x)
    @param lat latitude (y)
    
    @return tuple of converted values
    @return ValueError on error
    """
    x = __ll_parts(lon, reverse = True)
    y = __ll_parts(lat, reverse = True)
    
    return (x, y)

def __ll_parts(value, reverse = False, precision = 3):
    """!Converts deg to d:m:s string

    @param value value to be converted
    @param reverse True to convert from d:m:s to deg
    @param precision seconds precision (ignored if reverse is True)
    
    @return converted value (string/float)
    @return ValueError on error (reverse == True)
    """
    if not reverse:
        if value == 0.0:
            return '%s%.*f' % ('00:00:0', precision, 0.0)
    
        d = int(int(value))
        m = int((value - d) * 60)
        s = ((value - d) * 60 - m) * 60
        if m < 0:
            m = '00'
        elif m < 10:
            m = '0' + str(m)
        else:
            m = str(m)
        if s < 0:
            s = '00.0000'
        elif s < 10.0:
            s = '0%.*f' % (precision, s)
        else:
            s = '%.*f' % (precision, s)
        
        return str(d) + ':' + m + ':' + s
    else: # -> reverse
        try:
            d, m, s = value.split(':')
            hs = s[-1]
            s = s[:-1]
        except ValueError:
            try:
                d, m = value.split(':')
                hs = m[-1]
                m = m[:-1]
                s = '0.0'
            except ValueError:
                try:
                    d = value
                    hs = d[-1]
                    d = d[:-1]
                    m = '0'
                    s = '0.0'
                except ValueError:
                    raise ValueError
        
        if hs not in ('N', 'S', 'E', 'W'):
            raise ValueError
        
        coef = 1.0
        if hs in ('S', 'W'):
            coef = -1.0
        
        fm = int(m) / 60.0
        fs = float(s) / (60 * 60)
        
        return coef * (float(d) + fm + fs)
    
def GetCmdString(cmd):
    """
    Get GRASS command as string.
    
    @param cmd GRASS command given as tuple
    
    @return command string
    """
    scmd = ''
    if not cmd:
        return ''
    scmd = cmd[0]
    for k, v in cmd[1].iteritems():
        scmd += ' %s=%s' % (k, v)
    return scmd

def CmdToTuple(cmd):
    """!Convert command list to tuple for gcmd.RunCommand()"""
    if len(cmd) < 1:
        return None
        
    dcmd = {}
    for item in cmd[1:]:
        if '=' in item:
            key, value = item.split('=', 1)
            dcmd[str(key)] = str(value)
        else: # -> flags
            if not dcmd.has_key('flags'):
                dcmd['flags'] = ''
            dcmd['flags'] += item.replace('-', '')
                
    return (cmd[0],
            dcmd)

def PathJoin(*args):
    """!Check path created by os.path.join"""
    path = os.path.join(*args)
    if platform.system() == 'Windows' and \
            '/' in path:
        return path[1].upper() + ':\\' + path[3:].replace('/', '\\')
    
    return path
    
def reexec_with_pythonw():
    """!Re-execute Python on Mac OS"""
    if sys.platform == 'darwin' and \
            not sys.executable.endswith('MacOS/Python'):
        print >> sys.stderr, 're-executing using pythonw'
        os.execvp('pythonw', ['pythonw', __file__] + sys.argv[1:])

def ReadEpsgCodes(path):
    """!Read EPSG code from the file

    @param path full path to the file with EPSG codes

    @return dictionary of EPSG code
    @return string on error
    """
    epsgCodeDict = dict()
    try:
        f = open(path, "r")
        i = 0
        code = None
        for line in f.readlines():
            line = line.strip()
            if len(line) < 1:
                continue
                
            if line[0] == '#':
                descr = line[1:].strip()
            elif line[0] == '<':
                code, params = line.split(" ", 1)
                code = int(code.replace('<', '').replace('>', ''))

            if code is not None:
                epsgCodeDict[code] = (descr, params)
                code = None
            i += 1
        f.close()
    except StandardError, e:
        return str(e)
    
    return epsgCodeDict

def ReprojectCoordinates(coord, projOut, projIn = None, flags = ''):
    """!Reproject coordinates

    @param coord coordinates given as tuple
    @param projOut output projection
    @param projIn input projection (use location projection settings)

    @return reprojected coordinates (returned as tuple)
    """
    coors = gcmd.RunCommand('m.proj',
                            flags = flags,
                            input = '-',
                            proj_input = projIn,
                            proj_output = projOut,
                            fs = ';',
                            stdin = '%f;%f' % (coord[0], coord[1]),
                            read = True)
    if coors:
        coors = coors.split(';')
        e = coors[0]
        n = coors[1]
        try:
            proj = projOut.split(' ')[0].split('=')[1]
        except IndexError:
            proj = ''
        if proj in ('ll', 'latlong', 'longlat') and 'd' not in flags:
            return (proj, (e, n))
        else:
            try:
                return (proj, (float(e), float(n)))
            except ValueError:
                return (None, None)
    
    return (None, None)

def GetListOfLocations(dbase):
    """!Get list of GRASS locations in given dbase

    @param dbase GRASS database path

    @return list of locations (sorted)
    """
    listOfLocations = list()
    
    for location in glob.glob(os.path.join(dbase, "*")):
        try:
            if os.path.join(location, "PERMANENT") in glob.glob(os.path.join(location, "*")):
                listOfLocations.append(os.path.basename(location))
        except:
            pass
    
    ListSortLower(listOfLocations)
    
    return listOfLocations

def GetListOfMapsets(dbase, location, selectable = False):
    """!Get list of mapsets in given GRASS location

    @param dbase      GRASS database path
    @param location   GRASS location
    @param selectable True to get list of selectable mapsets, otherwise all

    @return list of mapsets - sorted (PERMANENT first)
    """
    listOfMapsets = list()
    
    if selectable:
        ret = gcmd.RunCommand('g.mapset',
                              read = True,
                              flags = 'l',
                              location = location,
                              gisdbase = dbase)
        
        if not ret:
            return listOfMapsets
            
        for line in ret.rstrip().splitlines():
            listOfMapsets += line.split(' ')
    else:
        for mapset in glob.glob(os.path.join(dbase, location, "*")):
            if os.path.isdir(mapset) and \
                    os.path.isfile(os.path.join(dbase, location, mapset, "WIND")) and \
                    os.path.basename(mapset) != 'PERMANENT':
                listOfMapsets.append(os.path.basename(mapset))
    
        ListSortLower(listOfMapsets)
        listOfMapsets.insert(0, 'PERMANENT')
    
    return listOfMapsets
