"""!
@package core.settings

@brief Default GUI settings

List of classes:
 - settings::Settings

Usage:
@code
from core.settings import UserSettings
@endcode

(C) 2007-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Luca Delucchi <lucadeluge gmail.com> (language choice)
"""

import os
import sys
import copy
import types

from core       import globalvar
from core.gcmd  import GException, GError
from core.utils import GetSettingsPath, PathJoin, rgb2str, _

class Settings:
    """!Generic class where to store settings"""
    def __init__(self):
        # settings file
        self.filePath = os.path.join(GetSettingsPath(), 'wx')
        
        # key/value separator
        self.sep = ';'
        
        # define default settings
        self._defaultSettings()  # -> self.defaultSettings
        
        # read settings from the file
        self.userSettings = copy.deepcopy(self.defaultSettings)
        try:
            self.ReadSettingsFile()
        except GException, e:
            print >> sys.stderr, e.value
        
        # define internal settings
        self._internalSettings() # -> self.internalSettings

    def _generateLocale(self):
        """!Generate locales
        """
        try:
            self.locs = os.listdir(os.path.join(os.environ['GISBASE'], 'locale'))
            self.locs.append('en') # GRASS doesn't ship EN po files
            self.locs.sort()
            # Add a default choice to not override system locale
            self.locs.insert(0, 'system')
        except:
            # No NLS
            self.locs = ['system']
        
        return 'system'
        
    def _defaultSettings(self):
        """!Define default settings
        """
        try:
            projFile = PathJoin(os.environ["GRASS_PROJSHARE"], 'epsg')
        except KeyError:
            projFile = ''
        
        id_loc = self._generateLocale()
        
        self.defaultSettings = {
            #
            # general
            #
            'general': {
                # use default window layout (layer manager, displays, ...)
                'defWindowPos' : {
                    'enabled' : True,
                    'dim' : '0,0,%d,%d,%d,0,%d,%d' % \
                        (globalvar.GM_WINDOW_SIZE[0],
                         globalvar.GM_WINDOW_SIZE[1],
                         globalvar.GM_WINDOW_SIZE[0],
                         globalvar.MAP_WINDOW_SIZE[0],
                         globalvar.MAP_WINDOW_SIZE[1])
                    },
                # workspace
                'workspace' : {
                    'posDisplay' : {
                        'enabled' : False
                        },
                    'posManager' : {
                        'enabled' : False
                        },
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
                # hide tabs
                'hideTabs' : {
                    'search' : False,
                    'pyshell' : False,
                    },
                'copySelectedTextToClipboard' : {
                    'enabled' : False
                    },
                },
            #
            # appearance
            #
            'appearance': {
                'outputfont' : {
                    'type' : 'Courier New',
                    'size': '10',
                    },
                # expand/collapse element list
                'elementListExpand' : {
                    'selection' : 0 
                    },
                'menustyle' : {
                    'selection' : 1
                    },
                'gSelectPopupHeight' : {
                    'value' : 200
                    },
                'iconTheme' : {
                    'type' : 'grass'
                    },
                'commandNotebook' : {
                    'selection' : 0
                    },
                },
            #
            # language
            #
            'language': {
                'locale': {
                    'lc_all' : id_loc
                }
            },
            #
            # display
            #
            'display': {
                'font' : {
                    'type' : '',
                    'encoding': 'ISO-8859-1',
                    },
                'driver': {
                    'type': 'cairo'
                    },
                'alignExtent' : {
                    'enabled' : True
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
                'mouseWheelZoom' : {
                    'selection' : 1,
                    },
                'scrollDirection' : {
                    'selection' : 0,
                    },
                'nvizDepthBuffer' : {
                    'value' : '16',
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
                    'enabled' : False
                    },
                'verbosity' : {
                    'selection' : 'grassenv'
                    },
                'addNewLayer' : {
                    'enabled' : True,
                    },
                'interactiveInput' : {
                    'enabled' : True,
                    },
                },
            #
            # d.rast
            #
            'rasterLayer': {
                'opaque': {
                    'enabled' : False
                    },
                'colorTable': {
                    'enabled' : False,
                    'selection' : 'rainbow'
                    },
                },
            #
            # d.vect
            #
            'vectorLayer': {
                'featureColor': {
                    'color' : (0, 0, 0),
                    'transparent' : {
                        'enabled': False
                        }
                    },
                'areaFillColor': {
                    'color' : (200, 200, 200),
                    'transparent' : {
                        'enabled': False
                        }
                    },
                'line': {
                    'width' : 0,
                    },
                'point': {
                    'symbol': 'basic/x',
                    'size' : 5,
                    },
                'showType': {
                    'point' : {
                        'enabled' : True
                        },
                    'line' : {
                        'enabled' : True
                        },
                    'centroid' : {
                        'enabled' : False
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
                },
            #
            # vdigit
            #
            'vdigit' : {
                # symbology
                'symbol' : {
                    'newSegment' : {
                        'enabled' : None,
                        'color' : (255, 0, 0, 255)
                        }, # red
                    'newLine' : {
                        'enabled' : None,
                        'color' : (0, 86, 45, 255)
                        }, # dark green
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
                        'enabled' : True,
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
                # close boundary (snap to the first node)
                'closeBoundary' : {
                    'enabled' : False,
                    }
                },
             # 
             # plots for profiles, histograms, and scatterplots
             #
            'profile': {
                'raster' : {
                    'pcolor'        : (0, 0, 255, 255), # line color
                    'pwidth'        : 1, # line width
                    'pstyle'        : 'solid', # line pen style
                    'datatype'      : 'cell', # raster type
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
             'histogram': {
                'raster' : {
                    'pcolor'        : (0, 0, 255, 255), # line color
                    'pwidth'        : 1, # line width
                    'pstyle'        : 'solid', # line pen style
                    'datatype'      : 'cell', # raster type
                    },
                'font' : {
                    'titleSize'     : 12,
                    'axisSize'      : 11,
                    'legendSize'    : 10,
                    },
                'grid' : {
                    'color'         : (200, 200, 200, 255),
                    'enabled'       : True,
                    },
                'x-axis' : {
                    'type'          : 'auto', # axis format
                    'min'           : 0, # axis min for custom axis range
                    'max'           : 0, # axis max for custom axis range
                    'log'           : False,
                    },
                'y-axis' : {
                    'type'          : 'auto', # axis format
                    'min'           : 0, # axis min for custom axis range
                    'max'           : 0, # axis max for custom axis range
                    'log'           : False,
                    },
                'legend' : {
                    'enabled'       : True
                    },
                },
             'scatter': {
                'raster' : {
                    'pcolor' : (0, 0, 255, 255),
                    'pfill' : 'solid',
                    'psize' : 1,
                    'ptype' : 'dot',
                    # FIXME: this is only a quick fix
                    # using also names used in a base class for compatibility
                    # probably used only for initialization
                    # base should be rewritten to not require this
                    'pwidth' : 1,  # required by wxplot/base, maybe useless here
                    'pstyle' : 'dot', # line pen style
                    'plegend' : _('Data point'),
                    0 : {'datatype' : 'CELL'},
                    1 : {'datatype' : 'CELL'},
                    },
                'font' : {
                    'titleSize'     : 12,
                    'axisSize'      : 11,
                    'legendSize'    : 10,
                    },
                'grid' : {
                    'color'         : (200, 200, 200, 255),
                    'enabled'       : True,
                    },
                'x-axis' : {
                    'type'          : 'auto', # axis format
                    'min'           : 0, # axis min for custom axis range
                    'max'           : 0, # axis max for custom axis range
                    'log'           : False,
                    },
                'y-axis' : {
                    'type'          : 'auto', # axis format
                    'min'           : 0, # axis min for custom axis range
                    'max'           : 0, # axis max for custom axis range
                    'log'           : False,
                    },
                'legend' : {
                    'enabled'       : True
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
            'nviz' : {
                'view' : {
                    'persp' : {
                        'value' : 20,
                        'step' : 2,
                        },
                    'position' : {
                        'x' : 0.84,
                        'y' : 0.16,
                        },
                    'twist' : {
                        'value' : 0,
                        },
                    'z-exag' : {
                        'min' : 0,
                        'max' : 10,
                        'value': 1,
                        },
                    'background' : {
                        'color' : (255, 255, 255, 255), # white
                        },
                    },
                'fly' : {
                    'exag' : {
                        'move' : 5,
                        'turn' : 5,
                        }
                    },
                'animation' : {
                    'fps' : 24,
                    'prefix' : _("animation")
                    },
                'surface' : {
                    'shine': {
                        'map' : False,
                        'value' : 60.0,
                        },
                    'color' : {
                        'map' : True,
                        'value' : (100, 100, 100, 255), # constant: grey
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
                'constant' : {
                    'color' : (100, 100, 100, 255),
                    'value' : 0.0,
                    'transp' : 0,
                    'resolution': 6
                },
                'vector' : {
                    'lines' : {
                        'show' : False,
                        'width' : 2,
                        'color' : (0, 0, 255, 255), # blue
                        'flat' : False,
                        'height' : 0,
                        'rgbcolumn': None,
                        'sizecolumn': None,
                        },
                    'points' : {
                        'show' : False,
                        'size' : 100,
                        'width' : 2,
                        'marker' : 2,
                        'color' : (0, 0, 255, 255), # blue
                        'height' : 0,
                        'rgbcolumn': None,
                        'sizecolumn': None,
                        }
                    },
                'volume' : {
                    'color' : {
                        'map' : True,
                        'value' : (100, 100, 100, 255), # constant: grey
                        },
                    'draw' : {
                        'mode'       : 0, # isosurfaces
                        'shading'    : 1, # gouraud
                        'resolution' : 3, # polygon resolution
                        'box'        : False # draw wire box
                        },
                    'shine': {
                        'map' : False,
                        'value' : 60,
                        },
                    'topo': {
                        'map' : None,
                        'value' : 0.0
                        },
                    'transp': {
                        'map' : None,
                        'value': 0
                        },
                    'mask': {
                        'map' : None,
                        'value': ''
                        },
                    'slice_position': {
                        'x1' : 0,
                        'x2' : 1,
                        'y1' : 0,
                        'y2' : 1,
                        'z1' : 0,
                        'z2' : 1,
                        'axis' : 0,
                        }
                    },
                'cplane' : {
                    'shading': 4,
                    'rotation':{
                        'rot': 180, 
                        'tilt': 0
                        }, 
                    'position':{
                        'x' : 0,
                        'y' : 0,
                        'z' : 0
                    }   
                },
                'light' : {
                    'position' : {
                        'x' : 0.68,
                        'y' : -0.68,
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
                'arrow': {
                    'color': (0, 0, 0),
                    },
                'scalebar': {
                    'color': (0, 0, 0),
                    }
                },
            'modeler' : {
                'disabled': {
                    'color': (211, 211, 211, 255), # light grey
                    },
                'action' : {
                    'color' : {
                        'valid'   :  (180, 234, 154, 255), # light green
                        'invalid' :  (255, 255, 255, 255), # white
                        'running' :  (255, 0, 0, 255),     # red
                        },
                    'size' : {
                        'width'  : 125,
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
                    'color' : {
                        'valid'   :  (234, 226, 154, 255), # light yellow
                        },
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

    def _internalSettings(self):
        """!Define internal settings (based on user settings)
        """
        self.internalSettings = {}
        for group in self.userSettings.keys():
            self.internalSettings[group] = {}
            for key in self.userSettings[group].keys():
                self.internalSettings[group][key] = {}

        # self.internalSettings['general']["mapsetPath"]['value'] = self.GetMapsetPath()
        self.internalSettings['appearance']['elementListExpand']['choices'] = \
            (_("Collapse all except PERMANENT and current"),
             _("Collapse all except PERMANENT"),
             _("Collapse all except current"),
             _("Collapse all"),
             _("Expand all"))
             
        self.internalSettings['language']['locale']['choices'] = tuple(self.locs)
        self.internalSettings['atm']['leftDbClick']['choices'] = (_('Edit selected record'),
                                                                  _('Display selected'))
        
        self.internalSettings['cmd']['verbosity']['choices'] = ('grassenv',
                                                                'verbose',
                                                                'quiet')
                                                                
        self.internalSettings['appearance']['iconTheme']['choices'] = ('grass',)
        self.internalSettings['appearance']['menustyle']['choices'] = \
                   (_("Classic (labels only)"),
                    _("Combined (labels and module names)"),
                    _("Expert (module names only)"))
        self.internalSettings['appearance']['gSelectPopupHeight']['min'] = 50
        # there is also maxHeight given to TreeCtrlComboPopup.GetAdjustedSize
        self.internalSettings['appearance']['gSelectPopupHeight']['max'] = 1000
        self.internalSettings['appearance']['commandNotebook']['choices'] = \
            (_("Basic top"),
             _("Basic left"),
             _("Fancy green"),
             _("List left"))

        self.internalSettings['display']['driver']['choices'] = ['cairo', 'png']
        self.internalSettings['display']['statusbarMode']['choices'] = None # set during MapFrame init
        self.internalSettings['display']['mouseWheelZoom']['choices'] = (_('Zoom and recenter'),
                                                                         _('Zoom to mouse cursor'),
                                                                         _('Nothing'))
        self.internalSettings['display']['scrollDirection']['choices'] = (_('Scroll forward to zoom in'),
                                                                         _('Scroll back to zoom in'))

        self.internalSettings['nviz']['view'] = {}
        self.internalSettings['nviz']['view']['twist'] = {}
        self.internalSettings['nviz']['view']['twist']['min'] = -180
        self.internalSettings['nviz']['view']['twist']['max'] = 180
        self.internalSettings['nviz']['view']['persp'] = {}
        self.internalSettings['nviz']['view']['persp']['min'] = 1
        self.internalSettings['nviz']['view']['persp']['max'] = 100
        self.internalSettings['nviz']['view']['height'] = {}
        self.internalSettings['nviz']['view']['height']['value'] = -1
        self.internalSettings['nviz']['view']['z-exag'] = {}
        self.internalSettings['nviz']['view']['z-exag']['llRatio'] = 1
        self.internalSettings['nviz']['view']['rotation'] = None
        self.internalSettings['nviz']['view']['focus'] = {}
        self.internalSettings['nviz']['view']['focus']['x'] = -1
        self.internalSettings['nviz']['view']['focus']['y'] = -1
        self.internalSettings['nviz']['view']['focus']['z'] = -1
        self.internalSettings['nviz']['view']['dir'] = {}
        self.internalSettings['nviz']['view']['dir']['x'] = -1
        self.internalSettings['nviz']['view']['dir']['y'] = -1
        self.internalSettings['nviz']['view']['dir']['z'] = -1
        self.internalSettings['nviz']['view']['dir']['use'] = False
        
        for decor in ('arrow', 'scalebar'):
            self.internalSettings['nviz'][decor] = {}
            self.internalSettings['nviz'][decor]['position'] = {}
            self.internalSettings['nviz'][decor]['position']['x'] = 0
            self.internalSettings['nviz'][decor]['position']['y'] = 0
            self.internalSettings['nviz'][decor]['size'] = 100
        self.internalSettings['nviz']['vector'] = {}
        self.internalSettings['nviz']['vector']['points'] = {}
        self.internalSettings['nviz']['vector']['points']['marker'] = ("x",
                                                                       _("box"),
                                                                       _("sphere"),
                                                                       _("cube"),
                                                                       _("diamond"),
                                                                       _("aster"),
                                                                       _("gyro"),
                                                                       _("histogram"))
        self.internalSettings['vdigit']['bgmap'] = {}
        self.internalSettings['vdigit']['bgmap']['value'] = ''
        
    def ReadSettingsFile(self, settings = None):
        """!Reads settings file (mapset, location, gisdbase)"""
        if settings is None:
            settings = self.userSettings
        
        self._readFile(self.filePath, settings)
        
        # set environment variables
        font = self.Get(group = 'display', key = 'font', subkey = 'type')
        enc  = self.Get(group = 'display', key = 'font', subkey = 'encoding')
        if font:
            os.environ["GRASS_FONT"] = font
        if enc:
            os.environ["GRASS_ENCODING"] = enc
        
    def _readFile(self, filename, settings = None):
        """!Read settings from file to dict

        @param filename settings file path
        @param settings dict where to store settings (None for self.userSettings)
        """
        if settings is None:
            settings = self.userSettings
        
        if not os.path.exists(filename):
            return
        
        try:
            fd = open(filename, "r")
        except IOError:
            sys.stderr.write(_("Unable to read settings file <%s>\n") % filename)
            return
        
        try:
            line = ''
            for line in fd.readlines():
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
                    value = self._parseValue(value, read = True)
                    self.Append(settings, group, key, subkey, value)
                    idx += 2
        except ValueError, e:
            print >> sys.stderr, _("Error: Reading settings from file <%(file)s> failed.\n"
                                   "\t\tDetails: %(detail)s\n"
                                   "\t\tLine: '%(line)s'\n") % { 'file' : filename,
                                                               'detail' : e,
                                                               'line' : line }
            fd.close()
        
        fd.close()
        
    def SaveToFile(self, settings = None):
        """!Save settings to the file"""
        if settings is None:
            settings = self.userSettings
        
        dirPath = GetSettingsPath()
        if not os.path.exists(dirPath):
            try:
                os.mkdir(dirPath)
            except:
                GError(_('Unable to create settings directory'))
                return
        
        try:
            file = open(self.filePath, "w")
            for group in settings.keys():
                for key in settings[group].keys():
                    subkeys = settings[group][key].keys()
                    file.write('%s%s%s%s' % (group, self.sep, key, self.sep))
                    for idx in range(len(subkeys)):
                        value = settings[group][key][subkeys[idx]]
                        if type(value) == types.DictType:
                            if idx > 0:
                                file.write('%s%s%s%s%s' % (os.linesep, group, self.sep, key, self.sep))
                            file.write('%s%s' % (subkeys[idx], self.sep))
                            kvalues = settings[group][key][subkeys[idx]].keys()
                            srange = range(len(kvalues))
                            for sidx in srange:
                                svalue = self._parseValue(settings[group][key][subkeys[idx]][kvalues[sidx]])
                                file.write('%s%s%s' % (kvalues[sidx], self.sep,
                                                       svalue))
                                if sidx < len(kvalues) - 1:
                                    file.write('%s' % self.sep)
                        else:
                            if idx > 0 and \
                                    type(settings[group][key][subkeys[idx - 1]]) == types.DictType:
                                file.write('%s%s%s%s%s' % (os.linesep, group, self.sep, key, self.sep))
                            value = self._parseValue(settings[group][key][subkeys[idx]])
                            file.write('%s%s%s' % (subkeys[idx], self.sep, value))
                            if idx < len(subkeys) - 1 and \
                                    type(settings[group][key][subkeys[idx + 1]]) != types.DictType:
                                file.write('%s' % self.sep)
                    file.write(os.linesep)
        except IOError, e:
            raise GException(e)
        except StandardError, e:
            raise GException(_('Writing settings to file <%(file)s> failed.'
                               '\n\nDetails: %(detail)s') % { 'file' : self.filePath,
                                                              'detail' : e })
        file.close()
        return self.filePath
        
    def _parseValue(self, value, read = False):
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

    def Get(self, group, key = None, subkey = None, internal = False):
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
            raise GException("%s '%s:%s:%s'" % (_("Unable to set "), group, key, subkey))
        
    def Append(self, dict, group, key, subkey, value, overwrite = True):
        """!Set value of key/subkey

        Create group/key/subkey if not exists
        
        @param dict settings dictionary to use
        @param group settings group
        @param key key
        @param subkey subkey (value or list)
        @param value value
        @param overwrite True to overwrite existing value
        """

        hasValue = True
        if group not in dict:
            dict[group] = {}
            hasValue = False
        
        if key not in dict[group]:
            dict[group][key] = {}
            hasValue = False
        
        if type(subkey) == types.ListType:
            # TODO: len(subkey) > 2
            if subkey[0] not in dict[group][key]:
                dict[group][key][subkey[0]] = {}
                hasValue = False
            if subkey[1] not in dict[group][key][subkey[0]]:
                hasValue = False
            
            try:
                if overwrite or (not overwrite and not hasValue):
                    dict[group][key][subkey[0]][subkey[1]] = value
            except TypeError:
                print >> sys.stderr, _("Unable to parse settings '%s'") % value + \
                    ' (' + group + ':' + key + ':' + subkey[0] + ':' + subkey[1] + ')'
        else:
            if subkey not in dict[group][key]:
                hasValue = False
            
            try:
                if overwrite or (not overwrite and not hasValue):
                    dict[group][key][subkey] = value
            except TypeError:
                print >> sys.stderr, _("Unable to parse settings '%s'") % value + \
                    ' (' + group + ':' + key + ':' + subkey + ')'
        
    def GetDefaultSettings(self):
        """!Get default user settings"""
        return self.defaultSettings

    def Reset(self, key = None):
        """!Reset to default settings

        @param key key in settings dict (None for all keys)
        """
        if not key:
            self.userSettings = copy.deepcopy(self.defaultSettings)
        else:
            self.userSettings[key] = copy.deepcopy(self.defaultSettings[key])
        
UserSettings = Settings()

def GetDisplayVectSettings():
    settings = list()
    if not UserSettings.Get(group = 'vectorLayer', key = 'featureColor', subkey = ['transparent', 'enabled']):
        featureColor = UserSettings.Get(group = 'vectorLayer', key = 'featureColor', subkey = 'color')
        settings.append('color=%s' % rgb2str.get(featureColor, ':'.join(map(str,featureColor))))
    else:
        settings.append('color=none')
    if not UserSettings.Get(group = 'vectorLayer', key = 'areaFillColor', subkey = ['transparent', 'enabled']):
        fillColor = UserSettings.Get(group = 'vectorLayer', key = 'areaFillColor', subkey = 'color')
        settings.append('fcolor=%s' % rgb2str.get(fillColor, ':'.join(map(str,fillColor))))
    else:
        settings.append('fcolor=none')
    
    settings.append('width=%s' % UserSettings.Get(group = 'vectorLayer', key = 'line', subkey = 'width'))
    settings.append('icon=%s' % UserSettings.Get(group = 'vectorLayer', key = 'point', subkey = 'symbol'))
    settings.append('size=%s' % UserSettings.Get(group = 'vectorLayer', key = 'point', subkey = 'size'))
    types = []
    for ftype in ['point', 'line', 'boundary', 'centroid', 'area', 'face']:
        if UserSettings.Get(group = 'vectorLayer', key = 'showType', subkey = [ftype, 'enabled']):
            types.append(ftype)
    settings.append('type=%s' % ','.join(types))

    return settings
