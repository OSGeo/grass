"""!
@package core.utils

@brief Misc utilities for wxGUI

(C) 2007-2009, 2011-2012 by the GRASS Development Team

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
import shlex
import re
import inspect

from core.globalvar import ETCDIR
sys.path.append(os.path.join(ETCDIR, "python"))

from grass.script import core as grass
from grass.script import task as gtask

from core.gcmd  import RunCommand
from core.debug import Debug
# from core.settings import UserSettings

def normalize_whitespace(text):
    """!Remove redundant whitespace from a string"""
    return string.join(string.split(text), ' ')

def split(s):
    """!Platform spefic shlex.split"""
    if sys.version_info >= (2, 6):
        return shlex.split(s, posix = (sys.platform != "win32"))
    elif sys.platform == "win32":
        return shlex.split(s.replace('\\', r'\\'))
    else:
        return shlex.split(s)

def GetTempfile(pref=None):
    """!Creates GRASS temporary file using defined prefix.

    @todo Fix path on MS Windows/MSYS

    @param pref prefer the given path

    @return Path to file name (string) or None
    """
    ret = RunCommand('g.tempfile',
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

def GetLayerNameFromCmd(dcmd, fullyQualified = False, param = None,
                        layerType = None):
    """!Get map name from GRASS command
    
    Parameter dcmd can be modified when first parameter is not
    defined.
    
    @param dcmd GRASS command (given as list)
    @param fullyQualified change map name to be fully qualified
    @param param params directory
    @param layerType check also layer type ('raster', 'vector', '3d-raster', ...)
    
    @return tuple (name, found)
    """
    mapname = ''
    found   = True
    
    if len(dcmd) < 1:
        return mapname, False
    
    if 'd.grid' == dcmd[0]:
        mapname = 'grid'
    elif 'd.geodesic' in dcmd[0]:
        mapname = 'geodesic'
    elif 'd.rhumbline' in dcmd[0]:
        mapname = 'rhumb'
    elif 'd.graph' in dcmd[0]:
        mapname = 'graph'
    else:
        params = list()
        for idx in range(len(dcmd)):
            try:
                p, v = dcmd[idx].split('=', 1)
            except ValueError:
                continue
            
            if p == param:
                params = [(idx, p, v)]
                break
            
            if p in ('map', 'input', 'layer',
                     'red', 'blue', 'green',
                     'h_map', 's_map', 'i_map',
                     'reliefmap', 'labels'):
                params.append((idx, p, v))

        if len(params) < 1:
            if len(dcmd) > 1:
                i = 1
                while i < len(dcmd):
                    if '=' not in dcmd[i] and not dcmd[i].startswith('-'):
                        task = gtask.parse_interface(dcmd[0])
                        # this expects the first parameter to be the right one
                        p = task.get_options()['params'][0].get('name', '')
                        params.append((i, p, dcmd[i]))
                        break
                    i += 1
            else:
                return mapname, False

        if len(params) < 1:
            return mapname, False

        # need to add mapset for all maps
        mapsets = {}
        for i, p, v in params:
            if p == 'layer':
                continue
            mapname = v
            mapset = ''
            if fullyQualified and '@' not in mapname:
                if layerType in ('raster', 'vector', '3d-raster', 'rgb', 'his'):
                    try:
                        if layerType in ('raster', 'rgb', 'his'):
                            findType = 'cell'
                        else:
                            findType = layerType
                        mapset = grass.find_file(mapname, element = findType)['mapset']
                    except AttributeError: # not found
                        return '', False
                    if not mapset:
                        found = False
                else:
                    mapset = grass.gisenv()['MAPSET']
            mapsets[i] = mapset
            
        # update dcmd
        for i, p, v in params:
            if p == 'layer':
                continue
            dcmd[i] = p + '=' + v
            if i in mapsets and mapsets[i]:
                dcmd[i] += '@' + mapsets[i]
        
        maps = list()
        ogr = False
        for i, p, v in params:
            if v.lower().rfind('@ogr') > -1:
                ogr = True
            if p == 'layer' and not ogr:
                continue
            maps.append(dcmd[i].split('=', 1)[1])
        
        mapname = '\n'.join(maps)
    
    return mapname, found

def GetValidLayerName(name):
    """!Make layer name SQL compliant, based on G_str_to_sql()
    
    @todo: Better use directly Ctypes to reuse venerable libgis C fns...
    """
    retName = str(name).strip()
    
    # check if name is fully qualified
    if '@' in retName:
        retName, mapset = retName.split('@')
    else:
        mapset = None
    
    cIdx = 0
    retNameList = list(retName)
    for c in retNameList:
        if not (c >= 'A' and c <= 'Z') and \
               not (c >= 'a' and c <= 'z') and \
               not (c >= '0' and c <= '9'):
            retNameList[cIdx] = '_'
        cIdx += 1
    retName = ''.join(retNameList)
    
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

def ListOfMapsets(get = 'ordered'):
    """!Get list of available/accessible mapsets

    @param get method ('all', 'accessible', 'ordered')
    
    @return list of mapsets
    @return None on error
    """
    mapsets = []
    
    if get == 'all' or get == 'ordered':
        ret = RunCommand('g.mapsets',
                         read = True,
                         quiet = True,
                         flags = 'l',
                         sep = 'newline')
        
        if ret:
            mapsets = ret.splitlines()
            ListSortLower(mapsets)
        else:
            return None
        
    if get == 'accessible' or get == 'ordered':
        ret = RunCommand('g.mapsets',
                         read = True,
                         quiet = True,
                         flags = 'p',
                         sep = 'newline')
        if ret:
            if get == 'accessible':
                mapsets = ret.splitlines()
            else:
                mapsets_accessible = ret.splitlines()
                for mapset in mapsets_accessible:
                    mapsets.remove(mapset)
                mapsets = mapsets_accessible + mapsets
        else:
            return None
    
    return mapsets

def ListSortLower(list):
    """!Sort list items (not case-sensitive)"""
    list.sort(cmp=lambda x, y: cmp(x.lower(), y.lower()))

def GetVectorNumberOfLayers(vector):
    """!Get list of vector layers connected to database"""
    layers = list()
    if not vector:
        return layers
    
    fullname = grass.find_file(name = vector, element = 'vector')['fullname']
    if not fullname:
        Debug.msg(5, "utils.GetVectorNumberOfLayers(): vector map '%s' not found" % vector)
        return layers
    
    ret, out, msg = RunCommand('v.category',
                               getErrorMsg = True,
                               read = True,
                               input = fullname,
                               option = 'layers')
    if ret != 0:
        sys.stderr.write(_("Vector map <%(map)s>: %(msg)s\n") % { 'map' : fullname, 'msg' : msg })
        return layers
    else:
        Debug.msg(1, "GetVectorNumberOfLayers(): ret %s" % ret)
    
    for layer in out.splitlines():
        layers.append(layer)
    
    Debug.msg(3, "utils.GetVectorNumberOfLayers(): vector=%s -> %s" % \
                  (fullname, ','.join(layers)))
    
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
    """!Get GRASS command as string.
    
    @param cmd GRASS command given as tuple
    
    @return command string
    """
    return ' '.join(CmdTupleToList(cmd))

def CmdTupleToList(cmd):
    """!Convert command tuple to list.
    
    @param cmd GRASS command given as tuple
    
    @return command in list
    """
    cmdList = []
    if not cmd:
        return cmdList
    
    cmdList.append(cmd[0])
    
    if 'flags' in cmd[1]:
        for flag in cmd[1]['flags']:
            cmdList.append('-' + flag)
    for flag in ('verbose', 'quiet', 'overwrite'):
        if flag in cmd[1] and cmd[1][flag] is True:
            cmdList.append('--' + flag)
    
    for k, v in cmd[1].iteritems():
        if k in ('flags', 'verbose', 'quiet', 'overwrite'):
            continue
        cmdList.append('%s=%s' % (k, v))
            
    return cmdList

def CmdToTuple(cmd):
    """!Convert command list to tuple for gcmd.RunCommand()"""
    if len(cmd) < 1:
        return None
    
    dcmd = {}
    for item in cmd[1:]:
        if '=' in item: # params
            key, value = item.split('=', 1)
            dcmd[str(key)] = str(value).replace('"', '')
        elif item[:2] == '--': # long flags
            flag = item[2:]
            if flag in ('verbose', 'quiet', 'overwrite'):
                dcmd[str(flag)] = True
        elif len(item) == 2 and item[0] == '-': # -> flags
            if 'flags' not in dcmd:
                dcmd['flags'] = ''
            dcmd['flags'] += item[1]
        else: # unnamed parameter
            module = gtask.parse_interface(cmd[0])
            dcmd[module.define_first()] = item
    
    return (cmd[0], dcmd)

def PathJoin(*args):
    """!Check path created by os.path.join"""
    path = os.path.join(*args)
    if platform.system() == 'Windows' and \
            '/' in path:
        return path[1].upper() + ':\\' + path[3:].replace('/', '\\')
    
    return path

def ReadEpsgCodes(path):
    """!Read EPSG code from the file

    @param path full path to the file with EPSG codes

    @return dictionary of EPSG code
    @return string on error
    """
    epsgCodeDict = dict()
    try:
        try:
            f = open(path, "r")
        except IOError:
            return _("failed to open '%s'" % path)

        code = None
        for line in f.readlines():
            line = line.strip()
            if len(line) < 1:
                continue
                
            if line[0] == '#':
                descr = line[1:].strip()
            elif line[0] == '<':
                code, params = line.split(" ", 1)
                try:
                    code = int(code.replace('<', '').replace('>', ''))
                except ValueError, e:
                    return e
            
            if code is not None:
                epsgCodeDict[code] = (descr, params)
                code = None
        
        f.close()
    except StandardError, e:
        return e
    
    return epsgCodeDict

def ReprojectCoordinates(coord, projOut, projIn = None, flags = ''):
    """!Reproject coordinates

    @param coord coordinates given as tuple
    @param projOut output projection
    @param projIn input projection (use location projection settings)

    @return reprojected coordinates (returned as tuple)
    """
    coors = RunCommand('m.proj',
                       flags = flags,
                       input = '-',
                       proj_input = projIn,
                       proj_output = projOut,
                       sep = ';',
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

    try:
        for location in glob.glob(os.path.join(dbase, "*")):
            try:
                if os.path.join(location, "PERMANENT") in glob.glob(os.path.join(location, "*")):
                    listOfLocations.append(os.path.basename(location))
            except:
                pass
    except UnicodeEncodeError, e:
        raise e
    
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
        ret = RunCommand('g.mapset',
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
                    os.path.isfile(os.path.join(dbase, location, mapset, "WIND")):
                listOfMapsets.append(os.path.basename(mapset))
    
    ListSortLower(listOfMapsets)
    return listOfMapsets

def GetColorTables():
    """!Get list of color tables"""
    ret = RunCommand('r.colors',
                     read = True,
                     flags = 'l')
    if not ret:
        return list()
    
    return ret.splitlines()

def _getGDALFormats():
    """!Get dictionary of avaialble GDAL drivers"""
    try:
        ret = grass.read_command('r.in.gdal',
                                 quiet = True,
                                 flags = 'f')
    except:
        ret = None
    
    return _parseFormats(ret), _parseFormats(ret, writableOnly = True)

def _getOGRFormats():
    """!Get dictionary of avaialble OGR drivers"""
    try:
        ret = grass.read_command('v.in.ogr',
                                 quiet = True,
                                 flags = 'f')
    except:
        ret = None

    return _parseFormats(ret), _parseFormats(ret, writableOnly = True)

def _parseFormats(output, writableOnly = False):
    """!Parse r.in.gdal/v.in.ogr -f output"""
    formats = { 'file'     : list(),
                'database' : list(),
                'protocol' : list()
                }
    
    if not output:
        return formats
    
    patt = None
    if writableOnly:
        patt = re.compile('\(rw\+?\)$', re.IGNORECASE)
    
    for line in output.splitlines():
        key, name = map(lambda x: x.strip(), line.strip().rsplit(':', -1))
        
        if writableOnly and not patt.search(key):
            continue
        
        if name in ('Memory', 'Virtual Raster', 'In Memory Raster'):
            continue
        if name in ('PostgreSQL', 'SQLite',
                    'ODBC', 'ESRI Personal GeoDatabase',
                    'Rasterlite',
                    'PostGIS WKT Raster driver',
                    'PostGIS Raster driver',
                    'CouchDB',
                    'MSSQLSpatial',
                    'FileGDB'):
            formats['database'].append(name)
        elif name in ('GeoJSON',
                      'OGC Web Coverage Service',
                      'OGC Web Map Service',
                      'WFS',
                      'GeoRSS',
                      'HTTP Fetching Wrapper'):
            formats['protocol'].append(name)
        else:
            formats['file'].append(name)
    
    for items in formats.itervalues():
        items.sort()
    
    return formats

formats = None

def GetFormats(writableOnly = False):
    """!Get GDAL/OGR formats"""
    global formats
    if not formats:
        gdalAll, gdalWritable = _getGDALFormats()
        ogrAll,  ogrWritable  = _getOGRFormats()
        formats = {
            'all' : {
                'gdal' : gdalAll,
                'ogr'  : ogrAll,
                },
            'writable' : {
                'gdal' : gdalWritable,
                'ogr'  : ogrWritable,
                },
            }
    
    if writableOnly:
        return formats['writable']
    
    return formats['all']

def GetSettingsPath():
    """!Get full path to the settings directory
    """
    try:
        verFd = open(os.path.join(ETCDIR, "VERSIONNUMBER"))
        version = int(verFd.readlines()[0].split(' ')[0].split('.')[0])
    except (IOError, ValueError, TypeError, IndexError), e:
        sys.exit(_("ERROR: Unable to determine GRASS version. Details: %s") % e)
    
    verFd.close()

    # keep location of settings files rc and wx in sync with lib/init/grass.py
    if sys.platform == 'win32':
        return os.path.join(os.getenv('APPDATA'), 'GRASS%d' % version)
    
    return os.path.join(os.getenv('HOME'), '.grass%d' % version)

def StoreEnvVariable(key, value = None, envFile = None):
    """!Store environmental variable

    If value is not given (is None) then environmental variable is
    unset.
    
    @param key env key
    @param value env value
    @param envFile path to the environmental file (None for default location)
    """
    windows = sys.platform == 'win32'
    if not envFile:
        gVersion = grass.version()['version'].split('.', 1)[0]
        if not windows:
            envFile = os.path.join(os.getenv('HOME'), '.grass%s' % gVersion, 'bashrc')
        else:
            envFile = os.path.join(os.getenv('APPDATA'), 'GRASS%s' % gVersion, 'env.bat')
    
    # read env file
    environ = dict()
    lineSkipped = list()
    if os.path.exists(envFile):
        try:
            fd = open(envFile)
        except IOError, e:
            sys.stderr.write(_("Unable to open file '%s'\n") % envFile)
            return
        for line in fd.readlines():
            line = line.rstrip(os.linesep)
            try:
                k, v = map(lambda x: x.strip(), line.split(' ', 1)[1].split('=', 1))
            except StandardError, e:
                sys.stderr.write(_("%s: line skipped - unable to parse '%s'\n"
                                   "Reason: %s\n") % (envFile, line, e))
                lineSkipped.append(line)
                continue
            if k in environ:
                sys.stderr.write(_("Duplicated key: %s\n") % k)
            environ[k] = v
        
        fd.close()
    
    # update environmental variables
    if value is None:
        if key in environ:
            del environ[key]
    else:
        environ[key] = value
    
    # write update env file
    try:
        fd = open(envFile, 'w')
    except IOError, e:
        sys.stderr.write(_("Unable to create file '%s'\n") % envFile)
        return
    if windows:
        expCmd = 'set'
    else:
        expCmd = 'export'
    
    for key, value in environ.iteritems():
        fd.write('%s %s=%s\n' % (expCmd, key, value))
    
    # write also skipped lines
    for line in lineSkipped:
        fd.write(line + os.linesep)
    
    fd.close()

def SetAddOnPath(addonPath = None, key = 'PATH'):
    """!Set default AddOn path

    @param addonPath path to addons (None for default)
    @param key env key - 'PATH' or 'BASE'
    """
    gVersion = grass.version()['version'].split('.', 1)[0]
    # update env file
    if not addonPath:
        if sys.platform != 'win32':
            addonPath = os.path.join(os.path.join(os.getenv('HOME'),
                                                  '.grass%s' % gVersion,
                                                  'addons'))
        else:
            addonPath = os.path.join(os.path.join(os.getenv('APPDATA'),
                                                  'GRASS%s' % gVersion,
                                                  'addons'))
    
    StoreEnvVariable(key = 'GRASS_ADDON_' + key, value = addonPath)
    os.environ['GRASS_ADDON_' + key] = addonPath
    
    # update path
    if addonPath not in os.environ['PATH']:
        os.environ['PATH'] = addonPath + os.pathsep + os.environ['PATH']
    
# From lib/gis/col_str.c, except purple which is mentioned
# there but not given RGB values
str2rgb = {'aqua': (100, 128, 255),
           'black': (0, 0, 0),
           'blue': (0, 0, 255),
           'brown': (180, 77, 25),
           'cyan': (0, 255, 255),
           'gray': (128, 128, 128),
           'green': (0, 255, 0),
           'grey': (128, 128, 128),
           'indigo': (0, 128, 255),
           'magenta': (255, 0, 255),
           'orange': (255, 128, 0),
           'purple': (128, 0, 128),
           'red': (255, 0, 0),
           'violet': (128, 0, 255),
           'white': (255, 255, 255),
           'yellow': (255, 255, 0)}
rgb2str = {}
for (s,r) in str2rgb.items():
    rgb2str[ r ] = s

def color_resolve(color):
    if len(color) > 0 and color[0] in "0123456789":
        rgb = tuple(map(int, color.split(':')))
        label = color
    else:
        # Convert color names to RGB
        try:
            rgb = str2rgb[color]
            label = color
        except KeyError:
            rgb = (200, 200, 200)
            label = _('Select Color')
    return (rgb, label)

command2ltype = {'d.rast'         : 'raster',
                 'd.rast3d'       : '3d-raster',
                 'd.rgb'          : 'rgb',
                 'd.his'          : 'his',
                 'd.shadedmap'    : 'shaded',
                 'd.legend'       : 'rastleg',
                 'd.rast.arrow'   : 'rastarrow',
                 'd.rast.num'     : 'rastnum',
                 'd.rast.leg'     : 'maplegend',
                 'd.vect'         : 'vector',
                 'd.thematic.area': 'thememap',
                 'd.vect.chart'   : 'themechart',
                 'd.grid'         : 'grid',
                 'd.geodesic'     : 'geodesic',
                 'd.rhumbline'    : 'rhumb',
                 'd.labels'       : 'labels',
                 'd.barscale'     : 'barscale',
                 'd.redraw'       : 'redraw',
                 'd.wms'          : 'wms',
                 'd.histogram'    : 'histogram',
                 'd.colortable'   : 'colortable',
                 'd.graph'        : 'graph'
                 }
ltype2command = {}
for (cmd, ltype) in command2ltype.items():
    ltype2command[ltype] = cmd


def GetGEventAttribsForHandler(method, event):
    """!Get attributes from event, which can be used by handler method. 

    Be aware of event class attributes.

    @param method - handler method (including self arg)
    @param event - event

    @return (valid kwargs for method, 
             list of method's args without default value 
             which were not found among event attributes)
    """
    args_spec = inspect.getargspec(method)

    args = args_spec[0]

    defaults =[]
    if args_spec[3]:
        defaults =  args_spec[3]

    # number of arguments without def value
    req_args = len(args) - 1 - len(defaults)

    kwargs = {}
    missing_args = []

    for i, a in enumerate(args):
        if hasattr(event, a):
            kwargs[a] = getattr(event, a)
        elif i < req_args:
            missing_args.append(a)

    return kwargs, missing_args
