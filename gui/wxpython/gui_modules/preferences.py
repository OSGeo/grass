"""!
@package preferences

@brief User preferences dialog

Sets default display font, etc.

Classes:
 - Settings
 - PreferencesBaseDialog
 - PreferencesDialog
 - DefaultFontDialog
 - MapsetAccess
 - NvizPreferencesDialog

(C) 2007-2010 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton (Arizona State University)
@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import copy
import stat
import types
try:
    import pwd
    havePwd = True
except ImportError:
    havePwd = False

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

import wx
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.colourselect as csel
import wx.lib.mixins.listctrl as listmix

from grass.script import core as grass

import gcmd
import utils
import globalvar
from debug import Debug as Debug

class Settings:
    """!Generic class where to store settings"""
    def __init__(self):
        #
        # key/value separator
        #
        self.sep = ';'
        
        try:
            projFile = utils.PathJoin(os.environ["GRASS_PROJSHARE"], 'epsg')
        except KeyError:
            projFile = ''
        
        #
        # default settings
        #
        self.defaultSettings = {
            #
            # general
            #
            'general': {
                # use default window layout (layer manager, displays, ...)
                'defWindowPos' : {
                    'enabled' : True,
                    'dim' : '%d,0,%d,%d,0,0,%d,%d' % \
                        (globalvar.MAP_WINDOW_SIZE[0] + 5,
                         globalvar.GM_WINDOW_SIZE[0],
                         globalvar.GM_WINDOW_SIZE[1],
                         globalvar.MAP_WINDOW_SIZE[0],
                         globalvar.MAP_WINDOW_SIZE[1])
                    },
                # expand/collapse element list
                'elementListExpand' : {
                    'selection' : 0 
                    },
                },
            'manager' : {
                # show opacity level widget
                'changeOpacityLevel' : {
                    'enabled' : False
                    }, 
                # ask when removing layer from layer tree
                'askOnRemoveLayer' : {
                    'enabled' : True
                    },
                # ask when quiting wxGUI or closing display
                'askOnQuit' : {
                    'enabled' : True
                    },
                },
            #
            # display
            #
            'display': {
                'font' : {
                    'type' : '',
                    'encoding': 'ISO-8859-1',
                    },
                'outputfont' : {
                    'type' : 'Courier New',
                    'size': '10',
                    },
                'driver': {
                    'type': 'cairo'
                    },
                'compResolution' : {
                    'enabled' : False
                    },
                'autoRendering': {
                    'enabled' : True
                    },
                'autoZooming' : {
                    'enabled' : False
                    },
                'statusbarMode': {
                    'selection' : 0
                    },
                'bgcolor': {
                    'color' : (255, 255, 255, 255),
                    },
                },
            #
            # projection
            #
            'projection' : {
                'statusbar' : {
                    'proj4'    : '',
                    'epsg'     : '',
                    'projFile' : projFile,
                    },
                'format' : {
                    'll'  : 'DMS',
                    'precision' : 2,
                    },
                },
            #
            # advanced
            #
            'advanced' : {
                'iconTheme' : {
                    'type' : 'grass2'
                    }, # grass2, grass, silk
                },
            #
            # Attribute Table Manager
            #
            'atm' : {
                'highlight' : {
                    'color' : (255, 255, 0, 255),
                    'width' : 2
                    },
                'leftDbClick' : {
                    'selection' : 1 # draw selected
                    },
                'askOnDeleteRec' : {
                    'enabled' : True
                    },
                'keycolumn' : {
                    'value' : 'cat'
                    },
                'encoding' : {
                    'value' : '',
                    }
                },
            #
            # Command
            #
            'cmd': {
                'overwrite' : {
                    'enabled' : False
                    },
                'closeDlg' : {
                    'enabled' : True
                    },
                'verbosity' : {
                    'selection' : 'grassenv'
                    },
                # d.rast
                'rasterOpaque' : {
                    'enabled' : False
                    },
                'rasterColorTable' : {
                    'enabled'   : False,
                    'selection' : 'rainbow',
                    },
                # d.vect
                'showType': {
                    'point' : {
                        'enabled' : True
                        },
                    'line' : {
                        'enabled' : True
                        },
                    'centroid' : {
                        'enabled' : True
                        },
                    'boundary' : {
                        'enabled' : True
                        },
                    'area' : {
                        'enabled' : True
                        },
                    'face' : {
                        'enabled' : True
                        },
                    },
                'addNewLayer' : {
                    'enabled' : True,
                    },
                'interactiveInput' : {
                    'enabled' : True,
                    },
                },
            #
            # Workspace
            #
            'workspace' : {
                'posDisplay' : {
                    'enabled' : False
                    },
                'posManager' : {
                    'enabled' : False
                    },
                },
            #
            # vdigit
            #
            'vdigit' : {
                # symbology
                'symbol' : {
                    'highlight' : {
                        'enabled' : None,
                        'color' : (255, 255, 0, 255)
                        }, # yellow
                    'highlightDupl' : {
                        'enabled' : None,
                        'color' : (255, 72, 0, 255)
                        }, # red
                    'point' : {
                        'enabled' : True,
                        'color' : (0, 0, 0, 255)
                        }, # black
                    'line' : {
                        'enabled' : True,
                        'color' : (0, 0, 0, 255)
                        }, # black
                    'boundaryNo' : {
                        'enabled' : True,
                        'color' : (126, 126, 126, 255)
                        }, # grey
                    'boundaryOne' : {
                        'enabled' : True,
                        'color' : (0, 255, 0, 255)
                        }, # green
                    'boundaryTwo' : {
                        'enabled' : True,
                        'color' : (255, 135, 0, 255)
                        }, # orange
                    'centroidIn' : {
                        'enabled' : True,
                        'color' : (0, 0, 255, 255)
                        }, # blue
                    'centroidOut' : {
                        'enabled' : True,
                        'color' : (165, 42, 42, 255)
                        }, # brown
                    'centroidDup' : {
                        'enabled' : True,
                        'color' : (156, 62, 206, 255)
                        }, # violet
                    'nodeOne' : {
                        'enabled' : True,
                        'color' : (255, 0, 0, 255)
                        }, # red
                    'nodeTwo' : {
                        'enabled' : True,
                        'color' : (0, 86, 45, 255)
                        }, # dark green
                    'vertex' : {
                        'enabled' : False,
                        'color' : (255, 20, 147, 255)
                        }, # deep pink
                    'area' : {
                        'enabled' : False,
                        'color' : (217, 255, 217, 255)
                        }, # green
                    'direction' : {
                        'enabled' : False,
                        'color' : (255, 0, 0, 255)
                        }, # red
                    },
                # display
                'lineWidth' : {
                    'value' : 2,
                    'units' : 'screen pixels'
                    },
                # snapping
                'snapping' : {
                    'value' : 10,
                    'units' : 'screen pixels'
                    },
                'snapToVertex' : {
                    'enabled' : False
                    },
                # digitize new record
                'addRecord' : {
                    'enabled' : True
                    },
                'layer' :{
                    'value' : 1
                    },
                'category' : {
                    'value' : 1
                    },
                'categoryMode' : {
                    'selection' : 0
                    },
                # delete existing feature(s)
                'delRecord' : {
                    'enabled' : True
                    },
                # query tool
                'query' : {
                    'selection' : 0,
                    'box' : True
                    },
                'queryLength' : {
                    'than-selection' : 0,
                    'thresh' : 0
                    },
                'queryDangle' : {
                    'than-selection' : 0,
                    'thresh' : 0
                    },
                # select feature (point, line, centroid, boundary)
                'selectType': {
                    'point' : {
                        'enabled' : True
                        },
                    'line' : {
                        'enabled' : True
                        },
                    'centroid' : {
                        'enabled' : True
                        },
                    'boundary' : {
                        'enabled' : True
                        },
                    },
                'selectThresh' : {
                    'value' : 10,
                    'units' : 'screen pixels'
                    },
                'checkForDupl' : {
                    'enabled' : False
                    },
                'selectInside' : {
                    'enabled' : False
                    },
                # exit
                'saveOnExit' : {
                    'enabled' : False,
                    },
                # break lines on intersection
                'breakLines' : {
                    'enabled' : False,
                    },
                },
            'profile': {
                'raster0' : {
                    'pcolor' : (0, 0, 255, 255), # profile line color
                    'pwidth' : 1, # profile line width
                    'pstyle' : 'solid', # profile line pen style
                    },
                'raster1' : {
                    'pcolor' : (255, 0, 0, 255), 
                    'pwidth' : 1, 
                    'pstyle' : 'solid', 
                    },
                'raster2' : {
                    'pcolor' : (0, 255, 0, 255), 
                    'pwidth' : 1, 
                    'pstyle' : 'solid', 
                    },
                'font' : {
                    'titleSize' : 12,
                    'axisSize' : 11,
                    'legendSize' : 10,
                    },
                'marker' : {
                    'color' : (0, 0, 0, 255),
                    'fill' : 'transparent',
                    'size' : 2,
                    'type' : 'triangle',
                    'legend' : _('Segment break'),
                    },
                'grid' : {
                    'color' : (200, 200, 200, 255),
                    'enabled' : True,
                    },
                'x-axis' : {
                    'type' : 'auto', # axis format
                    'min' : 0, # axis min for custom axis range
                    'max': 0, # axis max for custom axis range
                    'log' : False,
                    },
                'y-axis' : {
                    'type' : 'auto', # axis format
                    'min' : 0, # axis min for custom axis range
                    'max': 0, # axis max for custom axis range
                    'log' : False,
                    },
                'legend' : {
                    'enabled' : True
                    },
                },
            'gcpman' : {
                'rms' : {
                    'highestonly' : True,
                    'sdfactor' : 1,
                    },
                'symbol' : {
                    'color' : (0, 0, 255, 255),
                    'hcolor' : (255, 0, 0, 255),
                    'scolor' : (0, 255, 0, 255),
                    'ucolor' : (255, 165, 0, 255),
                    'unused' : True,
                    'size' : 8,
                    'width' : 2,
                    },
                },
            'georect' : {
                'symbol' : {
                    'color' : (0, 0, 255, 255),
                    'width' : 2,
                    },
                },
            'nviz' : {
                'view' : {
                    'persp' : {
                        'value' : 20,
                        'step' : 5,
                        },
                    'position' : {
                        'x' : 0.84,
                        'y' : 0.16,
                        },
                    'height' : {
                        'step' : 100,
                        },
                    'twist' : {
                        'value' : 0,
                        'step' : 5,
                        },
                    'z-exag' : {
                        'step' : 1,
                        },
                    'background' : {
                        'color' : (255, 255, 255, 255), # white
                        },
                    },
                'surface' : {
                    'shine': {
                        'map' : False,
                        'value' : 60.0,
                        },
                    'color' : {
                        'map' : True,
                        'value' : (0, 0, 0, 255), # constant: black
                        },
                    'draw' : {
                        'wire-color' : (136, 136, 136, 255),
                        'mode' : 1, # fine
                        'style' : 1, # surface
                        'shading' : 1, # gouraud
                        'res-fine' : 6,
                        'res-coarse' : 9,
                        },
                    'position' : {
                        'x' : 0,
                        'y' : 0,
                        'z' : 0,
                        },
                    },
                'vector' : {
                    'lines' : {
                        'show' : False,
                        'width' : 2,
                        'color' : (0, 0, 255, 255), # blue
                        'flat' : False,
                        'height' : 0,
                        },
                    'points' : {
                        'show' : False,
                        'size' : 100,
                        'width' : 2,
                        'marker' : 2,
                        'color' : (0, 0, 255, 255), # blue
                        'height' : 0,
                        }
                    },
                'volume' : {
                    'color' : {
                        'map' : True,
                        'value' : (0, 0, 0, 255), # constant: black
                        },
                    'draw' : {
                        'mode'       : 0, # isosurfaces
                        'shading'    : 1, # gouraud
                        'resolution' : 3, # polygon resolution
                        },
                    'shine': {
                        'map' : False,
                        'value' : 60,
                        },
                    },
                'light' : {
                    'position' : {
                        'x' : 0.68,
                        'y' : 0.68,
                        'z' : 80,
                        },
                    'bright'  : 80,
                    'color'   : (255, 255, 255, 255), # white
                    'ambient' : 20,
                    },
                'fringe' : {
                    'elev'   : 55,
                    'color'  : (128, 128, 128, 255), # grey
                    },
                },
            'modeler' : {
                'action' : {
                    'color' : {
                        'valid'   :  (180, 234, 154, 255), # light green
                        'invalid' :  (255, 255, 255, 255), # white
                        'running' :  (255, 0, 0, 255),     # red
                        'disabled' : (211, 211, 211, 255), # light grey
                        },
                    'size' : {
                        'width'  : 100,
                        'height' : 50,
                        },
                    'width': {
                        'parameterized' : 2,
                        'default'       : 1,
                        },
                    },
                'data' : { 
                    'color': {
                        'raster'   : (215, 215, 248, 255), # light blue
                        'raster3d' : (215, 248, 215, 255), # light green
                        'vector'   : (248, 215, 215, 255), # light red
                        },
                    'size' : {
                        'width' : 175,
                        'height' : 50,
                        },
                    },
                'loop' : {
                    'size' : {
                        'width' : 175,
                        'height' : 40,
                        },
                    },
                'if-else' : {
                    'size' : {
                        'width' : 150,
                        'height' : 40,
                        },
                    },
                },
            }

        # quick fix, http://trac.osgeo.org/grass/ticket/1233
        # TODO
        if sys.platform == 'darwin':
            self.defaultSettings['general']['defWindowPos']['enabled'] = False
        
        #
        # user settings
        #
        self.userSettings = copy.deepcopy(self.defaultSettings)
        try:
            self.ReadSettingsFile()
        except gcmd.GException, e:
            print >> sys.stderr, e.value

        #
        # internal settings (based on user settings)
        #
        self.internalSettings = {}
        for group in self.userSettings.keys():
            self.internalSettings[group] = {}
            for key in self.userSettings[group].keys():
                self.internalSettings[group][key] = {}

        # self.internalSettings['general']["mapsetPath"]['value'] = self.GetMapsetPath()
        self.internalSettings['general']['elementListExpand']['choices'] = \
            (_("Collapse all except PERMANENT and current"),
             _("Collapse all except PERMANENT"),
             _("Collapse all except current"),
             _("Collapse all"),
             _("Expand all"))
        self.internalSettings['atm']['leftDbClick']['choices'] = (_('Edit selected record'),
                                                                  _('Display selected'))
        self.internalSettings['advanced']['iconTheme']['choices'] = ('grass',
                                                                     'grass2',
                                                                     'silk')
        self.internalSettings['cmd']['verbosity']['choices'] = ('grassenv',
                                                                'verbose',
                                                                'quiet')
        self.internalSettings['display']['driver']['choices'] = ['cairo', 'png']
        self.internalSettings['display']['statusbarMode']['choices'] = globalvar.MAP_DISPLAY_STATUSBAR_MODE

        self.internalSettings['nviz']['view'] = {}
        self.internalSettings['nviz']['view']['twist'] = {}
        self.internalSettings['nviz']['view']['twist']['min'] = -180
        self.internalSettings['nviz']['view']['twist']['max'] = 180
        self.internalSettings['nviz']['view']['persp'] = {}
        self.internalSettings['nviz']['view']['persp']['min'] = 1
        self.internalSettings['nviz']['view']['persp']['max'] = 100
        self.internalSettings['nviz']['view']['height'] = {}
        self.internalSettings['nviz']['view']['height']['value'] = -1
        self.internalSettings['nviz']['vector'] = {}
        self.internalSettings['nviz']['vector']['points'] = {}
        self.internalSettings['nviz']['vector']['points']['marker'] = ("x",
                                                                       _("box"),
                                                                       _("sphere"),
                                                                       _("cube"),
                                                                       _("diamond"),
                                                                       _("dtree"),
                                                                       _("ctree"),
                                                                       _("aster"),
                                                                       _("gyro"),
                                                                       _("histogram"))
        self.internalSettings['vdigit']['bgmap'] = {}
        self.internalSettings['vdigit']['bgmap']['value'] = ''
        
    def ReadSettingsFile(self, settings=None):
        """!Reads settings file (mapset, location, gisdbase)"""
        if settings is None:
            settings = self.userSettings

        # look for settings file
        gisenv = grass.gisenv()
        gisdbase = gisenv['GISDBASE']
        location_name = gisenv['LOCATION_NAME']
        mapset_name = gisenv['MAPSET']
        
        filePath = os.path.join(os.path.expanduser("~"), '.grass7', 'wx') # MS Windows fix ?
        
        self.__ReadFile(filePath, settings)
        
        # set environment variables
        os.environ["GRASS_FONT"] = self.Get(group='display',
                                            key='font', subkey='type')
        os.environ["GRASS_ENCODING"] = self.Get(group='display',
                                                key='font', subkey='encoding')
        
    def __ReadFile(self, filename, settings=None):
        """!Read settings from file to dict

        @param filename settings file path
        @param settings dict where to store settings (None for self.userSettings)
        
        @return True on success
        @return False on failure
        """
        if settings is None:
            settings = self.userSettings

        if not os.path.exists(filename):
            return False
        
        try:
            file = open(filename, "r")
            line = ''
            for line in file.readlines():
                line = line.rstrip('%s' % os.linesep)
                group, key = line.split(self.sep)[0:2]
                kv = line.split(self.sep)[2:]
                subkeyMaster = None
                if len(kv) % 2 != 0: # multiple (e.g. nviz)
                    subkeyMaster = kv[0]
                    del kv[0]
                idx = 0
                while idx < len(kv):
                    if subkeyMaster:
                        subkey = [subkeyMaster, kv[idx]]
                    else:
                        subkey = kv[idx]
                    value = kv[idx+1]
                    value = self.__parseValue(value, read=True)
                    self.Append(settings, group, key, subkey, value)
                    idx += 2
        except ValueError, e:
            print >> sys.stderr, _("Error: Reading settings from file <%(file)s> failed.\n"
                                   "       Details: %(detail)s\n"
                                   "       Line: '%(line)s'") % { 'file' : filename,
                                                                  'detail' : e,
                                                                  'line' : line }
            file.close()
            return False
        
        file.close()
        return True

    def SaveToFile(self, settings=None):
        """!Save settings to the file"""
        if settings is None:
            settings = self.userSettings
        
        home = os.path.expanduser("~") # MS Windows fix ?
        
        gisenv = grass.gisenv()
        gisdbase = gisenv['GISDBASE']
        location_name = gisenv['LOCATION_NAME']
        mapset_name = gisenv['MAPSET']
        
        dirPath = os.path.join(home, '.grass7')
        if os.path.exists(dirPath) == False:
            try:
                os.mkdir(dirPath)
            except:
                wx.MessageBox(_('Cannot create directory for settings [home]/.grass7'),
                              _('Error saving preferences'))

        filePath = os.path.join(home, '.grass7', 'wx')
        
        try:
            file = open(filePath, "w")
            for group in settings.keys():
                for key in settings[group].keys():
                    file.write('%s%s%s%s' % (group, self.sep, key, self.sep))
                    subkeys = settings[group][key].keys()
                    for idx in range(len(subkeys)):
                        value = settings[group][key][subkeys[idx]]
                        if type(value) == types.DictType:
                            if idx > 0:
                                file.write('%s%s%s%s%s' % (os.linesep, group, self.sep, key, self.sep))
                            file.write('%s%s' % (subkeys[idx], self.sep))
                            kvalues = settings[group][key][subkeys[idx]].keys()
                            srange = range(len(kvalues))
                            for sidx in srange:
                                svalue = self.__parseValue(settings[group][key][subkeys[idx]][kvalues[sidx]])
                                file.write('%s%s%s' % (kvalues[sidx], self.sep,
                                                       svalue))
                                if sidx < len(kvalues) - 1:
                                    file.write('%s' % self.sep)
                            if idx < len(subkeys) - 1:
                                file.write('%s%s%s%s%s' % (os.linesep, group, self.sep, key, self.sep))
                        else:
                            value = self.__parseValue(settings[group][key][subkeys[idx]])
                            file.write('%s%s%s' % (subkeys[idx], self.sep, value))
                            if idx < len(subkeys) - 1 and \
                                    type(settings[group][key][subkeys[idx + 1]]) != types.DictType:
                                file.write('%s' % self.sep)
                    file.write(os.linesep)
        except IOError, e:
            raise gcmd.GException(e)
        except StandardError, e:
            raise gcmd.GException(_('Writing settings to file <%(file)s> failed.'
                                    '\n\nDetails: %(detail)s') % { 'file' : filePath,
                                                                   'detail' : e })
        
        file.close()
        
        return filePath

    def __parseValue(self, value, read=False):
        """!Parse value to be store in settings file"""
        if read: # -> read settings (cast values)
            if value == 'True':
                value = True
            elif value == 'False':
                value = False
            elif value == 'None':
                value = None
            elif ':' in value: # -> color
                try:
                    value = tuple(map(int, value.split(':')))
                except ValueError: # -> string
                    pass
            else:
                try:
                    value = int(value)
                except ValueError:
                    try:
                        value = float(value)
                    except ValueError:
                        pass
        else: # -> write settings
            if type(value) == type(()): # -> color
                value = str(value[0]) + ':' +\
                    str(value[1]) + ':' + \
                    str(value[2])
                
        return value

    def Get(self, group, key=None, subkey=None, internal=False):
        """!Get value by key/subkey

        Raise KeyError if key is not found
        
        @param group settings group
        @param key (value, None)
        @param subkey (value, list or None)
        @param internal use internal settings instead

        @return value
        """
        if internal is True:
            settings = self.internalSettings
        else:
            settings = self.userSettings
            
        try:
            if subkey is None:
                if key is None:
                    return settings[group]
                else:
                    return settings[group][key]
            else:
                if type(subkey) == type(tuple()) or \
                        type(subkey) == type(list()):
                    return settings[group][key][subkey[0]][subkey[1]]
                else:
                    return settings[group][key][subkey]  

        except KeyError:
            print >> sys.stderr, "Settings: unable to get value '%s:%s:%s'\n" % \
                (group, key, subkey)
        
    def Set(self, group, value, key = None, subkey = None, internal = False):
        """!Set value of key/subkey
        
        Raise KeyError if group/key is not found
        
        @param group settings group
        @param key key (value, None)
        @param subkey subkey (value, list or None)
        @param value value
        @param internal use internal settings instead
        """
        if internal is True:
            settings = self.internalSettings
        else:
            settings = self.userSettings
        
        try:
            if subkey is None:
                if key is None:
                    settings[group] = value
                else:
                    settings[group][key] = value
            else:
                if type(subkey) == type(tuple()) or \
                        type(subkey) == type(list()):
                    settings[group][key][subkey[0]][subkey[1]] = value
                else:
                    settings[group][key][subkey] = value
        except KeyError:
            raise gcmd.GException("%s '%s:%s:%s'" % (_("Unable to set "), group, key, subkey))
        
    def Append(self, dict, group, key, subkey, value):
        """!Set value of key/subkey

        Create group/key/subkey if not exists
        
        @param dict settings dictionary to use
        @param group settings group
        @param key key
        @param subkey subkey (value or list)
        @param value value
        """
        if not dict.has_key(group):
            dict[group] = {}

        if not dict[group].has_key(key):
            dict[group][key] = {}

        if type(subkey) == types.ListType:
            # TODO: len(subkey) > 2
            if not dict[group][key].has_key(subkey[0]):
                dict[group][key][subkey[0]] = {}
            try:
                dict[group][key][subkey[0]][subkey[1]] = value
            except TypeError:
                print >> sys.stderr, _("Unable to parse settings '%s'") % value + \
                    ' (' + group + ':' + key + ':' + subkey[0] + ':' + subkey[1] + ')'
        else:
            try:
                dict[group][key][subkey] = value
            except TypeError:
                print >> sys.stderr, _("Unable to parse settings '%s'") % value + \
                    ' (' + group + ':' + key + ':' + subkey + ')'
        
    def GetDefaultSettings(self):
        """!Get default user settings"""
        return self.defaultSettings

globalSettings = Settings()

class PreferencesBaseDialog(wx.Dialog):
    """!Base preferences dialog"""
    def __init__(self, parent, settings, title = _("User settings"),
                 size = (500, 375),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent # ModelerFrame
        self.title  = title
        self.size   = size
        self.settings = settings
        
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title,
                           style = style)
        
        # notebook
        self.notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)
        
        # dict for window ids
        self.winId = {}
        
        # create notebook pages
        
        # buttons
        self.btnDefault = wx.Button(self, wx.ID_ANY, _("Set to default"))
        self.btnSave = wx.Button(self, wx.ID_SAVE)
        self.btnApply = wx.Button(self, wx.ID_APPLY)
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnSave.SetDefault()
        
        # bindigs
        self.btnDefault.Bind(wx.EVT_BUTTON, self.OnDefault)
        self.btnDefault.SetToolTipString(_("Revert settings to default and apply changes"))
        self.btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        self.btnApply.SetToolTipString(_("Apply changes for the current session"))
        self.btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        self.btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        self.btnSave.SetDefault()
        self.btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        self.btnCancel.SetToolTipString(_("Close dialog and ignore changes"))

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self._layout()
        
    def _layout(self):
        """!Layout window"""
        # sizers
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item=self.btnDefault, proportion=1,
                     flag=wx.ALL, border=5)
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.AddButton(self.btnSave)
        btnStdSizer.AddButton(self.btnApply)
        btnStdSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=self.notebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                      flag=wx.EXPAND, border=0)
        mainSizer.Add(item=btnStdSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        
    def OnDefault(self, event):
        """!Button 'Set to default' pressed"""
        self.settings.userSettings = copy.deepcopy(self.settings.defaultSettings)
        
        # update widgets
        for gks in self.winId.keys():
            try:
                group, key, subkey = gks.split(':')
                value = self.settings.Get(group, key, subkey)
            except ValueError:
                group, key, subkey, subkey1 = gks.split(':')
                value = self.settings.Get(group, key, [subkey, subkey1])
            win = self.FindWindowById(self.winId[gks])
            if win.GetName() in ('GetValue', 'IsChecked'):
                value = win.SetValue(value)
            elif win.GetName() == 'GetSelection':
                value = win.SetSelection(value)
            elif win.GetName() == 'GetStringSelection':
                value = win.SetStringSelection(value)
            else:
                value = win.SetValue(value)
        
    def OnApply(self, event):
        """!Button 'Apply' pressed"""
        if self._updateSettings():
            self.parent.goutput.WriteLog(_('Settings applied to current session but not saved'))
            self.Close()

    def OnCloseWindow(self, event):
        self.Hide()
        
    def OnCancel(self, event):
        """!Button 'Cancel' pressed"""
        self.Close()
        
    def OnSave(self, event):
        """!Button 'Save' pressed"""
        if self._updateSettings():
            file = self.settings.SaveToFile()
            self.parent.goutput.WriteLog(_('Settings saved to file \'%s\'.') % file)
            self.Close()

    def _updateSettings(self):
        """!Update user settings"""
        for item in self.winId.keys():
            try:
                group, key, subkey = item.split(':')
                subkey1 = None
            except ValueError:
                group, key, subkey, subkey1 = item.split(':')
            
            id = self.winId[item]
            win = self.FindWindowById(id)
            if win.GetName() == 'GetValue':
                value = win.GetValue()
            elif win.GetName() == 'GetSelection':
                value = win.GetSelection()
            elif win.GetName() == 'IsChecked':
                value = win.IsChecked()
            elif win.GetName() == 'GetStringSelection':
                value = win.GetStringSelection()
            elif win.GetName() == 'GetColour':
                value = tuple(win.GetValue())
            else:
                value = win.GetValue()

            if key == 'keycolumn' and value == '':
                wx.MessageBox(parent=self,
                              message=_("Key column cannot be empty string."),
                              caption=_("Error"), style=wx.OK | wx.ICON_ERROR)
                win.SetValue(self.settings.Get(group='atm', key='keycolumn', subkey='value'))
                return False

            if subkey1:
                self.settings.Set(group, value, key, [subkey, subkey1])
            else:
                self.settings.Set(group, value, key, subkey)
        
        return True

class PreferencesDialog(PreferencesBaseDialog):
    """!User preferences dialog"""
    def __init__(self, parent, title = _("GUI settings"),
                 settings = globalSettings):
        
        PreferencesBaseDialog.__init__(self, parent = parent, title = title,
                                       settings = settings)
        
        # create notebook pages
        self._CreateGeneralPage(self.notebook)
        self._CreateDisplayPage(self.notebook)
        self._CreateCmdPage(self.notebook)
        self._CreateAttributeManagerPage(self.notebook)
        self._CreateProjectionPage(self.notebook)
        self._CreateWorkspacePage(self.notebook)
        self._CreateAdvancedPage(self.notebook)
        
        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)
        
    def _CreateGeneralPage(self, notebook):
        """!Create notebook page for general settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("General"))

        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("General settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)

        #
        # expand element list
        #
        row = 0
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Element list:")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        elementList = wx.Choice(parent=panel, id=wx.ID_ANY, 
                                choices=self.settings.Get(group='general', key='elementListExpand',
                                                          subkey='choices', internal=True),
                                name="GetSelection")
        elementList.SetSelection(self.settings.Get(group='general', key='elementListExpand',
                                                   subkey='selection'))
        self.winId['general:elementListExpand:selection'] = elementList.GetId()

        gridSizer.Add(item=elementList,
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 1))

        #
        # default window layout
        #
        row += 1
        defaultPos = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                 label=_("Save current window layout as default"),
                                 name='IsChecked')
        defaultPos.SetValue(self.settings.Get(group='general', key='defWindowPos', subkey='enabled'))
        defaultPos.SetToolTip(wx.ToolTip (_("Save current position and size of Layer Manager window and opened "
                                            "Map Display window(s) and use as default for next sessions.")))
        self.winId['general:defWindowPos:enabled'] = defaultPos.GetId()

        gridSizer.Add(item=defaultPos,
                      pos=(row, 0), span=(1, 2))
        
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        #
        # Layer Manager settings
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Layer Manager settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)


        #
        # ask when removing map layer from layer tree
        #
        row = 0
        askOnRemoveLayer = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                       label=_("Ask when removing map layer from layer tree"),
                                       name='IsChecked')
        askOnRemoveLayer.SetValue(self.settings.Get(group='manager', key='askOnRemoveLayer', subkey='enabled'))
        self.winId['manager:askOnRemoveLayer:enabled'] = askOnRemoveLayer.GetId()

        gridSizer.Add(item=askOnRemoveLayer,
                      pos=(row, 0), span=(1, 2))

        row += 1
        askOnQuit = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Ask when quiting wxGUI or closing display"),
                                name='IsChecked')
        askOnQuit.SetValue(self.settings.Get(group='manager', key='askOnQuit', subkey='enabled'))
        self.winId['manager:askOnQuit:enabled'] = askOnQuit.GetId()

        gridSizer.Add(item=askOnQuit,
                      pos=(row, 0), span=(1, 2))
        
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)

        panel.SetSizer(border)
        
        return panel

    def _CreateDisplayPage(self, notebook):
        """!Create notebook page for display settings"""
   
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Display"))

        border = wx.BoxSizer(wx.VERTICAL)

        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)

        #
        # font settings
        #
        row = 0
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Default font for GRASS displays:")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        fontButton = wx.Button(parent=panel, id=wx.ID_ANY,
                               label=_("Set font"), size=(100, -1))
        gridSizer.Add(item=fontButton,
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 1))

        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        row = 1
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Font for command output:")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        outfontButton = wx.Button(parent=panel, id=wx.ID_ANY,
                               label=_("Set font"), size=(100, -1))
        gridSizer.Add(item=outfontButton,
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 1))

        #
        # display settings
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Default display settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)

        #
        # display driver
        #
        row = 0
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Display driver:")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        listOfDrivers = self.settings.Get(group='display', key='driver', subkey='choices', internal=True)
        driver = wx.Choice(parent=panel, id=wx.ID_ANY, size=(150, -1),
                           choices=listOfDrivers,
                           name="GetStringSelection")
        driver.SetStringSelection(self.settings.Get(group='display', key='driver', subkey='type'))
        self.winId['display:driver:type'] = driver.GetId()
        
        gridSizer.Add(item=driver,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))
        
        #
        # Statusbar mode
        #
        row += 1
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Statusbar mode:")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        listOfModes = self.settings.Get(group='display', key='statusbarMode', subkey='choices', internal=True)
        statusbarMode = wx.Choice(parent=panel, id=wx.ID_ANY, size=(150, -1),
                                  choices=listOfModes,
                                  name="GetSelection")
        statusbarMode.SetSelection(self.settings.Get(group='display', key='statusbarMode', subkey='selection'))
        self.winId['display:statusbarMode:selection'] = statusbarMode.GetId()

        gridSizer.Add(item=statusbarMode,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))

        #
        # Background color
        #
        row += 1
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Background color:")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        bgColor = csel.ColourSelect(parent=panel, id=wx.ID_ANY,
                                    colour=self.settings.Get(group='display', key='bgcolor', subkey='color'),
                                    size=globalvar.DIALOG_COLOR_SIZE)
        bgColor.SetName('GetColour')
        self.winId['display:bgcolor:color'] = bgColor.GetId()
        
        gridSizer.Add(item=bgColor,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))
        
        #
        # Use computation resolution
        #
        row += 1
        compResolution = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                    label=_("Constrain display resolution to computational settings"),
                                    name="IsChecked")
        compResolution.SetValue(self.settings.Get(group='display', key='compResolution', subkey='enabled'))
        self.winId['display:compResolution:enabled'] = compResolution.GetId()

        gridSizer.Add(item=compResolution,
                      pos=(row, 0), span=(1, 2))

        #
        # auto-rendering
        #
        row += 1
        autoRendering = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                    label=_("Enable auto-rendering"),
                                    name="IsChecked")
        autoRendering.SetValue(self.settings.Get(group='display', key='autoRendering', subkey='enabled'))
        self.winId['display:autoRendering:enabled'] = autoRendering.GetId()

        gridSizer.Add(item=autoRendering,
                      pos=(row, 0), span=(1, 2))
        
        #
        # auto-zoom
        #
        row += 1
        autoZooming = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                  label=_("Enable auto-zooming to selected map layer"),
                                  name="IsChecked")
        autoZooming.SetValue(self.settings.Get(group='display', key='autoZooming', subkey='enabled'))
        self.winId['display:autoZooming:enabled'] = autoZooming.GetId()

        gridSizer.Add(item=autoZooming,
                      pos=(row, 0), span=(1, 2))

        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)
        
        panel.SetSizer(border)
                
        # bindings
        fontButton.Bind(wx.EVT_BUTTON, self.OnSetFont)
        outfontButton.Bind(wx.EVT_BUTTON, self.OnSetOutputFont)
        
        return panel

    def _CreateCmdPage(self, notebook):
        """!Create notebook page for commad dialog settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Command"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Command dialog settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)
        
        #
        # command dialog settings
        #
        row = 0
        # overwrite
        overwrite = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Allow output files to overwrite existing files"),
                                name="IsChecked")
        overwrite.SetValue(self.settings.Get(group='cmd', key='overwrite', subkey='enabled'))
        self.winId['cmd:overwrite:enabled'] = overwrite.GetId()
        
        gridSizer.Add(item=overwrite,
                      pos=(row, 0), span=(1, 2))
        row += 1
        # close
        close = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                            label=_("Close dialog when command is successfully finished"),
                            name="IsChecked")
        close.SetValue(self.settings.Get(group='cmd', key='closeDlg', subkey='enabled'))
        self.winId['cmd:closeDlg:enabled'] = close.GetId()
        
        gridSizer.Add(item=close,
                      pos=(row, 0), span=(1, 2))
        row += 1
        # add layer
        add = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                          label=_("Add created map into layer tree"),
                          name="IsChecked")
        add.SetValue(self.settings.Get(group='cmd', key='addNewLayer', subkey='enabled'))
        self.winId['cmd:addNewLayer:enabled'] = add.GetId()
    
        gridSizer.Add(item=add,
                      pos=(row, 0), span=(1, 2))
        
        row += 1
        # interactive input
        interactive = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                  label = _("Allow interactive input"),
                                  name = "IsChecked")
        interactive.SetValue(self.settings.Get(group='cmd', key='interactiveInput', subkey='enabled'))
        self.winId['cmd:interactiveInput:enabled'] = interactive.GetId()
        gridSizer.Add(item = interactive,
                      pos = (row, 0), span = (1, 2))
        
        row += 1
        # verbosity
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Verbosity level:")),
                      flag=wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0))
        verbosity = wx.Choice(parent=panel, id=wx.ID_ANY, size=(200, -1),
                              choices=self.settings.Get(group='cmd', key='verbosity', subkey='choices', internal=True),
                              name="GetStringSelection")
        verbosity.SetStringSelection(self.settings.Get(group='cmd', key='verbosity', subkey='selection'))
        self.winId['cmd:verbosity:selection'] = verbosity.GetId()
        
        gridSizer.Add(item=verbosity,
                      pos=(row, 1))
        
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)
        
        #
        # raster settings
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Raster settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)
        
        #
        # raster overlay
        #
        row = 0
        rasterOpaque = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                    label=_("Make null cells opaque"),
                                    name='IsChecked')
        rasterOpaque.SetValue(self.settings.Get(group='cmd', key='rasterOpaque', subkey='enabled'))
        self.winId['cmd:rasterOpaque:enabled'] = rasterOpaque.GetId()
        
        gridSizer.Add(item=rasterOpaque,
                      pos=(row, 0), span=(1, 2))

        # default color table
        row += 1
        rasterCTCheck = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                    label=_("Default color table"),
                                    name='IsChecked')
        rasterCTCheck.SetValue(self.settings.Get(group='cmd', key='rasterColorTable', subkey='enabled'))
        self.winId['cmd:rasterColorTable:enabled'] = rasterCTCheck.GetId()
        rasterCTCheck.Bind(wx.EVT_CHECKBOX, self.OnCheckColorTable)
        
        gridSizer.Add(item=rasterCTCheck,
                      pos=(row, 0))
        
        rasterCTName = wx.Choice(parent=panel, id=wx.ID_ANY, size=(200, -1),
                               choices=utils.GetColorTables(),
                               name="GetStringSelection")
        rasterCTName.SetStringSelection(self.settings.Get(group='cmd', key='rasterColorTable', subkey='selection'))
        self.winId['cmd:rasterColorTable:selection'] = rasterCTName.GetId()
        if not rasterCTCheck.IsChecked():
            rasterCTName.Enable(False)
        
        gridSizer.Add(item=rasterCTName,
                      pos=(row, 1))
        
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)
        
        #
        # vector settings
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Vector settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.FlexGridSizer (cols=7, hgap=3, vgap=3)
        
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Display:")),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        
        for type in ('point', 'line', 'centroid', 'boundary',
                     'area', 'face'):
            chkbox = wx.CheckBox(parent=panel, label=type)
            checked = self.settings.Get(group='cmd', key='showType',
                                        subkey=[type, 'enabled'])
            chkbox.SetValue(checked)
            self.winId['cmd:showType:%s:enabled' % type] = chkbox.GetId()
            gridSizer.Add(item=chkbox)

        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)
        
        panel.SetSizer(border)
        
        return panel

    def _CreateAttributeManagerPage(self, notebook):
        """!Create notebook page for 'Attribute Table Manager' settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Attributes"))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        #
        # highlighting
        #
        highlightBox = wx.StaticBox(parent=panel, id=wx.ID_ANY,
                                    label=" %s " % _("Highlighting"))
        highlightSizer = wx.StaticBoxSizer(highlightBox, wx.VERTICAL)

        flexSizer = wx.FlexGridSizer (cols=2, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)
        
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Color:"))
        hlColor = csel.ColourSelect(parent=panel, id=wx.ID_ANY,
                                    colour=self.settings.Get(group='atm', key='highlight', subkey='color'),
                                    size=globalvar.DIALOG_COLOR_SIZE)
        hlColor.SetName('GetColour')
        self.winId['atm:highlight:color'] = hlColor.GetId()

        flexSizer.Add(label, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(hlColor, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Line width (in pixels):"))
        hlWidth = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(50, -1),
                              initial=self.settings.Get(group='atm', key='highlight',subkey='width'),
                              min=1, max=1e6)
        self.winId['atm:highlight:width'] = hlWidth.GetId()

        flexSizer.Add(label, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(hlWidth, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        highlightSizer.Add(item=flexSizer,
                           proportion=0,
                           flag=wx.ALL | wx.EXPAND,
                           border=5)

        pageSizer.Add(item=highlightSizer,
                      proportion=0,
                      flag=wx.ALL | wx.EXPAND,
                      border=5)

        #
        # data browser related settings
        #
        dataBrowserBox = wx.StaticBox(parent=panel, id=wx.ID_ANY,
                                    label=" %s " % _("Data browser"))
        dataBrowserSizer = wx.StaticBoxSizer(dataBrowserBox, wx.VERTICAL)

        flexSizer = wx.FlexGridSizer (cols=2, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Left mouse double click:"))
        leftDbClick = wx.Choice(parent=panel, id=wx.ID_ANY,
                                choices=self.settings.Get(group='atm', key='leftDbClick', subkey='choices', internal=True),
                                name="GetSelection")
        leftDbClick.SetSelection(self.settings.Get(group='atm', key='leftDbClick', subkey='selection'))
        self.winId['atm:leftDbClick:selection'] = leftDbClick.GetId()

        flexSizer.Add(label, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(leftDbClick, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        # encoding
        label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                              label=_("Encoding (e.g. utf-8, ascii, iso8859-1, koi8-r):"))
        encoding = wx.TextCtrl(parent=panel, id=wx.ID_ANY,
                               value=self.settings.Get(group='atm', key='encoding', subkey='value'),
                               name="GetValue", size=(200, -1))
        self.winId['atm:encoding:value'] = encoding.GetId()

        flexSizer.Add(label, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(encoding, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        # ask on delete record
        askOnDeleteRec = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                     label=_("Ask when deleting data record(s) from table"),
                                     name='IsChecked')
        askOnDeleteRec.SetValue(self.settings.Get(group='atm', key='askOnDeleteRec', subkey='enabled'))
        self.winId['atm:askOnDeleteRec:enabled'] = askOnDeleteRec.GetId()

        flexSizer.Add(askOnDeleteRec, proportion=0)

        dataBrowserSizer.Add(item=flexSizer,
                           proportion=0,
                           flag=wx.ALL | wx.EXPAND,
                           border=5)

        pageSizer.Add(item=dataBrowserSizer,
                      proportion=0,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                      border=3)

        #
        # create table
        #
        createTableBox = wx.StaticBox(parent=panel, id=wx.ID_ANY,
                                    label=" %s " % _("Create table"))
        createTableSizer = wx.StaticBoxSizer(createTableBox, wx.VERTICAL)

        flexSizer = wx.FlexGridSizer (cols=2, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)

        label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                              label=_("Key column:"))
        keyColumn = wx.TextCtrl(parent=panel, id=wx.ID_ANY,
                                size=(250, -1))
        keyColumn.SetValue(self.settings.Get(group='atm', key='keycolumn', subkey='value'))
        self.winId['atm:keycolumn:value'] = keyColumn.GetId()
        
        flexSizer.Add(label, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(keyColumn, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        createTableSizer.Add(item=flexSizer,
                             proportion=0,
                             flag=wx.ALL | wx.EXPAND,
                             border=5)

        pageSizer.Add(item=createTableSizer,
                      proportion=0,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                      border=3)
        
        panel.SetSizer(pageSizer)

        return panel

    def _CreateProjectionPage(self, notebook):
        """!Create notebook page for workspace settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Projection"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        #
        # projections statusbar settings
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Projection statusbar settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(1)

        # epsg
        row = 0
        label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                              label=_("EPSG code:"))
        epsgCode = wx.ComboBox(parent=panel, id=wx.ID_ANY,
                               name="GetValue",
                               size = (150, -1))
        self.epsgCodeDict = dict()
        epsgCode.SetValue(str(self.settings.Get(group='projection', key='statusbar', subkey='epsg')))
        self.winId['projection:statusbar:epsg'] = epsgCode.GetId()
        
        gridSizer.Add(item=label,
                      pos=(row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=epsgCode,
                      pos=(row, 1), span=(1, 2))
        
        # proj
        row += 1
        label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                              label=_("Proj.4 string (required):"))
        projString = wx.TextCtrl(parent=panel, id=wx.ID_ANY,
                                 value=self.settings.Get(group='projection', key='statusbar', subkey='proj4'),
                                 name="GetValue", size=(400, -1))
        self.winId['projection:statusbar:proj4'] = projString.GetId()

        gridSizer.Add(item=label,
                      pos=(row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=projString,
                      pos=(row, 1), span=(1, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # epsg file
        row += 1
        label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                              label=_("EPSG file:"))
        projFile = wx.TextCtrl(parent=panel, id=wx.ID_ANY,
                               value = self.settings.Get(group='projection', key='statusbar', subkey='projFile'),
                               name="GetValue", size=(400, -1))
        self.winId['projection:statusbar:projFile'] = projFile.GetId()
        gridSizer.Add(item=label,
                      pos=(row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=projFile,
                      pos=(row, 1),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # note + button
        row += 1
        note = wx.StaticText(parent = panel, id = wx.ID_ANY,
                             label = _("Load EPSG codes (be patient), enter EPSG code or "
                                       "insert Proj.4 string directly."))
        gridSizer.Add(item=note,
                      span = (1, 2),
                      pos=(row, 0))

        row += 1
        epsgLoad = wx.Button(parent=panel, id=wx.ID_ANY,
                             label=_("&Load EPSG codes"))
        gridSizer.Add(item=epsgLoad,
                      flag = wx.ALIGN_RIGHT,
                      pos=(row, 1))
        
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        #
        # format
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Coordinates format"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(2)

        row = 0
        # ll format
        ll = wx.RadioBox(parent = panel, id = wx.ID_ANY,
                         label = " %s " % _("LL projections"),
                         choices = ["DMS", "DEG"],
                         name = "GetStringSelection")
        self.winId['projection:format:ll'] = ll.GetId()
        if self.settings.Get(group = 'projection', key = 'format', subkey = 'll') == 'DMS':
            ll.SetSelection(0)
        else:
            ll.SetSelection(1)
        
        # precision
        precision =  wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                                 min = 0, max = 12,
                                 name = "GetValue")
        precision.SetValue(int(self.settings.Get(group = 'projection', key = 'format', subkey = 'precision')))
        self.winId['projection:format:precision'] = precision.GetId()
                
        gridSizer.Add(item=ll,
                      pos=(row, 0))
        gridSizer.Add(item=wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Precision:")),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT,
                      border = 20,
                      pos=(row, 1))
        gridSizer.Add(item=precision,
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 2))
        
        
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)
        
        panel.SetSizer(border)

        # bindings
        epsgLoad.Bind(wx.EVT_BUTTON, self.OnLoadEpsgCodes)
        epsgCode.Bind(wx.EVT_COMBOBOX, self.OnSetEpsgCode)
        epsgCode.Bind(wx.EVT_TEXT_ENTER, self.OnSetEpsgCode)
        
        return panel

    def _CreateWorkspacePage(self, notebook):
        """!Create notebook page for workspace settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Workspace"))

        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Loading workspace"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)

        row = 0

        #
        # positioning
        #
        posDisplay = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                 label=_("Suppress positioning Map Display Window(s)"),
                                 name='IsChecked')
        posDisplay.SetValue(self.settings.Get(group='workspace', key='posDisplay', subkey='enabled'))
        self.winId['workspace:posDisplay:enabled'] = posDisplay.GetId()

        gridSizer.Add(item=posDisplay,
                      pos=(row, 0), span=(1, 2))

        row +=1 

        posManager = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                 label=_("Suppress positioning Layer Manager window"),
                                 name='IsChecked')
        posManager.SetValue(self.settings.Get(group='workspace', key='posManager', subkey='enabled'))
        self.winId['workspace:posManager:enabled'] = posManager.GetId()

        gridSizer.Add(item=posManager,
                      pos=(row, 0), span=(1, 2))

        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        panel.SetSizer(border)
        
        return panel

    def _CreateAdvancedPage(self, notebook):
        """!Create notebook page for advanced settings"""
        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Advanced"))

        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Advanced settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=3, vgap=3)
        gridSizer.AddGrowableCol(0)

        row = 0
        #
        # icon theme
        #
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Icon theme:")),
                       flag=wx.ALIGN_LEFT |
                       wx.ALIGN_CENTER_VERTICAL,
                       pos=(row, 0))
        iconTheme = wx.Choice(parent=panel, id=wx.ID_ANY, size=(125, -1),
                              choices=self.settings.Get(group='advanced', key='iconTheme',
                                                        subkey='choices', internal=True),
                              name="GetStringSelection")
        iconTheme.SetStringSelection(self.settings.Get(group='advanced', key='iconTheme', subkey='type'))
        self.winId['advanced:iconTheme:type'] = iconTheme.GetId()

        gridSizer.Add(item=iconTheme,
                      flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 1))
        
        row += 1
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Note: For changing the icon theme, "
                                                 "you must save the settings and restart this GUI.")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0), span=(1, 2))
                      
        sizer.Add(item=gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(item=sizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=3)

        panel.SetSizer(border)
        
        return panel

    def OnCheckColorTable(self, event):
        """!Set/unset default color table"""
        win = self.FindWindowById(self.winId['cmd:rasterColorTable:selection'])
        if event.IsChecked():
            win.Enable()
        else:
            win.Enable(False)
        
    def OnLoadEpsgCodes(self, event):
        """!Load EPSG codes from the file"""
        win = self.FindWindowById(self.winId['projection:statusbar:projFile'])
        path = win.GetValue()

        self.epsgCodeDict = utils.ReadEpsgCodes(path)
        list = self.FindWindowById(self.winId['projection:statusbar:epsg'])
        if type(self.epsgCodeDict) == type(''):
            wx.MessageBox(parent=self,
                          message=_("Unable to read EPSG codes: %s") % self.epsgCodeDict,
                          caption=_("Error"),  style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            self.epsgCodeDict = dict()
            list.SetItems([])
            list.SetValue('')
            self.FindWindowById(self.winId['projection:statusbar:proj4']).SetValue('')
            return
        
        choices = map(str, self.epsgCodeDict.keys())

        list.SetItems(choices)
        try:
            code = int(list.GetValue())
        except ValueError:
            code = -1
        win = self.FindWindowById(self.winId['projection:statusbar:proj4'])
        if self.epsgCodeDict.has_key(code):
            win.SetValue(self.epsgCodeDict[code][1])
        else:
            list.SetSelection(0)
            code = int(list.GetStringSelection())
            win.SetValue(self.epsgCodeDict[code][1])
    
    def OnSetEpsgCode(self, event):
        """!EPSG code selected"""
        winCode = self.FindWindowById(event.GetId())
        win = self.FindWindowById(self.winId['projection:statusbar:proj4'])
        if not self.epsgCodeDict:
            wx.MessageBox(parent=self,
                          message=_("EPSG code %s not found") % event.GetString(),
                          caption=_("Error"),  style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            winCode.SetValue('')
            win.SetValue('')
        
        try:
            code = int(event.GetString())
        except ValueError:
            wx.MessageBox(parent=self,
                          message=_("EPSG code %s not found") % str(code),
                          caption=_("Error"),  style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            winCode.SetValue('')
            win.SetValue('')
        
        
        try:
            win.SetValue(self.epsgCodeDict[code][1].replace('<>', '').strip())
        except KeyError:
            wx.MessageBox(parent=self,
                          message=_("EPSG code %s not found") % str(code),
                          caption=_("Error"),  style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            winCode.SetValue('')
            win.SetValue('')
        
    def OnSetFont(self, event):
        """'Set font' button pressed"""
        dlg = DefaultFontDialog(parent=self, id=wx.ID_ANY,
                                title=_('Select default display font'),
                                style=wx.DEFAULT_DIALOG_STYLE,
                                type='font')
        
        if dlg.ShowModal() == wx.ID_OK:
            # set default font and encoding environmental variables
            if dlg.font:
                os.environ["GRASS_FONT"] = dlg.font
                self.settings.Set(group='display', value=dlg.font,
                                  key='font', subkey='type')

            if dlg.encoding and \
                    dlg.encoding != "ISO-8859-1":
                os.environ["GRASS_ENCODING"] = dlg.encoding
                self.settings.Set(group='display', value=dlg.encoding,
                                  key='font', subkey='encoding')
                
        dlg.Destroy()
        
        event.Skip()

    def OnSetOutputFont(self, event):
        """'Set output font' button pressed"""
        
        dlg = DefaultFontDialog(parent=self, id=wx.ID_ANY,
                                title=_('Select output font'),
                                style=wx.DEFAULT_DIALOG_STYLE,
                                type='outputfont')
        
        if dlg.ShowModal() == wx.ID_OK:
            # set output font and font size variables
            if dlg.font:
                self.settings.Set(group='display', value=dlg.font,
                                  key='outputfont', subkey='type')

                self.settings.Set(group='display', value=dlg.fontsize,
                                  key='outputfont', subkey='size')

# Standard font dialog broken for Mac in OS X 10.6
#        type = self.settings.Get(group='display', key='outputfont', subkey='type')   
                           
#        size = self.settings.Get(group='display', key='outputfont', subkey='size')
#        if size == None or size == 0: size = 10
#        size = float(size)
        
#        data = wx.FontData()
#        data.EnableEffects(True)
#        data.SetInitialFont(wx.Font(pointSize=size, family=wx.FONTFAMILY_MODERN, faceName=type, style=wx.NORMAL, weight=0))

#        dlg = wx.FontDialog(self, data)

#        if dlg.ShowModal() == wx.ID_OK:
#            data = dlg.GetFontData()
#            font = data.GetChosenFont()

#            self.settings.Set(group='display', value=font.GetFaceName(),
#                                  key='outputfont', subkey='type')
#            self.settings.Set(group='display', value=font.GetPointSize(),
#                                  key='outputfont', subkey='size')
                
        dlg.Destroy()

        event.Skip()
        
    def _updateSettings(self):
        """!Update user settings"""
        PreferencesBaseDialog._updateSettings(self)
        
        #
        # update default window dimension
        #
        if self.settings.Get(group='general', key='defWindowPos', subkey='enabled') is True:
            dim = ''
            # layer manager
            pos = self.parent.GetPosition()
            size = self.parent.GetSize()
            dim = '%d,%d,%d,%d' % (pos[0], pos[1], size[0], size[1])
            # opened displays
            for page in range(0, self.parent.gm_cb.GetPageCount()):
                pos = self.parent.gm_cb.GetPage(page).maptree.mapdisplay.GetPosition()
                size = self.parent.gm_cb.GetPage(page).maptree.mapdisplay.GetSize()

                dim += ',%d,%d,%d,%d' % (pos[0], pos[1], size[0], size[1])

            self.settings.Set(group='general', key='defWindowPos', subkey='dim', value=dim)
        else:
            self.settings.Set(group='general', key='defWindowPos', subkey='dim', value='')

        return True

class DefaultFontDialog(wx.Dialog):
    """
    Opens a file selection dialog to select default font
    to use in all GRASS displays
    """
    def __init__(self, parent, id, title,
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE |
                 wx.RESIZE_BORDER,
                 settings=globalSettings,
                 type='font'):
        
        self.settings = settings
        self.type = type
        
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

        panel = wx.Panel(parent=self, id=wx.ID_ANY)
        
        self.fontlist = self.GetFonts()
        
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap=5, vgap=5)
        gridSizer.AddGrowableCol(0)

        label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                              label=_("Select font:"))
        gridSizer.Add(item=label,
                      flag=wx.ALIGN_TOP,
                      pos=(0,0))
        
        self.fontlb = wx.ListBox(parent=panel, id=wx.ID_ANY, pos=wx.DefaultPosition,
                                 choices=self.fontlist,
                                 style=wx.LB_SINGLE|wx.LB_SORT)
        self.Bind(wx.EVT_LISTBOX, self.EvtListBox, self.fontlb)
        self.Bind(wx.EVT_LISTBOX_DCLICK, self.EvtListBoxDClick, self.fontlb)

        gridSizer.Add(item=self.fontlb,
                flag=wx.EXPAND, pos=(1, 0))

        if self.type == 'font':
            if "GRASS_FONT" in os.environ:
                self.font = os.environ["GRASS_FONT"]
            else:
                self.font = self.settings.Get(group='display',
                                              key='font', subkey='type')
            self.encoding = self.settings.Get(group='display',
                                          key='font', subkey='encoding')

            label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                                  label=_("Character encoding:"))
            gridSizer.Add(item=label,
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(2, 0))

            self.textentry = wx.TextCtrl(parent=panel, id=wx.ID_ANY,
                                         value=self.encoding)
            gridSizer.Add(item=self.textentry,
                    flag=wx.EXPAND, pos=(3, 0))

            self.textentry.Bind(wx.EVT_TEXT, self.OnEncoding)

        elif self.type == 'outputfont':
            self.font = self.settings.Get(group='display',
                                              key='outputfont', subkey='type')
            self.fontsize = self.settings.Get(group='display',
                                          key='outputfont', subkey='size')
            label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                              label=_("Font size:"))
            gridSizer.Add(item=label,
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2, 0))
                      
            self.spin = wx.SpinCtrl(parent=panel, id=wx.ID_ANY)
            if self.fontsize:
                self.spin.SetValue(self.fontsize)
            self.spin.Bind(wx.EVT_SPINCTRL, self.OnSizeSpin)
            self.spin.Bind(wx.EVT_TEXT, self.OnSizeSpin)
            gridSizer.Add(item=self.spin,
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(3, 0))

        else: 
            return

        if self.font:
            self.fontlb.SetStringSelection(self.font, True)

        sizer.Add(item=gridSizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL,
                  border=5)

        border.Add(item=sizer, proportion=1,
                   flag=wx.ALL | wx.EXPAND, border=3)
        
        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(parent=panel, id=wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(parent=panel, id=wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        border.Add(item=btnsizer, proportion=0,
                   flag=wx.EXPAND | wx.ALIGN_RIGHT | wx.ALL, border=5)
        
        panel.SetAutoLayout(True)
        panel.SetSizer(border)
        border.Fit(self)
        
        self.Layout()
        
    def EvtRadioBox(self, event):
        if event.GetInt() == 0:
            self.fonttype = 'grassfont'
        elif event.GetInt() == 1:
            self.fonttype = 'truetype'

        self.fontlist = self.GetFonts(self.fonttype)
        self.fontlb.SetItems(self.fontlist)

    def OnEncoding(self, event):
        self.encoding = event.GetString()

    def EvtListBox(self, event):
        self.font = event.GetString()
        event.Skip()

    def EvtListBoxDClick(self, event):
        self.font = event.GetString()
        event.Skip()
        
    def OnSizeSpin(self, event):
        self.fontsize = self.spin.GetValue()
        event.Skip()
    
    def GetFonts(self):
        """
        parses fonts directory or fretypecap file to get a list of fonts for the listbox
        """
        fontlist = []

        cmd = ["d.font", "-l"]

        ret = gcmd.RunCommand('d.font',
                              read = True,
                              flags = 'l')

        if not ret:
            return fontlist

        dfonts = ret.splitlines()
        dfonts.sort(lambda x,y: cmp(x.lower(), y.lower()))
        for item in range(len(dfonts)):
           # ignore duplicate fonts and those starting with #
           if not dfonts[item].startswith('#') and \
                  dfonts[item] != dfonts[item-1]:
              fontlist.append(dfonts[item])

        return fontlist

class MapsetAccess(wx.Dialog):
    """!Controls setting options and displaying/hiding map overlay
    decorations
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 title=_('Manage access to mapsets'),
                 size = (350, 400),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        wx.Dialog.__init__(self, parent, id, title, size = size, style = style)

        self.all_mapsets_ordered = utils.ListOfMapsets(get = 'ordered')
        self.accessible_mapsets  = utils.ListOfMapsets(get = 'accessible')
        self.curr_mapset = grass.gisenv()['MAPSET']

        # make a checklistbox from available mapsets and check those that are active
        sizer = wx.BoxSizer(wx.VERTICAL)

        label = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label=_("Check a mapset to make it accessible, uncheck it to hide it.\n"
                                      "  Notes:\n"
                                      "    - The current mapset is always accessible.\n"
                                      "    - You may only write to the current mapset.\n"
                                      "    - You may only write to mapsets which you own."))
        
        sizer.Add(item=label, proportion=0,
                  flag=wx.ALL, border=5)

        self.mapsetlb = CheckListMapset(parent=self)
        self.mapsetlb.LoadData()
        
        sizer.Add(item=self.mapsetlb, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=5)

        # check all accessible mapsets
        for mset in self.accessible_mapsets:
            self.mapsetlb.CheckItem(self.all_mapsets_ordered.index(mset), True)

        # FIXME (howto?): grey-out current mapset
        #self.mapsetlb.Enable(0, False)

        # dialog buttons
        line = wx.StaticLine(parent=self, id=wx.ID_ANY,
                             style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTRE | wx.ALL, border=5)

        btnsizer = wx.StdDialogButtonSizer()
        okbtn = wx.Button(self, wx.ID_OK)
        okbtn.SetDefault()
        btnsizer.AddButton(okbtn)

        cancelbtn = wx.Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(cancelbtn)
        btnsizer.Realize()

        sizer.Add(item=btnsizer, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_RIGHT | wx.ALL, border=5)

        # do layout
        self.Layout()
        self.SetSizer(sizer)
        sizer.Fit(self)

        self.SetMinSize(size)
        
    def GetMapsets(self):
        """!Get list of checked mapsets"""
        ms = []
        i = 0
        for mset in self.all_mapsets_ordered:
            if self.mapsetlb.IsChecked(i):
                ms.append(mset)
            i += 1

        return ms

class CheckListMapset(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.CheckListCtrlMixin):
    """!List of mapset/owner/group"""
    def __init__(self, parent, pos=wx.DefaultPosition,
                 log=None):
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style=wx.LC_REPORT)
        listmix.CheckListCtrlMixin.__init__(self)
        self.log = log

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)

    def LoadData(self):
        """!Load data into list"""
        self.InsertColumn(0, _('Mapset'))
        self.InsertColumn(1, _('Owner'))
        ### self.InsertColumn(2, _('Group'))
        gisenv = grass.gisenv()
        locationPath = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'])

        for mapset in self.parent.all_mapsets_ordered:
            index = self.InsertStringItem(sys.maxint, mapset)
            mapsetPath = os.path.join(locationPath,
                                      mapset)
            stat_info = os.stat(mapsetPath)
            if havePwd:
                self.SetStringItem(index, 1, "%s" % pwd.getpwuid(stat_info.st_uid)[0])
                # FIXME: get group name
                ### self.SetStringItem(index, 2, "%-8s" % stat_info.st_gid) 
            else:
                # FIXME: no pwd under MS Windows (owner: 0, group: 0)
                self.SetStringItem(index, 1, "%-8s" % stat_info.st_uid)
                ### self.SetStringItem(index, 2, "%-8s" % stat_info.st_gid)
                
        self.SetColumnWidth(col=0, width=wx.LIST_AUTOSIZE)
        ### self.SetColumnWidth(col=1, width=wx.LIST_AUTOSIZE)
        
    def OnCheckItem(self, index, flag):
        """!Mapset checked/unchecked"""
        mapset = self.parent.all_mapsets_ordered[index]
        if mapset == self.parent.curr_mapset:
            self.CheckItem(index, True)
